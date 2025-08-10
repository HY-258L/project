#include "LDChip.h"
#include "spi.h"
#include "usart.h"
#include <string.h>
/************************************************************************************
//	nAsrStatus 用来在main主程序中表示程序运行的状态，不是LD3320芯片内部的状态寄存器
//	LD_ASR_NONE:			表示没有在作ASR识别
//	LD_ASR_RUNING：		表示LD3320正在作ASR识别中
//	LD_ASR_FOUNDOK:		表示一次识别流程结束后，有一个识别结果
//	LD_ASR_FOUNDZERO:	表示一次识别流程结束后，没有识别结果
//	LD_ASR_ERROR:			表示一次识别流程中LD3320芯片内部出现不正确的状态
*********************************************************************************/
uint8 nAsrStatus = 0;
uint8 nLD_Mode = LD_MODE_IDLE; //用来记录当前是在进行ASR识别还是在播放MP3
uint8 ucRegVal;
extern int MP3_reg;




















void JQ_Send(uint8_t Play_num){
    unsigned char cmd[6] = {0x7E, 0x04, 0x03, 0x00, 0x01, 0xEF};

    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "sending: %02X, %02X, %02X, %02X, %02X, %02X\r\n",
             cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

    for (uint8_t i = 0; i < 6; i++){
    	HAL_Delay(150);
        UART_Send(cmd[i]);
    }
}

void UART_Send(unsigned char ch){
    uint8_t rx_byte;
    HAL_UART_Transmit(&huart3, &ch, strlen(ch), HAL_MAX_DELAY);
    if (HAL_UART_Receive(&huart3, &rx_byte, 1, 100) == HAL_OK) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"播放中\r\n",
                          strlen("播放中\r\n"), HAL_MAX_DELAY);
    } else {
        HAL_UART_Transmit(&huart1, (uint8_t*)"未收到响应\r\n",
                          strlen("未收到响应\r\n"), HAL_MAX_DELAY);
    }
}


















static void LD_Delayms(uint16 i)
{
	HAL_Delay(i);
}

static void LD3320_delay(unsigned long uldata)
{
	HAL_Delay(uldata);
}

uint8_t Spi_RW_Data(uint8_t *p, uint8_t len)
{
	uint8_t pdata[len];
	HAL_SPI_TransmitReceive(&hspi1, p, pdata, len, 0XFFFF);
	return pdata[len - 1];
}

static void LD_WriteReg(uint8 data1, uint8 data2)
{
	uint8_t tx[3];
	LD_SPIS_L();

	LD_CS_L();
	tx[0] = 0x04;
	tx[1] = data1;
	tx[2] = data2;
	Spi_RW_Data(tx, 3);

	LD_CS_H();
}

static uint8 LD_ReadReg(uint8 reg_add)
{
	uint8 i;
	uint8_t tx[3];
	LD_SPIS_L();

	LD_CS_L();

	tx[0] = 0x05;
	tx[1] = reg_add;
	tx[2] = 0x00;
	i = Spi_RW_Data(tx, 3);
	LD_CS_H();
	return i;
}

static void LD_reset(void)
{
	LD_RST_H();
	LD3320_delay(100);
	LD_RST_L();
	LD3320_delay(100);
	LD_RST_H();
	LD3320_delay(100);
	LD_CS_L();
	LD3320_delay(100);
	LD_CS_H();
	LD3320_delay(100);
}

