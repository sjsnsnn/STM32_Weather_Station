 #include "stm32f10x.h"                  // 引入标准库，下面所有函数都靠它

  void AD_Init(void)
  {
      /* ===== 第 1 步：开时钟 ===== */

      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
      // 大白话：给 GPIOA 和 ADC1 这两个"部门"通电。
      // STM32 为了省电，所有外设出厂默认是关着的。
      // 你不手动开，引脚和 ADC 就是一块砖——什么都不会响应。
      // GPIOA 和 ADC1 都挂在 APB2 总线上，所以用 RCC_APB2PeriphClockCmd。

      RCC_ADCCLKConfig(RCC_PCLK2_Div6);
      // 大白话：给 ADC 设一个合适的"心跳速度"。
      // 系统时钟是 72MHz，但 ADC 最快只能跑 14MHz，跑太快就不准了。
      // 72MHz ÷6 = 12MHz，在 14MHz 以内，安全！
      // 如果你忘了这行，ADC 时钟默认是 72MHz ÷2 = 36MHz →超频 →读数乱跳。

      /* ===== 第 2 步：配置 GPIO 为模拟输入 ===== */

      GPIO_InitTypeDef GPIO_InitStructure;
      // 大白话：声明一个"配置单"结构体，往里面填参数，待会交给库函数去执行。
      // 江协风格：所有 GPIO 配置都是先填结构体，再调 GPIO_Init。

      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
      // 大白话：把引脚设成"模拟输入"模式。
      // 这一步非常关键！模拟输入会断开引脚上的数字电路（上拉/下拉电阻等），
      // 让外部电压信号"直通"到 ADC 内部。
      // 如果你忘了设成 AIN，设成了浮空输入之类的——数字电路会干扰电压，
      // 读出来的值会飘，可能满屏乱跳。

      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
      // 大白话：选定 PA0 这个引脚。
      // PA0 对应的就是 ADC 通道 0（ADC_Channel_0）。
      // 如果光敏接在 PA1，这里就改成 GPIO_Pin_1，后面通道号改成 1。

      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      // 大白话：设置引脚的翻转速度。
      // 对于输入模式，这个参数其实没有实际作用，但库函数要求你必须填一个值。
      // 随便给个 50MHz 就行，不影响功能。

      GPIO_Init(GPIOA, &GPIO_InitStructure);
      // 大白话：把刚才填好的"配置单"提交给 GPIOA，让它生效。
      // 这一行执行完，PA0 就正式变成模拟输入了。

      /* ===== 第 3 步：配置 ADC ===== */

      ADC_InitTypeDef ADC_InitStructure;
      // 大白话：再声明一个 ADC 专用的"配置单"。

      ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
      // 大白话：ADC1 独立工作，不和 ADC2 联动。
      // STM32F103 有 ADC1 和 ADC2 两个，它们可以配对使用（双模式），
      // 但我们只用一个，所以设成独立模式。

      ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
      // 大白话：转换结果"右对齐"。
      // ADC 结果是 12 位，但寄存器是 16 位的。右对齐的意思是：
      //   16 位寄存器: [0][0][0][0][D11][D10]...[D1][D0]
      // 高 4 位补 0，低 12 位放数据。这样读出来直接就是 0~4095，不用移位。
      // 如果左对齐，读出来是 0~65520，还得自己右移 4 位，麻烦。

      ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
      // 大白话：不用外部触发，我们用软件手动触发。
      // "外部触发"是指用定时器之类的东西自动启动转换。
      // 我们是初学者，先学最简单的方式——想读的时候，代码里手动喊一声"开始！"

      ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
      // 大白话：关闭连续转换。每次手动触发只转换一次。
      // 如果开启（ENABLE），ADC 会不停地转，转完一次马上开始下一次。
      // 单次模式更省资源，我们只需要的时候才读。

      ADC_InitStructure.ADC_ScanConvMode = DISABLE;
      // 大白话：关闭扫描模式。每次只转换一个通道。
      // 扫描模式是一次触发就把所有通道都转一遍。
      // 我们只有一个光敏传感器接在通道 0，不需要扫描。

      ADC_InitStructure.ADC_NbrOfChannel = 1;
      // 大白话：告诉 ADC "我要用 1 个通道"。
      // 扫描模式下这个值才有意义，但我们还是老老实实填上。

      ADC_Init(ADC1, &ADC_InitStructure);
      // 大白话：把配置单提交给 ADC1，让它生效。

      /* ===== 第 4 步：开启 ADC 并校准 ===== */

      ADC_Cmd(ADC1, ENABLE);
      // 大白话：正式启动 ADC1。
      // 前面的配置只是"设置好了"，这一行才是"按下电源开关"。
      // 注意：ADC 上电后必须做一次校准，否则第一次读数不准。

      ADC_ResetCalibration(ADC1);
      // 大白话：复位校准寄存器，恢复到出厂状态。
      // 这是校准的"前置动作"——先清零，再重新校准。

      while (ADC_GetResetCalibrationStatus(ADC1));
      // 大白话：等待复位完成。
      // 这是一个死循环——只要复位还没完成，就一直等。
      // 通常只需要几个时钟周期，肉眼感觉不到等待。

      ADC_StartCalibration(ADC1);
      // 大白话：开始校准！ADC 会自动测量内部的偏差值，并补偿掉。
      // 为什么要校准？芯片制造有误差，就像体重秤出厂时可能偏重 0.5kg。
      // 校准就是让 ADC "站上空秤"，记住这个偏差，以后每次读数自动减掉它。

      while (ADC_GetCalibrationStatus(ADC1));
      // 大白话：等待校准完成。同样的死循环等待。
      // 校准完成后，ADC 就可以精确地把电压转换成数字了。
  }
  
  uint16_t AD_GetValue(uint8_t ADC_Channel)
  {
      /* ===== 第 1 小步：选择通道 ===== */

      ADC_RegularChannelConfig(ADC1, ADC_Channel, 1,ADC_SampleTime_55Cycles5);
      // 大白话：告诉 ADC "我要读哪个通道"，顺便设一下采样时间。
      //
      // 参数拆解：
      //   ADC1             →用 ADC1 这个模块
      //   ADC_Channel      →你传进来的通道号（PA0 = 0，PA1 = 1 ...）
      //   ADC_SampleTime_55Cycles5 →采样时间设为 55.5 个时钟周期
      //
      // 什么是"采样时间"？
      //   ADC 不是瞬间完成转换的，它需要一小段时间去"感受"电压。
      //   采样时间越长，读数越稳定，但速度越慢。
      //   55.5 个周期 ≈55.5 ÷12MHz ≈4.6μs，对于光敏传感器绰绰有余。
      //   如果你用最快的 1.5 个周期，读数可能会抖；初学者先用长的，稳。

      /* ===== 第 2 小步：启动转换 ===== */

      ADC_SoftwareStartConvCmd(ADC1, ENABLE);
      // 大白话：软件喊一嗓子"开始转换！"
      //
      // 因为我们之前配了"软件触发"，所以转换不是自动开始的，
      // 必须由你手动调用这行代码，ADC 才开始把当前电压变成数字。
      // 你可以把它理解成——按了一下快门，开始拍照。

      /* ===== 第 3 小步：等待转换完成 ===== */

      while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
      // 大白话：死循环等着，直到 ADC 说"我转完了"。
      //
      // ADC_FLAG_EOC = End Of Conversion（转换结束标志）
      //   - 转换进行中 →这个标志是 RESET（0）→while 条件成立 →继续等
      //   - 转换完成   →这个标志变成 SET（1）→while 条件不成立 →跳出循环
      //
      // 等待时间有多长？
      //   采样时间(55.5周期) + 转换时间(12.5周期) = 68 个周期
      //   68 ÷12MHz ≈5.67μs→一百万分之五秒，你完全感觉不到。

      /* ===== 第 4 小步：读取结果 ===== */

      return ADC_GetConversionValue(ADC1);
      // 大白话：从 ADC 的数据寄存器里，把转换好的数字拿出来，返回给调用者。
      //
      // 因为之前我们设了"右对齐"，所以这里拿到的就是 0~4095 的整数。
      // 而且读这个寄存器的同时，硬件会自动把 EOC 标志清零——
      // 下次再启动转换时，不用你手动清标志，省一步。
  }
	
