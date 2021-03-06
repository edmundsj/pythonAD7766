#include "ArduinoDAQ.h"
#include "Arduino.h"
#include "AD7766.h"
#include "StepperMotor.h"
#include <scpiparser.h>

StepperMotor motor;
IntervalTimer motorTimer;

scpi_error_t identify(struct scpi_parser_context* context, struct scpi_token* command)
{
  Serial.println("OIC,Embedded SCPI Example,1,10");
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t resetDevice(struct scpi_parser_context* context, struct scpi_token* command)
{
  detachInterrupt(digitalPinToInterrupt(DATA_READY_PIN));
  detachInterrupt(digitalPinToInterrupt(EXTERNAL_SYNC_PIN));
  AD7766::Reset();
  motor.Reset();
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t measure(struct scpi_parser_context* context, struct scpi_token* command)
{
  AD7766::synchronizationCounter = 0;
  AD7766::dataCounter = 0;
  Serial.print("#");
  attachInterrupt(digitalPinToInterrupt(DATA_READY_PIN), AD7766::Sample, FALLING);
  attachInterrupt(digitalPinToInterrupt(EXTERNAL_SYNC_PIN), AD7766::recordSync, RISING);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t configure(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args = command;
  struct scpi_numeric commandData;
  //unsigned char output_value;

  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }

  commandData = scpi_parse_numeric(args->value, args->length, 1, 1, 25e6);
  AD7766::dataPointsToSample = (unsigned long) (commandData.value);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t sendSyncNumPoints(struct scpi_parser_context* context, struct scpi_token* command)
{
  Serial.println(AD7766::synchronizationCounter);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

/* Send the raw synchronization data - the measurements at which we have a L->H transition */
scpi_error_t sendSyncData(struct scpi_parser_context* context, struct scpi_token* command)
{
  Serial.print('#');
  for(int i = 0; i < AD7766::synchronizationCounter; i++) {
    Serial.write(AD7766::synchronizationData[i] >> 16); // MSB first
    Serial.write(AD7766::synchronizationData[i] >> 8);
    Serial.write(AD7766::synchronizationData[i]); // LSB last. Only send 24 bits, should be enough for even 100s of data.
  }
  
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t setPosition(struct scpi_parser_context* context, struct scpi_token* command)
{
  struct scpi_token* args = command;
  struct scpi_numeric commandData;
  
  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }
  commandData = scpi_parse_numeric(args->value, args->length, 0, 0, 10e6);
  motor.motorPosition = (uint32_t) (commandData.value);
  
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t queryPosition(struct scpi_parser_context* context, struct scpi_token* command)
{
  Serial.println(motor.motorPosition);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t queryDirection(struct scpi_parser_context* context, struct scpi_token* command)
{
  Serial.println(motor.motorDirection);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t rotateMotor(struct scpi_parser_context* context, struct scpi_token* command)
{
  // do some stuff with the motor controller
  struct scpi_token* args = command;
  struct scpi_numeric commandData;
  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }
  
  commandData = scpi_parse_numeric(args->value, args->length, 0, -1e6, 1e6);
  int motorSteps = int (commandData.value);
  
  motor.beginRotation(motorSteps);
  motorTimer.begin(interruptRotate, 1000*motor.motorPeriod);
  
  //motor.Rotate(motorSteps);
  
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t disableMotor(struct scpi_parser_context* context, struct scpi_token* command)
{
  motor.Disable();
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t enableMotor(struct scpi_parser_context* context, struct scpi_token* command)
{
  motor.Enable();
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t setMotorPeriod(struct scpi_parser_context* context, struct scpi_token* command)
{
  // do some stuff with the motor controller
  struct scpi_token* args = command;
  struct scpi_numeric commandData;
  while(args != NULL && args->type == 0)
  {
    args = args->next;
  }
  
  commandData = scpi_parse_numeric(args->value, args->length, 0, 1, 1e6);
  uint32_t motorPeriod = uint32_t (commandData.value);
  
  motor.motorPeriod = motorPeriod;
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t getMotorPeriod(struct scpi_parser_context* context, struct scpi_token* command)
{
  Serial.println(motor.motorPeriod);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t getMotorEnabled(struct scpi_parser_context* context, struct scpi_token* command)
{
  Serial.println(motor.motorEnabled);
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

scpi_error_t getMotorRotating(struct scpi_parser_context* context, struct scpi_token* command)
{
  Serial.println(int(motor.motorRotating));
  scpi_free_tokens(command);
  return SCPI_SUCCESS;
}

void interruptRotate(void) {
  motor.Rotate();

  // If we are out of steps or we get the command to stop rotating by having the motor disabled
  if(motor.stepsRemaining <= 0 || motor.motorEnabled == false) {
    motorTimer.end();
    motor.motorRotating = false;
  }
}
