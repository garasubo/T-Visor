#include "basic.h"
#include "vcpu.h"
#include "type.h"
#include "virtual_gic.h"
#include "hyp_call.h"
#include "memory_manage.h"
#include "debug.h"
#include "cpsr.h"
#include "cp_access.h"
#include "sctlr.h"
#include "timer.h"
#include "timer_event.h"
#include "hcr.h"
#include "logger.h"

static T_VCPU t_vcpu[NUM_OF_T_VCPU];
static UW num_vcpu = 0;
static T_VCPU *exec_cpu = NULL;
static T_VCPU *last_cpu = NULL;
static vcpu_stack_t* _sp = NULL;

static void _vcpu_init(T_CVCPU *cvcpu, T_VCPU *vcpu, UW id, UB vid, T_VM *vm){
    vcpu->id = id;
    vcpu->vid = vid;
    vcpu->pc = cvcpu->pc;
    vcpu->vtoff = cvcpu->vtoff;

    vcpu->vm = vm;

    //vcpu->mpid = 0xc0000000;
    Asm("mrc p15, 0, %0, c0, c0, 5":"=r"(vcpu->mpid));
    vcpu->mpid &= ~0x3;
    vcpu->mpid |= vid;

    Asm("mrc p15, 0, %0, c1, c0, 1":"=r"(vcpu->actlr));

    vcpu->sched_param = cvcpu->sched_param;

    vcpu_reset(vcpu);
}

void vcpu_reset(T_VCPU *vcpu){
    vcpu->state = VCPU_STATE_INIT;
    vcpu->cpsr = CPSR_DEFAULT;
    vcpu->ttbcr = 0;
    vcpu->sctlr = SCTLR_DEFALT;

    vcpu->vtctl = 0x2;
    vcpu->vtcmp = 0xffffffffffffffffLL;

    vcpu->vbar = 0;
    vcpu->sched_state =  scheduler_get_operations()->allocate(vcpu);
}

void vcpu_off(T_VCPU *vcpu){
    // scheduler_get_operations()->free(vcpu);
    vcpu->state = VCPU_STATE_INIT;
    if(vcpu==exec_cpu){
        exec_cpu = NULL;
    }
}

T_VCPU *vcpu_setup(T_CVCPU *cvcpu, UW num, T_VM *vm){
    if(num_vcpu+num >= NUM_OF_T_VCPU){
        tv_abort("! cannot create VCPU!\n");
        return NULL;
    }
    for(UW i=0;i<num;i++){
        _vcpu_init(&(cvcpu[i]), &(t_vcpu[num_vcpu+i]), num_vcpu+i, i, vm);
    }
    T_VCPU *ret = &(t_vcpu[num_vcpu]);
    num_vcpu += num;
    return ret;
}



void vcpu_set_current_sp(vcpu_stack_t *sp){
    _sp = sp;
}

vcpu_stack_t *vcpu_get_current_sp(void){
    return _sp;
}

void vcpu_ready(T_VCPU *vcpu){
    if(vcpu->state!=VCPU_STATE_INIT){
        tv_abort("! wrong state \n");
        return;
    }
    vcpu->state = VCPU_STATE_WAIT;
    scheduler_get_operations()->enque(vcpu);
}

