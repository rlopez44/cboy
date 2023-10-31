#include <stdbool.h>
#include "cboy/common.h"
#include "cboy/cpu.h"
#include "cboy/interrupts.h"
#include "cboy/gameboy.h"
#include "cboy/memory.h"
#include "cboy/log.h"

// request an interrupt by setting the appropriate bit in the IF register
void request_interrupt(gameboy *gb, INTERRUPT_TYPE interrupt)
{
    gb->cpu->if_register |= 1 << interrupt;
}

// set the appropriate bit in the IE register to enable the given interrupt
void enable_interrupt(gameboy *gb, INTERRUPT_TYPE interrupt)
{
    gb->cpu->ie_register |= 1 << interrupt;
}

uint8_t pending_interrupts(gameboy *gb)
{
    // NOTE: top three bits are unused and always set, so need to mask out
    return gb->cpu->if_register & gb->cpu->ie_register & 0x1f;
}

/* Service an interrupt, if any needs to be serviced.
 *
 * The interrupt priorities follow the same order as
 * their bits in the IF and IE registers (VBLANK has
 * the highest priority and JOYPAD has the lowest
 * priority). If the IME and IE registers allow the
 * execution of multiple pending interrupts, the CPU
 * will execute the eligible interrupt with the highest
 * priority.
 *
 * Returns the number of M-cycles needed to service
 * the interrupt (zero if no interrupt needs to be
 * serviced, 5 otherwise).
 * See: https://gbdev.io/pandocs/#interrupts
 */
uint8_t service_interrupt(gameboy *gb)
{
    uint8_t handler_addr = 0, // the address of the interrupt handler
            duration = 0; // number of M-cycles taken to service the interrupt

    // masks for the interrupt bits
    uint8_t vblank_mask = (1 << VBLANK),
            lcd_stat_mask = (1 << LCD_STAT),
            timer_mask = (1 << TIMER),
            serial_mask = (1 << SERIAL),
            joypad_mask = (1 << JOYPAD);

    /* An interrupt will be serviced only if the CPU's IME
     * flag is set and the interrupt's bits in the IE and
     * IF registers are both set.
     *
     * Recall that the top 3 bits of IF and IE are unused
     * and so must be masked out here.
     */
    uint8_t interrupts_to_service = pending_interrupts(gb);

    if (gb->cpu->ime_flag && interrupts_to_service)
    {
        // push the current PC onto the stack
        stack_push(gb, gb->cpu->reg->pc);

        /* Get the address of the interrupt handler for the
         * highest-priority interrupt that can be executed.
         * In addition, clear the IF bit for this interrupt.
         * The addresses are as follows:
         *
         * VBLANK:     0x40
         * LCD_STAT:   0x48
         * TIMER:      0x50
         * SERIAL:     0x58
         * JOYPAD:     0x60
         */
        if (interrupts_to_service & vblank_mask)        // VBLANK
        {
            gb->cpu->if_register &= ~vblank_mask;
            handler_addr = 0x40;
            LOG_DEBUG("Servicing VBlank IRQ\n");
        }
        else if (interrupts_to_service & lcd_stat_mask) // LCD_STAT
        {
            gb->cpu->if_register &= ~lcd_stat_mask;
            handler_addr = 0x48;
            LOG_DEBUG("Servicing STAT IRQ\n");
        }
        else if (interrupts_to_service & timer_mask)    // TIMER
        {
            gb->cpu->if_register &= ~timer_mask;
            handler_addr = 0x50;
            LOG_DEBUG("Servicing Timer IRQ\n");
        }
        else if (interrupts_to_service & serial_mask)   // SERIAL
        {
            gb->cpu->if_register &= ~serial_mask;
            handler_addr = 0x58;
            LOG_DEBUG("Servicing Serial IRQ\n");
        }
        else if (interrupts_to_service & joypad_mask)   // JOYPAD
        {
            gb->cpu->if_register &= ~joypad_mask;
            handler_addr = 0x60;
            LOG_DEBUG("Servicing Joypad IRQ\n");
        }
        else // shouldn't get here
        {
            LOG_ERROR("Invalid interrupt service request."
                      " IF: 0x%02x, IE: 0x%02x\n",
                      gb->cpu->if_register,
                      gb->cpu->ie_register);
            exit(1);
        }

        // set the high byte of the PC to zero and the low
        // byte to the address of the interrupt handler
        gb->cpu->reg->pc = (uint16_t)handler_addr;

        // disable interrupts in preparation for this
        // interrupt handler to be executed
        gb->cpu->ime_flag = false;

        // servicing the interrupt takes 5 M-cycles
        duration += 5;
    }

    return duration;
}
