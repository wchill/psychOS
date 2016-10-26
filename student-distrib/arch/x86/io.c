#include <arch/x86/io.h>

inline uint8_t inportb(uint16_t port) {
	uint32_t val;
	asm volatile("xorl %0, %0\n \
			inb   (%w1), %b0" 
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return (uint8_t) val;
}

inline uint16_t inportw(uint16_t port) {
	uint32_t val;
	asm volatile("xorl %0, %0\n   \
			inw   (%w1), %w0"
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return (uint16_t) val;
}

inline uint32_t inportl(uint16_t port) {
	uint32_t val;
	asm volatile("inl   (%w1), %0"
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return val;
}

inline void outportb(uint16_t port, uint8_t data) {
	asm volatile("outb %b1, (%w0)"
			:
			: "d" (port), "a" (data)
			: "memory", "cc" );
}

inline void outportw(uint16_t port, uint16_t data) {
	asm volatile("outw %w1, (%w0)"
			:
			: "d" (port), "a" (data)
			: "memory", "cc" );
}

inline void outportl(uint16_t port, uint32_t data) {
	asm volatile("outl %1, (%w0)"
			:
			: "d" (port), "a" (data)
			: "memory", "cc" );
}

