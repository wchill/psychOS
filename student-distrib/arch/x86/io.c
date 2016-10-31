#include <arch/x86/io.h>

/*
 * inportb
 * Gets a byte from provided port and returns it.
 * 
 * @param port   The port to read from
 * 
 * @returns      A byte from the provided port
 */
inline uint8_t inportb(uint16_t port) {
	uint32_t val;
	asm volatile("xorl %0, %0\n \
			inb   (%w1), %b0" 
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return (uint8_t) val;
}

/*
 * inportw
 * Gets a word from provided port and returns it.
 * 
 * @param port   The port to read from
 * 
 * @returns      A word from the provided port
 */
inline uint16_t inportw(uint16_t port) {
	uint32_t val;
	asm volatile("xorl %0, %0\n   \
			inw   (%w1), %w0"
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return (uint16_t) val;
}

/*
 * inportl
 * Gets a long from provided port and returns it.
 * 
 * @param port   The port to read from
 * 
 * @returns      A long from the provided port
 */
inline uint32_t inportl(uint16_t port) {
	uint32_t val;
	asm volatile("inl   (%w1), %0"
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return val;
}

/*
 * outportb
 * writes data (a byte) to provided port
 * 
 * @param port   The port to write to
 * @param data   The byte to write to the port
 */
inline void outportb(uint16_t port, uint8_t data) {
	asm volatile("outb %b1, (%w0)"
			:
			: "d" (port), "a" (data)
			: "memory", "cc" );
}

/*
 * outportw
 * writes data (a word) to provided port
 * 
 * @param port   The port to write to
 * @param data   The long to write to the port
 */
inline void outportw(uint16_t port, uint16_t data) {
	asm volatile("outw %w1, (%w0)"
			:
			: "d" (port), "a" (data)
			: "memory", "cc" );
}

/*
 * outportl
 * writes data (a long) to provided port
 * 
 * @param port   The port to write to
 * @param data   The long to write to the port
 */
inline void outportl(uint16_t port, uint32_t data) {
	asm volatile("outl %1, (%w0)"
			:
			: "d" (port), "a" (data)
			: "memory", "cc" );
}

