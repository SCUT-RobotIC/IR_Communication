### 基于STM32F103CBT6的红外模块
##### author：DrSabi
##### date：2026/05/17
##### version：1.0
##### 所有加粗均为本人踩坑总结，望后人哀之且鉴之

#### 协议设计：
<center>

![协议](/User/image/logic.jpg)
Figure 0 红外通信协议设计

</center>

当接收到1120us的高电平即为逻辑‘0’，接收到3360us的高电平即为逻辑‘1’

#### 硬件：
<center>

![发射](/User/image/tx_schematic.png)
Figure 1 红外发射模块的schematic

</center>

发射部分使用一个IO口驱动一颗MOS控制二极管开关
**图中二极管画反是因为嘉立创的封装反，绘图时务必注意采购的红外二极管散热焊盘为负极，而嘉立创封装为正极**
二极管的参数十分模糊，官方手册几乎为零，测试参数大致为：
* 工作电流If=1A时压降为Vf=1.5V
* 工作电流If=0.5A时压降Vf=1.4V
此处R6为功率电阻，工作占空比一般≤50%，功率至少3W，实践中可以考虑用焊接并联的形式提升电阻耐流能力，同时加强散热设计。实测在使用铜散热片时连续发送1min后温度会升至80℃，**因此软件上需要做到不长时间连续发送**。

<center>

![接收](/User/image/rx_schematic.png)
Figure 2 红外接收模块的schematic

</center>

接收部分使用红外接收管(非成品)，接收管到红外信号时会逐渐导通，通过三极管的放大从而实现电平变化输入IO口。
**采购时务必注意接收管的频率要和发射管一致**

#### 软件：
* 打开/User/IR.c&IR.h
##### IR.h文件：
宏定义部分定义了GPIO口和引脚，以及：
<html>
   <head>

      #define low  0
      #define high 1

      #define ir_period_one     12
      #define ir_period_zero    4
      #define ir_period_done    4
      /****************************
      以上是在.c文件会用到的妙妙工具
      ****************************/
      #define ir_data_len 20//数据包长度

   </head>
</html>
定义一个结构体存放红外的数据、状态、指针等信息：
<html>
   <head>

      typedef struct{
         uint8_t data_tx[ir_data_len];//存放发送数据
         uint8_t data_rx[ir_data_len];//存放接收数据
         uint8_t ptr_tx;//发送数据指针
         uint8_t ptr_rx;//接收数据指针
         uint8_t cnt_tx;//发送数据计数
         uint8_t cnt_rx;//接收数据计数
         uint8_t flag_tx;//发送状态机
         uint8_t flag_rx;//接收状态机
      }ir_data_t;

   </head>
</html>

对于状态机的状态，我们进行如下定义，未进行-0，进行中-1，完成-2：
<html>
   <head>

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

   </head>
</html>

##### .c文件：
进行基础的include，定义一个static的全局变量存放红外数据：
<html>
   <head>

      static ir_data_t ir_data;

   </head>
</html>

初始化函数，该置零的置零：
<html>
   <head>

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

   </head>  
</html>

重置函数，当ir_data结构体中数据不合法时重新初始化
<html>
   <head>

      void IR_rst(void){
         if(ir_data.cnt_rx>=20||ir_data.cnt_tx>=20){
            IR_init();
         }
      }

   </head>
</html>

数据更新函数，将给定数据更新到结构体中然后将发送状态机置为进行中
**不调用此函数会使flag_tx标志位保持0或2，从而不进行发送，代码中将此函数放在了时钟中断回调函数中，发送完就调用此函数进行数据更新，实现连续发送**

<html>
   <head>

      void IR_update(uint8_t* data){
         memcpy(ir_data.data_tx, data, ir_data_len);
         ir_data.flag_tx = IR_tx_transmiting;
      }

   </head>  
</html>

发送函数，其中cnt_tx会在时钟中断回调函数中进行 ++ 操作：

<center>

![发送逻辑](/User/image/logictx.drawio.png)
Figure 3 红外发送逻辑

</center>

<html>
   <head>

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
      
   </head>
</html>

##### main.c文件：

<center>

![接收逻辑](/User/image/logicrx.drawio.png)
Figure 4 红外接收逻辑

</center>

接收到高电平开始cnt_rx计时，低电平结束计时，然后将cnt_rx和ir_period_one/zero±1进行比较，**符合则写入0/1，不符合就丢弃该bit**，不论符合与否均进入下一位，直到接收完成或数据不合法重置
<html>
   <head>

      void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
         if (GPIO_Pin == GPIO_PIN_1) {
            if(HAL_GPIO_ReadPin(ir_input_port, ir_input_pin) == GPIO_PIN_RESET){
                        HAL_GPIO_WritePin(ir_input_flag_port,ir_input_flag_pin,1);//指示灯
                  if(ir_data.cnt_rx >= ir_period_one-1 & ir_data.cnt_rx <= ir_period_one+1){
                     ir_data.data_rx[ir_data.ptr_rx/8] |= (1 << (ir_data.ptr_rx%8));
                  }
                  else if(ir_data.cnt_rx >= ir_period_zero-1 & ir_data.cnt_rx <= ir_period_zero+1){
                     ir_data.data_rx[ir_data.ptr_rx/8] &= ~(1 << (ir_data.ptr_rx%8));
                  }
                  if(ir_data.ptr_rx<8*ir_data_len){  
                     ir_data.ptr_rx++;
                  }
                  else{   
                     ir_data.ptr_rx=0;
                  }
                  ir_data.flag_rx=IR_rx_done;
            }
            else{//(HAL_GPIO_ReadPin(ir_input_port, ir_input_pin) == GPIO_PIN_SET)
                        HAL_GPIO_WritePin(ir_input_flag_port,ir_input_flag_pin,0);
                  ir_data.flag_rx=IR_rx_receiving;
            }
            ir_data.cnt_rx=0;
         }
      }

   </head>
</html>

### 改进空间
1. 我希望在开始发送前添加一个引导信号，一个20拍的拉高、4拍拉低表示一个包开始传输，放置两个连续的数据传输发生粘连，在发送和接收逻辑、标志中添加一些内容以实现。
2. 两块板都烧录该程序能实现互发数据，我在IR.h中塞了个try[20]数组赋值给data_tx，使其不断发送try这个包，我希望添加包校验机制，添加一个50HZ定时器，检测：若data_rx!=try让这个灯进行2Hz闪烁，闪烁的实现方式自行决定。
   
这个灯在哪：
<html>
   <head>

      #define ir_power_port GPIOB
      #define ir_power_pin 	GPIO_PIN_10
   </head>
</html>
 