#include "qfb_driver.hh"

int qfb_drvr_open(ParmBlkPtr params, DCtlPtr dce, uint32_t slot) {
  (void)params;
  dprintf("--- mac_qfb_driver opened! ---\n");
  dprintf("Allocating memory.\n");
  /* Allocate memory for our Locals */
  ReserveMemSys(sizeof(Locals)+sizeof(SlotIntQElement));
  Handle local_handle = NewHandleSysClear(sizeof(Locals));
  if(!local_handle) return MemError();
#ifdef SUPPORT_AUX
  dprintf("Locking handle.\n");
  HLock(local_handle);
#else
  dprintf("Moving handle high.\n");
  MoveHHi(local_handle);
#endif
  dce->dCtlStorage = local_handle;
  HLocker<Locals> locals(local_handle);
  locals->qfb = reinterpret_cast<volatile QFB*>(0xF0000000 | (slot<<24));
  locals->vram = reinterpret_cast<uint8_t*>(slot << 28);
  locals->slot = slot;
  dprintf("VRAM: %p\tRegs: %p\nSlot: %X\n", locals->vram,
          locals->qfb, locals->slot);
  if(locals->qfb->version != 'qfb1') {
    DebugStr("\pWrong QFB version");
    DisposeHandle(local_handle);
    return openErr;
  }
  dprintf("Allocating Slot Interrupt Queue element.\n");
  locals->slot_queue_element = reinterpret_cast<SlotIntQElement*>(NewPtrSysClear(sizeof(SlotIntQElement)));
  if(locals->slot_queue_element == nullptr) {
    int ret = MemError();
    locals.unlock();
    DisposeHandle(local_handle);
    return ret;
  }
  dprintf("Initializing Slot Interrupt Queue element.\n");
  locals->slot_queue_element->sqType = 6; // sIQType
  locals->slot_queue_element->sqAddr = qfb_interrupt_service_routine;
  locals->slot_queue_element->sqParm = reinterpret_cast<uint32_t>(locals->qfb);
  dprintf("Installing gray palette.\n");
  qfb_gray_clut(locals);
  dprintf("Setting mode: %u x %u x %u\n",
          locals->qfb->user_width, locals->qfb->user_height,
          locals->qfb->user_depth);
  /* set up user-specified width and height, but 1-bpp, because that's the mode
     A/UX and MacOS expect to be active on open */
  locals->qfb->width = locals->qfb->user_width;
  locals->qfb->height = locals->qfb->user_height;
  switch(locals->qfb->user_depth) {
  case 1:
    locals->cur_mode = ONE_BIT_MODE;
    locals->qfb->depth = 1;
    break;
  case 2:
    locals->cur_mode = TWO_BIT_MODE;
    locals->qfb->depth = 2;
    break;
  case 4:
    locals->cur_mode = FOUR_BIT_MODE;
    locals->qfb->depth = 4;
    break;
  case 8:
    locals->cur_mode = EIGHT_BIT_MODE;
    locals->qfb->depth = 8;
    break;
  case 16:
    locals->cur_mode = SIXTEEN_BIT_MODE;
    locals->qfb->depth = 16;
    break;
  case 24: case 32:
    locals->cur_mode = THIRTY_TWO_BIT_MODE;
    locals->qfb->depth = 32;
    break;
  }
  locals->qfb->base = QFB_VRAM_SLOT_BASE;
  dprintf("Splatting gray pattern.\n");
  qfb_gray_pixels(locals, 0);
  locals.unlock();
  auto ret = qfb_enable_interrupts(dce);
  dprintf("Open complete!\n");
  return ret;
}

int qfb_drvr_close(ParmBlkPtr params, DCtlPtr dce) {
  (void)params;
  dprintf("Driver closing.\n");
  if(dce->dCtlStorage) {
    HLocker<Locals> locals(dce->dCtlStorage);
    if(locals->slot_queue_element) {
      if(locals->irq_enabled) {
        dprintf("Driver closed while interrupts still enabled.\n");
        locals->irq_enabled = false;
        locals->qfb->irq_mask = 0;
        SIntRemove(locals->slot_queue_element, locals->slot);
      }
      DisposePtr(reinterpret_cast<Ptr>(locals->slot_queue_element));
      locals->slot_queue_element = nullptr;
    }
    locals.unlock();
    DisposeHandle(dce->dCtlStorage);
    dce->dCtlStorage = nullptr;
  }
  return noErr;
}

void _putchar(char c) {
  uint32_t ch = static_cast<uint32_t>(static_cast<unsigned char>(c));
  if(ch > 0 && ch <= 255) {
    *reinterpret_cast<volatile uint32_t*>(0xFC00003C) = ch;
  }
}

uint32_t qfb_calculate_stride(uint32_t width, uint32_t depth) {
  /* this must mirror the calculation in mac_qfb.c */
  if(depth == 24) depth = 32;
  return ((width * depth + 31) / 8) & ~(uint32_t)3;
}

uint16_t qfb_calculate_num_pages(uint32_t width, uint32_t height, uint32_t depth) {
  uint32_t rowbytes = qfb_calculate_stride(width, depth);
  uint32_t modesize = rowbytes * height;
  uint32_t num_pages = (QFB_VRAM_SIZE - QFB_VRAM_SLOT_BASE) / modesize;
  if(num_pages > 32000) return 32000;
  else return num_pages;
}

int qfb_enable_interrupts(DCtlPtr dce) {
  dprintf("Enabling vertical blank interrupt.\n");
  HLocker<Locals> locals(dce->dCtlStorage);
  if(locals->irq_enabled) {
    dprintf("(but it was already enabled!)\n");
    return noErr; /* nothing to do */
  }
  SIntInstall(locals->slot_queue_element, locals->slot);
  locals->qfb->irq_mask = QFB_IRQ_VBL;
  locals->irq_enabled = true;
  return noErr;
}

int qfb_disable_interrupts(DCtlPtr dce) {
  dprintf("Disabling vertical blank interrupt.\n");
  HLocker<Locals> locals(dce->dCtlStorage);
  if(!locals->irq_enabled) {
    dprintf("(but it was already disabled!)\n");
    return noErr; /* nothing to do */
  }
  locals->irq_enabled = false;
  locals->qfb->irq_mask = 0;
  SIntRemove(locals->slot_queue_element, locals->slot);
  return noErr;
}

void mystrcpy(char* dstp, const char* srcp) {
  while((*dstp++ = *srcp++)) {}
}

void mymemcpy(void* dst, const void* src, size_t len) {
  uint8_t* dstp = reinterpret_cast<uint8_t*>(dst);
  const uint8_t* srcp = reinterpret_cast<const uint8_t*>(src);
  while(len-- > 0) *dstp++ = *srcp++;
}
