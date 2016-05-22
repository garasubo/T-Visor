#include "basic.h"
#include "type.h"
#include "virtual_psci.h"
#include "vcpu.h"
#include "vm.h"
#include "debug.h"

static UW virtual_psci_cpu_suspend(UW state, UW address, UW cid){
    return PSCI_RET_NOT_SUPPORTED;
}

static UW virtual_psci_cpu_off(void){
    vcpu_off(vcpu_get_executing());
    return 0;
}

static UW virtual_psci_cpu_on(UW target, UW address, UW cid){
    UW id = target & 0x3;
    T_VM *vm = vcpu_get_executing()->vm;
    if(id >= vm->vcpu_num){
        tv_abort("id error\n");
        return PSCI_RET_INVAL_ID_PARAMETERS;
    }
    T_VCPU *vcpu = &(vm->vcpus[id]);

    if(vcpu->state!=VCPU_STATE_INIT){
        return PSCI_RET_ALREADY_ON;
    }
    vcpu_reset(vcpu);
    vcpu->reg[0] = cid;
    vcpu->pc = address;
    vcpu_ready(vcpu);
    return 0;
}

UW virtual_psci(void)
{
    tv_abort("psci call!\n");
    UW* sp = vcpu_get_current_sp()->reg;
    UW fid = sp[0];
    UW ret = 0;
    switch(fid){
    case PSCI_FN_ID_CPU_SUSPEND:
        ret = virtual_psci_cpu_suspend(sp[1], sp[2], sp[3]);
        break;
    case PSCI_FN_ID_CPU_OFF:
        ret = virtual_psci_cpu_off();
        break;
    case PSCI_FN_ID_CPU_ON:
        ret = virtual_psci_cpu_on(sp[1], sp[2], sp[3]);
        break;
    default:
        ret = PSCI_RET_NOT_SUPPORTED;
        tv_abort("unimplemented function!\n");
        break;
    }
    return ret;
}

