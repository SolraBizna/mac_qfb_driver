        .set QFB_VERSION, 0x0
        .set QFB_MODE_WIDTH, 0x4
        .set QFB_MODE_HEIGHT, 0x8
        .set QFB_MODE_DEPTH, 0xC
        .set QFB_MODE_PAGE, 0x10
        .set QFB_MODE_STRIDE, 0x14
        .set QFB_MODE_NUM_PAGES, 0x18
        .set QFB_PAL_INDEX, 0x1C
        .set QFB_PAL_COLOR, 0x20
        .set QFB_LUT_INDEX, 0x24
        .set QFB_LUT_COLOR, 0x28
        .set QFB_IRQ_MASK, 0x2C
        .set QFB_IRQ, 0x30
        .set QFB_CUSTOM_WIDTH, 0x34
        .set QFB_CUSTOM_HEIGHT, 0x38
        .set QFB_CUSTOM_DEPTH, 0x3C
        .set QFB_IRQ_VBL, 1
        /* A0: Address of an SEBlock, our parameters */
        CLR.W seStatus(%A0) /* presume success */
        /* save non-volatile registers */
        MOVEM.L %A2/%A3/%D2, -(%SP)
        /* read our slot number */
        CLR.L %D0
        MOVE.B seSlot(%A0), %D0
        /* A2 := address of VRAM */
        ORI.L #0xF0, %D0
        MOVEQ.L #24, %D1
        LSL.L %D1, %D0
        MOVEA.L %D0, %A2
        /* A3 := address of control registers */
        ORI.L #0xC00000, %D0
        MOVEA.L %D0, %A3
        /* Reset */
        MOVE.L #0, QFB_VERSION(%A3)
        /* Disable interrupts */
        CLR.L QFB_IRQ_MASK(%A3)
        MOVE.L #QFB_IRQ_VBL, QFB_IRQ(%A3)
        /* Set up the requested custom mode, at 1-bpp */
        MOVE.L QFB_CUSTOM_WIDTH(%A3), QFB_MODE_WIDTH(%A3)
        MOVE.L QFB_CUSTOM_HEIGHT(%A3), QFB_MODE_HEIGHT(%A3)
        MOVE.L #1, QFB_MODE_DEPTH(%A3)
        MOVE.L #0, QFB_MODE_PAGE(%A3)
        /* Put in colors */
        CLR.L QFB_PAL_INDEX(%A3)
        MOVE.L #0xFFFFFF, QFB_PAL_COLOR(%A3)
        MOVE.L #1, QFB_PAL_INDEX(%A3)
        CLR.L QFB_PAL_COLOR(%A3)
        /* We don't need to do anything to the gamma ramp, resetting the card
           makes it linear */
        /* Let's fill the screen with gray! */
        MOVE.L #0xAAAAAAAA, %D2
        MOVE.L QFB_MODE_HEIGHT(%A3), %D1
_screenFillLoopOuter:
        MOVE.L QFB_MODE_STRIDE(%A3), %D0
        MOVE.L %A2, %A1
_screenFillLoopInner:
        MOVE.L %D2, (%A1)+
        SUBQ.L #4, %D0
        BGT _screenFillLoopInner
        ADD.L QFB_MODE_STRIDE(%A3), %A2
        NOT.L %D2
        SUBQ.L #1, %D1
        BGT _screenFillLoopOuter
        /* now it gets real... now it's time to patch our sResources */
        /* ...or it would be if QEMU hadn't done it for us. */
        /* save our own parameter block in A2 */
        MOVEA %A0, %A2
        /* make room on the stack for the spBlock */
        SUBA #spBlock.size, %SP
        /* put a pointer to our brand new Slot Manager parameter block in A0 */
        MOVE %SP, %A0
        /* deallocate stack-allocated spBlock */
        ADDA #spBlock.size, %SP
        /* restore registers */
        MOVEM.L (%SP)+, %A2/%A3/%D2
        RTS
        /* note that this code will only be correct with a fairly recent ROM
           (ha ha), so if you add support for older Macs to QEMU, or want to
           use this driver in another emulator, you'll need to pay attention to
           some gnarly issues. see "Designing Cards & Drivers for the
           Macintosh Family" and, as always, the various Inside Macintoshes. */
        