void vcpu_start(T_VCPU *vcpu){
    if(exec_cpu!=NULL){
        tv_abort("! There is executing vcpu !");
    }
    if(vcpu==NULL){
        return;
    }
    if(vcpu->state==VCPU_STATE_START){
        tv_abort("! this is already start !");
        return;
    }

    vcpu->state = VCPU_STATE_START;
    hyp_set_elr(vcpu->pc);


    //FlushTLB();
    SetTTBR(vcpu->ttbr0, vcpu->ttbr1);
    SetTTBCR(vcpu->ttbcr);
    CSSELR_WRITE(vcpu->csselr);
    SCTLR_WRITE(vcpu->sctlr);
    CPACR_WRITE(vcpu->cpacr);
    DACR_WRITE(vcpu->dacr);
    DFAR_WRITE(vcpu->dfar);
    DFSR_WRITE(vcpu->dfsr);
    IFAR_WRITE(vcpu->ifar);
    IFSR_WRITE(vcpu->ifsr);
    ADFSR_WRITE(vcpu->adfsr);
    AIFSR_WRITE(vcpu->aifsr);
    PRRR_WRITE(vcpu->prrr);
    NMRR_WRITE(vcpu->nmrr);
    CIDR_WRITE(vcpu->cidr);
    TPIDRPRW_WRITE(vcpu->tpidrprw);
    TPIDRURO_WRITE(vcpu->tpidruro);
    TPIDRURW_WRITE(vcpu->tpidrurw);

    VBAR_WRITE(vcpu->vbar);
    VMPIDR_WRITE(vcpu->mpid);

    MAIR0_WRITE(vcpu->mair0);
    MAIR1_WRITE(vcpu->mair1);

    PAR_WRITE(vcpu->par_lo, vcpu->par_hi);

    vtimer_set(vcpu->vtctl,vcpu->vtcmp);
    vtimer_set_offset(vcpu->vtoff);
    if(vcpu->timer_event!=NULL){
        timer_event_remove(vcpu->timer_event);
        vcpu->timer_event = NULL;
    }
    exec_cpu = vcpu;
    last_cpu = vcpu;

    virtual_gic_restore_state(vcpu->vm->vgic, vcpu->vid);

    Asm("msr spsr_hyp, %0"::"r"(vcpu->cpsr));

    Asm("ldmia %[addr], {r0,r1,r2,r3,ip}\n"
        "msr spsr_svc, r0\n"
        "msr spsr_abt, r1\n"
        "msr spsr_und, r2\n"
        "msr spsr_irq, r3\n"
        "msr spsr_fiq, ip"
            ::[addr]"r"(vcpu->spsr)
            :"r0","r1","r2","r3","ip","memory");

    Asm("ldmia %[addr], {r4,r5,r6,r7,r8,r9}\n"
        "msr sp_svc, r4\n"
        "msr sp_abt, r5\n"
        "msr sp_und, r6\n"
        "msr sp_irq, r7\n"
        "msr sp_fiq, r8\n"
        "msr sp_usr, r9"
            ::[addr]"r"(vcpu->sp)
            :"r4","r5","r6","r7","r8","r9","memory");

    Asm("ldmia %[addr], {r0,r1,r2,r3,ip}\n"
        "msr r8_fiq, r0\n"
        "msr r9_fiq, r1\n"
        "msr r10_fiq, r2\n"
        "msr r11_fiq, r3\n"
        "msr r12_fiq, ip"
            ::[addr]"r"(vcpu->fiq_banked)
            :"r0","r1","r2","r3","ip","memory");


    Asm("ldmia %[addr], {r4,r5,r6,r7,r8,r9}\n"
        "msr lr_svc, r4\n"
        "msr lr_abt, r5\n"
        "msr lr_und, r6\n"
        "msr lr_irq, r7\n"
        "msr lr_fiq, r8\n"
        "msr lr_usr, r9"
            ::[addr]"r"(vcpu->lr)
            :"r4","r5","r6","r7","r8","r9","memory");


    _sp->fpu_state.fpexc = vcpu->fpu_state.fpexc;
    _sp->fpu_state.fpscr = vcpu->fpu_state.fpscr;
    Asm("mov r0, %[addr]\n"
        "mov r1, %[sp]\n"
        "vldmia r0!, {d16-d31}\n"
        "vstmia r1!, {d16-d31}\n"
        "vldmia r0!, {d16-d31}\n"
        "vstmia r1!, {d16-d31}"
            ::[addr]"r"(vcpu->fpu_state.vfpr),[sp]"r"(_sp->fpu_state.vfpr)
            :"d16","d17","d18","d19","d20","d21","d22","d23",
            "d24","d25","d26","d27","d28","d29","d30","d31",
            "r0","r1","memory");
    for(HW i=0;i<14;i++){
        _sp->reg[i] = vcpu->reg[i];
    }

    SetVTTBR((UD)(UW)vcpu->vm->page_table| (UD)(vcpu->id&0xff) << 48);
    Asm("clrex");

}

