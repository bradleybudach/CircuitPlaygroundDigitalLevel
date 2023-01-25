#define _USE_MATH_DEFINES // for use of M_PI as pi

#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_Circuit_Playground.h>
#include <math.h>
#include <vector>

using namespace std;

double minimumBrightness = 15; // minimum brightness for the indicator pixels out of 255
double accuracyThreshold = 2; // accuracy threshold in degrees.
vector<vector<float>> averageValues; // holds values of x, y, and z acceleration to be averaged
double startTime = 0; // time measurement for button press
bool aIsPressed = false; // if button is pressed
bool bIsPressed = false; // if button is pressed
double desiredAngle = 0; // 45 degrees in radians
double angleDisplayTime = 0; // used to pause indicator lights when the board is showing the angle to the user.
float pastAngle; // Stores the angle before the user presses a button.

void setup() {
  // put your setup code here, to run once:
  CircuitPlayground.begin();
  Serial.begin(9600);
  CircuitPlayground.setAccelRange(LIS3DH_RANGE_2_G); // high accuracy accelerometer
}

void loop() {
  // Increase Angle With User Input
  if (!aIsPressed && CircuitPlayground.leftButton()) {
    startTime = millis();
    aIsPressed = true;
  } else if (aIsPressed && !CircuitPlayground.leftButton()) {
    double totalTime = millis() - startTime;
    if (totalTime <= 1000) { //short button press
      if (desiredAngle < 1.5708) { // increase the angle by 5 degrees
        desiredAngle += (5*M_PI/180);
      }

      if (desiredAngle > 1.5708) { // current max of 90 degrees, I may change this in the future if possible.
        desiredAngle = 1.5708;
      }

      angleDisplayTime = millis() + 3000;
    } else if (totalTime >= 1000) { // sets desired angle to whatever the angle was right before the button was pressed rounded to 5 degrees, shows the user an estimation of that angle later.
      int a = abs(round(pastAngle/5)*5);
      desiredAngle = (a*M_PI/180);
      angleDisplayTime = millis() + 3000;
    }
    aIsPressed = false;
  }

  // Decrease Angle With User Input
  if (!bIsPressed && CircuitPlayground.rightButton()) {
    startTime = millis();
    bIsPressed = true;
  } else if (bIsPressed && !CircuitPlayground.rightButton()) {
    double totalTime = millis() - startTime;
    if (totalTime <= 1000) { //short button press
      if (desiredAngle > 0) { // increase the angle by 5 degrees
        desiredAngle -= (5*M_PI/180);
      }

      if (desiredAngle < 0) {
        desiredAngle = 0;
      }

      angleDisplayTime = millis() + 3000;
    } else if (totalTime >= 1000) { // sets desired angle to whatever the angle was right before the button was pressed rounded to 5 degrees, shows the user an estimation of that angle later.
      int a = abs(round(pastAngle/5)*5);
      desiredAngle = (a*M_PI/180);
      angleDisplayTime = millis() + 3000;
    }
    bIsPressed = false;
  }

  if ((aIsPressed || bIsPressed) && millis() - startTime >= 1000) {
    CircuitPlayground.setBrightness(minimumBrightness);
    for (int i = 0; i < 10; i++) {
      CircuitPlayground.setPixelColor(i, 50, 50, 50);
    }
  }

  bool isDisplaying = millis() <= angleDisplayTime;
  if (isDisplaying) { // Have the pixels display every frame during the display time.
    displayAngle(round(desiredAngle*180/M_PI)); 
  } 


  float x = CircuitPlayground.motionX();
  float y = CircuitPlayground.motionY();
  float z = CircuitPlayground.motionZ();
  averageValues.push_back({x, y, z});

  if (averageValues.size() > 20) { // needs to do 20 calculations to get an average
    averageValues.erase(averageValues.begin()); // when it goes over 20 it will delete the first item in the list
    
    float maxX = -10;
    float minX = 10;
    float maxY = -10;
    float minY = 10;
    float averageX = 0;
    float averageY = 0;
    float averageZ = 0;
    for (int i = 0; i < 20; i++) {
      float val_x = averageValues[i][0];
      float val_y = averageValues[i][1];
      float val_z = averageValues[i][2];

      averageX += val_x;
      averageY += val_y;
      averageZ += val_z;

      if (val_x > maxX) {
        maxX = val_x;
      } else if (val_x < minX) {
        minX = val_x;
      }

      if (val_y > maxY) {
        maxY = val_y;
      } else if (val_y < minY) {
        minY = val_y;
      }
    }


    float rangeX = maxX-minX;
    float rangeY = maxY-minY;

    if (rangeX < 10 && rangeY < 10 && !isDisplaying && !aIsPressed && !bIsPressed) { // if the last 20 calculations have been within a range (means that shaking it will not make it glitch out) and if the program is not currently paused for displaying the angle.
      averageX /= 20;
      averageY /= 20;
      averageZ /= 20;

      float directionVectorForce = sqrt(pow(averageX, 2) + pow(averageY, 2)); //Aggregate force on the board at any angle.
      float desiredVectorForce = averageZ*tan(desiredAngle); // the vector force at the desired angle.
      float forceDifference = desiredVectorForce - directionVectorForce; // difference between the desired force and the actual force
      float absForce = abs(forceDifference);

      float angle = 0;
      float absAngle = 0;
      float indicatorAngleDifference = 0;
      float xy_orientationAngle = 0;
      if (averageZ != 0) {
        angle = atan(directionVectorForce/averageZ)*(180/M_PI); // Measure of the angle from the horizon (0 is perfectly level > or < than 0 is the angle of the tilt).
        pastAngle = angle;
        absAngle = abs(angle - (desiredAngle*180/M_PI)); // absolute value of the difference between the current angle and the desired angle. 
        indicatorAngleDifference = abs(abs(angle) - (desiredAngle*180/M_PI)); // absolute value of the current angle so indication works when the angle goes negative.
      }
      if (averageY != 0) {
        xy_orientationAngle = atan(averageX/averageY)*(180/M_PI); // Measure of the direction the board is facing (tilt from side to side, not how level it is) Used for direction indication.
      }

      if (absAngle < accuracyThreshold) { // if it is level to a given angle
        CircuitPlayground.setBrightness(minimumBrightness);
        for (int i = 0; i < 10; i++) { // Set all pixels to green
          CircuitPlayground.setPixelColor(i, 0, 255, 0);
        }
      } else if ((forceDifference > 0 && averageX < 0) || (forceDifference < 0 && averageX > 0)) { // Right side lights. Flips if you go past the angle you need (force difference < 0)
        vector<int> nearbyPixels; //pixels near the main one that will light up to show direction intensity
        int mainPixel = -1; // -1 default means there is no mainPixel, changed to the desired pixel if there is one.

        // Set the brightness based on the intensity of the force, clamped between the minimum brightness variable and the max cp.pixels.brightness allows
        CircuitPlayground.setBrightness(clampBrightness((absForce * 30), minimumBrightness, 255));
        
        if (xy_orientationAngle >= -12.86 && xy_orientationAngle <= 0) {
          if (indicatorAngleDifference <= 60) {
            nearbyPixels.push_back(4);
            nearbyPixels.push_back(5);
          } else {
            nearbyPixels.push_back(3);
            nearbyPixels.push_back(4);
            nearbyPixels.push_back(6);
            nearbyPixels.push_back(5);
          }
        } else if (xy_orientationAngle >= -38.57 && xy_orientationAngle <= 0) {
          mainPixel = 5;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(4);
            nearbyPixels.push_back(6);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(3);
            nearbyPixels.push_back(4);
            nearbyPixels.push_back(6);
            nearbyPixels.push_back(7);
          }
        } else if (xy_orientationAngle >= -64.29 && xy_orientationAngle <= 0) {
          mainPixel = 6;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(5);
            nearbyPixels.push_back(7);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(4);
            nearbyPixels.push_back(5);
            nearbyPixels.push_back(7);
            nearbyPixels.push_back(8);
          }
        } else if (xy_orientationAngle <= 12.86 && xy_orientationAngle > 0) {
          if (indicatorAngleDifference <= 60) {
            nearbyPixels.push_back(0);
            nearbyPixels.push_back(9);
          } else {
            nearbyPixels.push_back(0);
            nearbyPixels.push_back(1);
            nearbyPixels.push_back(8);
            nearbyPixels.push_back(9);
          }
        } else if (xy_orientationAngle <= 38.57 && xy_orientationAngle > 0) {
          mainPixel = 9;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(8);
            nearbyPixels.push_back(0);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(0);
            nearbyPixels.push_back(1);
            nearbyPixels.push_back(7);
            nearbyPixels.push_back(8);
          }
        } else if (xy_orientationAngle <= 64.29 && xy_orientationAngle > 0) {
          mainPixel = 8;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(7);
            nearbyPixels.push_back(9);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(6);
            nearbyPixels.push_back(7);
            nearbyPixels.push_back(9);
            nearbyPixels.push_back(0);
          }
        } else {
          mainPixel = 7;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(6);
            nearbyPixels.push_back(8);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(5);
            nearbyPixels.push_back(6);
            nearbyPixels.push_back(8);
            nearbyPixels.push_back(9);
          }
        }

        displayIndicatorPixels(mainPixel, nearbyPixels, 10);
      } else if ((forceDifference > 0 and averageX > 0) || (forceDifference < 0 and averageX < 0)) { // Left side lights. Flips if you go past the angle you need (force difference < 0)
        vector<int> nearbyPixels; //pixels near the main one that will light up to show direction intensity
        int mainPixel = -1; // -1 default means there is no mainPixel, changed to the desired pixel if there is one.

        // Set the brightness based on the intensity of the force, clamped between the minimum brightness variable and the max cp.pixels.brightness allows
        CircuitPlayground.setBrightness(clampBrightness((absForce * 30), minimumBrightness, 255));

        if (xy_orientationAngle >= -12.86 && xy_orientationAngle <= 0) {
          if (indicatorAngleDifference <= 60) {
            nearbyPixels.push_back(0);
            nearbyPixels.push_back(9);
          } else {
            nearbyPixels.push_back(0);
            nearbyPixels.push_back(9);
            nearbyPixels.push_back(1);
            nearbyPixels.push_back(8);
          }
        } else if (xy_orientationAngle >= -38.57 && xy_orientationAngle <= 0) {
          mainPixel = 0;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(9);
            nearbyPixels.push_back(1);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(9);
            nearbyPixels.push_back(1);
            nearbyPixels.push_back(8);
            nearbyPixels.push_back(2);
          }
        } else if (xy_orientationAngle >= -64.29 && xy_orientationAngle <= 0) {
          mainPixel = 1;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(2);
            nearbyPixels.push_back(0);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(2);
            nearbyPixels.push_back(0);
            nearbyPixels.push_back(9);
            nearbyPixels.push_back(3);
          }
        } else if (xy_orientationAngle <= 12.86 && xy_orientationAngle > 0) {
          if (indicatorAngleDifference <= 60) {
            nearbyPixels.push_back(4);
            nearbyPixels.push_back(5);
          } else {
            nearbyPixels.push_back(4);
            nearbyPixels.push_back(5);
            nearbyPixels.push_back(3);
            nearbyPixels.push_back(6);
          }
        } else if (xy_orientationAngle <= 38.57 && xy_orientationAngle > 0) {
          mainPixel = 4;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(3);
            nearbyPixels.push_back(5);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(3);
            nearbyPixels.push_back(5);
            nearbyPixels.push_back(2);
            nearbyPixels.push_back(6);
          }
        } else if (xy_orientationAngle <= 64.29 && xy_orientationAngle > 0) {
          mainPixel = 3;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(2);
            nearbyPixels.push_back(4);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(2);
            nearbyPixels.push_back(4);
            nearbyPixels.push_back(1);
            nearbyPixels.push_back(5);
          }
        } else {
          mainPixel = 2;
          if (indicatorAngleDifference > 30 && indicatorAngleDifference < 60) {
            nearbyPixels.push_back(1);
            nearbyPixels.push_back(3);
          } else if (indicatorAngleDifference >= 60) {
            nearbyPixels.push_back(1);
            nearbyPixels.push_back(3);
            nearbyPixels.push_back(0);
            nearbyPixels.push_back(4);
          }
        }
        
        displayIndicatorPixels(mainPixel, nearbyPixels, 10);
      } else {
        CircuitPlayground.clearPixels();
      }         
    } else {
      CircuitPlayground.clearPixels();
    } 

  }
}

