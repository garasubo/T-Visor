#include "basic.h"
#include "debug.h"
#include "page_table.h"
#include "type.h"
#include "gic.h"
#include "virtual_gic.h"
#include "cpsr.h"
#include "port.h"
#include "vcpu.h"
#include "hcr.h"
#include "sctlr.h"
#include "cp_access.h"
#include "memory_manage.h"
#include "timer.h"
#include "cortexa7.h"
#include "logger.h"
#include "timer_event.h"
#include "rbtree.h"
#include "vint_sender.h"
#include "virtual_device_handle.h"
#include "virtual_psci.h"
#include "coproc.h"

#define INST_ARM 0
#define INST_THUMB 1

extern char* sleep_vector;
extern void system_init(void);

static void hyp_message(UW *sp){
    tv_error_message("nakayoshi internet\n");
}


static void tmp_func(UW *sp){
    int i;
    for(i=0;i<10;i++){
        virtual_gic_send_software_int(vcpu_get_executing()->vm->vgic,i);
    }
}

static void dummy_func(UW *sp){
    return;
}

static void dummy_csw(void){
    T_VCPU *vcpu = vcpu_get_executing();
    vcpu_save_state(vcpu);
    vm_stop(vcpu->vm);
    vcpu->state = VCPU_STATE_WAIT;
    vcpu_stop();
    vm_start(vcpu->vm);
    vcpu_start(vcpu);
}

static void (*hyp_call_table[])(UW *) = {
    (void (*)(UW *))system_init,    //0
    hyp_message,    //1
    tmp_func,       //2
    dummy_func,  //3
    (void (*)(UW *))vint_sender_interface,          //4
    dummy_func,                                     //5
    (void (*)(UW *))scheduler_set_flag,             //6
    (void (*)(UW *))dummy_csw,             //7
};

static int hyp_check_inst_mode(void){
    UW spsr;
    Asm("mrs %0, spsr":"=r"(spsr));
    return (spsr&(1 << 5)) >> 5;
}

static void decode_imm_shift(UW *reg, UB type, UB imm5){
    W *val = (W *)reg;
    switch(type){
        case 0x00:
            *reg = *reg << imm5;
            break;
        case 0x01:
            if(imm5==0) *reg = 0;
            else *reg = *reg >> imm5;
            break;
        case 0x02:
            if(imm5==0) imm5 = 32;
            *val = *val >> imm5;
            break;
        case 0x3:
            if(imm5==0){
                *reg = *reg >> 1;
                // carry flag check;
                if(vcpu_get_executing()->cpsr&(1 << 29)){
                    *reg |= (1 << 31);
                }
            } else {
                UW high = *reg & ((1 << imm5)-1);
                UW low = *reg >> imm5;
                *reg = (high << (32-imm5)) | low;
            }
            break;
        default:
            break;
    }
}

#define HC_TABLE_SIZE (sizeof(hyp_call_table)/sizeof(hyp_call_table[0]))

