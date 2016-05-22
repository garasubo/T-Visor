#include "virtual_gic.h"
#include "gic.h"
#include "debug.h"
#include "port.h"
#include "vcpu.h"
#include "vm.h"
#include "virtual_device_handle.h"
#include "hyp_call.h"

vgic_t _vgic[NUM_OF_T_VM];
static UW _vgic_num = 0;
vgic_core_state_t _state[NUM_OF_T_VCPU];
static UW _state_num = 0;

static bool virtual_gic_send_software_int_inner(vgic_t *vgic, UW mask, UW vintvec);
static UW virtual_gic_read_lr(vgic_t *vgic, UW vid, UW tar);

vgic_t *vgic_alloc(void){
    return &(_vgic[_vgic_num++]);
}

vgic_core_state_t *vgic_core_state_calloc(UW num){
    vgic_core_state_t *ret = &(_state[_state_num]);
    _state_num += num;
    return ret;
}

static void virtual_gic_handle(void *param, UW addr, UW iss)
{
    vgic_t *vgic = vcpu_get_executing()->vm->vgic;

    UB reg_id = (iss&0xf0000) >> 16;
    // length
    UB len = (iss&0xc00000) >> 22;
    bool wnr = (iss&(1 << 6)) > 0;
    if(wnr){
        UW reg_value = hyp_reg_read(reg_id, vcpu_get_current_sp()->reg);
        switch(len){
            case 0x0:
                virtual_gicd_write_emulate_b(vgic, addr, reg_value);
                break;
            case 0x1:
                virtual_gicd_write_emulate_h(vgic, addr, reg_value);
                break;
            case 0x2:
                virtual_gicd_write_emulate_w(vgic, addr, reg_value);
                break;
            default:
                tv_abort("length error\n");
        }
    } else {
        UW value = 0;
        switch(len){
            case 0x0:
                value = virtual_gicd_read_emulate_b(vgic, addr);
                break;
            case 0x1:
                value = virtual_gicd_read_emulate_h(vgic, addr);
                break;
            case 0x2:
                value = virtual_gicd_read_emulate_w(vgic, addr);
                break;
            default:
                tv_abort("length error\n");
                break;
        }
        hyp_reg_write(reg_id, value, vcpu_get_current_sp()->reg);
    }
}

static HW virtual_gic_default_balance_func(vgic_t *vgic, UW mask, UW vintvec)
{
    T_VCPU *vcpu = vcpu_get_last();
    if(vgic->vm==vcpu->vm && (mask & (1 << vcpu->vid))){
        return vcpu->vid;
    }
    for(UB i=0;i<vgic->vm->vcpu_num;i++) if(mask & (1 << i)){
        UW er=vgic->gicd_isenabler[vintvec >> 5];
        if(er&(1 << (vintvec&0x1f))){
            return i;
        }
    }
    return -1;
}

void virtual_gic_init(vgic_t *vgic, T_VM *vm){
    vgic->vm = vm;
    UW i;

    for(i=0;i<SGI_ID_MAX;i++){
        vgic->ptov[i] = i;
        vgic->vtop[i] = i;
    }
    for(i=SGI_ID_MAX;i<SPI_ID_MAX;i++){
        vgic->ptov[i] = -1;
        vgic->vtop[i] = -1;
    }
    for(i=0;i<SPI_ID_MAX/32;i++){
        vgic->use_irq[i] = 0;
    }

    vgic->core_state = vgic_core_state_calloc(vgic->vm->vcpu_num);
    for(i=0;i<vgic->vm->vcpu_num;i++){
        vgic->core_state[i].id = i;
        for(int j=0;j<SPI_ID_MAX/32;j++){
            vgic->core_state[i].irq_no_mask[j] = 0;
        }
        vgic->core_state[i].ppi_isenabler = 0xffff;
        vgic->core_state[i].saved_hcr = 0x5;
    }

    vgic->balance_func = virtual_gic_default_balance_func;


    // set defalut values
    vgic->gicd_isenabler[0] = 0xffff;


    vgic->gicd_typer = INW(GICD_TYPER);
    vgic->gicd_typer &= ~(1 << 10);
    vgic->gicd_typer &= ~0xe0;
    vgic->gicd_typer |= (vgic->vm->vcpu_num - 1) << 5;

    vgic->gicd_iidr = INW(GICD_IIDR);
    for(i=0;i<SPI_ID_MAX/32;i++){
        vgic->gicd_igroupr[i] = 0;
    }
    vgic->gicd_icfgr[0] = 0xaaaaaaaa;
    vgic->gicd_icfgr[1] = 0x55540000;
    for(i=PPI_ID_MAX/2;i<SPI_ID_MAX/16;i++){
        vgic->gicd_icfgr[i] =  0x55555555;
    }

    vgic->real_pri = 0;

}