void vcpu_stop(void){
    exec_cpu = NULL;
}

void vcpu_save_state(T_VCPU *vcpu){
    if(vcpu==NULL){
        tv_abort("! Cannot save NULL VCPU\n");
        return;
    }

    vcpu->ttbr0 = GetTTBR0();
    vcpu->ttbr1 = GetTTBR1();
    vcpu->ttbcr = GetTTBCR();
    CSSELR_READ(vcpu->csselr);
    SCTLR_READ(vcpu->sctlr);
    CPACR_READ(vcpu->cpacr);
    DACR_READ(vcpu->dacr);
    DFSR_READ(vcpu->dfsr);
    DFAR_READ(vcpu->dfar);
    IFSR_READ(vcpu->ifsr);
    IFAR_READ(vcpu->ifar);
    ADFSR_READ(vcpu->adfsr);
    AIFSR_READ(vcpu->aifsr);
    PRRR_READ(vcpu->prrr);
    NMRR_READ(vcpu->nmrr);
    CIDR_READ(vcpu->cidr);
    TPIDRPRW_READ(vcpu->tpidrprw);
    TPIDRURO_READ(vcpu->tpidruro);
    TPIDRURW_READ(vcpu->tpidrurw);

    VBAR_READ(vcpu->vbar);
    MAIR0_READ(vcpu->mair0);
    MAIR1_READ(vcpu->mair1);

    PAR_READ(vcpu->par_lo, vcpu->par_hi);

    vcpu->vtctl = vtimer_get_status();
    vcpu->vtcmp = vtimer_get_cmp_value();


    vcpu->pc = hyp_get_elr();
    Asm("mrs %0, spsr_hyp":"=r"(vcpu->cpsr));
    Asm("mrs r0, spsr_svc \n"
        "mrs r1, spsr_abt \n"
        "mrs r2, spsr_und \n"
        "mrs r3, spsr_irq \n"
        "mrs ip, spsr_fiq \n"
        "stmia %[addr], {r0,r1,r2,r3,ip}"
            ::[addr]"r"(vcpu->spsr)
            :"r0","r1","r2","r3","ip","memory");

    Asm("mrs r0, sp_svc \n"
        "mrs r1, sp_abt \n"
        "mrs r2, sp_und \n"
        "mrs r3, sp_irq \n"
        "mrs r4, sp_fiq \n"
        "mrs ip, sp_usr \n"
        "stmia %[addr], {r0,r1,r2,r3,r4,ip}"
            ::[addr]"r"(vcpu->sp)
            :"r0","r1","r2","r3","r4","ip","memory");

    Asm("mrs r0, lr_svc \n"
        "mrs r1, lr_abt \n"
        "mrs r2, lr_und \n"
        "mrs r3, lr_irq \n"
        "mrs r4, lr_fiq \n"
        "mrs ip, lr_usr \n"
        "stmia %[addr], {r0,r1,r2,r3,r4,ip}"
            ::[addr]"r"(vcpu->lr)
            :"r0","r1","r2","r3","r4","ip","memory");

    Asm("mrs r0, r8_fiq\n"
        "mrs r1, r9_fiq\n"
        "mrs r2, r10_fiq\n"
        "mrs r3, r11_fiq\n"
        "mrs ip, r12_fiq\n"
        "stmia %[addr], {r0,r1,r2,r3,ip}"
            ::[addr]"r"(vcpu->fiq_banked)
            :"r0","r1","r2","r3","ip","memory");

    vcpu->fpu_state.fpexc = _sp->fpu_state.fpexc;
    vcpu->fpu_state.fpscr = _sp->fpu_state.fpscr;
    Asm("mov r0, %[sp]\n"
        "mov r1, %[addr]\n"
        "vldmia r0!, {d16-d31}\n"
        "vstmia r1!, {d16-d31}\n"
        "vldmia r0!, {d16-d31}\n"
        "vstmia r1!, {d16-d31}"
            ::[addr]"r"(vcpu->fpu_state.vfpr),[sp]"r"(_sp->fpu_state.vfpr)
            :"d16","d17","d18","d19","d20","d21","d22","d23",
            "d24","d25","d26","d27","d28","d29","d30","d31",
            "r0","r1","memory");
    for(HW i=0;i<14;i++){
        vcpu->reg[i] = _sp->reg[i];
    }

    virtual_gic_save_state(vcpu->vm->vgic, vcpu->vid);
}

