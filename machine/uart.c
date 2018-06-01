#include <string.h>
#include "uart.h"
#include "fdt.h"

volatile uint32_t* uart;

void uart_putchar(uint8_t ch)
{
/*#ifdef __riscv_atomic
    int32_t r;
    do {
      __asm__ __volatile__ (
        "amoor.w %0, %2, %1\n"
        : "=r" (r), "+A" (uart[UART_REG_TXFIFO])
        : "r" (ch));
    } while (r < 0);
#else*/
//    volatile uint32_t *tx = uart + UART_REG_TXFIFO;
//    while ((int32_t)(*tx) < 0);
//    *tx = ch;
//#endif
/*  register char a0 asm("a0") = ch;
  asm volatile ("li t1, 0x11300" "\n\t"    //The base address of UART config registers
                "sb a0, 0(t1)"  "\n\t"
                :
                :
                :"a0","t1","cc","memory");
*/

register char a0 asm("a0") = ch; 
   asm volatile ("li t1, 0x11200" "\n\t" //The base address of UART config registers
                 "uart_status: lb t2, 40(t1)" "\n\t"
                 "andi t2, t2, 0x20" "\n\t"
                 "beqz t2, uart_status" "\n\t"
                 "sb a0, 0(t1)"  "\n\t"
                 :
                 :
                 :"x0","a0","t1","t2", "cc", "memory");
}
int uart_getchar()
{
  int8_t ch = uart[UART_REG_RXFIFO];
  if (ch <= 0 || ch>127) return -1;
  return ch;

}

struct uart_scan
{
  int compat;
  uint64_t reg;
};

static void uart_open(const struct fdt_scan_node *node, void *extra)
{
  struct uart_scan *scan = (struct uart_scan *)extra;
  memset(scan, 0, sizeof(*scan));
}

static void uart_prop(const struct fdt_scan_prop *prop, void *extra)
{
  struct uart_scan *scan = (struct uart_scan *)extra;
  if (!strcmp(prop->name, "compatible") && !strcmp((const char*)prop->value, "shakti,uart0")) {
    scan->compat = 1;
  } else if (!strcmp(prop->name, "reg")) {
    fdt_get_address(prop->node->parent, prop->value, &scan->reg);
  }
}

static void uart_done(const struct fdt_scan_node *node, void *extra)
{
  struct uart_scan *scan = (struct uart_scan *)extra;
  if (!scan->compat || !scan->reg || uart) return;

  // Enable Rx/Tx channels
  uart = (void*)(uintptr_t)scan->reg;
  //uart[UART_REG_TXCTRL] = UART_TXEN;
 // uart[UART_REG_RXCTRL] = UART_RXEN;
   asm volatile ("li t1, 0x11200" "\n\t" //The base address of UART config registers
                 "li t2, 0x83" "\n\t"    //Access divisor registers
                 "sb t2, 24(t1)" "\n\t"  //Writing to UART_ADDR_LINE_CTRL
                 "li t2, 0x0"    "\n\t"  //The upper bits of uart div
                 "sb x0, 8(t1)" "\n\t"   //Storing upper bits of uart div
                 "li t2, 0x5" "\n\t"    //The lower bits of uart div
                 "sb t2, 0(t1)" "\n\t"     
                 "li t2, 0x3" "\n\t"
                 "sb t2, 24(t1)" "\n\t"
                 "li t2, 0x6" "\n\t"
                 "sb t2, 16(t1)" "\n\t"
                 "sb x0, 8(t1)" "\n\n"
                 :
                 :
                 :"x0","t1","t2", "cc", "memory");
}

void query_uart(uintptr_t fdt)
{
  struct fdt_cb cb;
  struct uart_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.open = uart_open;
  cb.prop = uart_prop;
  cb.done = uart_done;
  cb.extra = &scan;

  fdt_scan(fdt, &cb);
}
