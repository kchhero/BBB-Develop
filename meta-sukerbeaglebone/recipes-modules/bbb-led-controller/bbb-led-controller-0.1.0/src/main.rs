use actix_web::{get, web, App, HttpResponse, HttpServer, Responder};
use actix_files::NamedFile;
use std::fs::File;
use std::io::prelude::*;

const LED_DEVICE_PATH: &str = "/dev/my_leds";

fn toggle_led_by_driver(pin_index: u32) -> Result<(), String> {
    // 1. /dev/my_leds file open
    let mut file = match File::create(LED_DEVICE_PATH) {
        Ok(file) => file,
        Err(e) => {
            let error_msg = format!("Failed to open {}: {}", LED_DEVICE_PATH, e);
            eprintln!("{}", error_msg);
            return Err(error_msg);
        }
    };

    // 2. index convert to string
    let pin_index_str = pin_index.to_string();
    println!("Attempting to write '{}' to {}", pin_index_str, LED_DEVICE_PATH);
    match file.write_all(pin_index_str.as_bytes()) {
        Ok(_) => {
            println!("Successfully wrote '{}' to {}", pin_index_str, LED_DEVICE_PATH);
            Ok(())
        }
        Err(e) => {
            let error_msg = format!("Failed to write to {}: {}", LED_DEVICE_PATH, e);
            eprintln!("{}", error_msg);
            Err(error_msg)
        }
    }
}

#[get("/")]
async fn index() -> Result<NamedFile, std::io::Error> {
    // 보드에 설치될 HTML 파일의 절대 경로를 사용합니다.
    Ok(NamedFile::open_async("/usr/share/bbb-led-controller/index.html").await?)
}

// POST /led/{line}/toggle --> e.g., /led/12/toggle
async fn toggle_led(path: web::Path<u32>) -> impl Responder {
    println!("Received request to toggle LED on line {}", path);
    let line_num = path.into_inner();
    let driver_index = match line_num {
        12 => 0, // P8_12 -> index 0
        13 => 1, // P8_11 -> index 1
        14 => 2, // P8_16 -> index 2
        15 => 3, // P8_15 -> index 3
        _ => {
            return HttpResponse::BadRequest().body(format!("Error: Line {} is not a valid LED line.", line_num));
        }
    };

    match toggle_led_by_driver(driver_index) {
        Ok(_) => HttpResponse::Ok().body(format!("Line {} (index {}) LED toggle", line_num, driver_index)),
        Err(e) => HttpResponse::InternalServerError().body(e),
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    println!("Starting web server on http://0.0.0.0:8080");
    println!("Open http://<BeagleBone_IP>:8080 in your browser.");

    HttpServer::new(move ||
    {
        App::new()
           .service(index)
           .route("/led/{line}/toggle", web::post().to(toggle_led))
    })
   .bind(("0.0.0.0", 8080))?
   .run()
   .await
}