void virtual_gic_register_handle(void){
    virtual_device_handle_register(GICD_BASE_ADDR, GICD_BASE_ADDR + 0x1000, virtual_gic_handle, NULL);
}


void virtual_gic_register_int(vgic_t *vgic, UHW pintvec, UHW vintvec){
    vgic->ptov[pintvec] = vintvec;
    vgic->vtop[vintvec] = pintvec;
    //vgic->use_irq[pintvec/32] |= 1 << (pintvec % 32);
}

void virtual_gic_save_state(vgic_t *vgic, UW id){
    vgic_core_state_t *state = &(vgic->core_state[id]);

    state->vmcr = INW(GICH_VMCR);
    state->saved_elsr0 = INW(GICH_ELSR0);
    state->saved_apr = INW(GICH_APR);
    for(UHW i=0;i<GICH_LR_NUM;i++){
        UW prev = gic_read_lr(i);
        state->saved_lr[i] = prev;
    }
    vgic->core_state[id].saved_hcr = INW(GICH_HCR);
}

void virtual_gic_restore_state(vgic_t *vgic, UW id){
    OUTW(GICH_HCR, vgic->core_state[id].saved_hcr);
    OUTW(GICH_VMCR, vgic->core_state[id].vmcr);
    for(UW i=0;i<GICH_LR_NUM;i++){
        gic_write_lr(i, vgic->core_state[id].saved_lr[i]);
    }
}

