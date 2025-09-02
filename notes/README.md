# My Note
## 1. GPIO control 1
   #### device tree develop using C and dts, legacy work flow. Some LED circuit toggle control.
   * 2025/08/15 : start   
   * 2025/08/17 : complete
   
   #### refs : BeagleBone-Black revD board pin layout ==> [beaglebone-black.pdf](https://github.com/user-attachments/files/21827462/beaglebone-black.pdf)
       
   #### P8 pin number : 11,12,15,16   
   #### P9 pin number : 1,2 => GND
   

![20250818_132013](https://github.com/user-attachments/assets/18edf058-d9f0-482f-9d2f-9a91c4a87eba)
![20250818_132024](https://github.com/user-attachments/assets/2e2824c6-c735-4f48-91b3-83553e8bb40d)


   
## 2. GPIO control 2 (from RUST)
   #### I'll be using Rust to control GPIO. Both the device tree and the app will use Rust.
   * 2025/08/18 : start

   #### BeagleBone Black runs a lightweight web server. A simple web server (actix-web) was created in Rust and provides API endpoints (/led/on, /led/off) for controlling the LED.
   - step : $ board booting > $ insmod led_driver.ko > $ ifconfig eth0 192.168.10.2 up > $ bbb-led-controller > PC web browser connect 192.168.10.2:8080 
   
   #### The UI is accessed via a web browser on a PC or smartphone. Users access the BeagleBone Black's IP address from their PC or smartphone and control the LED via the web UI.
   --> complete : https://github.com/kchhero/BBB-Develop/commit/b6fb8448bc80d87877c50e1501460ea5d51772d3


https://github.com/user-attachments/assets/64102ca5-d0a9-471f-a96d-ad6ac5cfcf4a

   #### We've completed a crucial test. As shown in the video above, clicking the left/right 50ms movement on the local PC will activate the PCA9685 driver and servo motor via the Beagle board connected via Ethernet. Now, if we connect a WiFi module to this, we can also activate it on our smartphone.


## 3. Wifi module Bring-up
   ### Hardware
   #### use : ESP32-DEVKIT-V4 board
   ![20250826_212224](https://github.com/user-attachments/assets/d11a86c5-587a-4552-80a3-1542bf9c9bc8)
| ESP32 DEVKIT V4 핀 | BeagleBone Black 핀 | 설명 |
|------|---|---|
| VIN (또는 5V)	| P9_05 (VDD_5V) | ESP32에 5V 전원 공급 |
| GND | P9_01 (DGND) | 공통 접지 연결 (필수) |
| TXD0 (또는 TX) | P9_11 (UART4_RXD) | ESP32의 17 pin -> BBB의 수신(RX) |
| RXD0 (또는 RX) | P9_13 (UART4_TXD) | ESP32의 16 pin -> BBB의 송신(TX) |
   ### Sofrware
   #### Test module

## 4. index.html
```
async function sendState(channel) {
   const state = motorStates[channel];
   try {
       await fetch('/api/servo', {
           method: 'POST',
           headers: { 'Content-Type': 'application/json' },
           body: JSON.stringify({
               channel: channel,
               on: state.on ? 1 : 0,
               angle: state.angle,
               speed: state.speed
           })
       });
   } catch (error) { console.error('Error:', error); }
}
```
<img width="1160" height="532" alt="image" src="https://github.com/user-attachments/assets/5fe0d60e-2ca0-496d-8cbb-d6c9cf744631" />

## 5. Robot arm basic movement test
Since building the entire mechanism myself was too difficult, I purchased the ICF0608 kit.
Six servo motors are controlled using the PCA9685 driver.
I completed basic testing of the ESP32, but I accidentally connected the 5V power supply incorrectly, which caused the regulator to burn out. So, I ordered a new ESP32 devKit.
Anyway, I manually operated the robot arm on a PC.
I set the min and max ranges for each joint to prevent motor failure due to over-speeding.

#### commit : 385fec08bb8d0d071c49cdc037c83915e6f5f7ae

https://github.com/user-attachments/assets/a0644558-17d0-46f5-9b42-fc4f5f0466f5

#### modified index.html
<img width="1325" height="537" alt="image" src="https://github.com/user-attachments/assets/fe8849af-a9f2-4a5c-b261-b658c37146db" />

