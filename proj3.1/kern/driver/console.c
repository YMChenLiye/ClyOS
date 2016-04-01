#include <types.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>

//由于PC的历史原因产生的愚蠢的I/O延迟需要
static void
delay(void){
	inb(0x84);
	inb(0x84);
	inb(0x84);
	inb(0x84);
}


/***** Serial I/O code *****/
#define COM1            0x3F8

#define COM_RX          0       // In:  Receive buffer (DLAB=0)
#define COM_TX          0       // Out: Transmit buffer (DLAB=0)
#define COM_DLL         0       // Out: Divisor Latch Low (DLAB=1)
#define COM_DLM         1       // Out: Divisor Latch High (DLAB=1)
#define COM_IER         1       // Out: Interrupt Enable Register
#define COM_IER_RDI     0x01    // Enable receiver data interrupt
#define COM_IIR         2       // In:  Interrupt ID Register
#define COM_FCR         2       // Out: FIFO Control Register
#define COM_LCR         3       // Out: Line Control Register
#define COM_LCR_DLAB    0x80    // Divisor latch access bit
#define COM_LCR_WLEN8   0x03    // Wordlength: 8 bits
#define COM_MCR         4       // Out: Modem Control Register
#define COM_MCR_RTS     0x02    // RTS complement
#define COM_MCR_DTR     0x01    // DTR complement
#define COM_MCR_OUT2    0x08    // Out2 complement
#define COM_LSR         5       // In:  Line Status Register
#define COM_LSR_DATA    0x01    // Data available
#define COM_LSR_TXRDY   0x20    // Transmit buffer avail
#define COM_LSR_TSRE    0x40    // Transmitter off

#define MONO_BASE       0x3B4
#define MONO_BUF        0xB0000
#define CGA_BASE        0x3D4
#define CGA_BUF         0xB8000
#define CRT_ROWS        25
#define CRT_COLS        80
#define CRT_SIZE        (CRT_ROWS * CRT_COLS)

#define LPTPORT         0x378


static uint16_t *crt_buf;
static uint16_t crt_pos;
static uint16_t addr_6845;

//TEXT-mode CGA/VGA display output

static void
cga_init(void){
	volatile uint16_t *cp = (uint16_t)CGA_BUF;
	uint16_t was = *cp;
	*cp = (uint16_t)0xA55A;
	if(*cp != 0xA55A){
		cp = (uint16_t*)MONO_BUF;
		addr_6845 = MONO_BASE;
	}else{
		*cp = was;
		addr_6845 = CGA_BASE;
	}

	//确切的光标位置
	uint32_t pos;
	outb(addr_6845,14);
	pos = inb(addr_6845 + 1)<<8;
	outb(addr_6845,15);
	pos |= inb(addr_6845 + 1);

	crt_buf = (uint16_t*)cp;
	crt_pos = pos;
}

static bool serial_exists = 0;

static void
serial_init(void){
	//turn off the FIFO
	outb(COM1 + COM_FCR,0);

	//Set speed;requires DLAB latch
	//Line Control Register,set Divisor latch access bit
	outb(COM1 + COM_LCR,COM_LCR_DLAB);				//outb(0x03FB,0x80)
	//Divisor Latch Low (DLAB=1)
	outb(COM1 + COM_DLL,(uint8_t)(115200/9600));	//outb(0x03F8,12)
	//Divisor Latch Low(DLAB=1)
	outb(COM1 + COM_DLM,0);

	//8 data bits,1 stop bit,parity off;turn off DLAB latch
	outb(COM1 + COM_LCR,COM_LCR_WLEN8 & ~COM_LCR_DLAB);

	//No modem controls
	outb(COM1 + COM_MCR,0);
	//enable rcv interrupts
	outb(COM1 + COM_IER,COM_IER_RDI);

	//Clear any preexisting overrun indication and interrrupts
	//Serial port doesn't exist if COM_LSR returns 0xFF
	serial_exists = (inb(COM1 + COM_LSR) != 0xFF);

	(void) inb(COM1+COM_IIR);
	(void) inb(COM1+COM_RX);
}

