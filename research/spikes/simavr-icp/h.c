#include <stdio.h>
#include <stdlib.h>
#include "sim_avr.h"
#include "sim_elf.h"
#include "avr_ioport.h"
static avr_t *avr; static int n=0;
static void pb1(struct avr_irq_t*irq,uint32_t v,void*p){ printf("PB1=%u at %.3f ms\n",v, avr->cycle*1000.0/avr->frequency); n++; }
int main(){ elf_firmware_t f={0}; elf_read_firmware("fw.elf",&f);
 avr=avr_make_mcu_by_name("atmega328p"); avr_init(avr); f.frequency=8000000; avr_load_firmware(avr,&f); avr->frequency=8000000;
 avr_irq_register_notify(avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('B'),1),pb1,NULL);
 avr_irq_t *pb0=avr_io_getirq(avr,AVR_IOCTL_IOPORT_GETIRQ('B'),0);
 uint64_t t_hi=80000, t_lo=160000; int s=0;
 while(avr->cycle<400000){ if(!s&&avr->cycle>=t_hi){avr_raise_irq(pb0,1);s=1;printf("PB0 rise at %.3f ms\n",avr->cycle/8000.0);} if(s==1&&avr->cycle>=t_lo){avr_raise_irq(pb0,0);s=2;printf("PB0 fall at %.3f ms\n",avr->cycle/8000.0);} avr_run(avr);} 
 return n==5?0:1; /* initial PB1=0 notification + 2 x (rise, compare-clear) */ }
