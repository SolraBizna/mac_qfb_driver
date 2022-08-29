#include "qfb_driver.hh"

int qfb_drvr_open(ParmBlkPtr params, DCtlPtr dce, uint32_t slot) {
  (void)params;
  /* Allocate memory for our Locals */
  ReserveMemSys(sizeof(Locals)+sizeof(SlotIntQElement));
  Handle local_handle = NewHandleSysClear(sizeof(Locals));
  if(!local_handle) return MemError();
#ifdef SUPPORT_AUX
  HLock(local_handle);
#else
  MoveHHi(local_handle);
#endif
  dce->dCtlStorage = local_handle;
  HLocker<Locals> locals(local_handle);
  locals->vram = reinterpret_cast<uint8_t*>(0xF0000000 | (slot<<24));
  locals->qfb = reinterpret_cast<volatile QFB*>(locals->vram + 0xC00000);
  locals->slot = slot;
  if(locals->qfb->version != 'qfb0') {
    DebugStr("\pWrong QFB version");
    DisposeHandle(local_handle);
    return openErr;
  }
  locals->slot_queue_element = reinterpret_cast<SlotIntQElement*>(NewPtrSysClear(sizeof(SlotIntQElement)));
  if(locals->slot_queue_element == nullptr) {
    int ret = MemError();
    locals.unlock();
    DisposeHandle(local_handle);
    return ret;
  }
  locals->slot_queue_element->sqType = 6; // sIQType
  locals->slot_queue_element->sqAddr = qfb_interrupt_service_routine;
  locals->slot_queue_element->sqParm = reinterpret_cast<uint32_t>(locals->qfb);
  qfb_gray_clut(locals);
  /* set up user-specified width and height, but 1-bpp, because that's the mode
     A/UX and MacOS expect to be active on open */
  locals->qfb->width = locals->qfb->user_width;
  locals->qfb->height = locals->qfb->user_height;
  locals->cur_mode = ONE_BIT_MODE;
  locals->qfb->depth = 1;
  locals->qfb->page = 0;
  qfb_gray_pixels(locals, 0);
  locals.unlock();
  return qfb_enable_interrupts(dce);
}

int qfb_drvr_close(ParmBlkPtr params, DCtlPtr dce) {
  (void)params;
  DisposeHandle(dce->dCtlStorage);
  return noErr;
}

void _putchar(char c) {
  uint32_t ch = static_cast<uint32_t>(static_cast<unsigned char>(c));
  if(ch > 0 && ch <= 255) {
    *reinterpret_cast<volatile uint32_t*>(0xFCC0003C) = ch;
  }
}

uint32_t qfb_calculate_stride(uint32_t width, uint32_t depth) {
  /* this must mirror the calculation in mac_qfb.c */
  uint32_t pot;
  for(pot = 64; pot < width; pot = pot << 1) {}
  switch (depth) {
  default:
  case 1:
    return pot / 8;
  case 2:
    return pot / 4;
  case 4:
    return pot / 2;
  case 8:
    return pot;
  case 16:
    return pot * 2;
  case 24:
  case 32:
    return pot * 4;
  }
}

uint16_t qfb_calculate_num_pages(uint32_t width, uint32_t height, uint32_t depth) {
  uint32_t rowbytes = qfb_calculate_stride(width, depth);
  uint32_t modesize = rowbytes * height;
  uint32_t num_pages = QFB_VRAM_SIZE / modesize;
  if(num_pages > 32000) return 32000;
  else return num_pages;
}

int qfb_enable_interrupts(DCtlPtr dce) {
  HLocker<Locals> locals(dce->dCtlStorage);
  if(locals->irq_enabled) return noErr; /* nothing to do */
  SIntInstall(locals->slot_queue_element, locals->slot);
  locals->qfb->irq_mask = QFB_IRQ_VBL;
  locals->irq_enabled = true;
  return noErr;
}

int qfb_disable_interrupts(DCtlPtr dce) {
  HLocker<Locals> locals(dce->dCtlStorage);
  if(!locals->irq_enabled) return noErr; /* nothing to do */
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
