#ifndef QFB_DRIVER_HH
#define QFB_DRIVER_HH

#include <Multiverse.h>
#include "extraverse.h"
#ifdef DEBUG_QFB
#include "printf.h"
#endif

#include "HLocker.hh"

struct QFB {
  /* Reading this returns 'qfb0', writing it resets the device. */
  uint32_t version;
  /* Current width, height, depth, and base */
  uint32_t width, height, depth, base;
  /* Rowbytes for the current mode (R/O) */
  uint32_t rowbytes;
  /* Unused register, reserved */
  uint32_t reserved;
  /* Palette index and RW port */
  uint32_t pal_index, pal_color;
  /* Gamma LUT index and RW port */
  uint32_t lut_index, lut_color;
  /* IRQ mask, list of interrupts that are ENABLED */
  uint32_t irq_mask;
  /* IRQ status. Reading returns outstanding IRQs. Write an IRQ to ack it. */
  uint32_t irq;
  /* Width, height, and depth specified on the QEMU command line. (R/O) */
  uint32_t user_width, user_height, user_depth;
};
#define QFB_IRQ_VBL 1

struct Locals {
  volatile QFB* qfb;
  uint8_t* vram;
  SlotIntQElement* slot_queue_element;
  uint8_t cur_mode, slot;
  bool gray_mode_enabled;
  bool irq_enabled;
};

extern "C" { // so MacsBug symbols are valid

int qfb_drvr_open(ParmBlkPtr params, DCtlPtr dce, uint32_t slot);
int qfb_drvr_close(ParmBlkPtr params, DCtlPtr dce);
int qfb_drvr_control(ParmBlkPtr params, DCtlPtr dce);
int qfb_drvr_status(ParmBlkPtr params, DCtlPtr dce);
void qfb_gray_clut(HLocker<Locals>& locals);
void qfb_gray_pixels(HLocker<Locals>& locals, uint32_t page);
int qfb_common_set_entries(CntrlParam* params, HLocker<Locals>& locals);
uint32_t qfb_calculate_stride(uint32_t width, uint32_t depth);
uint16_t qfb_calculate_num_pages(uint32_t width, uint32_t height, uint32_t depth);
int qfb_enable_interrupts(DCtlPtr dce);
int qfb_disable_interrupts(DCtlPtr dce);
short qfb_interrupt_service_routine(uint32_t);
void mystrcpy(char* dst, const char* src);
void mymemcpy(void* dst, const void* src, size_t size);

/* Status routines */
int qfb_get_mode(CntrlParam* params, DCtlPtr dce);
int qfb_get_entries(CntrlParam* params, DCtlPtr dce);
int qfb_get_page_count(CntrlParam* params, DCtlPtr dce);
int qfb_get_page_base(CntrlParam* params, DCtlPtr dce);
int qfb_get_gray(CntrlParam* params, DCtlPtr dce);
int qfb_get_interrupt(CntrlParam* params, DCtlPtr dce);
int qfb_get_gamma(CntrlParam* params, DCtlPtr dce);
int qfb_get_default_mode(CntrlParam* params, DCtlPtr dce);
int qfb_get_connection(CntrlParam* params, DCtlPtr dce);
int qfb_get_video_parameters(CntrlParam* params, DCtlPtr dce);
int qfb_get_next_resolution(CntrlParam* params, DCtlPtr dce);
int qfb_get_mode_timing(CntrlParam* params, DCtlPtr dce);
int qfb_get_gamma_info_list(CntrlParam* params, DCtlPtr dce);
int qfb_retrieve_gamma_table(CntrlParam* params, DCtlPtr dce);

/* Control routines */
int qfb_reset(CntrlParam* params, DCtlPtr dce);
int qfb_kill_io(CntrlParam* params, DCtlPtr dce);
int qfb_set_mode(CntrlParam* params, DCtlPtr dce);
int qfb_set_entries(CntrlParam* params, DCtlPtr dce);
int qfb_set_gamma(CntrlParam* params, DCtlPtr dce);
int qfb_gray_page(CntrlParam* params, DCtlPtr dce);
int qfb_set_gray(CntrlParam* params, DCtlPtr dce);
int qfb_set_interrupt(CntrlParam* params, DCtlPtr dce);
int qfb_direct_set_entries(CntrlParam* params, DCtlPtr dce);
int qfb_set_default_mode(CntrlParam* params, DCtlPtr dce);

}

#ifdef DEBUG_QFB
#define dprintf(format, ...) printf(format ,##__VA_ARGS__)
#else
#define dprintf(...)
#endif

#define ONE_BIT_MODE 0x80
#define TWO_BIT_MODE 0x81
#define FOUR_BIT_MODE 0x82
#define EIGHT_BIT_MODE 0x83
#define SIXTEEN_BIT_MODE 0x84
#define THIRTY_TWO_BIT_MODE 0x85
#define FIRST_VALID_MODE ONE_BIT_MODE
#define LAST_VALID_MODE THIRTY_TWO_BIT_MODE

#define QFB_VRAM_SIZE 0x2000000 /* 32MiB */
/* the lowest address of VRAM accessible in the regular slot space */
#define QFB_VRAM_SLOT_BASE 0x10000

#endif