float clampBrightness(float x, float min, float max) { // clamps the brightness between two values.
  if (x > max) {
    return max;
  } else if (x < min) {
    return min;
  } else {
    return x;
  }
}

void displayIndicatorPixels(int mainPixel, vector<int> nearbyPixels, int intensity) { // Displays pixels that are indicating the direction off from the desired angle. 
  vector<int> clearPixels = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  if (mainPixel >= 0) {
    CircuitPlayground.setPixelColor(mainPixel, 255, 0, 0); // Displays the main pixel at max brightness.
  }

  for (int i = 0; i < nearbyPixels.size(); i++) {
    for (int j = 0; j < clearPixels.size(); j++) {
      if (clearPixels[j] == nearbyPixels[i]) {
        clearPixels.erase(clearPixels.begin()+j); // Avoids clearing the nearby pixels that will be displaying
      }
    }
    CircuitPlayground.setPixelColor(nearbyPixels[i], intensity, 0, 0); // displays the pixels near the main pixel with varriable brightness.
  }

  for (int i = 0; i < clearPixels.size(); i++) {
    CircuitPlayground.setPixelColor(clearPixels[i], 0, 0, 0); // clears all pixels other than the indicator ones.
  }
}

void displayAngle(int angle) { // displays the selected angle to the user. 
  CircuitPlayground.setBrightness(minimumBrightness);
  int increments = round(angle/5); // increments of 5

  if (increments % 2 == 0) { // if increments is even
    increments = increments/2;
    for (int i = 0; i < increments; i++) {
      CircuitPlayground.setPixelColor(i, 0, 0, 255);
    }
  } else { // if increments is odd
    increments = (increments-1)/2;
    for (int i = 0; i < increments; i++) {
      CircuitPlayground.setPixelColor(i, 0, 0, 255);
    }
    CircuitPlayground.setPixelColor(increments, 255, 255, 0);
  }
}