//lpt_putc - 拷贝console输出到并口
static void
lpt_putc(int c){
	int i;
	for(i=0;!(inb(LPTPORT + 1) & 0x80) && i < 12800;i++){
		delay();
	}
	outb(LPTPORT + 0,c);
	outb(LPTPORT + 2,0x08 | 0x04 | 0x01);
	outb(LPTPORT + 2,0x08);
}

//cga_putc - 打印字符到console
static void
cga_putc(int c){
	//set black on white
	if(!(c & ~0xFF)){
		c |= 0x0700;
	}

	switch (c & 0xFF){
		case '\b':
			if(crt_pos > 0){
				crt_pos--;
				crt_buf[crt_pos] = (c & ~0xff) | ' ';
			}
			break;
		case '\n':
			crt_pos += CRT_COLS;
		case '\r':
			crt_pos -= (crt_pos & CRT_COLS);
			break;
		default:
			crt_buf[crt_pos ++] =c;//write the character
			break;
	}

	//这段代码应该是当屏幕显示不下所有字符时，将最前面的一行丢弃，并且下面的各行依次向上移动一行
	if(crt_pos >= CRT_SIZE){
		int i;
		memmove(crt_buf,crt_buf + CRT_COLS,(CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
		for(i=CRT_SIZE - CRT_COLS;i < CRT_SIZE; i++){
			crt_buf[i] = 0x0700 | ' ';
		}
		crt_pos -= CRT_COLS;
	}

	//move that little blinky thing
	outb(addr_6845,14);
	outb(addr_6845 + 1,crt_pos >> 8);
	outb(addr_6845,15);
	outb(addr_6845 + 1,crt_pos);
}


//serial_putc - 打印字符到串口
static void
serial_putc(int c){
	int i;
	for(i=0;!(inb(COM1 + COM_LSR) & COM_LSR_TXRDY) && i<12800;i++){
		delay();
	}
	outb(COM1 + COM_TX,c);
}

/* *
 * 在这里我们管理console的输入buffer，我们储存在键盘或串口当只要适合的中断发生时产生的字符
 */
#define CONSBUFSIZE 512

static struct{
	uint8_t buf[CONSBUFSIZE];
	uint32_t rpos;
	uint32_t wpos;
}cons;

/* *
 * cons_intr - 被设备中断例程调用来满足输入字符到控制台输入缓存
 */
static void
cons_intr(int (*proc)(void)){
	int c;
	while((c=(*proc)()) != -1){
		if(c!=0){
			cons.buf[cons.wpos++] = c;
			if(cons.wpos == CONSBUFSIZE){
				cons.wpos = 0;
			}
		}
	}
}

/* serial_proc_data - 从串口画得数据 */
static int
serial_proc_data(void){
	if(!(inb(COM1 + COM_LSR) & COM_LSR_DATA)){
		return -1;
	}
	return inb(COM1 + COM_RX);
}

/* serial_intr - 尝试从串口获得输入字符*/
void
serial_intr(void){
	if(serial_exists){
		cons_intr(serial_proc_data);
	}
}

/**********Keyboard input code*******/

#define NO				0

#define SHIFT			(1<<0)
#define CTL				(1<<1)
#define ALT				(1<<2)

#define CAPSLOCK		(1<<3)
#define NUMLOCK			(1<<4)
#define SCROLLLOCK		(1<<5)

#define E0ESC			(1<<6)

static uint8_t shiftcode[256] = {
	[0x1D] CTL,
	[0x2A] SHIFT,
	[0x36] SHIFT,
	[0x38] ALT,
	[0x9D] CTL,
	[0xB8] ALT
};

static uint8_t togglecode[256] = {
	[0x3A] CAPSLOCK,
	[0x45] NUMLOCK,
	[0x46] SCROLLLOCK
};

static uint8_t normalmap[256] = {
	NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
	'7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
	'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
	'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
	'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
	'\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
	'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
	NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
	NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
	'8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
	[0xC7] KEY_HOME,    [0x9C] '\n' /*KP_Enter*/,
	[0xB5] '/' /*KP_Div*/,  [0xC8] KEY_UP,
	[0xC9] KEY_PGUP,    [0xCB] KEY_LF,
	[0xCD] KEY_RT,      [0xCF] KEY_END,
	[0xD0] KEY_DN,      [0xD1] KEY_PGDN,
	[0xD2] KEY_INS,     [0xD3] KEY_DEL
};

static uint8_t shiftmap[256] = {
	NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
	'&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
	'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
	'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
	'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
	'"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
	'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
	NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
	NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
	'8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
	[0xC7] KEY_HOME,    [0x9C] '\n' /*KP_Enter*/,
	[0xB5] '/' /*KP_Div*/,  [0xC8] KEY_UP,
	[0xC9] KEY_PGUP,    [0xCB] KEY_LF,
	[0xCD] KEY_RT,      [0xCF] KEY_END,
	[0xD0] KEY_DN,      [0xD1] KEY_PGDN,
	[0xD2] KEY_INS,     [0xD3] KEY_DEL
};

#define C(x) (x - '@')

static uint8_t ctlmap[256] = {
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
	C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
	C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
	C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
	NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
	C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
	[0x97] KEY_HOME,
	[0xB5] C('/'),      [0xC8] KEY_UP,
	[0xC9] KEY_PGUP,    [0xCB] KEY_LF,
	[0xCD] KEY_RT,      [0xCF] KEY_END,
	[0xD0] KEY_DN,      [0xD1] KEY_PGDN,
	[0xD2] KEY_INS,     [0xD3] KEY_DEL
};

static uint8_t *charcode[4] = {
	normalmap,
	shiftmap,
	ctlmap,
	ctlmap
};


/* *
 * kbd_proc_data - 从键盘获得数据
 *
 * kbd_proc_data()函数从键盘获得数据
 * 如果我们完成了一个字符，返回它，不然就返回0.如果没有数据，就返回-1
 */
static int
kbd_proc_data(void){
	int c;
	uint8_t data;
	static uint32_t shift;

	if((inb(KBSTATP) & KBS_DIB) == 0){
		return -1;
	}

	data = inb(KBDATAP);

	if(data == 0xE0){
		//E0 escape character
		shift |= E0ESC;
		return 0;
	}
	else if(data & 0x80){
		//Key released 释放
		data = (shift & E0ESC ? data : data & 0x7F);
		shift &= ~(shiftcode[data] | E0ESC);
		return 0;
	}
	else if(shift & E0ESC){
		// Last character was an E0 escape; or with 0x80
		data |= 0x80;
		shift &= ~E0ESC;
	}

	shift |= shiftcode[data];
	shift ^= togglecode[data];

	c = charcode[shift & (CTL | SHIFT)][data];
	if(shift & CAPSLOCK){
		if('a' <= c && c<= 'z'){
			c += 'A' - 'a';
		}
		else if('A' <= c && c<= 'Z'){
			c += 'a' - 'A';
		}
	}

	//处理特殊的按键
	//ctrl-alt-del：reboot
	if(!(~shift & (CTL | ALT)) && c == KEY_DEL){
		cprintf("Rebooting!\n");
		outb(0x92,0x3);// courtesy of Chris Frost
	}
	return c;
}

/* kbd_intr - 尝试从键盘获取字符*/
static void
kbd_intr(void){
	cons_intr(kbd_proc_data);
}

static void
kbd_init(void){
	kbd_intr();
}




//cons_init - 初始化console设备
void
cons_init(void){
	cga_init();
	serial_init();
	if(!serial_exists){
		cprintf("serial port does not exist!!\n");
	}
}

//cons_putc - 打印单个字符@c 到console设备
void
cons_putc(int c){
	lpt_putc(c);
	cga_putc(c);
	serial_putc(c);
}


/* *
 * cons_getc - 返回从console中下一个字符，如果没有正在等待的话，就是0
 */
int
cons_getc(void){
	int c;

	//轮询任何pending的输入字符
	//所以这个函数可以工作在关闭了中断的情况下
	//(例如，从kernel monitor调用的时候)
	serial_intr();
	kbd_intr();

	//从输入缓存中截取下一个字符
	if(cons.rpos != cons.wpos){
		c = cons.buf[cons.rpos++];
		if(cons.rpos == CONSBUFSIZE){
			cons.rpos =0;
		}
		return c;
	}
	return 0;
}
