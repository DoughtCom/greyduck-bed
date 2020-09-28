//Transforming the motor's rotary motion into linear motion by using a threaded rod:
//Threaded rod's pitch = 2 mm. This means that one revolution will move the nut 2 mm.
//Default stepping = 400 step/revolution.
// 400 step = 1 revolution = 8 mm linear motion. (4 start 2 mm pitch screw)
// 1 cm = 10 mm =>> 10/8 * 400 = 4000/8 = 500 steps are needed to move the nut by 1 cm.
 
#include <AccelStepper.h>
 
//User-defined values
long receivedSteps = 0; //Number of steps
long receivedSpeed = 800; //Steps / second
long receivedAcceleration = 0; //Steps / second^2
char receivedCommand;
long currentLimit = 0; //Where we are wanting to go
int currentIndex = 0;
//-------------------------------------------------------------------------------
int directionMultiplier = 1; // = 1: positive direction, = -1: negative direction
bool newData, runallowed = false; // booleans for new data from serial, and runallowed flag
bool relative = false; // if we're running a command to go to a position manually and not moving the bed up or down
AccelStepper stepper(1, 9, 8);// direction Digital 9 (CCW), pulses Digital 8 (CLK)
long stepArray[] = {17000, 10000, 10000, 13800}; //The array of steps to move the bed in conjunction with the non linear motion of the compression actuators
long speedUpArray[] = {4300, 2000, 1000, 800}; //The corresponding speeds with the above steps, this should be a complex object, but... you know I have no idea what I'm doing
long speedDownArray[] = {3000, 2200, 1500, 1100};
long maxSteps = 50100; //the max steps the bed can/should move.
bool running = false; //if we're currently in the middle of running our command
bool disableActuators = false; //if we should move the steppers without moving the linear actuators

const int ACTUATOR_PIN = 3;  // the Arduino pin for turning on the actuator arms via the relay
const int DIRECTION_PIN = 2;  // the Arduino pin for the direction the arms go
 
void setup()
{
    pinMode(ACTUATOR_PIN, OUTPUT);
    pinMode(DIRECTION_PIN, OUTPUT);
    //Disable the actuators immediately
    digitalWrite(ACTUATOR_PIN, HIGH);
    digitalWrite(DIRECTION_PIN, LOW);
    

    Serial.begin(9600); //define baud rate
    Serial.println("Demonstration of AccelStepper Library"); //print a messages
    Serial.println("Send 'C' for printing the commands.");
 
    //setting up some default values for maximum speed and maximum acceleration
    Serial.println("Default speed: 400 steps/s, default acceleration: 800 steps/s^2.");
    stepper.setMaxSpeed(400); //SPEED = Steps / second
    stepper.setAcceleration(4000); //ACCELERATION = Steps /(second)^2
 
    stepper.disableOutputs(); //disable outputs
}
 
void loop()
{
    //Constantly looping through these 2 functions.
    //We only use non-blocking commands, so something else (should also be non-blocking) can be done during the movement of the motor
 
    checkSerial(); //check serial port for new commands
    RunTheMotor(); //function to handle the motor  
}
 
void RunTheMotor() //function for the motor
{
    if (runallowed == true)
    {
        stepper.enableOutputs(); //enable pins
        stepper.run(); //step the motor (this will step the motor by 1 step at each loop)  
    }
    else //program enters this part if the runallowed is FALSE, we do not do anything
    {
        stepper.disableOutputs(); //disable outputs
        return;
    }

    long currentPosition = stepper.currentPosition() * directionMultiplier;

    //If we're going up, this isn't a direct command and we haven't gone through all the speed indexes
    if (currentPosition >= currentLimit && currentIndex < 3 && relative == false && directionMultiplier > 0)
    {
        Serial.println("New Stepper Index Positive"); //print the action

        currentIndex++;

        currentLimit += stepArray[currentIndex];
        receivedSteps = stepArray[currentIndex];
        receivedSpeed = speedUpArray[currentIndex];

        Serial.println("Position");
        Serial.println(currentPosition);
        Serial.println("Limit");
        Serial.println(currentLimit);

        stepper.setMaxSpeed(receivedSpeed); //set speed
        running = true;
    }
    //If we're going down, this isn't a direct command and we haven't gone through all of the speed indexes 
    else if (currentPosition * -1 <= currentLimit && currentIndex < 3 && relative == false && directionMultiplier < 0)
    {
        Serial.println("New Stepper Index Negative");

        currentIndex++;

        currentLimit += stepArray[3 - currentIndex] *  -1; //Basically start from the back of the array and then make it a negative number
        receivedSteps = stepArray[3 - currentIndex];
        receivedSpeed = speedDownArray[3 - currentIndex];

        Serial.println("Position");
        Serial.println(currentPosition);
        Serial.println("Limit");
        Serial.println(currentLimit);

        stepper.setMaxSpeed(receivedSpeed); //set speed
        running = true;
    } else if (currentIndex == 3 && (currentPosition >= maxSteps - 1 || currentPosition == 0) && running) //Then let's finish our task
    {
        currentIndex = 0;
        digitalWrite(ACTUATOR_PIN, HIGH);
        running = false;
    }
}
 