UW virtual_gicd_read_emulate_w(vgic_t *vgic,UW addr)
{
    if(addr==GICD_CTRL){
        return (vgic->enable_g0 | ((UW)vgic->enable_g1 << 1));
    }
    else if(addr==GICD_TYPER){
        return vgic->gicd_typer;
    }
    else if(addr==GICD_IIDR){
        return INW(GICD_IIDR);
    }
    else if(addr==GICD_ISENABLER(0) || addr==GICD_ICENABLER(0)){
        UW id = vcpu_get_executing()->vid;
        return vgic->core_state[id].ppi_isenabler;
    }
    else if(GICD_ISENABLER(1)<=addr && addr<GICD_ICENABLER(0)){
        return vgic->gicd_isenabler[(addr-GICD_ISENABLER(0))/4];
    }
    else if(GICD_ICENABLER(1)<=addr && addr<GICD_ISPENDER(0)){
        return vgic->gicd_isenabler[(addr-GICD_ICENABLER(0))/4];
    }
    else if(addr==GICD_ISPENDER(0) || addr==GICD_ICENABLER(0)){
        UW id = vcpu_get_executing()->vid;
        UW value = vgic->core_state[id].irq_no_mask[0];

        for(UW i=0;i<GICH_LR_NUM;i++){
            UW lr = INW(GICH_LR(i));
            if((lr&(1 << 28)) && (lr&0x1ff)/32==0){
               value |= 1 << ((lr&0x1ff)%32);
            }
        }
        return value;
    }
    else if(GICD_ISPENDER(1)<=addr && addr <=GICD_ISPENDER(SPI_ID_MAX/32)){
        UW value = 0;
        UW idx = (addr-GICD_ISPENDER(0))/4;
        for(UW i=0;i<vgic->vm->vcpu_num;i++){
            value |= vgic->core_state[i].irq_no_mask[idx];
            for(UW j=0;j<GICH_LR_NUM;j++){
                UW lr = virtual_gic_read_lr(vgic, i, j);
                if((lr&(1 << 28)) && (lr&0x1ff)/32==idx){
                   value |= 1 << ((lr&0x1ff)%32);
                }
            }
        }
        return value;
    }
    else if(GICD_ICPENDER(1)<=addr && addr <=GICD_ICPENDER(SPI_ID_MAX/32)){
        UW value = 0;
        UW idx = (addr-GICD_ICPENDER(0))/4;
        for(UW i=0;i<vgic->vm->vcpu_num;i++){
            value |= vgic->core_state[i].irq_no_mask[idx];
            for(UW j=0;j<GICH_LR_NUM;j++){
                UW lr = virtual_gic_read_lr(vgic, i, j);
                if((lr&(1 << 28)) && (lr&0x1ff)/32==idx){
                   value |= 1 << ((lr&0x1ff)%32);
                }
            }
        }
        return value;
    }
    else if(GICD_IPRIORITYR(PPI_ID_MAX/4)<=addr && addr<=GICD_IPRIORITYR(SPI_ID_MAX/4)){
        return ((UW *)vgic->gicd_ipriorityr)[(addr-GICD_IPRIORITYR(0))/4];
    }
    else if(GICD_IPRIORITYR(0)<=addr && addr<GICD_IPRIORITYR(PPI_ID_MAX/4)){
        UW id = vcpu_get_executing()->vid;
        return ((UW *)vgic->core_state[id].ppi_ipriorityr)[(addr-GICD_IPRIORITYR(0))/4];
    }
    else if(GICD_ITARGETSR(0)<= addr && addr<GICD_ITARGETSR(PPI_ID_MAX/4)){
        UW id = vcpu_get_executing()->vid;
        UB value = 1 << id;
        return (value << 24)|(value << 16)|(value << 8)|value;
    }
    else if(GICD_ITARGETSR(PPI_ID_MAX/4)<=addr && addr<GICD_ITARGETSR(SPI_ID_MAX/4)){
        return ((UW *)vgic->gicd_itargetsr)[(addr-GICD_ITARGETSR(0))/4];
    }
    else if(GICD_ICFGR(PPI_ID_MAX/16)<=addr && addr<GICD_ICFGR(SPI_ID_MAX/16)){
        return vgic->gicd_icfgr[(addr-GICD_ICFGR(0))/4];
    }
    else {
        tv_abort("unsupported read access!\n");
        return 0;
    }
}
UHW virtual_gicd_read_emulate_h(vgic_t *vgic,UW addr)
{
    UW value = virtual_gicd_read_emulate_w(vgic, addr&~0x3);
    return (value >> (8 * (addr&0x3))) & 0xffff;
}

UB virtual_gicd_read_emulate_b(vgic_t *vgic,UW addr)
{
    UW value = virtual_gicd_read_emulate_w(vgic, addr&~0x3);
    return (value >> (8 * (addr&0x3))) & 0xff;
}

