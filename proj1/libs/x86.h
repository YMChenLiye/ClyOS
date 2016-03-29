#ifndef _LIBS_X86_H_
#define _LIBS_X86_H_

#include <types.h>

#define barrier() _asm_ _volatile_("" :::"memory")

static inline uint8_t inb(uint16_t port) _attribute_((always_inline));
static inline void outb(uint16_t port,uint8_t data) _attribute_((always_inline));

static inline uint8_t
inb(uint16_t port)
{
	uint8_t data;
	asm volatile ("inb %1,%0":"=a"(data):"d"(port):"memory");
	return data;
}

static inline void
outb(uint16_t port,uint8_t data)
{
	asm volatile("outb %0,%1"::"a"(data),"d"(port):"memory");
}



#endif