void hyp_reg_write(UB reg_id, UW value, UW *sp){
    if(0 <= reg_id && reg_id <= 7){
        sp[(UW)reg_id] = value;
    } else if(8 <= reg_id && reg_id <= 12) {
        UW spsr;
        Asm("mrs %0, spsr":"=r"(spsr));
        spsr &= 0x1f;
        if(spsr==PSM_FIQ){
            switch(reg_id){
                case 8:
                    Asm("msr r8_fiq, %0":"=r"(value));
                    break;
                case 9:
                    Asm("msr r9_fiq, %0":"=r"(value));
                    break;
                case 10:
                    Asm("msr r10_fiq, %0":"=r"(value));
                    break;
                case 11:
                    Asm("msr r11_fiq, %0":"=r"(value));
                    break;
                default:
                    Asm("msr r12_fiq, %0":"=r"(value));
                    break;
            }
        } else {
            sp[(UW)reg_id] = value;
        }
    } else if(reg_id==14) {
        UW spsr;
        Asm("mrs %0, spsr":"=r"(spsr));
        spsr &= 0x1f;
        switch(spsr){
            case PSM_USR:
                sp[13] = value;
                break;
            case PSM_FIQ:
                Asm("msr lr_fiq, %0"::"r"(value));
                break;
            case PSM_IRQ:
                Asm("msr lr_irq, %0"::"r"(value));
                break;
            case PSM_SVC:
                Asm("msr lr_svc, %0"::"r"(value));
                break;
            case PSM_ABT:
                Asm("msr lr_abt, %0"::"r"(value));
                break;
            case PSM_UND:
                Asm("msr lr_und, %0"::"r"(value));
                break;
            case PSM_SYS:
                sp[13] = value;
                break;
            default:
                tv_abort("!!! unknown !!!\n");
                break;

        }
    } else if(reg_id==13) {
        UW spsr;
        Asm("mrs %0, spsr":"=r"(spsr));
        spsr &= 0x1f;
        switch(spsr){
            case PSM_USR:
                Asm("msr sp_usr, %0"::"r"(value));
                break;
            case PSM_FIQ:
                Asm("msr sp_fiq, %0"::"r"(value));
                break;
            case PSM_IRQ:
                Asm("msr sp_irq, %0"::"r"(value));
                break;
            case PSM_SVC:
                Asm("msr sp_svc, %0"::"r"(value));
                break;
            case PSM_ABT:
                Asm("msr sp_abt, %0"::"r"(value));
                break;
            case PSM_UND:
                Asm("msr sp_und, %0"::"r"(value));
                break;
            case PSM_SYS:
                Asm("msr sp_usr, %0"::"r"(value));
                break;
            default:
                tv_abort("!!! unknown !!!\n");
                break;

        }
    } else if(reg_id==15) {
        Asm("msr elr_hyp, %0"::"r"(value));
    } else {
        tv_abort("!!! unknown !!!\n");
    }
}

UW hyp_reg_read(UB reg_id, UW *sp){
    if(0 <= reg_id && reg_id <= 7){
        return sp[(UW)reg_id];
    } else if(8 <= reg_id && reg_id <= 12) {
        UW spsr;
        Asm("mrs %0, spsr":"=r"(spsr));
        spsr &= 0x1f;
        UW value;
        if(spsr==PSM_FIQ){
            switch(reg_id){
                case 8:
                    Asm("mrs %0, r8_fiq":"=r"(value));
                    break;
                case 9:
                    Asm("mrs %0, r9_fiq":"=r"(value));
                    break;
                case 10:
                    Asm("mrs %0, r10_fiq":"=r"(value));
                    break;
                case 11:
                    Asm("mrs %0, r11_fiq":"=r"(value));
                    break;
                default:
                    Asm("mrs %0, r12_fiq":"=r"(value));
                    break;
            }
            return value;
        } else {
            return sp[(UW)reg_id];
        }
    } else if(reg_id==14) {
        UW spsr;
        Asm("mrs %0, spsr":"=r"(spsr));
        spsr &= 0x1f;
        UW value;
        switch(spsr){
            case PSM_USR:
                value = sp[13];
                break;
            case PSM_FIQ:
                Asm("mrs %0, lr_fiq":"=r"(value));
                break;
            case PSM_IRQ:
                Asm("mrs %0, lr_irq":"=r"(value));
                break;
            case PSM_SVC:
                Asm("mrs %0, lr_svc":"=r"(value));
                break;
            case PSM_ABT:
                Asm("mrs %0, lr_abt":"=r"(value));
                break;
            case PSM_UND:
                Asm("mrs %0, lr_und":"=r"(value));
                break;
            case PSM_SYS:
                value = sp[13];
                break;
            default:
                value = 0;
                tv_puts("!!! unknown !!!\n");
                break;

        }
        return value;
    } else if(reg_id==13) {
        UW spsr;
        Asm("mrs %0, spsr":"=r"(spsr));
        spsr &= 0x1f;
        UW value = 0;
        switch(spsr){
            case PSM_USR:
                Asm("mrs %0, sp_usr":"=r"(value));
                break;
            case PSM_FIQ:
                Asm("mrs %0, sp_fiq":"=r"(value));
                break;
            case PSM_IRQ:
                Asm("mrs %0, sp_irq":"=r"(value));
                break;
            case PSM_SVC:
                Asm("mrs %0, sp_svc":"=r"(value));
                break;
            case PSM_ABT:
                Asm("mrs %0, sp_abt":"=r"(value));
                break;
            case PSM_UND:
                Asm("mrs %0, sp_und":"=r"(value));
                break;
            case PSM_SYS:
                Asm("mrs %0, sp_usr":"=r"(value));
                break;
            default:
                tv_puts("!!! unknown !!!\n");
                break;

        }
        return value;
    } else if(reg_id==15) {
        UW value;
        Asm("mrs %0, elr_hyp":"=r"(value));
        return value;
    } 
    tv_puts("!!! unknown !!!\n");
    return 0;
}

