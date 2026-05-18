#include "IR.h"
#include "main.h"
#include "string.h"

static ir_data_t ir_data;
uint8_t try[ir_data_len]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0xA5,0x5A,0x0F,0xF0};
void IR_init(void)
{
    ir_data.cnt_tx = 0;
    ir_data.cnt_rx = 0;
    ir_data.ptr_tx = 0;
    ir_data.ptr_rx = 0;
    ir_data.flag_tx = IR_tx_sleep;
    ir_data.flag_rx = IR_rx_sleep;
    memset(ir_data.data_tx, 0, ir_data_len);
    memset(ir_data.data_rx, 0, ir_data_len);
}

void IR_rst(void){
	if(ir_data.cnt_rx>=20||ir_data.cnt_tx>=20){
		IR_init();
	}
}

void IR_data_upload(uint8_t *data){
	memcpy(ir_data.data_tx,data,ir_data_len);
	ir_data.flag_tx=IR_tx_transmiting;
}

void IR_send(void){
    if(ir_data.flag_tx==IR_tx_transmiting){
        if(ir_data.data_tx[ir_data.ptr_tx/8] & (1 << (ir_data.ptr_tx%8))){//send "1"
            if(ir_data.cnt_tx<ir_period_one){
                HAL_GPIO_WritePin(ir_output_port, ir_output_pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(ir_output_flag_port, ir_output_flag_pin, GPIO_PIN_SET);
            }
            else if(ir_data.cnt_tx<ir_period_one+ir_period_done){
                HAL_GPIO_WritePin(ir_output_port, ir_output_pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(ir_output_flag_port, ir_output_flag_pin, GPIO_PIN_RESET);
            }
            else{
                ir_data.cnt_tx=0;
                if(ir_data.ptr_tx<8*ir_data_len){  
                    ir_data.ptr_tx++;
                    IR_send();
                }
                else{   
                    ir_data.ptr_tx=0;
                    ir_data.flag_tx=IR_tx_done;
                }
            }
        }
        else{//send "0"
            if(ir_data.cnt_tx<ir_period_zero){
                HAL_GPIO_WritePin(ir_output_port, ir_output_pin, GPIO_PIN_SET);
								HAL_GPIO_WritePin(ir_output_flag_port, ir_output_flag_pin, GPIO_PIN_SET);
            }
            else if(ir_data.cnt_tx<ir_period_zero+ir_period_done){
                HAL_GPIO_WritePin(ir_output_port, ir_output_pin, GPIO_PIN_RESET);
								HAL_GPIO_WritePin(ir_output_flag_port, ir_output_flag_pin, GPIO_PIN_RESET);
            }
            else{
                ir_data.cnt_tx=0;
                if(ir_data.ptr_tx<8*ir_data_len){  
                    ir_data.ptr_tx++;
                    IR_send();
                }
                else{   
                    ir_data.ptr_tx=0;
                    ir_data.flag_tx=IR_tx_done;
                }
            }
        }
				ir_data.cnt_tx++;
    }
}

void IR_step(void){
    if(ir_data.flag_rx==IR_rx_receiving){
      ir_data.cnt_rx++;
    }
    IR_send();
}