///用户修改
void LD3320_main(void)
{
	uint8 nAsrRes = 0;
    uint8_t i;
	LD_reset(); //复位LD3320
    char *code_s[]={"1、手电筒\r\n","2、汽车\r\n","3、红色\r\n","4、绿色\r\n","5、蓝色\r\n"};
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "%s, %s, %s, %s, %s\r\n",
             code_s[0], code_s[1], code_s[2], code_s[3], code_s[4]);

    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
    for(i=0;i<5;i++){
        printf("%s",code_s[i]);
    }

	nAsrStatus = LD_ASR_NONE; //初始状态：没有在作ASR     nAsrStatus   上面有定义         #define LD_ASR_NONE					0x00	//表示没有在作ASR识别
	LD_Delayms(1);
	while (1)
	{
		switch (nAsrStatus)
		{
		case LD_ASR_RUNING: //#define LD_ASR_RUNING				0x01	//表示LD3320正在作ASR识别中

		case LD_ASR_ERROR: //#define LD_ASR_ERROR	 			0x31	//	表示一次识别流程中LD3320芯片内部出现不正确的状态
			break;
		case LD_ASR_NONE:				//#define LD_ASR_NONE					0x00	//表示没有在作ASR识别
			nAsrStatus = LD_ASR_RUNING; //初始状态：没有在作ASR     nAsrStatus   上面有定义         #define LD_ASR_NONE					0x00	//表示没有在作ASR识别

			if (RunASR() == 0) //启动一次ASR识别流程：ASR初始化，ASR添加关键词语，启动ASR运算
			{
				nAsrStatus = LD_ASR_ERROR;
				HAL_UART_Transmit(&huart1, (uint8_t*)"RunASR LD_ASR_ERROR\r\n",
				  	                                   strlen("RunASR LD_ASR_ERROR\r\n"), HAL_MAX_DELAY);
				printf("RunASR LD_ASR_ERROR\r\n");
			}





	    	switch (MP3_reg){
	    		  	  				case 1:
	    		  	  					JQ_Send(0x01);
	    		  	  					MP3_reg = 0;
	    		  	  					break;
	    		  	  				case 2:
	    		  	  					JQ_Send(0x02);
	    		  	  					MP3_reg = 0;
	    		  	  					break;
	    		  	  				case 3:
	    		  	  					JQ_Send(0x03);
	    		  	  					MP3_reg = 0;
	    		  	  					break;
	    		  	  				case 4:
	    		  	  					JQ_Send(0x04);
	    		  	  					MP3_reg = 0;
	    		  	  					break;
	    		  	  				case 5:
	    		  	  					JQ_Send(0x05);
	    		  	  					MP3_reg = 0;
	    		  	  					break;
	    		  	  				default:
	    		  	  					MP3_reg = 0;
	    		  	  					break;
	        }














			HAL_UART_Transmit(&huart1, (uint8_t*)"开始识别\r\n",
							  	                       strlen("开始识别\r\n"), HAL_MAX_DELAY);
            printf("开始识别\r\n");
			break;
		case LD_ASR_FOUNDOK:
			nAsrRes = LD_GetResult(); //一次ASR识别流程结束，去取ASR识别结果
			HAL_UART_Transmit(&huart1, (uint8_t*)"识别码:%d\r\n",
							  	                       strlen("识别码:%d\r\n"), HAL_MAX_DELAY);
			printf("识别码:%d\r\n", nAsrRes);
			printf("%s\r\n",code_s[nAsrRes-1]);
			switch (nAsrRes) //对结果执行相关操作,客户修改
			{
				case CODE_1: //命令“流水灯”
					MP3_reg = 1;
					HAL_UART_Transmit(&huart1, (uint8_t*)"code 1\r\n",
												  	                       strlen("code 1\r\n"), HAL_MAX_DELAY);
					snprintf(buffer, sizeof(buffer),
					             "MP3_reg:%d\r\n",
								 MP3_reg);
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
					printf("code 1\r\n");
			//IO1=1;
					break;
				case CODE_2: //命令“闪烁”
					MP3_reg = 2;
					snprintf(buffer, sizeof(buffer),
					             "MP3_reg:%d\r\n",
								 MP3_reg);
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
					printf("code 2\r\n");
			// 	//IO1=0;x
					break;
				case CODE_3: //命令“按键触发”
					MP3_reg = 3;
					snprintf(buffer, sizeof(buffer),
					             "MP3_reg:%d\r\n",
								 MP3_reg);
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
					printf("code 3\r\n");
					break;
				case CODE_4: //命令“全灭”
					MP3_reg = 4;
					snprintf(buffer, sizeof(buffer),
					             "MP3_reg:%d\r\n",
								 MP3_reg);
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
					printf("code 4\r\n");
					break;
				case CODE_5: //命令“状态”
					MP3_reg = 5;
					snprintf(buffer, sizeof(buffer),
					             "MP3_reg:%d\r\n",
								 MP3_reg);
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
					printf("code 5\r\n");
					break;
				default:
					break;
			}
			nAsrStatus = LD_ASR_NONE;
			break;
		case LD_ASR_FOUNDZERO:
			printf("无法识别\r\n");
			HAL_UART_Transmit(&huart1, (uint8_t*)"无法识别\r\n",
										  	                       strlen("无法识别\r\n"), HAL_MAX_DELAY);
		default:
			nAsrStatus = LD_ASR_NONE;
			break;
		} //switch
	}	  // while
}

//ASR：DSP 忙闲状态
//0x21 表示闲，查询到为闲状态可以进行下一步 ASR 动作
uint8 LD_Check_ASRBusyFlag_b2(void)
{
	uint8_t j;
	uint8_t flag = 0;
	uint8_t sta;
	for (j = 0; j < 20; j++)
	{
		sta = 0;
		sta = LD_ReadReg(0xb2);
		if (sta == 0x21)
		{
			flag = 1;
			break;
		}
		LD3320_delay(10);
	}
	return flag;
}

