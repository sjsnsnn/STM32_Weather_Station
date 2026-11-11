 #ifndef __MOTOR_H
 #define __MOTOR_H

 #define MOTOR_MAX_RPM   425.0f   /* 5V 实测 100% 空载约 425 RPM */
  void Motor_Init(void);
  void Motor_SetSpeed(int8_t Speed);

 #endif
