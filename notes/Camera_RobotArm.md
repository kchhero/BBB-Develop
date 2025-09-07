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

### 2. Coordinate Transformation (Calibration)
##### a) Fixed Camera Calibration
  - Using the chessboard/marker (ArUco) function, derive the camera to robot arm coordinate transformation matrix.
  - Example: cv2.calibrateCamera + cv2.aruco.estimatePoseSingleMarkers
##### b) Wrist Camera Calibration
  - Aligns with the gripper end coordinate system.
  - OpenCV PnP (SolvePnP) or simple offset method. <br><br>

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