static uint8 LD_AsrAddFixed(void)
{
	uint8 k, flag;
	uint8 nAsrAddLength;
#define DATE_A 5  //数组二维数值
#define DATE_B 20 //数组一维数值
	//添加关键词，用户修改
	uint8 sRecog[DATE_A][DATE_B] = {
		"shou dian tong",
		"qi che",
		"hong se",
		"lv se",
		"lan se"};
	uint8 pCode[DATE_A] = {
		CODE_1,
		CODE_2,
		CODE_3,
		CODE_4,
		CODE_5}; //添加识别码，用户修改
	flag = 1;
	for (k = 0; k < DATE_A; k++)
	{
		if (LD_Check_ASRBusyFlag_b2() == 0) //查询DSP 忙闲状态
		{
			HAL_UART_Transmit(&huart1, (uint8_t*)"LD_AsrAddFixed DSP busy K:%d\r\n",
										  	                       strlen("LD_AsrAddFixed DSP busy K:%d\r\n"), HAL_MAX_DELAY);
			printf("LD_AsrAddFixed DSP busy K:%d\r\n", k);
			flag = 0;
			break;
		}

		LD_WriteReg(0xc1, pCode[k]); //ASR：识别字 Index
		LD_WriteReg(0xc3, 0);		 //ASR：识别字添加时写入 00
		LD_WriteReg(0x08, 0x04);	 //清除 FIFO_EXT 内容（清除指定 FIFO 后再写入一次 00H）
		LD3320_delay(1);
		LD_WriteReg(0x08, 0x00);
		LD3320_delay(1);

		for (nAsrAddLength = 0; nAsrAddLength < DATE_B; nAsrAddLength++)
		{
			if (sRecog[k][nAsrAddLength] == 0)
				break;
			LD_WriteReg(0x5, sRecog[k][nAsrAddLength]); //FIFO_EXT 数据口
		}
		LD_WriteReg(0xb9, nAsrAddLength); //ASR：当前添加识别句的字符串长度（拼音字符串）
		LD_WriteReg(0xb2, 0xff);		  //ASR：DSP 忙闲状态 写入忙
		LD_WriteReg(0x37, 0x04);		  //语音识别控制命令下发寄存器 写 04H：通知 DSP 要添加一项识别句
	}
	return flag;
}

static uint8 LD_AsrRun(void)
{
	LD_WriteReg(0x35, MIC_VOL); //麦克风（MIC）音量 建议设置值为 40H-55H
	LD_WriteReg(0x1C, 0x09);	//ADC 开关控制
	LD_WriteReg(0xBD, 0x20);	//初始化控制寄存器
	LD_WriteReg(0x08, 0x01);	//清除 FIFO_DATA  清除指定 FIFO 后再写入一次 00H
	LD3320_delay(5);
	LD_WriteReg(0x08, 0x00);
	LD3320_delay(5);

	if (LD_Check_ASRBusyFlag_b2() == 0)
	{
		HAL_UART_Transmit(&huart1, (uint8_t*)"LD_AsrRun DSP busy\r\n",
												  	                       strlen("LD_AsrRun DSP busy\r\n"), HAL_MAX_DELAY);
		printf("LD_AsrRun DSP busy\r\n");
		return 0;
	}

	LD_WriteReg(0xB2, 0xff); //ASR：DSP 忙闲状态
	LD_WriteReg(0x37, 0x06); //写 06H：通知 DSP 开始识别语音
	LD_WriteReg(0x37, 0x06); //写 06H：通知 DSP 开始识别语音
	LD3320_delay(5);
	LD_WriteReg(0x1C, 0x0b); //写 0BH 麦克风输入 ADC 通道可用
	LD_WriteReg(0x29, 0x10); //中断允许（可读写）第 4 位：同步中断允许，1 表示允许
	LD_WriteReg(0xBD, 0x00); //初始化控制寄存器 写入 00H；然后启动；为 ASR 模块
	return 1;
}

