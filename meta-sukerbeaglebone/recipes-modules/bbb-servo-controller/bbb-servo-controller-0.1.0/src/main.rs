//See memo.txt for details
use actix_web::{web, App, HttpServer, Responder, HttpResponse};
use serde::Deserialize;
use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

use linux_embedded_hal::I2cdev;
use pwm_pca9685::{Channel, Pca9685};

// --- 설정값 ---
const I2C_BUS: u8 = 2;
const SERVO_MIN_PULSE: u16 = 110;
const SERVO_MAX_PULSE: u16 = 615;

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

struct AppState {
    motor_states: Arc<Mutex<HashMap<u8, ServoState>>>,
    motor_configs: Arc<Mutex<HashMap<u8, ServoConfig>>>,
    pca: Arc<Mutex<Pca9685<I2cdev>>>,
}

#[derive(Deserialize)]
struct ServoCommand {
    channel: u8,
    on: u8,
    angle: f32,
    speed: u8,
}

// --- 함수들 ---
fn ui_angle_to_servo_angle(ui_angle: f32) -> f32 { (ui_angle + 90.0).clamp(0.0, 180.0) }

fn servo_angle_to_pulse(angle: f32) -> u16 {
    let pulse_range = (SERVO_MAX_PULSE - SERVO_MIN_PULSE) as f32;
    (SERVO_MIN_PULSE as f32 + (angle / 180.0) * pulse_range).round() as u16
}

fn u8_to_channel(ch: u8) -> Option<Channel> {
    match ch {
        1=>Some(Channel::C1),
        2=>Some(Channel::C2),
        3=>Some(Channel::C3),
        4=>Some(Channel::C4),
        5=>Some(Channel::C5),
        6=>Some(Channel::C6), //
        7=>Some(Channel::C7), //
        8=>Some(Channel::C8), 
        9=>Some(Channel::C9), 
        10=>Some(Channel::C10),
        11=>Some(Channel::C11),
        12=>Some(Channel::C12),//
        13=>Some(Channel::C13),//
        14=>Some(Channel::C14),//
        15=>Some(Channel::C15),//
        _ => None,
    }
}

// --- API 핸들러들 ---
async fn configure_servos(
    configs_from_ui: web::Json<Vec<ServoConfig>>,
    data: web::Data<AppState>,
) -> impl Responder {
    let mut states = data.motor_states.lock().unwrap();
    let mut stored_configs = data.motor_configs.lock().unwrap();
    let mut pca = data.pca.lock().unwrap();
    states.clear();
    stored_configs.clear();

    for config in configs_from_ui.iter() {
        //let standby_angle = if config.invert { -config.standby } else { config.standby };
        let standby_angle = config.standby;
        let initial_servo_angle = ui_angle_to_servo_angle(standby_angle);
        println!("----------------------------------------------");
        println!("channel: {}", config.ch);
        println!("standby angle: {}", standby_angle);
        println!("initial servo angle: {}", initial_servo_angle);
        println!("----------------------------------------------");
        states.insert(config.ch, ServoState {
            on: false,
            current_angle: initial_servo_angle,
            target_angle: initial_servo_angle,
            speed: (10.0 / 100.0) * 720.0 + 10.0,
        });
        stored_configs.insert(config.ch, *config);

        if let Some(channel_enum) = u8_to_channel(config.ch) {
            let initial_pulse = servo_angle_to_pulse(initial_servo_angle);
            let _ = pca.set_channel_on_off(channel_enum, 0, initial_pulse);
            println!("initial_pulse: {}", initial_pulse);
        }
    }
    
    println!("Configuration received. {} servos initialized.", stored_configs.len());
    HttpResponse::Ok().finish()
}

async fn set_servo_state(
    cmd: web::Json<ServoCommand>,
    data: web::Data<AppState>,
) -> impl Responder {
    let mut states = data.motor_states.lock().unwrap();
    let configs = data.motor_configs.lock().unwrap();

    if let (Some(state), Some(config)) = (states.get_mut(&cmd.channel), configs.get(&cmd.channel)) {
        state.on = cmd.on == 1;
        state.speed = (cmd.speed as f32 / 100.0) * 720.0 + 10.0;
        
        let mut angle = cmd.angle;
        if config.invert { angle = -angle; }
        
        let clamped_angle = angle.clamp(config.min, config.max);
        
        if state.on {
            state.target_angle = ui_angle_to_servo_angle(clamped_angle);
        } else {
            state.target_angle = state.current_angle;
        }
        
        println!("Received: Ch {}, On: {}, Target Angle(Servo): {:.1}", cmd.channel, state.on, state.target_angle);
        HttpResponse::Ok().finish()
    } else {
        HttpResponse::NotFound().body("Channel not yet configured from UI")
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // --- I2C 및 PCA9685 초기화 ---
    let i2c = I2cdev::new(format!("/dev/i2c-{}", I2C_BUS)).expect("Failed to open I2C bus");
    let pca = Pca9685::new(i2c, 0x40).expect("PCA9685 init failed");
    let pca = Arc::new(Mutex::new(pca));
    {
        let mut pca_locked = pca.lock().unwrap();
        pca_locked.set_prescale(121).expect("PCA9685 prescale failed");
        pca_locked.enable().expect("PCA9685 enable failed");
    }

    // --- 상태 변수 초기화 ---
    let motor_states = Arc::new(Mutex::new(HashMap::<u8, ServoState>::new()));
    let motor_configs = Arc::new(Mutex::new(HashMap::<u8, ServoConfig>::new()));
    
    // --- 서보 모터 제어 스레드 ---
    let states_clone = Arc::clone(&motor_states);
    let pca_clone = Arc::clone(&pca);
    thread::spawn(move || {
        let update_interval = Duration::from_millis(20);
        loop {
            let mut states = states_clone.lock().unwrap();
            if !states.is_empty() {
                let mut pca = pca_clone.lock().unwrap();
                for (ch_num, state) in states.iter_mut() {
                    if state.on && (state.current_angle - state.target_angle).abs() > 0.1 {
                        let max_delta = state.speed * 0.020;
                        let diff = state.target_angle - state.current_angle;
                        state.current_angle += diff.signum() * max_delta.min(diff.abs());

                        if let Some(channel_enum) = u8_to_channel(*ch_num) {
                            let pulse = servo_angle_to_pulse(state.current_angle);
                            let _ = pca.set_channel_on_off(channel_enum, 0, pulse);
                        }
                    }
                }
            }
            drop(states);
            thread::sleep(update_interval);
        }
    });

    // --- Actix 웹 서버 설정 ---
    let app_state = web::Data::new(AppState { motor_states, motor_configs, pca });
    println!("Starting web server. Waiting for configuration from UI...");

    HttpServer::new(move || {
        App::new()
            .app_data(app_state.clone())
            .service(web::resource("/api/configure").route(web::post().to(configure_servos)))
            .service(web::resource("/api/servo").route(web::post().to(set_servo_state)))
            .service(actix_files::Files::new("/", "/usr/share/bbb-servo-controller").index_file("index.html"))
    })
    .bind(("0.0.0.0", 8080))?
    .run()
    .await
}