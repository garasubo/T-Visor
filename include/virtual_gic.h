#ifndef __INCLUDED_VIRTUAL_GIC_H__
#define __INCLUDED_VIRTUAL_GIC_H__

#include "basic.h"
#include "type.h"

struct t_vgic;
typedef struct t_vgic vgic_t;
typedef HW (*vgic_balance_func_t)(vgic_t *vgic, UW mask, UW vintvec);

#include "gic.h"
#include "vm.h"

typedef struct {
    UW id;
    UW vmcr;    //GIC state;
    UW pending_lr[SPI_ID_MAX];
    UW saved_lr[GICH_LR_NUM]; 
    UW saved_elsr0;
    UW saved_apr;
    UW saved_hcr;
    UW irq_no_mask[SPI_ID_MAX/32];  // pendingなirqを記録する

    UW ppi_isenabler;
    UB ppi_ipriorityr[PPI_ID_MAX];
} vgic_core_state_t;



struct t_vgic{
    T_VM *vm;
    HW ptov[SPI_ID_MAX];    // 実irq idを仮想irq idに
    HW vtop[SPI_ID_MAX];
    UW use_irq[SPI_ID_MAX/32];     // このgicが利用する実irq id
    UW real_pri;

    vgic_balance_func_t balance_func;

    vgic_core_state_t *core_state;

    bool enable_g0;
    bool enable_g1;
    UW gicd_typer;
    UW gicd_iidr;
    UW gicd_igroupr[SPI_ID_MAX/32];
    UW gicd_isenabler[SPI_ID_MAX/32];
    UB gicd_ipriorityr[SPI_ID_MAX];
    UB gicd_itargetsr[SPI_ID_MAX];
    UW gicd_icfgr[SPI_ID_MAX/16];
    UW gicd_nsacr[SPI_ID_MAX/16];
};

vgic_t *vgic_alloc(void);

void virtual_gic_init(vgic_t *vgic, T_VM *vm);
void virtual_gic_register_handle(void);

void virtual_gic_save_state(vgic_t *vgic, UW id);
void virtual_gic_restore_state(vgic_t *vgic, UW id);

UW virtual_gicd_read_emulate_w(vgic_t *vgic,UW addr);
UHW virtual_gicd_read_emulate_h(vgic_t *vgic,UW addr);
UB virtual_gicd_read_emulate_b(vgic_t *vgic,UW addr);
void virtual_gicd_write_emulate_w(vgic_t *vgic,UW addr, UW reg_value);
void virtual_gicd_write_emulate_h(vgic_t *vgic,UW addr, UW reg_value);
void virtual_gicd_write_emulate_b(vgic_t *vgic,UW addr, UW reg_value);

void virtual_gic_register_int(vgic_t *vgic, UHW pintvec, UHW vintvec);

bool virtual_gic_send_hardware_int(vgic_t *vgic, UW intvec);
bool virtual_gic_send_software_int(vgic_t *vgic, UW intvec);

void virtual_gic_maintenance(vgic_t *vgic, UW id);

void virtual_gic_stop(vgic_t *vgic);
void virtual_gic_start(vgic_t *vgic);

void virtual_gic_enable_spi(vgic_t *vgic, UW vintvec);
void virtual_gic_enable_ppi(vgic_t *vgic, UW vintvec, UW id);

#endif
