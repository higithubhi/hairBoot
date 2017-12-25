/********************************************************************************
 * 文件名  ：bootloader.c
 * 描述    ：bootloader     
 * 作者    ：泽畔无材  zepanwucai@gmail.com
 *修改时间 ：2013-11-18
**********************************************************************************/

#include "bootloader.h"

struct interrupt_vector {  
    u16 interrupt_instruction;      
    u16 interrupt_handler;  
};
extern void __iar_program_start(void);
//redirected interrupt table  
__root const struct interrupt_vector __intvec[32] @ ".intvec" 
= {  
    {0x8200, (u16)__iar_program_start}, /* reset */  
    {0x8200, (u16)FLASH_APP_START + 4}, /* trap  */  
    {0x8200, (u16)FLASH_APP_START + 8}, /* irq0  */  
    {0x8200, (u16)FLASH_APP_START + 12}, /* irq1  */  
    {0x8200, (u16)FLASH_APP_START + 16}, /* irq2  */  
    {0x8200, (u16)FLASH_APP_START + 20}, /* irq3  */  
    {0x8200, (u16)FLASH_APP_START + 24}, /* irq4  */  
    {0x8200, (u16)FLASH_APP_START + 28}, /* irq5  */  
    {0x8200, (u16)FLASH_APP_START + 32}, /* irq6  */  
    {0x8200, (u16)FLASH_APP_START + 36}, /* irq7  */  
    {0x8200, (u16)FLASH_APP_START + 40}, /* irq8  */  
    {0x8200, (u16)FLASH_APP_START + 44}, /* irq9  */  
    {0x8200, (u16)FLASH_APP_START + 48}, /* irq10 */  
    {0x8200, (u16)FLASH_APP_START + 52}, /* irq11 */  
    {0x8200, (u16)FLASH_APP_START + 56}, /* irq12 */  
    {0x8200, (u16)FLASH_APP_START + 60}, /* irq13 */  
    {0x8200, (u16)FLASH_APP_START + 64}, /* irq14 */  
    {0x8200, (u16)FLASH_APP_START + 68}, /* irq15 */  
    {0x8200, (u16)FLASH_APP_START + 72}, /* irq16 */  
    {0x8200, (u16)FLASH_APP_START + 76}, /* irq17 */  
    {0x8200, (u16)FLASH_APP_START + 80}, /* irq18 */  
    {0x8200, (u16)FLASH_APP_START + 84}, /* irq19 */  
    {0x8200, (u16)FLASH_APP_START + 88}, /* irq20 */  
    {0x8200, (u16)FLASH_APP_START + 92}, /* irq21 */  
    {0x8200, (u16)FLASH_APP_START + 96}, /* irq22 */  
    {0x8200, (u16)FLASH_APP_START + 100}, /* irq23 */  
    {0x8200, (u16)FLASH_APP_START + 104}, /* irq24 */  
    {0x8200, (u16)FLASH_APP_START + 108}, /* irq25 */  
    {0x8200, (u16)FLASH_APP_START + 112}, /* irq26 */  
    {0x8200, (u16)FLASH_APP_START + 116}, /* irq27 */  
    {0x8200, (u16)FLASH_APP_START + 120}, /* irq28 */  
    {0x8200, (u16)FLASH_APP_START + 124}, /* irq29 */  
};  

