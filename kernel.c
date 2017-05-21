#include <stddef.h>
#include <stdint.h>

// Memory-Mapped I/O output
static inline void mmio_write(uint32_t reg, uint32_t data)
{
	*(volatile uint32_t*)reg = data;
}
 
// Memory-Mapped I/O input
static inline uint32_t mmio_read(uint32_t reg)
{
	return *(volatile uint32_t*)reg;
}
 
// Loop <delay> times in a way that the compiler won't optimize away
static inline void delay(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : "=r"(count): [count]"0"(count) : "cc");
}
 
enum
{
    // The GPIO registers base address.
    GPIO_BASE = 0x3F200000, // for raspi2 & 3, 0x20200000 for raspi1
 
    // The offsets for reach register.
 
    // Controls actuation of pull up/down to ALL GPIO pins.
    GPPUD = (GPIO_BASE + 0x94),
 
    // Controls actuation of pull up/down for specific GPIO pin.
    GPPUDCLK0 = (GPIO_BASE + 0x98),
 
    // The base address for UART.
    UART0_BASE = 0x3F201000, // for raspi2 & 3, 0x20201000 for raspi1
 
    // The offsets for reach register for the UART.
    UART0_DR     = (UART0_BASE + 0x00),
    UART0_RSRECR = (UART0_BASE + 0x04),
    UART0_FR     = (UART0_BASE + 0x18),
    UART0_ILPR   = (UART0_BASE + 0x20),
    UART0_IBRD   = (UART0_BASE + 0x24),
    UART0_FBRD   = (UART0_BASE + 0x28),
    UART0_LCRH   = (UART0_BASE + 0x2C),
    UART0_CR     = (UART0_BASE + 0x30),
    UART0_IFLS   = (UART0_BASE + 0x34),
    UART0_IMSC   = (UART0_BASE + 0x38),
    UART0_RIS    = (UART0_BASE + 0x3C),
    UART0_MIS    = (UART0_BASE + 0x40),
    UART0_ICR    = (UART0_BASE + 0x44),
    UART0_DMACR  = (UART0_BASE + 0x48),
    UART0_ITCR   = (UART0_BASE + 0x80),
    UART0_ITIP   = (UART0_BASE + 0x84),
    UART0_ITOP   = (UART0_BASE + 0x88),
    UART0_TDR    = (UART0_BASE + 0x8C),
};
 
void uart_init()
{
	// Disable UART0.
	mmio_write(UART0_CR, 0x00000000);
	// Setup the GPIO pin 14 && 15.
 
	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	mmio_write(GPPUD, 0x00000000);
	delay(150);
 
	// Disable pull up/down for pin 14,15 & delay for 150 cycles.
	mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
 
	// Write 0 to GPPUDCLK0 to make it take effect.
	mmio_write(GPPUDCLK0, 0x00000000);
 
	// Clear pending interrupts.
	mmio_write(UART0_ICR, 0x7FF);
 
	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200.
 
	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	mmio_write(UART0_IBRD, 1);
	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	mmio_write(UART0_FBRD, 40);
 
	// Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
	mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));
 
	// Mask all interrupts.
	mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	                       (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
 
	// Enable UART0, receive & transfer part of UART.
	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}
 
void uart_putc(unsigned char c)
{
	// Wait for UART to become ready to transmit.
	while ( mmio_read(UART0_FR) & (1 << 5) ) { }
	mmio_write(UART0_DR, c);
}
 
unsigned char uart_getc()
{
    // Wait for UART to have received something.
    while ( mmio_read(UART0_FR) & (1 << 4) ) { }
    return mmio_read(UART0_DR);
}
 
void uart_puts(const char* str)
{
	for (size_t i = 0; str[i] != '\0'; i ++)
		uart_putc((unsigned char)str[i]);
}

#define MAX_REG_NUM		64
#define MAX_PRIORITY_LEVEL	3	
#define MAX_TASK_NUM_PER_PRI	2
#define MAX_TASK_NUM		(MAX_TASK_NUM_PER_PRI * MAX_PRIORITY_LEVEL)
#define MAX_STACK_SIZE		32 * 1024

