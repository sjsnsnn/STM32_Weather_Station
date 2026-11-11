#ifndef __PID_H
#define __PID_H

#include <stdint.h>

typedef struct
  {
      /* 参数 */
      float kp;
      float ki;
      float kd;

      /* 状态 */
      float integral;        /* 积分累计 */
      float prev_measured;   /* 上一次反馈值 */
      float d_filtered;      /* 滤波后的微分项 */
      uint8_t initialized;   /* 第一次调用标志 */

      /* 限制 */
      float out_min;
      float out_max;
      float integral_min;
      float integral_max;
      float deadband;

      /* 微分滤波系数：0~1，越大越平滑 */
      float d_alpha;
  } PID_t;

  void PID_Init(PID_t *pid,
                float kp, float ki, float kd,
                float out_min, float out_max);

  void PID_Reset(PID_t *pid);

  float PID_Update(PID_t *pid,
                   float setpoint,
                   float measured,
                   float dt);

  #endif
