#include "hyp_call.h"
#include "vcpu.h"
#include "coproc.h"
#include "debug.h"
#include "cp_access.h"

void coproc_cp15_emulate(T_VCPU *vcpu, UW iss){
    bool is_write = iss&0x1;
    UW crn = (iss >> 10)&0xf;
    UW crm = (iss >> 1)&0xf;
    UW opc1 = (iss >> 14)&0x7;
    UW opc2 = (iss >> 17)&0x7;
    UW rt = (iss >> 5)&0xf;
    /* bool cv = iss >> 24; */
    /* UW cond = (iss >> 20)&0xf; */

    // actlr
    if(crn==1&&crm==0&&opc1==0&&opc2==1){
        if(is_write) return;

        hyp_reg_write(rt, vcpu->actlr, vcpu_get_current_sp()->reg);
    }
    // l2ctlr
    else if(crn==9&&crm==0&&opc1==1&&opc2==2){
        if(is_write) return;
        hyp_reg_write(rt, vcpu->vm->l2ctlr, vcpu_get_current_sp()->reg);
    }
    // l2ectlr
    else if(crn==9&&crm==0&&opc1==1&&opc2==2){
        if(is_write) return;
        hyp_reg_write(rt, 0, vcpu_get_current_sp()->reg);
    }
    // mair0
    else if(crn==10&&crm==2&&opc1==0&&opc2==0){
        if(is_write) {
            vcpu->mair0 = hyp_reg_read(rt, vcpu_get_current_sp()->reg);
            MAIR0_WRITE(vcpu->mair0);
        }
        else {
            hyp_reg_write(rt, vcpu->mair0, vcpu_get_current_sp()->reg);
        }
    }
    // mair1
    else if(crn==10&&crm==2&&opc1==0&&opc2==1){
        if(is_write) {
            vcpu->mair0 = hyp_reg_read(rt, vcpu_get_current_sp()->reg);
            MAIR0_WRITE(vcpu->mair1);
        }
        else {
            hyp_reg_write(rt, vcpu->mair1, vcpu_get_current_sp()->reg);
        }
    }
    else {
        tv_error_message("unsupported trap!\n");
    }
}
