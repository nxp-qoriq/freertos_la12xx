// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef INC_PMC_H_
#define INC_PMC_H_

#include "types.h"
#include <platform_def.h>
#include "ppc.h"

#define PMC_START() {					\
		u32 pmgc0 = mfpmr(PMR_PMGC0);		\
		pmgc0 &= ~PMGC0_FAC;			\
		pmgc0 |= PMGC0_FCECE;			\
		pmgc0 &= ~PMGC0_PMIE;			\
		mtpmr(PMR_PMGC0, pmgc0);		\
	}

#define PMC_STOP() {					\
		u32 pmgc0 = mfpmr(PMR_PMGC0);		\
		pmgc0 |= PMGC0_FAC;			\
		pmgc0 &= ~(PMGC0_PMIE | PMGC0_FCECE);	\
		mtpmr(PMR_PMGC0, pmgc0);		\
	}

#define PMC_SET_EVENT(PMR_NUM, EVENT) {			\
		u32 pmlca;				\
		pmlca = mfpmr(PMR_NUM); 		\
		pmlca = (pmlca & ~PMLCA_EVENT_MASK) |	\
			((EVENT << PMLCA_EVENT_SHIFT) &	\
			PMLCA_EVENT_MASK); 		\
		mtpmr(PMR_NUM, pmlca);			\
	}


#define PMC_SET_USER_KERNEL(PMR_NUM, USER, KERNEL) {	\
		u32 pmlca;				\
		pmlca = mfpmr(PMR_NUM); 		\
		if (USER)				\
			pmlca &= ~PMLCA_FCU;		\
		else					\
			pmlca |= PMLCA_FCU;		\
		if (KERNEL)				\
			pmlca &= ~PMLCA_FCS;		\
		else					\
			pmlca |= PMLCA_FCS;		\
		mtpmr(PMR_NUM, pmlca);			\
	}


#define PMC_SET_MARKED(PMR_NUM, MARK0, MARK1) {		\
		u32 pmlca;				\
		pmlca = mfpmr(PMR_NUM); 		\
		if (MARK0)				\
			pmlca &= ~PMLCA_FCM0;		\
		else					\
			pmlca |= PMLCA_FCM0;		\
		if (MARK1)				\
			pmlca &= ~PMLCA_FCM1;		\
		else					\
			pmlca |= PMLCA_FCM1;		\
		mtpmr(PMR_NUM, pmlca);			\
	}

#define PMC_CTR_READ(PMC_NUM) ({			\
		u32 pmc_ctr;				\
		pmc_ctr= mfpmr(PMC_NUM);		\
		pmc_ctr;				\
	})


#endif /* INC_PMC_H_ */