T_VCPU *vcpu_find_by_id(UW id){
    return &(t_vcpu[id]);
}

T_VCPU *vcpu_get_executing( void )
{
    return exec_cpu;
}

T_VCPU *vcpu_get_last( void )
{
    return last_cpu;
}

void vcpu_wakeup(T_VCPU *vcpu){
    if(vcpu->timer_event){
        timer_event_remove(vcpu->timer_event);
        vcpu->timer_event = NULL;
    }
    scheduler_get_operations()->unblock(vcpu);
    vcpu->state = VCPU_STATE_WAIT;
}


static void vcpu_wakeup_by_timer(T_VCPU *vcpu){
    vcpu->timer_event = NULL; // ????
    scheduler_get_operations()->unblock(vcpu);
    vcpu->state = VCPU_STATE_WAIT;
}

void vcpu_sleep(T_VCPU *vcpu){
    if(exec_cpu!=vcpu){
        tv_abort("!!! cannot sleep !!!");
        return;
    }
    vcpu_save_state(vcpu);
    scheduler_get_operations()->yield();
    vcpu->state = VCPU_STATE_SLEEP;
    if((vcpu->vtctl&0x7) ==0x5){
        vcpu->timer_event = timer_event_insert( vcpu->vtcmp + vcpu->vtoff, (void (*)(void *))vcpu_wakeup_by_timer, (void *)vcpu );
    }
    exec_cpu = NULL;
}

void vcpu_make_wait(T_VCPU *vcpu){
    if(exec_cpu==vcpu){
        vcpu_save_state(vcpu);
        scheduler_get_operations()->yield();
        vcpu->state = VCPU_STATE_SLEEP;
        exec_cpu = NULL;
    } else {
        tv_abort("why wait!?\n");
    }
}


void vcpu_preemption(T_VCPU *vcpu){
    if(vcpu==NULL){
        if(exec_cpu==NULL && vcpu_get_last()){
            vm_stop(vcpu_get_last()->vm);
        } else {
            exec_cpu = NULL;
        }
        return;
    }
    if(exec_cpu==NULL) {
        vm_start(vcpu->vm);
        vcpu_start(vcpu);
        return;
    }
    if(exec_cpu==vcpu) return;
    if(last_cpu==vcpu){
        exec_cpu = vcpu;
        return;
    }
    vcpu_save_state(exec_cpu);
    scheduler_get_operations()->block(exec_cpu);
    if(exec_cpu->vm != vcpu->vm){
        vm_stop(exec_cpu->vm);
        vm_start(vcpu->vm);
    }
    exec_cpu->state = VCPU_STATE_WAIT;
    exec_cpu = NULL;
    vcpu_start(vcpu);
    scheduler_clear_flag();
}