void hyp_memory_passthrough(UW iss, UW *addr, UW *sp){
    // read or write
    bool is_write = (iss&(1 << 6)) > 0;
    // reg_id
    UB reg_id = (iss&0xf0000) >> 16;
    // length
    UB len = (iss&0xc00000) >> 22;
    // signed
    bool sse = (iss&(1 << 21)) > 0;

    if(is_write){
        UW value = hyp_reg_read(reg_id, sp);
        switch(len){
            case 0x0:
                OUTB(addr,value&0xff);
                break;
            case 0x1:
                OUTHW(addr,value&0xffff);
                break;
            case 0x2:
                OUTW(addr,value);
                break;
            default:
                tv_puts("!!! unknown length !!!\n");
                break;
        }
    } else {
        if(sse){
            W value;
            switch(len){
                case 0x0:
                    value = INB(addr);
                    break;
                case 0x1:
                    value = INHW(addr);
                    break;
                case 0x2:
                    value = INW(addr);
                    break;
                default:
                    tv_puts("!!! unknown length !!!\n");
                    value = 0;
                    break;
            }
            hyp_reg_write(reg_id,value,sp);
        } else {
            UW value;
            switch(len){
                case 0x0:
                    value = (UB)INB(addr);
                    break;
                case 0x1:
                    value = (UHW)INHW(addr);
                    break;
                case 0x2:
                    value = (UW)INW(addr);
                    break;
                default:
                    tv_puts("!!! unknown length !!!\n");
                    value = 0;
                    break;
            }
            hyp_reg_write(reg_id,value,sp);
        }
    }
}

UW hyp_get_elr( void ){
    UW r0;
    Asm("mrs %0, elr_hyp":"=r"(r0));
    return r0;
}

void hyp_set_elr(UW r0){
    Asm("msr elr_hyp, %0"::"r"(r0));
}

extern bool in_irq_handle;

// This implementation is not completed
static void check_write_back(UW inst){
    // TODO: Double format
    if(!(inst&(0x7 << 25))){
        tv_abort("not support\n");

        if(inst&(1 << 22)){

        }

    }
    else {
        // pre-index and post-index
        if(inst&(1 << 21)){
            tv_abort("not support\n");

            UB regn_id = (inst&(0xf << 12)) >> 12;
            // use register
            if(inst&(1 << 25)){
                UB type = (inst&(0x3 << 5))>>5;
                UB imm5 = (inst&(0x1f << 7))>>7;
                UB regm_id = inst&0xf;
                UW regm_value = hyp_reg_read(regm_id, vcpu_get_current_sp()->reg);
                decode_imm_shift(&regm_value, type, imm5);
                UW regn_value = hyp_reg_read(regn_id, vcpu_get_current_sp()->reg); if(inst&(1 << 23)){
                    hyp_reg_write(regn_id, regn_value+regm_value, vcpu_get_current_sp()->reg);
                } else {
                    hyp_reg_write(regn_id, regn_value-regm_value, vcpu_get_current_sp()->reg);
                }
            }
            // imm
            else{
                UHW imm = inst & 0xfff;
                UW regn_value = hyp_reg_read(regn_id, vcpu_get_current_sp()->reg);
                if(inst&(1 << 23)){
                    hyp_reg_write(regn_id, regn_value+imm, vcpu_get_current_sp()->reg);
                } else {
                    hyp_reg_write(regn_id, regn_value-imm, vcpu_get_current_sp()->reg);
                }

            }
        }
    }
}

