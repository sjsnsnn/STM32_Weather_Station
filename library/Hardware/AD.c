 #include "stm32f10x.h"                  // 引入标准库，下面所有函数都靠它
 #include "AD.h"
 
 volatile uint16_t ADC_Values[ADC_CHANNEL_COUNT];
 
  void AD_Init(void)
  {
      /* ===== 第 1 步：开时钟 ===== */

      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

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

      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
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

               /*配置 DMA =====
       *
       * 这是全新的部分！DMA 是"搬运工"，你要告诉它：
       *   - 从哪里搬？（源地址 = ADC 数据寄存器）
       *   - 搬到哪里？（目的地址 = ADC_Values 数组）
       *   - 搬多少个？（2 个，对应两个通道）
       *   - 搬完怎么办？（从头再来，永不停歇）
       */

      DMA_InitTypeDef DMA_InitStructure;

      DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
      // 告诉 DMA "从 ADC1 的数据寄存器取数据"。
      // 注意取地址（&），DMA 需要知道"从哪里搬"。

      DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADC_Values;
      // 告诉 DMA "搬到 ADC_Values 数组里"。
      // 数组名本身就是首地址，不需要加 &。

      DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
      // 搬运方向：外设是"源头"，内存是"目的地"。从 ADC →到数组。

      DMA_InitStructure.DMA_BufferSize = ADC_CHANNEL_COUNT;
      // 缓冲区大小 = 2。每次搬运 2 个数据。

      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      // 外设地址不递增——来源始终是同一 ADC1->DR。

      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      // 内存地址要递增！第一个放 [0]，第二个放 [1]。
      // ★忘了开这个 →两个通道的数据都堆到 [0]，[1] 永远是 0！

      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
      DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
      // 每次搬 16 位（半字）。ADC 结果 12 位存在 16 位寄存器里，数组也是 uint16_t。

      DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
      // ★循环模式——搬 2 个后从头再来，永不停。
      // 这就是"连续采集"的关键！DMA 搬完 [0] 和 [1] 后自动回到 [0] 重新写。
      // 如果设成 DMA_Mode_Normal，搬完一轮就"下班"了，数组不再更新。

      DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
      // 优先级中等。只用了 1 个 DMA 通道，填什么都行。

      DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
      // 关闭"内存到内存"模式。我们要的是"外设到内存"。

      DMA_Init(DMA1_Channel1, &DMA_InitStructure);
      // 提交给 DMA1 的通道 1。
      // 为什么是通道 1？STM32 硬件规定了 ADC1 必须用 DMA1_Channel1。

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

      ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
      // 大白话：关闭连续转换。每次手动触发只转换一次。
      // 如果开启（ENABLE），ADC 会不停地转，转完一次马上开始下一次。
      // 单次模式更省资源，我们只需要的时候才读。

      ADC_InitStructure.ADC_ScanConvMode = ENABLE;
      // 大白话：关闭扫描模式。每次只转换一个通道。
      // 扫描模式是一次触发就把所有通道都转一遍。
      // 我们只有一个光敏传感器接在通道 0，不需要扫描。

      ADC_InitStructure.ADC_NbrOfChannel = ADC_CHANNEL_COUNT;
      // 大白话：告诉 ADC "我要用 1 个通道"。
      // 扫描模式下这个值才有意义，但我们还是老老实实填上。

      ADC_Init(ADC1, &ADC_InitStructure);
      // 大白话：把配置单提交给 ADC1，让它生效。
	  
	   ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
      // 通道 0（PA0 / 光敏），排第 1 个采集。
      // 结果 →ADC_Values[0]

      ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_55Cycles5);
      // 通道 1（PA1 / NTC），排第 2 个采集。
      // 结果 →ADC_Values[1]

      ADC_DMACmd(ADC1, ENABLE);

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
	  
	  DMA_Cmd(DMA1_Channel1, ENABLE);
      // DMA 正式上班，等待 ADC 的通知来搬数据。

      ADC_SoftwareStartConvCmd(ADC1, ENABLE);
      // 软件触发第一次转换。
      // 因为开了连续模式，ADC 转完这一次后会自动开始下一次。
      // 你只需要触发这一次，后面全是自动的。
  }