void vcpu_debug_print( T_VCPU *vcpu ){
    if(vcpu==NULL) return;
    if(vcpu->state==VCPU_STATE_SLEEP){
        tv_puts("This cpu is sleeping\n");
    }
    tv_print_string_hex("#vcpu id:", vcpu->id);
    tv_print_string_hex("#pc:", vcpu->pc);
    tv_print_string_hex("#cpsr:", vcpu->cpsr);
    tv_print_string_hex("#r0:     ", vcpu->reg[0]);
    tv_print_string_hex("#r1:     ", vcpu->reg[1]);
    tv_print_string_hex("#r2:     ", vcpu->reg[2]);
    tv_print_string_hex("#r3:     ", vcpu->reg[3]);
    tv_print_string_hex("#r4:     ", vcpu->reg[4]);
    tv_print_string_hex("#r5:     ", vcpu->reg[5]);
    tv_print_string_hex("#r6:     ", vcpu->reg[6]);
    tv_print_string_hex("#r7:     ", vcpu->reg[7]);
    tv_print_string_hex("#r8:     ", vcpu->reg[8]);
    tv_print_string_hex("#r9:     ", vcpu->reg[9]);
    tv_print_string_hex("#r10:    ", vcpu->reg[10]);
    tv_print_string_hex("#r11:    ", vcpu->reg[11]);
    tv_print_string_hex("#sp_svc: ", vcpu->sp[ID_SVC]);
    tv_print_string_hex("#sp_abt: ", vcpu->sp[ID_ABT]);
    tv_print_string_hex("#sp_und: ", vcpu->sp[ID_UND]);
    tv_print_string_hex("#sp_fiq: ", vcpu->sp[ID_FIQ]);
    tv_print_string_hex("#sp_irq: ", vcpu->sp[ID_IRQ]);
    tv_print_string_hex("#sp_usr: ", vcpu->sp[ID_USR]);
    tv_print_string_hex("#lr_svc: ", vcpu->lr[ID_SVC]);
    tv_print_string_hex("#lr_abt: ", vcpu->lr[ID_ABT]);
    tv_print_string_hex("#lr_und: ", vcpu->lr[ID_UND]);
    tv_print_string_hex("#lr_fiq: ", vcpu->lr[ID_FIQ]);
    tv_print_string_hex("#lr_irq: ", vcpu->lr[ID_IRQ]);
    tv_print_string_hex("#lr_usr: ", vcpu->lr[ID_USR]);
    tv_print_string_hex("#spsr_svc: ", vcpu->spsr[ID_SVC]);
    tv_print_string_hex("#spsr_abt: ", vcpu->spsr[ID_ABT]);
    tv_print_string_hex("#spsr_und: ", vcpu->spsr[ID_UND]);
    tv_print_string_hex("#spsr_fiq: ", vcpu->spsr[ID_FIQ]);
    tv_print_string_hex("#spsr_irq: ", vcpu->spsr[ID_IRQ]);
    tv_print_string_hex("#vtcmp: ", vcpu->vtcmp);
    tv_print_string_hex("#vtctl: ", vcpu->vtctl);
    tv_print_string_hex("#ttbcr: ", vcpu->ttbcr);
    tv_print_string_hex("#ttbr0: ", vcpu->ttbr0);
    tv_print_string_hex("#ttbr1: ", vcpu->ttbr1);
    tv_print_string_hex("#mair0: ", vcpu->mair0);
    tv_print_string_hex("#mair1: ", vcpu->mair1);
    tv_print_string_hex("#time : ", vtimer_get_value());

    UW i = 0;
    vgic_t *vgic = vcpu->vm->vgic;
    tv_print_string_hex("#elsr0: ", vgic->core_state[(UW)vcpu->vid].saved_elsr0);
    tv_print_string_hex("#apr  : ", vgic->core_state[(UW)vcpu->vid].saved_apr);
    for(i=0;i<GICH_LR_NUM;i++){
        tv_print_string_hex("#giclr: ", vgic->core_state[(UW)vcpu->vid].saved_lr[i]);
    }
}
