
#ifndef __ASM_ARCH_TRIX_CLK_H__
#define __ASM_ARCH_TRIX_CLK_H__

/**
 * @brief	get SoC timer value elapsed from power on, at 200MHz.
 * @fn		u32 get_arch_timestamp(void);
 * @retval	timer value
 */
unsigned long get_arch_timestamp(void);
/**
 * @brief	show SoC timer value in logging, at 200MHz.
 * @fn		void show_arch_timestamp(void);
 * @retval	void
 */
void show_arch_timestamp(void);
#endif
