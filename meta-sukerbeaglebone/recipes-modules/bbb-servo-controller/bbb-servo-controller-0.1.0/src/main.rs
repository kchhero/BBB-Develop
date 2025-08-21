use std::env;
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;
use pwm_pca9685::{Address, Channel, Pca9685};
use linux_embedded_hal::I2cdev;

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len()!= 4 {        
        eprintln!("잘못된 사용법입니다!");
        eprintln!("사용법: {} <정지 펄스> <시계방향 최대속도 펄스> <반시계방향 최대속도 펄스>", args[0]);
        eprintln!("예시: {} 307 205 410", args[0]);
        std::process::exit(1);
    }

    //parse to int number
    let pulse_stop: u16 = args[1].parse().expect("정지 펄스 값은 숫자여야 합니다.");
    let pulse_cw_max: u16 = args[2].parse().expect("시계방향 펄스 값은 숫자여야 합니다.");
    let pulse_ccw_max: u16 = args[3].parse().expect("반시계방향 펄스 값은 숫자여야 합니다.");

    println!("서보모터 설정 값: 정지={}, CW 최대={}, CCW 최대={}", pulse_stop, pulse_cw_max, pulse_ccw_max);

    let dev = I2cdev::new("/dev/i2c-2").unwrap();
    let address = Address::default();
    
    let pwm = Arc::new(Mutex::new(Pca9685::new(dev, address).unwrap()));
    let pwm_clone = Arc::clone(&pwm);

    //Ctrl+C and initialize PCA9685 driver module
    ctrlc::set_handler(move || {
        println!("\nCtrl+C received! Shutting down servos and exiting.");
        let mut pwm_guard = pwm_clone.lock().unwrap();
        pwm_guard.set_all_on_off(&[0; 16], &[0; 16]).unwrap();
        pwm_guard.disable().unwrap();
        std::process::exit(0);
    }).expect("Error setting Ctrl-C handler");

    // init PCA9685
    {
        let mut pwm_guard = pwm.lock().unwrap();
        pwm_guard.set_prescale(121).unwrap(); // 50Hz
        pwm_guard.enable().unwrap();
    }

    println!("Controlling continuous rotation servos on Channel 3 and 11.");
    println!("Press Ctrl+C to stop.");

    loop {
        let mut pwm_guard = pwm.lock().unwrap();

        // 1. CW rotation
        println!("Rotating Clockwise (CW)...");
        pwm_guard.set_channel_on_off(Channel::C3, 0, pulse_cw_max).unwrap();
        pwm_guard.set_channel_on_off(Channel::C11, 0, pulse_cw_max).unwrap();
        drop(pwm_guard);
        thread::sleep(Duration::from_millis(100));
        //thread::sleep(Duration::from_secs(0.1));

        // 2. Stop
        println!("Stopping...");
        let mut pwm_guard = pwm.lock().unwrap();
        pwm_guard.set_channel_on_off(Channel::C3, 0, pulse_stop).unwrap();
        pwm_guard.set_channel_on_off(Channel::C11, 0, pulse_stop).unwrap();
        drop(pwm_guard);
        thread::sleep(Duration::from_secs(1));

        // 3. CCW rotation
        println!("Rotating Counter-Clockwise (CCW)...");
        let mut pwm_guard = pwm.lock().unwrap();
        pwm_guard.set_channel_on_off(Channel::C3, 0, pulse_ccw_max).unwrap();
        pwm_guard.set_channel_on_off(Channel::C11, 0, pulse_ccw_max).unwrap();
        drop(pwm_guard);
        thread::sleep(Duration::from_millis(100));
        //thread::sleep(Duration::from_secs(0.1));

        // 4. Stop
        println!("Stopping...");
        let mut pwm_guard = pwm.lock().unwrap();
        pwm_guard.set_channel_on_off(Channel::C3, 0, pulse_stop).unwrap();
        pwm_guard.set_channel_on_off(Channel::C11, 0, pulse_stop).unwrap();
        drop(pwm_guard);
        thread::sleep(Duration::from_secs(1));
    }
}