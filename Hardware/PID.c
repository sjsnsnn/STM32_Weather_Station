#include "stm32f10x.h"
#include "PID.h"

/* 把 x 限制在 [lo, hi] 区间内 */
static float PID_Clamp(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/*
 * 初始化 PID 控制器
 *
 * 积分限幅按输出范围反推：
 *   积分项的最终贡献是 ki * integral，要让它不超过输出上下限，
 *   所以 integral 的允许范围取 out_min/ki ~ out_max/ki。
 *   ki = 0 时不做积分，限幅设为 0。
 */
void PID_Init(PID_t *pid,
              float kp, float ki, float kd,
              float out_min, float out_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->integral      = 0.0f;
    pid->prev_measured = 0.0f;
    pid->d_filtered    = 0.0f;
    pid->initialized   = 0;

    pid->out_min = out_min;
    pid->out_max = out_max;

    if (ki != 0.000001f)
    {
        pid->integral_min = out_min / ki;
        pid->integral_max = out_max / ki;
    }
    else
    {
        pid->integral_min = 0.0f;
        pid->integral_max = 0.0f;
    }

    pid->deadband = 0.0f;   /* 默认无死区，由调用者按需设置 */
    pid->d_alpha  = 0.0f;   /* 0 = 微分不滤波 */
}

/* 清空运行状态，保留参数和限幅设置 */
void PID_Reset(PID_t *pid)
{
    pid->integral      = 0.0f;
    pid->prev_measured = 0.0f;
    pid->d_filtered    = 0.0f;
    pid->initialized   = 0;
}

/*
 * 位置式 PID + 微分先行 + 积分限幅
 *
 *   u = kp*e + ki*∫e dt + kd*d(测量值)/dt
 *
 * 微分作用在"测量值"而不是"误差"上：
 *   设定值突变时（比如上位机把目标转速从 100 改到 300），
 *   如果对误差求导，会得到一个巨大的微分尖峰；
 *   对测量值求导则不会，冲击更小。
 *
 * 输出饱和时不再累加积分（条件积分），防止积分饱和导致退饱和迟缓。
 */
float PID_Update(PID_t *pid,
                 float setpoint,
                 float measured,
                 float dt)
{
     float error;
      float p_term, i_term, d_out, out;
      float d_meas;
      uint8_t can_integrate;

    if (dt <= 0.0f)
    {
        return 0.0f;
    }

    error = setpoint - measured;

     /* 死区：误差在 ±deadband内当作 0 */
      if (error < pid->deadband && error > -pid->deadband)
          error = 0.0f;
	  
	  /* ---- 微分项：对测量值求导（微分先行） ---- */
      if (pid->initialized)
      {
          d_meas = (measured - pid->prev_measured) / dt;

          /* 目标不变时，误差导数 = -测量值导数 */
          d_meas = -d_meas;

          /* 一阶低通：d_alpha 越大越平滑，但相位滞后越大 */
          pid->d_filtered = pid->d_alpha * pid->d_filtered
                          + (1.0f - pid->d_alpha) * d_meas;
      }
      else
      {
          pid->d_filtered  = 0.0f;
          pid->initialized = 1;
      }
      pid->prev_measured = measured;
	  
	  /* ---- 三项合成 ---- */
      p_term = pid->kp * error;
      i_term = pid->ki * pid->integral;
      d_out  = pid->kd * pid->d_filtered;
      out    = p_term + i_term + d_out;
	  
	/* ---- 条件积分（抗积分饱和） ----
         输出已顶到上限、误差还要求更大 -> 不积分；
         输出已顶到下限、误差还要求更小 -> 不积分；
         反向误差时允许积分，用来“退饱和” */
      can_integrate = (uint8_t)(
          ((out < pid->out_max) || (error < 0.0f)) &&
          ((out > pid->out_min) || (error > 0.0f))
      );
	  
	  if(can_integrate)
	  {
		 pid->integral += error * dt;
		  pid->integral = PID_Clamp(pid->integral,
                                    pid->integral_min,
                                    pid->integral_max);

          i_term = pid->ki * pid->integral;
          out    = p_term + i_term + d_out;
	  }
	  
	  return PID_Clamp(out, pid->out_min, pid->out_max);
}