void checkSerial() //function for receiving the commands
{  
    if (Serial.available() > 0) //if something comes from the computer
    {
        receivedCommand = Serial.read(); // pass the value to the receivedCommad variable
        newData = true; //indicate that there is a new data by setting this bool to true
 
        if (newData == true) //we only enter this long switch-case statement if there is a new command from the computer
        {
            switch (receivedCommand) //we check what is the command
            {
            case 'U':
                relative = false;
                directionMultiplier = 1; //We define the direction
                Serial.println("Bed Going Up."); //print the action
                currentIndex = 0;
                currentLimit += stepArray[0];
                receivedSteps = maxSteps;
                receivedSpeed = speedUpArray[0];
                Serial.println(currentLimit);
                Serial.println(stepper.currentPosition());
                RotateRelative(); //Run the function
                //example: P2000 400 - 2000 steps (5 revolution with 400 step/rev microstepping) and 400 steps/s speed
                //In theory, this movement should take 5 seconds
                break;

            case 'D':
                relative = false;
                directionMultiplier = -1; //We define the direction
                Serial.println("Bed Going Down."); //print the action
                currentIndex = 0;
                currentLimit -= stepArray[0];
                receivedSteps = maxSteps;
                receivedSpeed = speedDownArray[3];
                Serial.println(currentLimit);
                Serial.println(stepper.currentPosition());
                RotateRelative(); //Run the function
                //example: P2000 400 - 2000 steps (5 revolution with 400 step/rev microstepping) and 400 steps/s speed
                //In theory, this movement should take 5 seconds
                break;


            case 'P': //P uses the move() function of the AccelStepper library, which means that it moves relatively to the current position.              
                relative = true;
                receivedSteps = Serial.parseFloat(); //value for the steps
                receivedSpeed = Serial.parseFloat(); //value for the speed
                directionMultiplier = 1; //We define the direction
                Serial.println("Positive direction."); //print the action
                RotateRelative(); //Run the function
 
                //example: P2000 400 - 2000 steps (5 revolution with 400 step/rev microstepping) and 400 steps/s speed
                //In theory, this movement should take 5 seconds
                break;         
 
            case 'N': //N uses the move() function of the AccelStepper library, which means that it moves relatively to the current position.      
                relative = true;
                receivedSteps = Serial.parseFloat(); //value for the steps
                receivedSpeed = Serial.parseFloat(); //value for the speed 
                directionMultiplier = -1; //We define the direction
                Serial.println("Negative direction."); //print action
                RotateRelative(); //Run the function
 
                //example: N2000 400 - 2000 steps (5 revolution with 400 step/rev microstepping) and 500 steps/s speed; will rotate in the other direction
                //In theory, this movement should take 5 seconds
                break;
 
            case 'S': // Stops the motor
               
                stepper.stop(); //stop motor
                stepper.disableOutputs(); //disable power
                Serial.println("Stopped."); //print action
                runallowed = false; //disable running
                digitalWrite(ACTUATOR_PIN, HIGH);
                break;

            case 'p': // Pauses the motor
               
                Serial.println("Paused."); //print action
                runallowed = false; //disable running
                digitalWrite(ACTUATOR_PIN, HIGH);
                break;

            case 'r': // Runs the paused program
               
                Serial.println("Unpaused."); //print action
                runallowed = true; //disable running
                digitalWrite(ACTUATOR_PIN, LOW);
                break;
 
            case 'A': // Updates acceleration
 
                runallowed = false; //we still keep running disabled, since we just update a variable
                stepper.disableOutputs(); //disable power
                receivedAcceleration = Serial.parseFloat(); //receive the acceleration from serial
                stepper.setAcceleration(receivedAcceleration); //update the value of the variable
                Serial.print("New acceleration value: "); //confirm update by message
                Serial.println(receivedAcceleration); //confirm update by message
                break;
 
            case 'u': // Updates position
                stepper.setCurrentPosition(Serial.parseFloat());
                Serial.println(stepper.currentPosition());

                break;

            case 'g': // Gets position
                Serial.println(stepper.currentPosition());

                break;

            case 'd': //disable actuator
                disableActuators = true;
                
                break;
            case 'e': //enable actuator
                disableActuators = false;

                break;
            default:  

                break;
            }
        }
        //after we went through the above tasks, newData is set to false again, so we are ready to receive new commands again.
        newData = false;       
    }
}
 
void RotateRelative()
{
    //We move X steps from the current position of the stepper motor in a given direction.
    //The direction is determined by the multiplier (+1 or -1)
   
    if (directionMultiplier > 0)
    {
        digitalWrite(DIRECTION_PIN, LOW);
    }
    else
    {
        digitalWrite(DIRECTION_PIN, HIGH);
    }
    
    if (!disableActuators)
    {
        digitalWrite(ACTUATOR_PIN, LOW);
    }

    runallowed = true; //allow running - this allows entering the RunTheMotor() function.
    stepper.setMaxSpeed(receivedSpeed); //set speed
    stepper.move(directionMultiplier * receivedSteps); //set relative distance and direction
}
 
void PrintCommands()
{  
    //Printing the commands
    Serial.println(" 'U' : Move the bed up, running the command sequences.");
    Serial.println(" 'D' : Move the bed down, running the command sequences.");
    Serial.println(" 'C' : Prints all the commands and their functions.");
    Serial.println(" 'P' : Rotates the motor in positive (CW) direction, relative.");
    Serial.println(" 'N' : Rotates the motor in negative (CCW) direction, relative.");
    Serial.println(" 'S' : Stops the motor immediately without the ability to resume."); 
    Serial.println(" 'A' : Sets an acceleration value."); 
    Serial.println(" 'u' : Updates the position the arduino keeps track of, of where the stepper motors are.");
    Serial.println(" 'g' : Gets the position the arduino keeps track of, of where the stepper motors are.");
    Serial.println(" 'e' : Enables linear actuators.");
    Serial.println(" 'd' : Disables linear actuators.");
    Serial.println(" 'p' : Pauses the up or down program in it's current position and stops everything immediately.");
    Serial.println(" 'r' : Resumes the up or down program in it's current position immediately.");
}