static uint8 RunASR(void)
{
	uint8 i = 0;
	uint8 asrflag = 0;
	for (i = 0; i < 5; i++) //防止由于硬件原因导致LD3320芯片工作不正常，所以一共尝试5次启动ASR识别流程
	{
		LD_Init_ASR(); //初始化ASR
		LD3320_delay(100);
		if (LD_AsrAddFixed() == 0) //添加关键词语到LD3320芯片中
		{
			LD_reset(); //LD3320芯片内部出现不正常，立即重启LD3320芯片
			HAL_UART_Transmit(&huart1, (uint8_t*)"LD_AsrAddFixed\r\n",
													  	                       strlen("LD_AsrAddFixed\r\n"), HAL_MAX_DELAY);
			printf("LD_AsrAddFixed\r\n");
			LD3320_delay(50); //并从初始化开始重新ASR识别流程
			continue;
		}
		LD3320_delay(10);
		if (LD_AsrRun() == 0)
		{
			LD_reset(); //LD3320芯片内部出现不正常，立即重启LD3320芯片
			HAL_UART_Transmit(&huart1, (uint8_t*)"LD_AsrRun\r\n",
													  	                       strlen("LD_AsrRun\r\n"), HAL_MAX_DELAY);
			printf("LD_AsrRun\r\n");
			LD3320_delay(50); //并从初始化开始重新ASR识别流程
			continue;
		}
		asrflag = 1;
		break; //ASR流程启动成功，退出当前for循环。开始等待LD3320送出的中断信号
	}
	return asrflag;
}

//ASR：读取 ASR 结果（最佳）
static uint8 LD_GetResult(void)
{
	return LD_ReadReg(0xc5);
}

static void ProcessInt(void)
{
	uint8 nAsrResCount = 0;

	ucRegVal = LD_ReadReg(0x2B); //中断请求编号	第 4 位 表示语音识别有结果产生；MCU 可清零。第 2 位 表示芯片内部 FIFO 中断发生。

	// 语音识别产生的中断
	//（有声音输入，不论识别成功或失败都有中断）
	LD_WriteReg(0x29, 0); //中断允许（可读写）
	LD_WriteReg(0x02, 0); //FIFO 中断允许	第 0 位：允许 FIFO_DATA 中断；第 2 位：允许 FIFO_EXT 中断；

	if ((ucRegVal & 0x10) && LD_ReadReg(0xb2) == 0x21 && LD_ReadReg(0xbf) == 0x35) //判断语音识别有结果产生 && 读取DSP 忙闲状态 && 读取ASR 状态报告寄存器 0x35，可以确定是一次语音识别流程正常结束
	{
		nAsrResCount = LD_ReadReg(0xba); //读取中断辅助信息，语音识别有几个识别候选 N 个识别候选
		HAL_UART_Transmit(&huart1, (uint8_t*)"获取结果:\r\n",
															strlen("获取结果:\r\n"), HAL_MAX_DELAY);
		printf("获取结果:%d\r\n",nAsrResCount);
		if (nAsrResCount > 0 && nAsrResCount <= 4)
		{
			nAsrStatus = LD_ASR_FOUNDOK;
		}
		else //0 或者大于 4：没有识别候选
		{
			nAsrStatus = LD_ASR_FOUNDZERO;
		}
	}
	else if (ucRegVal & 0x08) //第 3 位：读取值为 1 表示芯片内部已经出现错误 需重启
	{
		HAL_UART_Transmit(&huart1, (uint8_t*)"芯片内部已经出现错误",
															strlen("芯片内部已经出现错误"), HAL_MAX_DELAY);
		printf("芯片内部已经出现错误");
		LD_reset(); //复位LD3320
		nAsrStatus = LD_ASR_NONE;
	}
	else
	{
		nAsrStatus = LD_ASR_FOUNDZERO; //执行没有识别
	}

	LD_WriteReg(0x2b, 0); //中断请求编号（可读写）
	LD_WriteReg(0x1C, 0); //写0:ADC不可用
	LD_WriteReg(0x29, 0); //清空中断允许
	LD_WriteReg(0x02, 0); //清空FIFO 中断允许
	LD_WriteReg(0x2B, 0); //清空中断请求编号（可读写）
	LD_WriteReg(0xBA, 0); //中断辅助信息，（读或设为 00）
	LD_WriteReg(0xBC, 0); //ASR：识别过程强制结束
	LD_WriteReg(0x08, 1); //清除FIFO_DATA
	LD_WriteReg(0x08, 0); //清除FIFO_DATA后 再次写0
}

void LD_IRQHandler(void)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)"听到声音 ",
													strlen("听到声音 "), HAL_MAX_DELAY);
	printf("听到声音 ");
	ProcessInt();
}

