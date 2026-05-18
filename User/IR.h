#ifndef __IR_H__
#define __IR_H__

#include <stdint.h>

#define low  0
#define high 1

#define ir_period_one     12
#define ir_period_zero    4
#define ir_period_done    4
/****************************
      以上是在.c文件会用到的妙妙工具
****************************/
#define ir_data_len 20//数据包长度

#define ir_input_port  GPIOA
#define ir_input_pin   GPIO_PIN_1

#define ir_output_port GPIOB
#define ir_output_pin  GPIO_PIN_9

#define ir_power_port GPIOB
#define ir_power_pin 	GPIO_PIN_10

#define ir_output_flag_port	GPIOA
#define ir_output_flag_pin	GPIO_PIN_5

#define ir_input_flag_port 	GPIOB
#define ir_input_flag_pin		GPIO_PIN_0
//  '1' for 12 period high of 72khz and 4 period low of 72khz
//  '0' for 4 period high of 72khz and 4 period low of 72khz

typedef enum{
    IR_rx_sleep = 0,
    IR_rx_receiving = 1,
    IR_rx_done = 2
}ir_rx_state_t;

typedef enum{
    IR_tx_sleep = 0,
    IR_tx_transmiting = 1,
    IR_tx_done = 2
}ir_tx_state_t;

typedef struct{
    uint8_t data_tx[ir_data_len];
    uint8_t data_rx[ir_data_len];
    uint8_t ptr_tx;
    uint8_t ptr_rx;
    uint8_t cnt_tx;
    uint8_t cnt_rx;
    uint8_t flag_tx;
    uint8_t flag_rx;
}ir_data_t;

void IR_init(void);
void IR_send(void);
void IR_receive(void);
void IR_rst(void);
void IR_data_upload(uint8_t *data);
void IR_step(void);			 
#endif
