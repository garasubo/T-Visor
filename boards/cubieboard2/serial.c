#include "debug.h"
#include "port.h"
#include "basic.h"

#define	UARTxBase(x)		(0x01c28000 + 0x000400*(x))
#define UARTx_RBR(x)		(UARTxBase(x) + 0x00)
#define UARTx_THR(x)		(UARTxBase(x) + 0x00)
#define UARTx_DLL(x)		(UARTxBase(x) + 0x00)
#define UARTx_DLH(x)		(UARTxBase(x) + 0x04)
#define UARTx_IER(x)		(UARTxBase(x) + 0x04)
#define UARTx_IIR(x)		(UARTxBase(x) + 0x08)
#define UARTx_FCR(x)		(UARTxBase(x) + 0x08)
#define UARTx_LCR(x)		(UARTxBase(x) + 0x0c)
#define UARTx_MCR(x)		(UARTxBase(x) + 0x10)
#define UARTx_LSR(x)		(UARTxBase(x) + 0x14)
#define UARTx_MSR(x)		(UARTxBase(x) + 0x18)
#define UARTx_SCH(x)		(UARTxBase(x) + 0x1c)
#define UARTx_USR(x)		(UARTxBase(x) + 0x7c)
#define UARTx_TFL(x)		(UARTxBase(x) + 0x80)
#define UARTx_RFL(x)		(UARTxBase(x) + 0x84)
#define UARTx_HALT(x)		(UARTxBase(x) + 0xa4)

void serial_init(void){
    return;
    OUTW(UARTx_LCR(0),0x03);

    // FIFOを一応リセットしておいてFIFO mode
    OUTW(UARTx_FCR(0),0x06);
    OUTW(UARTx_FCR(0),0x01);
    // 割込みは全無効化
    OUTW(UARTx_IER(0),0x00);
}

B get_serial(){
    return '\0';
    while((INB(UARTx_LSR(0))&0x01)==0){
        if((INB(UARTx_IIR(0))&0xff)==0x6){
            INB(UARTx_LSR(0));
        }
    }

    return INB(UARTx_RBR(0));
}

void put_serial(UB val){
    return;
    while((INB(UARTx_LSR(0))&0x20)==0);
    OUTB(UARTx_THR(0), val);
}
