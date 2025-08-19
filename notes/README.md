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