void virtual_gicd_write_emulate_w(vgic_t *vgic,UW addr, UW reg_value)
{
    if(addr==GICD_CTRL){
        virtual_gicd_write_emulate_b(vgic, addr, reg_value);
    }
    else if(addr==GICD_ISENABLER(0)){
        UW id = vcpu_get_executing()->vid;
        vgic->core_state[id].ppi_isenabler |= reg_value;

        for(UW j=0;j<32;j++) if(reg_value&(1 << j)){
            int p = vgic->vtop[j];
            if(p>=0){
                vgic->use_irq[p/32] |= 1 << (p % 32);
                if(vgic->enable_g0 || vgic->enable_g1){
                    gic_enable_int(p, vgic->core_state[id].ppi_ipriorityr[j]+ vgic->real_pri);
                }
            }
        }
    }
    else if(GICD_ISENABLER(1)<=addr && addr<GICD_ICENABLER(0)){
        UW i = (addr-GICD_ISENABLER(0))/4;

        vgic->gicd_isenabler[i] |= reg_value;

        for(UW j=0;j<32;j++) if(reg_value&(1 << j)){
            int p = vgic->vtop[i*32+j];
            if(p>=0){
                vgic->use_irq[p/32] |= 1 << (p % 32);
                if(vgic->enable_g0 || vgic->enable_g1){
                    gic_enable_int(p, vgic->gicd_ipriorityr[i*32+j] + vgic->real_pri);
                }
            }
        }

    }
    else if(addr==GICD_ICENABLER(0)){
        UW id = vcpu_get_executing()->vid;
        vgic->core_state[id].ppi_isenabler &= ~reg_value;

        for(UW j=0;j<32;j++) if(reg_value&(1 << j)){
            int p = vgic->vtop[j];
            if(p>=0){
                vgic->use_irq[p/32] |= 1 << (p % 32);
                if(vgic->enable_g0 || vgic->enable_g1){
                    /* gic_disable_int(p); */
                }
            }
        }
    }
    else if(GICD_ICENABLER(1)<=addr && addr<GICD_ISPENDER(0)){
        UW i = (addr-GICD_ICENABLER(0))/4;

        vgic->gicd_isenabler[i] &= ~reg_value;

        for(UW j=0;j<32;j++) if(reg_value&(1 << j)){
            int p = vgic->vtop[i*32+j];
            if(p>=0){
                vgic->use_irq[p/32] &= ~(1 << (p % 32));
                if(vgic->enable_g0 || vgic->enable_g1){
                    gic_disable_int(p);
                }
            }
        }
    } else if(GICD_IPRIORITYR(0)<=addr && addr<GICD_IPRIORITYR(PPI_ID_MAX/4)){
        UW id = vcpu_get_executing()->vid;
        ((UW *)vgic->core_state[id].ppi_ipriorityr)[(addr-GICD_IPRIORITYR(0))/4] = reg_value;
    } else if(GICD_IPRIORITYR(PPI_ID_MAX/4)<=addr && addr<GICD_IPRIORITYR(SPI_ID_MAX/4)){
        ((UW *)vgic->gicd_ipriorityr)[(addr-GICD_IPRIORITYR(0))/4] = reg_value;
    } else if(GICD_ICFGR(PPI_ID_MAX/16)<=addr && addr<GICD_ICFGR(SPI_ID_MAX/16)){
        for(UB i=0;i<4;i++){
            virtual_gicd_write_emulate_b(vgic, addr+i, (reg_value >> (8*i))& 0xff);
        }
    }
    else if(GICD_ITARGETSR(PPI_ID_MAX/4)<=addr && addr <GICD_ITARGETSR(SPI_ID_MAX/4)){
        ((UW *)vgic->gicd_itargetsr)[(addr-GICD_ITARGETSR(0))/4] = reg_value;
    } else if(addr==GICD_SGIR){
        UB tlf = (reg_value >> 24) & 0x3;
        UB ctl = (reg_value >> 16) & 0xff;
        UB id = reg_value & 0xf;

        T_VCPU *vcpu = vcpu_get_executing();

        switch(tlf){
        case 0x0:
            virtual_gic_send_software_int_inner(vgic, ctl, id);
            break;
        case 0x1:
            virtual_gic_send_software_int_inner(vgic, (1 << vgic->vm->vcpu_num)-1 - (1 << vcpu->vid), id);
            break;
        case 0x2:
            virtual_gic_send_software_int_inner(vgic, 1 << vcpu->vid, id);
            break;
        default:
            break;
        }

    } else {
        tv_abort("# unsupported access\n");
    }

}

