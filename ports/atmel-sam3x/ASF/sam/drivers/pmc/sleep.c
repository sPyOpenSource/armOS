/**
 * \file
 *
 * \brief Sleep mode access
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/* [TR]
 * pmc_save_clock_settings() and pmc_restore_clock_settings() don't work properly
 * in ASF 3.7.2. These functions were re-worked with code from ASF 3.5.
 */

#include <compiler.h>
#include "sleep.h"

/* SAM3 and SAM4 series */
#if (SAM3S || SAM3N || SAM3XA || SAM3U || SAM4S || SAM4E)
# include "pmc.h"
# include "board.h"

/**
 * Save clock settings and shutdown PLLs
 */
static void pmc_save_clock_settings(
		uint32_t *p_osc_setting,
		uint32_t *p_pll0_setting,
		uint32_t *p_pll1_setting,
		uint32_t *p_mck_setting)
{
	if (p_osc_setting) {
		*p_osc_setting = PMC->CKGR_MOR;
	}
	if (p_pll0_setting) {
		*p_pll0_setting = PMC->CKGR_PLLAR;
	}
	if (p_pll1_setting) {
#if (SAM3S || SAM4S)
		*p_pll1_setting = PMC->CKGR_PLLBR;
#elif (SAM3U || SAM3XA)
		*p_pll1_setting = PMC->CKGR_UCKR;
#else
		*p_pll1_setting = 0;
#endif
	}
	if (p_mck_setting) {
		*p_mck_setting  = PMC->PMC_MCKR;
	}

	// Switch MCK to Main clock (internal or external 12MHz) for fast wakeup
	// If MAIN_CLK is already the source, just skip
	if ((PMC->PMC_MCKR & PMC_MCKR_CSS_Msk) == PMC_MCKR_CSS_MAIN_CLK) {
		return;
	}
	// If we have to enable the MAIN_CLK
	if ((PMC->PMC_SR & PMC_SR_MOSCXTS) == 0) {
		// Intend to use internal RC as source of MAIN_CLK
		pmc_osc_enable_fastrc(CKGR_MOR_MOSCRCF_12_MHz);
		pmc_switch_mainck_to_fastrc(CKGR_MOR_MOSCRCF_12_MHz);
	}
	pmc_switch_mck_to_mainck(PMC_MCKR_PRES_CLK_1);
}

/**
 * Restore clock settings
 */
static void pmc_restore_clock_setting(
		uint32_t osc_setting,
		uint32_t pll0_setting,
		uint32_t pll1_setting,
		uint32_t mck_setting)
{
	uint32_t mckr;
	if ((pll0_setting & CKGR_PLLAR_MULA_Msk) &&
	pll0_setting != PMC->CKGR_PLLAR) {
		PMC->CKGR_PLLAR = 0;
		PMC->CKGR_PLLAR = CKGR_PLLAR_ONE | pll0_setting;
		while (!(PMC->PMC_SR & PMC_SR_LOCKA));
	}
	#if (SAM3S || SAM4S)
	if ((pll1_setting & CKGR_PLLBR_MULB_Msk) &&
	pll1_setting != PMC->CKGR_PLLBR) {
		PMC->CKGR_PLLBR = 0;
		PMC->CKGR_PLLBR = pll1_setting ;
		while (!(PMC->PMC_SR & PMC_SR_LOCKB));
	}
	#elif (SAM3U || SAM3XA)
	if ((pll1_setting & CKGR_UCKR_UPLLEN) &&
	pll1_setting != PMC->CKGR_UCKR) {
		PMC->CKGR_UCKR = 0;
		PMC->CKGR_UCKR = pll1_setting;
		while (!(PMC->PMC_SR & PMC_SR_LOCKU));
	}
	#else
	UNUSED(pll1_setting);
	#endif
	/* Switch to faster clock */
	mckr = PMC->PMC_MCKR;
	// Set PRES
	PMC->PMC_MCKR = (mckr & ~PMC_MCKR_PRES_Msk)
	| (mck_setting & PMC_MCKR_PRES_Msk);
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY));
	// Set CSS and others
	PMC->PMC_MCKR = mck_setting;
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY));
	/* Shutdown fastrc */
	if (0 == (osc_setting & CKGR_MOR_MOSCRCEN)) {
		pmc_osc_disable_fastrc();
	}
}

/** If clocks are switched to FASTRC for WAIT mode */
static volatile bool b_is_fastrc_used = false;

void pmc_sleep(int sleep_mode)
{
	switch (sleep_mode) {
	case SAM_PM_SMODE_SLEEP_WFI:
	case SAM_PM_SMODE_SLEEP_WFE:
#if (SAM4S || SAM4E)
		SCB->SCR &= (uint32_t)~SCR_SLEEPDEEP;
		cpu_irq_enable();
		__WFI();
		break;
#else
		PMC->PMC_FSMR &= (uint32_t)~PMC_FSMR_LPM;
		SCB->SCR &= (uint32_t)~SCR_SLEEPDEEP;
		cpu_irq_enable();
		if (sleep_mode == SAM_PM_SMODE_SLEEP_WFI)
			__WFI();
		else
			__WFE();
		break;
#endif
	case SAM_PM_SMODE_WAIT: {
		uint32_t mor, pllr0, pllr1, mckr;
		cpu_irq_disable();
		b_is_fastrc_used = true;
		pmc_save_clock_settings(&mor, &pllr0, &pllr1, &mckr);

		/* Enter wait mode */
		cpu_irq_enable();
		pmc_enable_waitmode();

		cpu_irq_disable();
		pmc_restore_clock_setting(mor, pllr0, pllr1, mckr);
		b_is_fastrc_used = false;
		cpu_irq_enable();
		break;
	}

	case SAM_PM_SMODE_BACKUP:
		SCB->SCR |= SCR_SLEEPDEEP;
#if (SAM4S || SAM4E)
		SUPC->SUPC_CR = SUPC_CR_KEY(0xA5u) | SUPC_CR_VROFF_STOP_VREG;
		cpu_irq_enable();
		__WFI() ;
#else
		cpu_irq_enable();
		__WFE() ;
#endif
		break;
	}
}

bool pmc_is_wakeup_clocks_restored(void)
{
	return !b_is_fastrc_used;
}

#endif /* #if (SAM3S || SAM3N || SAM3XA || SAM3U || SAM4S || SAM4E) */
