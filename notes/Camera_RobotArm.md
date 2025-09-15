## Road Map

### 1. Camera Roles
##### a) Fixed Camera:
  - Perceives the entire workspace (table, etc.) with a bird's-eye view.
  - Detects object position (x, y coordinates).
  - Converts to the robot arm base coordinate system (e.g., pixel to actual distance conversion).
##### b) Wrist Camera:
  - Precise position correction just before picking.
  - Verifies that the object is properly positioned within the gripper.
  - Determines orientation (angle). <br><br>

![20250913_172442](https://github.com/user-attachments/assets/24d2d261-70f6-4b43-ad78-141c4fe3190d)

### 2. Coordinate Transformation (Calibration)
##### a) Fixed Camera Calibration
  - Using the chessboard/marker (ArUco) function, derive the camera to robot arm coordinate transformation matrix.
  - Example: cv2.calibrateCamera + cv2.aruco.estimatePoseSingleMarkers
##### b) Wrist Camera Calibration
  - Aligns with the gripper end coordinate system.
  - OpenCV PnP (SolvePnP) or simple offset method. <br><br>
  
![20250913_173605](https://github.com/user-attachments/assets/e6505059-fa8f-40fd-83db-6aeebfe7b696)

### 3. Basic Pipeline
##### a) Fixed Camera: Object detection (color/shape/deep learning YOLO possible)
  - Converts object position to the robot arm coordinate system.
  - Translates the robot arm to the object. Go to top
##### b) Activate wrist camera → Fine-tune position/angle
  - Close gripper → Pick up object
  - Move to target location (e.g., basket) → Place object <br><br>

### 4. Implementation Sequence
  - Track a single object (e.g., red ball)
  - Detect (x,y) from fixed camera → Move robot arm
  - Calibrate wrist camera
  - Adjust the last few centimeters to center object in frame
  - Control gripper
  - Motor angle → Test picking motion
  - 
![20250913_172446](https://github.com/user-attachments/assets/081743bc-3704-4311-b587-76a038a06034) 
![20250913_172414](https://github.com/user-attachments/assets/38aaaf1d-94cd-43ba-a62d-7d69ce4bff0e)

## ARUCO method
I'm organizing the data for positions 1 through 10 into a table.
I plan to explore ways to train the model using the above data.

### DATA
```
<10>
Marker 3 centroid: (140,304.75)
Marker 0 centroid: (316.25,367.25)
Target (mm): X=-96.1364 Y=34.0909
Calculated base angle: 160.475 deg
ch 15 : angle 115
ch 14 : angle 40
ch 13 : angle 45
ch 12 : angle 30

<9>
Marker 0 centroid: (316.25,368)
Marker 3 centroid: (201.5,199.75)
Target (mm): X=-62.5909 Y=91.7727
Calculated base angle: 124.295 deg
ch 15 : angle 105
ch 14 : angle 55
ch 13 : angle 10
ch 12 : angle 20

<8>
Marker 0 centroid: (315.75,368)
Marker 3 centroid: (186.25,98.5)
Target (mm): X=-70.6364 Y=147            
Calculated base angle: 115.665 deg
ch 15 : angle 95
ch 14 : angle 35
ch 13 : angle 42
ch 12 : angle 20
```

<This is a video of stacking the objects in marker 8 at the red location.>


https://github.com/user-attachments/assets/35f055e5-c893-46a7-8d66-051726d96fde