void virtual_gicd_write_emulate_h(vgic_t *vgic,UW addr, UW reg_value)
{
    if(vgic==NULL) return;
    if(addr==GICD_CTRL){
        virtual_gicd_write_emulate_b(vgic, addr, reg_value);
    } else if(GICD_ISENABLER(0)<=addr && addr<GICD_ISPENDER(0)){
        virtual_gicd_write_emulate_w(vgic, addr&~0x3, reg_value << (8 * (addr&0x3)));
    }
    else if(GICD_IPRIORITYR(0)<=addr && addr < GICD_IPRIORITYR(PPI_ID_MAX/4)){
        UW id = vcpu_get_executing()->vid;
        ((HW *)vgic->core_state[id].ppi_ipriorityr)[(addr-GICD_IPRIORITYR(0))/2] = reg_value;
    } else if(GICD_IPRIORITYR(PPI_ID_MAX/4)<=addr && addr<GICD_IPRIORITYR(0x100)){
        ((HW *)vgic->gicd_ipriorityr)[(addr-GICD_IPRIORITYR(0))/2] = reg_value;
    } else if(GICD_IGROUPR(0)<=addr && addr<GICD_ISENABLER(0)){
        tv_abort("! use group\n");
    } else if(GICD_ICFGR(PPI_ID_MAX/16)<=addr && addr<GICD_ICFGR(SPI_ID_MAX/16)){
        for(UB i=0;i<2;i++){
            virtual_gicd_write_emulate_b(vgic, addr+i, (reg_value >> (8*i))& 0xff);
        }
    } else if(GICD_ITARGETSR(PPI_ID_MAX/4)<=addr && addr <GICD_ITARGETSR(SPI_ID_MAX/4)){
        ((UHW *)vgic->gicd_itargetsr)[(addr-GICD_ITARGETSR(0))/2] = (UHW)reg_value;
    } else {
        tv_abort("# unsupported access\n");
    }
}

void virtual_gicd_write_emulate_b(vgic_t *vgic,UW addr, UW reg_value)
{
    if(addr==GICD_CTRL){
        vgic->enable_g0 = reg_value&0x1;
        vgic->enable_g1 = reg_value&0x2;
        if(reg_value>0){
            for(UHW i=SGI_ID_MAX;i<SPI_ID_MAX;i++){
                if(vgic->use_irq[i/32]&(1 << (i%32))){
                    gic_enable_int(i,vgic->gicd_ipriorityr[i] + vgic->real_pri);
                }
            }
        } else {
            for(UHW i=SGI_ID_MAX;i<SPI_ID_MAX;i++){
                if(vgic->use_irq[i/32]&(1 << (i%32))) gic_disable_int(i);
            }
        }
    } else if(GICD_ISENABLER(0)<=addr && addr<GICD_ISPENDER(0)){
        virtual_gicd_write_emulate_w(vgic, addr&~0x3, reg_value << (8 * (addr&0x3)));
    } else if(GICD_IPRIORITYR(0)<=addr && addr < GICD_IPRIORITYR(PPI_ID_MAX/4)){
        UW id = vcpu_get_executing()->vid;
        vgic->core_state[id].ppi_ipriorityr[addr-GICD_IPRIORITYR(0)] = reg_value;
    } else if(GICD_IPRIORITYR(PPI_ID_MAX/4)<=addr && addr<GICD_IPRIORITYR(0x100)){
        vgic->gicd_ipriorityr[addr-GICD_IPRIORITYR(0)] = reg_value;
    } else if(GICD_SPENDSGIR(0)<=addr && addr<GICD_SPENDSGIR(SGI_ID_MAX/4)){
        virtual_gic_send_software_int_inner(vgic, reg_value, (addr-GICD_SPENDSGIR(0)));
    } else if(GICD_IGROUPR(0)<=addr && addr<GICD_ISENABLER(0)){
        tv_abort("! use group\n");
    }
    // interrupt configure register
    else if(GICD_ICFGR(PPI_ID_MAX/16)<=addr && addr<GICD_ICFGR(SPI_ID_MAX/16)){
        UW i = addr-GICD_ICFGR(0);
        ((UB *)vgic->gicd_icfgr)[i] = reg_value | 0x55;
        for(UW j=0;j<4;j++){
            HW id = vgic->vtop[i*4+j];
            if(id>0){
                UW val = INW(GICD_ICFGR(id/16));
                if(reg_value&(1 << (j*2+1))){
                    val |= 1 << (id%16*2 + 1);
                } else {
                    val &= ~(1 << (id%16*2 + 1));
                }
                OUTW(GICD_ICFGR(id/16),val);
            }
        }
    }
    else if(GICD_ITARGETSR(PPI_ID_MAX/4)<=addr && addr <GICD_ITARGETSR(SPI_ID_MAX/4)){
        vgic->gicd_itargetsr[addr-GICD_ITARGETSR(0)] = reg_value;
        /*
        tv_enable_print();
        tv_print_string_hex("#target_addr: ", addr);
        tv_print_string_hex("#reg_value: ", reg_value);
        tv_disable_print();
        */

    } else {
        tv_abort("# unsupported access\n");
        /*
        tv_enable_print();
        tv_print_string_hex("#target_addr: ", addr);
        tv_print_string_hex("#reg_value: ", reg_value);
        vcpu_save_state(vcpu_get_executing());
        vcpu_debug_print(vcpu_get_executing());
        tv_disable_print();
        */
    }
}

