use std::collections::HashMap;
use std::error::Error;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

use linux_embedded_hal::I2cdev;
use nix::sys::termios;
use pwm_pca9685::{Channel, Pca9685};
#[macro_use]
extern crate serde;

use serde::Deserialize;

// --- 설정값 ---
const I2C_BUS: u8 = 2;
const SERIAL_PORT: &str = "/dev/ttyS4"; // ESP32와 연결된 UART 포트
const SERVO_MIN_PULSE: f32 = 150.0;
const SERVO_MAX_PULSE: f32 = 600.0;

// --- 구조체 정의 ---
#[derive(Clone, Copy, Debug, Default)]
struct ServoState {
    on: bool,
    current_angle: f32,
    target_angle: f32,
    speed: f32,
}

#[derive(Clone, Copy, Debug, Deserialize)]
struct ServoConfig {
    ch: u8,
    standby: f32,
    min: f32,
    max: f32,
    invert: bool,
}

// --- 함수들 ---
fn ui_angle_to_servo_angle(ui_angle: f32) -> f32 { (ui_angle + 90.0).clamp(0.0, 180.0) }

fn servo_angle_to_pulse(angle: f32) -> u16 {
    let angle_clamped = angle.clamp(0.0, 180.0);    
    let pulse = SERVO_MIN_PULSE + (SERVO_MAX_PULSE - SERVO_MIN_PULSE) * (angle_clamped / 180.0);
    pulse.round() as u16
}

fn u8_to_channel(ch: u8) -> Option<Channel> {
    match ch {
        6=>Some(Channel::C6), 7=>Some(Channel::C7), 12=>Some(Channel::C12),
        13=>Some(Channel::C13), 14=>Some(Channel::C14), 15=>Some(Channel::C15),
        _ => None,
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    // --- I2C 및 PCA9685 초기화 ---
    let i2c = I2cdev::new(format!("/dev/i2c-{}", I2C_BUS))?;
    let pca = Pca9685::new(i2c, 0x40).map_err(|e| format!("{:?}", e))?;
    let pca = Arc::new(Mutex::new(pca));
    {
        let mut pca_locked = pca.lock().unwrap();
        pca_locked.set_prescale(121).map_err(|e| format!("Failed to set prescale: {:?}", e))?;
        pca_locked.enable().map_err(|e| format!("Failed to enable PCA9685: {:?}", e))?;
    }

    // --- 상태 변수 초기화 ---
    let motor_states = Arc::new(Mutex::new(HashMap::<u8, ServoState>::new()));
    let motor_configs = Arc::new(Mutex::new(HashMap::<u8, ServoConfig>::new()));
    
    // --- 서보 모터 제어 스레드 ---
    let states_clone = Arc::clone(&motor_states);
    let pca_clone = Arc::clone(&pca);
    thread::spawn(move || {
        let update_interval = Duration::from_millis(10);
        loop {
            let mut states = states_clone.lock().unwrap();
            if !states.is_empty() {
                let mut pca = pca_clone.lock().unwrap();
                for (ch_num, state) in states.iter_mut() {
                    if state.on {
                        let diff = state.target_angle - state.current_angle;
                        if diff.abs() > 0.1 {
                            let max_delta = state.speed * 0.020;
                            state.current_angle += diff.signum() * max_delta.min(diff.abs());

                            if let Some(channel_enum) = u8_to_channel(*ch_num) {
                                let pulse = servo_angle_to_pulse(state.current_angle);
                                let _ = pca.set_channel_on_off(channel_enum, 0, pulse);
                            }
                        }
                    }
                }
            }
            drop(states);
            thread::sleep(update_interval);
        }
    });

    // --- 시리얼 포트 설정 ---
    let file = File::open(SERIAL_PORT)?;
    let mut termios_settings = termios::tcgetattr(&file)?;
    termios::cfmakeraw(&mut termios_settings);
    termios::cfsetispeed(&mut termios_settings, termios::BaudRate::B115200)?;
    termios::tcsetattr(&file, termios::SetArg::TCSANOW, &termios_settings)?;
    println!("Serial listener started. Waiting for configuration from ESP32...");
    
    let mut reader = BufReader::new(file);
    let mut line_buffer = String::new();

    // --- 메인 루프: 시리얼 명령 수신 및 처리 ---
    loop {
        line_buffer.clear();
        if reader.read_line(&mut line_buffer)? > 0 {
            let line = line_buffer.trim();
            if let Some(json_str) = line.strip_prefix("CONF:") {
                // 설정 명령 처리
                let configs_from_ui: Vec<ServoConfig> = serde_json::from_str(json_str)?;
                let mut states = motor_states.lock().unwrap();
                let mut configs = motor_configs.lock().unwrap();
                let mut pca = pca.lock().unwrap();
                states.clear();
                configs.clear();

                for config in configs_from_ui.iter() {
                    let standby_angle = if config.invert { -config.standby } else { config.standby };
                    let initial_servo_angle = ui_angle_to_servo_angle(standby_angle);
                    
                    states.insert(config.ch, ServoState {
                        on: false,
                        current_angle: initial_servo_angle,
                        target_angle: initial_servo_angle,
                        speed: (20.0 / 100.0) * 720.0 + 10.0,
                    });
                    configs.insert(config.ch, *config);

                    if let Some(channel_enum) = u8_to_channel(config.ch) {
                        let initial_pulse = servo_angle_to_pulse(initial_servo_angle);
                        let _ = pca.set_channel_on_off(channel_enum, 0, initial_pulse);
                    }
                }
                println!("Configuration received. {} servos initialized.", configs.len());

            } else if let Some(cmd_str) = line.strip_prefix("CMD:") {
                // 제어 명령 처리
                let parts: Vec<&str> = cmd_str.split(':').collect();
                if parts.len() == 4 {
                    if let (Ok(ch), Ok(on), Ok(angle), Ok(speed)) = (
                        parts[0].parse::<u8>(), parts[1].parse::<u8>(),
                        parts[2].parse::<f32>(), parts[3].parse::<u8>(),
                    ) {
                        let mut states = motor_states.lock().unwrap();
                        let configs = motor_configs.lock().unwrap();
                        if let (Some(state), Some(config)) = (states.get_mut(&ch), configs.get(&ch)) {
                            state.on = on == 1;
                            state.speed = (speed as f32 / 100.0) * 720.0 + 10.0;
                            
                            let mut final_angle = angle;
                            if config.invert { final_angle = -final_angle; }
                            
                            let clamped_angle = final_angle.clamp(config.min, config.max);
                            
                            if state.on {
                                state.target_angle = ui_angle_to_servo_angle(clamped_angle);
                            } else {
                                state.target_angle = state.current_angle;
                            }
                        }
                    }
                }
            }
        }
    }
}