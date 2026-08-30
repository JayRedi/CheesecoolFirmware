    .section .text
    .option rvc
    .align 2
    .global ram_usb_irq_trace

/* WCH fast-interrupt entry: log a snapshot, then tail-jump to the
 * original interrupt handler, which owns the interrupt return (mret). */
ram_usb_irq_trace:
    lui     t0, 0x40023
    addi    t0, t0, 0x400          /* USBFS base: 0x40023400 */
    lbu     t1, 6(t0)              /* INT_FG */
    lbu     t2, 7(t0)              /* INT_ST */
    lbu     t3, 5(t0)              /* MIS_ST */
    lbu     t4, 3(t0)              /* DEV_ADDR */

    li      t5, 0x20002280         /* trace header: next sequence */
    lw      t6, 0(t5)
    li      a0, 32
    bgeu    t6, a0, 1f             /* full: do not overwrite */
    slli    a1, t6, 4
    li      a2, 0x20002290         /* 32 x 16-byte entries */
    add     a2, a2, a1
    sw      t6, 0(a2)              /* sequence */
    li      a0, 1
    sb      a0, 4(a2)              /* event = IRQ_ENTRY */
    sb      t1, 5(a2)              /* INT_FG */
    sb      t2, 6(a2)              /* INT_ST */
    sb      t3, 7(a2)              /* MIS_ST */
    sb      t4, 8(a2)              /* DEV_ADDR */
    lbu     a0, 0(t0)
    sb      a0, 9(a2)              /* BASE_CTRL */
    lbu     a0, 1(t0)
    sb      a0, 10(a2)             /* UDEV_CTRL */
    addi    t6, t6, 1
    sw      t6, 0(t5)
1:
    li      t6, 0x00002778         /* original Safety USBFS_IRQHandler */
    jr      t6                     /* tail-chain; original executes mret */