static UW virtual_gic_read_lr(vgic_t *vgic, UW vid, UW tar){
    if(vcpu_get_executing()==&(vgic->vm->vcpus[vid])){
        return gic_read_lr(tar);
    } else {
        return vgic->core_state[vid].saved_lr[tar];
    }
}

static UW virtual_gic_read_elsr0(vgic_t *vgic, UW vid){
    if(vcpu_get_executing()==&(vgic->vm->vcpus[vid])){
        return INW(GICH_ELSR0);
    } else {
        return vgic->core_state[vid].saved_elsr0;
    }
}

static UW virtual_gic_read_apr(vgic_t *vgic, UW vid){
    if(vcpu_get_executing()==&(vgic->vm->vcpus[vid])){
        return INW(GICH_APR);
    } else {
        return vgic->core_state[vid].saved_apr;
    }
}

static void virtual_gic_write_lr(vgic_t *vgic, UW vid, UW tar, UW lr_value){
    if(vcpu_get_executing()==&(vgic->vm->vcpus[vid])){
        gic_write_lr(tar, lr_value);
    } else {
        vgic->core_state[vid].saved_lr[tar]=lr_value;
        vgic->core_state[vid].saved_elsr0 &= ~(1 << tar);
    }
}

static bool virtual_gic_set_lr(vgic_t *vgic, UW vid, UW lr_value)
{
    UW is_empty = virtual_gic_read_elsr0(vgic, vid);
    UW is_active = virtual_gic_read_apr(vgic, vid);
    UB pri = gic_lr_read_pri(lr_value);
    UHW irq_no = gic_lr_read_vid(lr_value);
    vgic_core_state_t *state = &(vgic->core_state[vid]);

    if(state->irq_no_mask[irq_no/32]&((UW)1 << (irq_no%32))){
        tv_error_message("!!! same irq_no is pending in memory  !!!\n");
        tv_message(lr_value);
        tv_message(hyp_get_elr());
        tv_message(vid);
        tv_message(vcpu_get_executing()->vid);
        // tv_getc();
        return false;
    }

    if(irq_no <= SGI_ID_MAX){
        for(UW i=0;i<GICH_LR_NUM;i++){
            UW prev = virtual_gic_read_lr(vgic, vid, i);
            if((!(is_empty&(1 << i)))&&(prev&0xfffff)==(lr_value&0xfffff)){
                if(is_active&(1 << i)){
                    state->irq_no_mask[irq_no/32] |= ((UW)1 << (irq_no%32));
                    state->pending_lr[irq_no] = lr_value;
                    gic_set_np_int();
                    tv_puts("# irq active and pending.\n");
                    return true;
                }
                tv_error_message("!!! same irq_no is pending in lr!!!\n");
                tv_message(lr_value);
                tv_message(prev);
                tv_message(hyp_get_elr());
                tv_message(vid);
                tv_message(vcpu_get_executing()->vid);
                // tv_getc();
                return false;
            }
        }
    }
    else {
        for(UW i=0;i<GICH_LR_NUM;i++){
            UW prev = virtual_gic_read_lr(vgic, vid, i);
            if(!(is_empty&(1 << i))&&irq_no==gic_lr_read_vid(prev)){
                if(is_active&(1 << i)){
                    state->irq_no_mask[irq_no/32] |= ((UW)1 << (irq_no%32));
                    state->pending_lr[irq_no] = lr_value;
                    gic_set_np_int();
                    tv_puts("# irq active and pending.\n");
                    return true;
                }
                tv_error_message("!!! same irq_no is pending in lr!!!\n");
                tv_message(lr_value);
                tv_message(prev);
                tv_message(hyp_get_elr());
                tv_message(vid);
                tv_message(vcpu_get_executing()->vid);
                // tv_getc();
                return false;
            }
        }
    }

    for(UW i=0;i<GICH_LR_NUM;i++){
        if(is_empty&(1 << i)){
            virtual_gic_write_lr(vgic, vid, i, lr_value);
            return true;
        }
        UW prev_lr  = virtual_gic_read_lr(vgic, vid, i);
        UB prev_pri = gic_lr_read_pri(prev_lr);

        if(prev_pri > pri){
            virtual_gic_write_lr(vgic, vid, i, lr_value);
            lr_value = prev_lr;
            pri = prev_pri;
            // tv_puts("!!! premeption !!!\n");
        }
    }
    irq_no = gic_lr_read_vid(lr_value);
    vgic->core_state[vid].pending_lr[irq_no] = lr_value;
    vgic->core_state[vid].irq_no_mask[irq_no/32] |= 1 << (irq_no%32);
    gic_set_np_int();
    tv_error_message("# irq pending in memory\n");
    /*
    tv_enable_print();
    for(i=0;i<GICH_LR_NUM;i++){
        tv_print_string_hex("#lr: ");
    }
    tv_disable_print();
    */
    //tv_getc();
    return true;
}


