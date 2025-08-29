use std::{env, sync::{Arc, Mutex}};
use std::thread;
use std::time::Duration;
use pwm_pca9685::{Address, Channel, Pca9685};
use linux_embedded_hal::I2cdev;
use actix_web::{get, web, App, HttpResponse, HttpServer, Responder};
use actix_web::post;
use actix_files::NamedFile;

struct AppState {
    pwm: Arc<Mutex<Pca9685<I2cdev>>>,
}

#[get("/")]
async fn index() -> Result<NamedFile, std::io::Error> {
    Ok(NamedFile::open_async("/usr/share/bbb-servo-controller/index.html").await?)
}

//POST /servo/3/cw/100  (3번 채널, 시계방향, 100ms)
async fn move_servo(
    path: web::Path<(u16, String, u64)>,
    app_state: web::Data<AppState>,
) -> impl Responder {
    let (channel_num, direction, duration_ms) = path.into_inner();

    // 채널 번호를 Channel 타입으로 변환
    let channel = match channel_num {
        0 => Channel::C0,
        1 => Channel::C1,
        2 => Channel::C2,
        3 => Channel::C3,
        4 => Channel::C4,
        5 => Channel::C5,
        6 => Channel::C6,
        7 => Channel::C7,
        8 => Channel::C8,
        9 => Channel::C9,
        10 => Channel::C10,
        11 => Channel::C11,
        12 => Channel::C12,
        13 => Channel::C13,
        14 => Channel::C14,
        15 => Channel::C15,
        _ => return HttpResponse::BadRequest().body("Invalid channel number. Use 0-15."),
    };

    // 방향에 따라 펄스 값을 결정 (임시 값, 실제 값으로 교체 필요)
    let (pulse_stop, pulse_move) = match direction.as_str() {
        "cw" => (307, 280), // 정지, 시계방향
        "ccw" => (307, 330), // 정지, 반시계방향
        _ => return HttpResponse::BadRequest().body("Invalid direction. Use 'cw' or 'ccw'."),
    };

    // web server handler does not use 'sleep'
    let pwm_clone = Arc::clone(&app_state.pwm);
    let direction_clone = direction.clone();
    thread::spawn(move || {
        println!("Moving servo on channel {} {} for {}ms", channel_num, direction_clone, duration_ms);
        // 1. start rotation
        {
            let mut pwm_guard = pwm_clone.lock().unwrap();
            pwm_guard.set_channel_on_off(channel, 0, pulse_move).unwrap();
        }
        // 2. wait
        thread::sleep(Duration::from_millis(duration_ms));
        // 3. 회전 정지
        {
            let mut pwm_guard = pwm_clone.lock().unwrap();
            pwm_guard.set_channel_on_off(channel, 0, pulse_stop).unwrap();
        }
        println!("Finished moving servo on channel {}", channel_num);
    });

    HttpResponse::Ok().body(format!(
        "Servo {} move command received: {} for {}ms",
        channel_num, direction, duration_ms
    ))
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // I2C device and PCA9685 initialization
    let dev = I2cdev::new("/dev/i2c-2").unwrap();
    let address = Address::default();
    let mut pwm = Pca9685::new(dev, address).unwrap();
    pwm.set_prescale(121).unwrap(); // 50Hz
    pwm.enable().unwrap();

    // AppState creation
    let app_state = web::Data::new(AppState {
        pwm: Arc::new(Mutex::new(pwm)),
    });

    println!("Starting servo web server on http://0.0.0.0:8080");
    println!("Open http://<BeagleBone_IP>:8080 in your browser.");

    HttpServer::new(move || {
        App::new()
           .app_data(app_state.clone())
           .service(index)
           .route("/servo/{channel}/{direction}/{duration_ms}", web::post().to(move_servo))
    })
   .bind(("0.0.0.0", 8080))?
   .run()
   .await
}