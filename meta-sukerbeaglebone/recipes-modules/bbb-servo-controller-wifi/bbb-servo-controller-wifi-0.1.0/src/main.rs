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

// --- 설정값 ---
const I2C_BUS: u8 = 2;
const SERIAL_PORT: &str = "/dev/ttyS4";
const SERVO_MIN_PULSE: u16 = 125; // 0도
const SERVO_MAX_PULSE: u16 = 615; // 180도

// 모터의 상태를 저장하는 구조체
#[derive(Clone, Copy, Debug)]
struct ServoState {
    on: bool,
    current_angle: f32,
    target_angle: f32,
    speed: f32, // 초당 이동할 각도
}

impl Default for ServoState {
    fn default() -> Self {
        ServoState { 
            on: false, 
            current_angle: 90.0, 
            target_angle: 90.0, 
            speed: 360.0 
        }
    }
}

// UI 각도(-90~90)를 서보 각도(0~180)로 변환
fn ui_angle_to_servo_angle(ui_angle: f32) -> f32 {
    (ui_angle + 90.0).max(0.0).min(180.0)
}

// 서보 각도(0~180)를 펄스 값으로 변환
fn servo_angle_to_pulse(angle: f32) -> u16 {
    let pulse_range = (SERVO_MAX_PULSE - SERVO_MIN_PULSE) as f32;
    (SERVO_MIN_PULSE as f32 + (angle / 180.0) * pulse_range).round() as u16
}

fn u8_to_channel(channel_num: u8) -> Option<Channel> {
    match channel_num {
        0 => Some(Channel::C0),
        1 => Some(Channel::C1),
        2 => Some(Channel::C2),
        3 => Some(Channel::C3),
        4 => Some(Channel::C4),
        5 => Some(Channel::C5),
        6 => Some(Channel::C6),
        7 => Some(Channel::C7),
        8 => Some(Channel::C8),
        9 => Some(Channel::C9),
        10 => Some(Channel::C10),
        11 => Some(Channel::C11),
        12 => Some(Channel::C12),
        13 => Some(Channel::C13),
        14 => Some(Channel::C14),
        15 => Some(Channel::C15),
        _ => None, // 15번을 초과하는 채널은 없음
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    // --- I2C 및 PCA9685 초기화 ---
    let i2c = I2cdev::new(format!("/dev/i2c-{}", I2C_BUS))?;
    let mut pca = Pca9685::new(i2c, 0x40).map_err(|e| format!("{:?}", e))?;
    pca.set_prescale(121).map_err(|e| format!("{:?}", e))?;
    pca.enable().map_err(|e| format!("{:?}", e))?;
    let pca = Arc::new(Mutex::new(pca));

    // --- 모터 상태 초기화 (Arc<Mutex<...>>로 감싸 스레드 간 공유) ---
    let motor_states = Arc::new(Mutex::new(HashMap::<u8, ServoState>::new()));
    let channels = [0, 1, 2, 3, 8, 9];
    {
        let mut states = motor_states.lock().unwrap();
        for &ch in &channels {
            states.insert(ch, ServoState::default());
        }
    }

    // --- 서보 모터 제어 스레드 ---
    let states_clone = Arc::clone(&motor_states);
    let pca_clone = Arc::clone(&pca);
    thread::spawn(move || {
        let update_interval = Duration::from_millis(20); // 50Hz
        loop {
            let mut states = states_clone.lock().unwrap();
            let mut pca = pca_clone.lock().unwrap();

            for (ch_num, state) in states.iter_mut() {
                // 목표 각도와 현재 각도가 다를 때만 움직임
                if (state.current_angle - state.target_angle).abs() > 0.1 {
                    let max_delta = state.speed * (update_interval.as_millis() as f32 / 1000.0);
                    let diff = state.target_angle - state.current_angle;

                    // 다음 스텝 계산
                    if diff.abs() <= max_delta {
                        state.current_angle = state.target_angle; // 목표 도달
                    } else {
                        state.current_angle += diff.signum() * max_delta; // 속도에 맞춰 이동
                    }

                    // 계산된 현재 각도를 펄스 값으로 변환하여 PCA9685에 전송
                    if let Some(channel_enum) = u8_to_channel(*ch_num) {
                        let pulse = servo_angle_to_pulse(state.current_angle);
                        if let Err(e) = pca.set_channel_on_off(channel_enum, 0, pulse) {
                            eprintln!("PCA Error ch {}: {:?}", ch_num, e);
                        }
                    }
                }
            }
            drop(states); // Mutex lock 해제
            drop(pca);
            thread::sleep(update_interval);
        }
    });

    // --- 시리얼 포트 설정 및 메인 스레드 (명령 수신) ---
    let file = File::open(SERIAL_PORT)?;
    let mut termios = termios::tcgetattr(&file)?;
    termios::cfmakeraw(&mut termios);
    termios::cfsetispeed(&mut termios, termios::BaudRate::B115200)?;
    termios::tcsetattr(&file, termios::SetArg::TCSANOW, &termios)?;
    println!("Listening for commands on {}...", SERIAL_PORT);
    let mut reader = BufReader::new(file);
    
    loop {
        let mut line_buffer = String::new();
        if reader.read_line(&mut line_buffer)? > 0 {
            let parts: Vec<&str> = line_buffer.trim().split(':').collect();
            if parts.len() == 4 {
                if let (Ok(ch), Ok(on), Ok(angle), Ok(speed)) = (
                    parts[0].parse::<u8>(),
                    parts[1].parse::<u8>(),
                    parts[2].parse::<f32>(),
                    parts[3].parse::<u8>(),
                ) {
                    let mut states = motor_states.lock().unwrap();
                    if let Some(state) = states.get_mut(&ch) {
                        state.on = on == 1;
                        
                        // UI Speed(1~100)를 실제 Speed(초당 각도)로 변환
                        state.speed = (speed as f32 / 100.0) * 720.0 + 10.0; // 최소 속도 보장, 최대 720도/초

                        if state.on {
                            // On 상태이면 목표 각도를 새로 설정
                            state.target_angle = ui_angle_to_servo_angle(angle);
                        } else {
                            // Off 상태이면 즉시 멈추도록 목표 각도를 현재 각도로 설정
                            state.target_angle = state.current_angle;
                        }
                        println!("Received Cmd: Ch {}, On: {}, Target Angle: {:.1}, Speed Setting: {}", ch, state.on, state.target_angle, speed);
                    }
                }
            }
        }
    }
}