int main(void)
{
    u16 tryCnt = 65535;
    u8 ch, page;
    u8 i = 10;  //0.5s
    u8 buf[BLOCK_BYTES];
    u8 verify;
    u8* addr;
    //set hsi = 16M,复位后默认hsi
    CLK->CKDIVR &= (uint8_t)(~CLK_CKDIVR_HSIDIV); //清空(00)即不分频
    while(!(CLK->ICKR & 0x02));   //等待内部时钟稳定
    //set baudrate = 115200, 复位后默认开启uart，格式默认，只需设置波特率及使能收发
//    UART1->BRR2 |= (uint8_t)((uint8_t)(((BaudRate_Mantissa100 - (BaudRate_Mantissa * 100)) << 4) / 100) & (uint8_t)0x0F) |\
//                   (uint8_t)((BaudRate_Mantissa >> 4) & (uint8_t)0xF0);  
//    UART1->BRR1 |= (uint8_t)BaudRate_Mantissa; 
    //设置为9600
    UART1->BRR2 = 0x2;
    UART1->BRR1 = 0x68;
    UART1->CR2 = (uint8_t)(UART1_CR2_TEN | UART1_CR2_REN);  //使能收发        
    //bootloader通信过程
    while(i)    
    {
        if(UART1->SR & (u8)UART1_FLAG_RXNE)    //wait for head           
        {
            ch = (uint8_t)UART1->DR;    
            if(ch == BOOT_HEAD) break;
        }
        tryCnt--;
        if(tryCnt == 0) i--;
    }
    
    if(i == 0)
    {    //goto app
        goto goApp;
    }
    else
    {               
        UART1_SendB(BOOT_OK);   
        while(1)
        {
            ch = UART1_RcvB();
            switch(ch)
            {
            case BOOT_HEAD:
               UART1_SendB(BOOT_OK);
               break;
            case BOOT_GO:
                goApp:
                //FLASH->IAPSR &= FLASH_MEMTYPE_PROG; //锁住flash
                //goto app
                //UART1_SendB(BOOT_OK);
                asm("JP $8200");
                break;
            case BOOT_WRITE:                
                page = UART1_RcvB();
                addr = (u8*)(FLASH_START + (page << BLOCK_SHIFT));
                verify = 0;
                for(i = 0; i < BLOCK_BYTES; i++)   
                {
                    buf[i] = UART1_RcvB();
                    verify += buf[i];
                }
                if(verify == UART1_RcvB())  //通信校验成功
                {
                    FLASH_ProgBlock(addr, buf);
                    //verify flash 
                    for(i = 0; i < BLOCK_BYTES; i++)   
                    {
                        verify -= addr[i];
                    }
                    if(verify == 0)  //写入校验成功
                    {
                        UART1_SendB(BOOT_OK);
                        break;
                    }
                    //else,写入校验失败，可能是flash损坏
                }
            default: //上面校验失败的情况也会到这里来
                UART1_SendB(BOOT_ERR);
                break;
            }
        }
    }
}

void UART1_SendB(u8 ch)
{
    while (!(UART1->SR & (u8)UART1_FLAG_TXE));
    UART1->DR = ch;    
}

u8 UART1_RcvB(void)
{
     while(!(UART1->SR & (u8)UART1_FLAG_RXNE));
     return ((uint8_t)UART1->DR);
}

//addr must at begin of block
__ramfunc void FLASH_ProgBlock(uint8_t * addr, uint8_t *Buffer)
{
    static u8 bFirst=0;
    if(bFirst==0)
    {
        bFirst=1;
        //unlock flash,解锁flash
        FLASH->PUKR = FLASH_RASS_KEY1;
        FLASH->PUKR = FLASH_RASS_KEY2;      
        //清除app代码        
        FLASH->CR2 |= FLASH_CR2_ERASE;
        FLASH->NCR2 &= (uint8_t)(~FLASH_NCR2_NERASE);
        uint16_t appAddr;
        for (appAddr = FLASH_APP_START; appAddr < FLASH_END; appAddr++)
        {
            *((PointerAttr uint8_t*)appAddr) = 0;    
        }
    }
    u8 i;
    /* Standard programming mode */ /*No need in standard mode */
    FLASH->CR2 |= FLASH_CR2_PRG;
    FLASH->NCR2 &= (uint8_t)(~FLASH_NCR2_NPRG);
    /* Copy data bytes from RAM to FLASH memory */    
    for (i = 0; i < BLOCK_BYTES; i++)
    {
        *((PointerAttr uint8_t*) (uint16_t)addr + i) = ((uint8_t)(Buffer[i]));    
    }
    
}