// send only one core
bool virtual_gic_send_hardware_int(vgic_t *vgic, UW eoir)
{
    UB gicd_ctrl = vgic->enable_g0 || vgic->enable_g1;
    UW intvec = eoir & 0x3ff;
    UW vintvec = vgic->ptov[intvec];

    if(vintvec!=-1){
        bool grp1 = (vgic->gicd_igroupr[vintvec >> 5]&(1 << (vintvec&0x1f))) > 0;

        if(!gicd_ctrl){
            tv_abort("!!! gicd is disable !!!\n");
            return false;
        }

        HW target=-1;
        UW er;
        if(vintvec >= PPI_ID_MAX){
            er = vgic->gicd_isenabler[vintvec >> 5];
            if(er&(1 << (vintvec&0x1f))){
                UW mask = vgic->gicd_itargetsr[vintvec];
                for(UW i=0; i<vgic->vm->vcpu_num; i++) if(mask&(1 << i)){
                    T_VCPU *vcpu = &(vgic->vm->vcpus[i]);
                    if(vcpu->state==VCPU_STATE_SLEEP) vcpu_wakeup(vcpu);
                }
                target = vgic->balance_func(vgic, mask, vintvec);
            }
        } else {
            if(vcpu_get_last()==NULL){
                tv_abort("cannot specify core id!\n");
            } else {
                T_VCPU *vcpu = vcpu_get_last();
                target = vcpu->vid; 
                er = vgic->core_state[target].ppi_isenabler;
                if(vcpu->state==VCPU_STATE_SLEEP) vcpu_wakeup(vcpu);
            }
        }
        if(target<0){
            tv_error_message("!!! there are no target !!!\n");
            return false;
        }


        if(er&(1 << (vintvec&0x1f))){
            UB pri;
            if(vintvec >= PPI_ID_MAX){
                pri = vgic->gicd_ipriorityr[vintvec];
            } else {
                pri = vgic->core_state[target].ppi_ipriorityr[vintvec];
            }

            UW mask = gic_make_virtual_hardware_interrupt( vintvec, intvec, pri, grp1 );
            return virtual_gic_set_lr(vgic, target, mask);
        } else {
            tv_abort("!!! disabled int vec !!!\n");
        }

    }
    return false;
}