typedef enum {
	ENUM_TASK_STATE_RUN	= 100,
	ENUM_TASK_STATE_READY,
	ENUM_TASK_STATE_SUSPEND,
	ENUM_TASK_STATE_WAIT
}ENUM_TASK_STATE;

typedef enum {
	STATUS_OK = 0,
	STATUS_FAILED = -1	
}ENUM_STATUS;

typedef void (*fn) (void);

typedef struct tcb {
	uint32_t regs[MAX_REG_NUM];
	uint32_t priority;
	uint32_t state;
	uint32_t *sp;
	uint32_t *bp;
	uint32_t stack_size;
	uint32_t used;
	fn routine;
}tcb; 


tcb task_info[MAX_TASK_NUM];
int current;


tcb *get_empty_task_item(tcb *task_info, uint32_t priority)
{
	int i;

	for (i = priority * MAX_TASK_NUM_PER_PRI; i < (priority + 1) * MAX_TASK_NUM_PER_PRI; i++) {
		if (task_info[i].used == 0) {
			task_info[i].used = 1;
			return &task_info[i];
		}
	}
	return NULL;
}

static int next_task(int priority)
{
	int i;

	for (i = priority * MAX_TASK_NUM_PER_PRI; i < MAX_TASK_NUM; i ++) {
		if (task_info[i].used == 1 && task_info[i].state == ENUM_TASK_STATE_READY) {
			return i;
		}
	}
	return -1;
}

void task_context_save(tcb *task)
{
	task = NULL;
}

void task_context_switch(tcb *task)
{
	task = NULL;
}

ENUM_STATUS create_task(fn routine, uint32_t priority, uint32_t stack_size)
{
	tcb *ptr = NULL;

	if (routine == NULL || stack_size > MAX_STACK_SIZE ||
		priority >= MAX_PRIORITY_LEVEL)
		return STATUS_FAILED;

	ptr = get_empty_task_item(task_info, priority);
	if (ptr) {
		ptr->priority = priority;
		ptr->stack_size = stack_size;
		ptr->routine = routine;
		ptr->state = ENUM_TASK_STATE_READY;
		return STATUS_OK;	
	}
	return STATUS_FAILED;
}

void start_task(tcb *task)
{
	task->state = ENUM_TASK_STATE_RUN;
	task_context_switch(task);
	task->routine();
}

void preempt_task(tcb *old_task, tcb *new_task)
{
	old_task->state = ENUM_TASK_STATE_READY;
	task_context_save(old_task);
	task_context_switch(new_task);
	new_task->routine();
}

void active_task(tcb *task)
{
	task->state = ENUM_TASK_STATE_READY;
}

void terminate_task(tcb *task)
{
	task->state = ENUM_TASK_STATE_SUSPEND;
	task_context_save(task);
}

void task1(void)
{
	uart_puts("task1\r\n");
	//terminate_task(&task[current]);
}

void task2(void)
{
	uart_puts("task2\r\n");
	//terminate_task(&task[current]);
}

void init_task(void)
{
	int i, j;

	for (i = 0; i < MAX_TASK_NUM; i ++) {
		for (j = 0; j < MAX_REG_NUM; j ++) {
			task_info[i].regs[j] = 0;
		}
		task_info[i].priority = 0;
		task_info[i].routine = NULL;
		task_info[i].state = ENUM_TASK_STATE_READY;
	}
	create_task(task1, 2, 1024 * 8);
	create_task(task2, 1, 1024 * 8);
	current = -1;
}

void do_shedule(void)
{
	int next = next_task(0);

	//uart_puts("do shedule\r\n");
	if (current == -1) {
		current = next;
		start_task(&task_info[current]);
	} else {
		if (next != current) {
			preempt_task(&task_info[current], &task_info[next]);
		}
	}
}

#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
	// Declare as unused
	(void) r0;
	(void) r1;
	(void) atags;
 
	uart_init();
	uart_puts("Hello, kernel World!\r\n");
	init_task();
 
	while (1) {
		do_shedule();
	}
		//uart_putc(uart_getc());
}