void hyp_call(UW hsr){
    UW *sp = vcpu_get_current_sp()->reg;
    UW ec = hsr >> 26;
    UW iss = hsr&((1 << 25)-1);

    if(ec==0x12){
        if(iss < HC_TABLE_SIZE) hyp_call_table[iss](sp);
        else hyp_message(sp);
    } else if(ec==0x13){
        sp[0] = virtual_psci();
        if(hyp_check_inst_mode()==INST_ARM){
            hyp_set_elr( hyp_get_elr()+4 );
        } else {
            hyp_set_elr( hyp_get_elr()+2 );
        }
    } else if(ec==0x20){
        UW r0;
        Asm("mrc p15, 4, %0, c1, c1, 0":"=r"(r0));
        r0 |= HCR_VA;
        Asm("mcr p15, 4, %0, c1, c1, 0"::"r"(r0));
        tv_abort("virtual prefetch abort\n");
    } else if(ec==0x24){
        // trapの起きたメモリアドレス読み込み
        UW r0,r1;
        Asm("mrc p15, 4, %0, c6, c0, 4":"=r"(r0)); //HPFAR
        Asm("mrc p15, 4, %0, c6, c0, 0":"=r"(r1)); //HDFAR

        UD addr = (((UD)r0&0xfffffff8) << 8) + (r1&0xfff);
        if(!(iss&(1 << 24))){
            tv_abort("!!! data abort !!!\n");
            UW r0;
            Asm("mrc p15, 4, %0, c1, c1, 0":"=r"(r0));
            r0 |= HCR_VA;
            Asm("mcr p15, 4, %0, c1, c1, 0"::"r"(r0));
            SCTLR_READ(r0);
            if(r0&SCTLR_VE){
                hyp_set_elr(0x10);
            } else {
                VBAR_READ(r0);
                hyp_set_elr(r0+0x10);
            }
            return;
        }

        // elr_hypを修正して、1つ先の命令を実行させる
        // thumb命令セットの可能性があるので、それを見る
        if(hyp_check_inst_mode()==INST_ARM){
            // write_back処理がある場合、ここで行なう

            // elrからinstructionを取得。ページテーブル変換を考慮する
            UW inst_addr = hyp_get_elr();
            UW par;
            Asm("mcr p15, 0, %0, c7, c8, 4"::"r"(inst_addr));
            Asm("mrc p15, 0, %0, c7, c4, 0":"=r"(par));
            UW inst = *(UW *)(((par >> 12) << 12)|(inst_addr&0xfff));

            check_write_back(inst);


            hyp_set_elr( hyp_get_elr()+4 );
        } else {
            hyp_set_elr( hyp_get_elr()+2 );
        }

        virtual_device_handle(addr, iss);


        // MRC p15, 4, <Rt>, c6, c0, 4 HPFAR
    } else if(ec==0x01){
        T_VCPU *vcpu = vcpu_get_executing();
        if(hyp_check_inst_mode()==INST_ARM){
            hyp_set_elr( hyp_get_elr()+4 );
        } else {
            hyp_set_elr( hyp_get_elr()+2 );
        }
        if((iss&0x1)==0){
            vcpu_sleep(vcpu);
        } else {
            // wfe
            scheduler_set_flag();
        }

    } else if(ec==0x03){
        T_VCPU *vcpu = vcpu_get_executing();
        coproc_cp15_emulate(vcpu, iss);
        if(hyp_check_inst_mode()==INST_ARM){
            hyp_set_elr( hyp_get_elr()+4 );
        } else {
            hyp_set_elr( hyp_get_elr()+2 );
        }
    } else {
        hyp_message(sp);
    }

    if(scheduler_get_flag()){
        vcpu_preemption(scheduler_get_operations()->schedule());
    }

    while(vcpu_get_executing()==NULL){
        Asm("mcr p15, 4, %0, c12, c0, 0\n"
            "ldr lr, =1f\n"
            "dsb\n"
            "cpsie aif\n"
            "wfi\n"
            "1: nop"
                ::"r"(&sleep_vector)
                :"lr");

    }
}
