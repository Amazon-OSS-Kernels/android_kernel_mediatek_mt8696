/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

/*
 * Header files for basic Optee KREE functions.
 */

#ifndef __OKREE_H__
#define __OKREE_H__

#define KREE_TeeServiceCall					OKREE_TeeServiceCall
#define KREE_CreateSession					OKREE_CreateSession
#define KREE_CreateSessionWithTag				OKREE_CreateSessionWithTag
#define KREE_CloseSession					OKREE_CloseSession
#define KREE_RegisterSharedmem					OKREE_RegisterSharedmem
#define KREE_UnregisterSharedmem				OKREE_UnregisterSharedmem
#define KREE_AllocSecuremem					OKREE_AllocSecuremem
#define KREE_AllocSecurememWithTag				OKREE_AllocSecurememWithTag
#define KREE_ZallocSecurememWithTag				OKREE_ZallocSecurememWithTag
#define KREE_ReferenceSecuremem					OKREE_ReferenceSecuremem
#define KREE_UnreferenceSecuremem				OKREE_UnreferenceSecuremem
#define KREE_AllocSecurechunkmem				OKREE_AllocSecurechunkmem
#define KREE_AllocSecurechunkmemWithTag				OKREE_AllocSecurechunkmemWithTag
#define KREE_ZallocSecurechunkmemWithTag			OKREE_ZallocSecurechunkmemWithTag
#define KREE_AllocSecurechunkmemEx				OKREE_AllocSecurechunkmemEx
#define KREE_AllocSecurechunkmemWithTagEx			OKREE_AllocSecurechunkmemWithTagEx
#define KREE_ZallocSecurechunkmemWithTagEx			OKREE_ZallocSecurechunkmemWithTagEx
#define KREE_ReferenceSecurechunkmem				OKREE_ReferenceSecurechunkmem
#define KREE_UnreferenceSecurechunkmem				OKREE_UnreferenceSecurechunkmem
#define KREE_ReadSecurechunkmem					OKREE_ReadSecurechunkmem
#define KREE_WriteSecurechunkmem				OKREE_WriteSecurechunkmem
#define KREE_GetSecurechunkReleaseSize				OKREE_GetSecurechunkReleaseSize
#define KREE_StartSecurechunkmemSvc				OKREE_StartSecurechunkmemSvc
#define KREE_StopSecurechunkmemSvc				OKREE_StopSecurechunkmemSvc
#define KREE_QuerySecurechunkmem				OKREE_QuerySecurechunkmem
#define KREE_ServGetChunkmemPool				OKREE_ServGetChunkmemPool
#define KREE_ServReleaseChunkmemPool				OKREE_ServReleaseChunkmemPool
#define KREE_GetTEETotalSize					OKREE_GetTEETotalSize

#endif	/* __OKREE_H__ */