static bool virtual_gic_send_software_int_inner(vgic_t *vgic, UW mask, UW vintvec)
{
    // tv_error_message("send software int\n");
    // tv_message(hyp_get_elr());
    // tv_message(vcpu_get_executing()->vid);
    if(!vgic->enable_g0 && !vgic->enable_g1) {
        return false;
    }
    if(vintvec!=-1){
        if(vintvec>=PPI_ID_MAX){
            W id = vgic->balance_func( vgic, mask, vintvec );
            UW er = vgic->gicd_isenabler[vintvec >> 5];
            if((er&(1 << (vintvec&0x1f))) && id>=0){
                if(vgic->vm->vcpus[id].state==VCPU_STATE_SLEEP){
                    // tv_print_string_hex("# wake up:", id);
                    vcpu_wakeup(&(vgic->vm->vcpus[id]));
                }
                UB pri = vgic->gicd_ipriorityr[vintvec];
                bool grp1 = (vgic->gicd_igroupr[vintvec >> 5]&(1 << (vintvec&0x1f))) > 0;
                UW val = gic_make_virtual_software_interrupt( vintvec, pri, grp1 );
                return virtual_gic_set_lr(vgic, id , val);
            }
        }
        else {
            bool flag = false;
            for(UW i=0;i<vgic->vm->vcpu_num;i++) {
                if((mask&(1 << i))
                        && (vgic->core_state[i].ppi_isenabler&(1 << vintvec))){
                    if(vgic->vm->vcpus[i].state==VCPU_STATE_SLEEP){
                        // tv_print_string_hex("# wake up:", i);
                        vcpu_wakeup(&(vgic->vm->vcpus[i]));
                    }
                    UB pri = vgic->core_state[i].ppi_ipriorityr[vintvec];
                    bool grp1 = (vgic->gicd_igroupr[vintvec >> 5]&(1 << vintvec)) > 0;
                    UW val;
                    if(vintvec>=SGI_ID_MAX){
                        val = gic_make_virtual_software_interrupt( vintvec, pri, grp1 );
                    } else {
                        val = gic_make_virtual_software_sgi( vintvec, vcpu_get_executing()->vid, pri, grp1 );
                    }
                    flag |= virtual_gic_set_lr(vgic, i , val);
                }
            }
            return flag;
        }
    }
    return false;
}

bool virtual_gic_send_software_int(vgic_t *vgic, UW vintvec)
{
    if(vintvec>=PPI_ID_MAX){
        return virtual_gic_send_software_int_inner(vgic, vgic->gicd_itargetsr[vintvec], vintvec);
    } else {
        return false;
    }
}

void virtual_gic_maintenance(vgic_t *vgic, UW id)
{
    // tv_puts("# this is maintenance int\n");
    UW misr = INW(GICH_MISR);
    if(misr&0x1){
    } else if(misr&(1 << 3)){
        vgic_core_state_t *state = &(vgic->core_state[id]);
        UB max_pri = 0xff;
        HW tar = -1;
        for(UHW i=0;i<SPI_ID_MAX;i++) if(state->irq_no_mask[i >> 5]&((UW)1 << (i&0x1f))){
            UB pri = gic_lr_read_pri(state->pending_lr[i]);
            if(pri<max_pri){
                max_pri = pri;
                tar = i;
            } else if(pri==max_pri && (state->pending_lr[tar]&(1 << 29))){
                tar = i;
            }
        }
        if(tar>=0) {
            state->irq_no_mask[tar >> 5] &= ~((UW)1 << (tar&0x1f));
            virtual_gic_set_lr(vgic, id, vgic->core_state[id].pending_lr[tar]);
        } else {
            gic_clear_np_int();
        }
    } else if(misr&(1 << 5)){
    } else {
    }
}

void virtual_gic_start(vgic_t *vgic){
}


void virtual_gic_stop(vgic_t *vgic){
}

void virtual_gic_enable_spi(vgic_t *vgic, UW vintvec){
    vgic->gicd_isenabler[vintvec/32] |= 1 << (vintvec%32);
    gic_enable_int(vgic->vtop[vintvec], vgic->gicd_ipriorityr[vintvec] + vgic->real_pri);
}

void virtual_gic_enable_ppi(vgic_t *vgic, UW vintvec, UW id){
    vgic->core_state[id].ppi_isenabler |= 1 << vintvec;
    gic_enable_int(vgic->vtop[vintvec], vgic->core_state[id].ppi_ipriorityr[vintvec] + vgic->real_pri);
}
