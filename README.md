# PixyUSB
USB code to connect Pixy version 1 camera to RoboRio

The Java library is pretty straightforward to use, just drop jna.jar and libpixyusb.so from the lib/ directory in the archive into wpilib/user/java/lib/ from your root user directory. The robot code in the src/ directory is a basic example for testing the Pixy functionality.

The PixyUSB directory is a cross compiled library and example program to operate multiple Pixy version 1 cameras from the roborio USB ports.
The libpixyusb directory is the raw source code that would need to be cross compiled to work  on the roborio

Update:
The PixyTest file is a VSCode example used to operate multiple Pixy Version 1 cameras to operate through the USB connection.

The instructions below are already included in the PixyTest code.

To use Pixy USB libraries add the lib subdirectory to the your project subdirectory. 

Ex: c:\Users\<username>\Workspace\PixyTest\lib

Add the .java files to the project where your robot files are located.

Edit your build.gradle file dependancies section. Add in the following lines:

nativeLib fileTree(dir: 'lib', include: '*.so')
compile fileTree(dir: 'lib', include: ['*.jar'])

That should let you compile and download the code to the roborio. Open a dashboard and press the enumerate button. From the driverstation open console and see what uid numbers come up. Go into the Robot code and make sure that the uid is in the lines 
Pixy.ensureAvailable(uid, uid2, ...)
and 
pixy = new Pixy(uid)

Re-download and you should be able to pull up a camera stream from the pixy. To get a second pixy you need to identify the second uid and add it into the Pixy.ensureAvailable(uid, uid2, ...)
Then add a new pixy
pixy2 = new Pixy(uid2);
Replicate the calls in the robot section using pixy2

Call up a second camera stream window on Smartdashboard. Edit the window properties and select the second camera.