static void LD_Init_Common(void)
{
	uint8_t sta = 0;
	sta = LD_ReadReg(0x06); //FIFO 状态
	if (sta & 0x08){
		HAL_UART_Transmit(&huart1, (uint8_t*)"FIFO_DATA 已满，不能写\r\n",
														strlen("FIFO_DATA 已满，不能写\r\n"), HAL_MAX_DELAY);
		printf("FIFO_DATA 已满，不能写\r\n");
	}
	if (sta & 0x20){
		HAL_UART_Transmit(&huart1, (uint8_t*)"FIFO_EXT 已满，不能写\r\n",
														strlen("FIFO_EXT 已满，不能写\r\n"), HAL_MAX_DELAY);
		printf("FIFO_EXT 已满，不能写\r\n");
	}
	LD_WriteReg(0x17, 0x35); //写 35H 对 LD3320 进行软复位（Soft Reset）
	LD3320_delay(5);
	sta = LD_ReadReg(0x06); //FIFO 状态
	if (sta & 0x08){
		HAL_UART_Transmit(&huart1, (uint8_t*)"FIFO_DATA 已满，不能写\r\n",
														strlen("FIFO_DATA 已满，不能写\r\n"), HAL_MAX_DELAY);
		printf("FIFO_DATA 已满，不能写\r\n");
	}
	if (sta & 0x20){
		HAL_UART_Transmit(&huart1, (uint8_t*)"FIFO_EXT 已满，不能写\r\n",
														strlen("FIFO_EXT 已满，不能写\r\n"), HAL_MAX_DELAY);
		printf("FIFO_EXT 已满，不能写\r\n");
	}

	LD_WriteReg(0x89, 0x03); //模拟电路控制 初始化
	LD3320_delay(5);
	LD_WriteReg(0xCF, 0x43); //内部省电模式设置 初始化写入 43H
	LD3320_delay(5);
	LD_WriteReg(0xCB, 0x02);

	/*PLL setting*/
	LD_WriteReg(0x11, LD_PLL_11); //时钟频率设置 1
	if (nLD_Mode == LD_MODE_MP3)  //判断当前进行ASR识别还是在播放MP3
	{
		LD_WriteReg(0x1E, 0x00);
		LD_WriteReg(0x19, LD_PLL_MP3_19);
		LD_WriteReg(0x1B, LD_PLL_MP3_1B);
		LD_WriteReg(0x1D, LD_PLL_MP3_1D);
	}
	else
	{
		LD_WriteReg(0x1E, 0x00);		  //ADC 专用控制，应初始化为 00H
		LD_WriteReg(0x19, LD_PLL_ASR_19); //时钟频率设置 2
		LD_WriteReg(0x1B, LD_PLL_ASR_1B); //时钟频率设置 3
		LD_WriteReg(0x1D, LD_PLL_ASR_1D); //时钟频率设置 4
	}
	LD3320_delay(5);

	LD_WriteReg(0xCD, 0x04); //DSP 休眠设置,初始化时写入 04H 允许 DSP 休眠。
	LD_WriteReg(0x17, 0x4c); //写 4CH 可以使 DSP 休眠，比较省电。
	LD3320_delay(1);
	LD_WriteReg(0xB9, 0x00); //ASR：当前添加识别句的字符串长度（拼音字符串） 初始化时写入 00H
	LD_WriteReg(0xCF, 0x4F); //内部省电模式设置 MP3 初始化和 ASR 初始化时写入 4FH
	LD_WriteReg(0x6F, 0xFF); //对芯片进行初始化时设置为 0xFF
}

static void LD_Init_ASR(void)
{
	nLD_Mode = LD_MODE_ASR_RUN;
	LD_Init_Common();

	LD_WriteReg(0xBD, 0x00); //初始化控制寄存器 写入 00H；然后启动；为 ASR 模块；
	LD_WriteReg(0x17, 0x48); //写 48H 可以激活 DSP；
	LD3320_delay(5);
	LD_WriteReg(0x3C, 0x80); //FIFO_EXT 下限低 8 位（LowerBoundary L）
	LD_WriteReg(0x3E, 0x07); //FIFO_EXT 下限高 8 位（LowerBoundary H）
	LD_WriteReg(0x38, 0xff); //FIFO_EXT 上限低 8 位（UpperBoundary L）
	LD_WriteReg(0x3A, 0x07); //FIFO_EXT 上限高 8 位（UpperBoundary L）
	LD_WriteReg(0x40, 0);	 //FIFO_EXT MCU 水线低 8 位（MCU water mark L）
	LD_WriteReg(0x42, 8);	 //FIFO_EXT MCU 水线高 8 位（MCU water mark H）
	LD_WriteReg(0x44, 0);	 //FIFO_EXT DSP 水线低 8 位（DSP water mark L）
	LD_WriteReg(0x46, 8);	 //FIFO_EXT DSP 水线高 8 位（DSP water mark H）
	LD3320_delay(1);
}
///寄存器操作 end
/*********************************************END OF FILE**********************/
