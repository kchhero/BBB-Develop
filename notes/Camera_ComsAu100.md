#### This is a low-cost camera module called the Coms-AU100.
It was originally intended for use as a PC webcam, but the housing was too large, so I'm planning to disassemble it and use only the PCB.

![20250904_140040](https://github.com/user-attachments/assets/35d504a3-944a-4ad0-a7d7-95d397ddb8cd)


## Step1. /dev/video0, /dev/video1 check
```
IMAGE_INSTALL:append = " kernel-modules v4l-utils usbutils"

root@beaglebone:~# lsusb
Bus 001 Device 002: ID 4c4a:4a55 Jieli Technology USB Composite Device
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
root@beaglebone:~# ls -al /dev/video*
crw-rw---- 1 root video 81, 0 Dec 24 10:58 /dev/video0
crw-rw---- 1 root video 81, 1 Dec 24 10:58 /dev/video1
root@beaglebone:~#
```

## Step2. Check camera Format and Scaling, 1 frame capture
```
v4l2-ctl --list-devices
v4l2-ctl --all -d /dev/video0
v4l2-ctl --list-formats-ext -d /dev/video0

$ ffmpeg -f video4linux2 -input_format mjpeg -video_size 640x480 -i /dev/video0 -vframes 1 -t 2 test.jpg

```

#### It's a little easier to focus on the PC and work on BBB with the command below.
```
$ ffplay -f v4l2 -video_size 640x480 -input_format mjpeg /dev/video0
```

### Camera Placement Strategy Workflow
Fixed Camera --> Wrist-Mounted Camera (Near End-Effector)

