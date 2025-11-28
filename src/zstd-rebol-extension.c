//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// SPDX-License-Identifier: MIT
// =============================================================================
// Rebol/Zstd extension
// =============================================================================

#include "zstd-rebol-extension.h"

RL_LIB *RL; // Link back to reb-lib from embedded extensions

//==== Globals ===============================================================//
extern MyCommandPointer Command[];
REBCNT Handle_ZstdEncoder;
REBCNT Handle_ZstdDecoder;

u32* arg_words;
u32* type_words;

//============================================================================//

static const char* init_block = ZSTD_EXT_INIT_CODE;

int CompressZstd(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error);
int DecompressZstd(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error);
int Common_mold(REBHOB *hob, REBSER *ser);

int ZstdEncHandle_free(void* hndl);
int ZstdEncHandle_get_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg);
int ZstdEncHandle_set_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg);

int ZstdDecHandle_free(void* hndl);
int ZstdDecHandle_get_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg);
int ZstdDecHandle_set_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg);

void* ZstdDefaultAllocFunc(void *opaque, size_t size);
void ZstdDefaultFreeFunc(void *opaque, void *address);

RXIEXT const char *RX_Init(int opts, RL_LIB *lib) {
	RL = lib;
	REBYTE ver[8];
	RL_VERSION(ver);
	debug_print(
		"RXinit zstd-extension; Rebol v%i.%i.%i\n",
		ver[1], ver[2], ver[3]);

	if (MIN_REBOL_VERSION > VERSION(ver[1], ver[2], ver[3])) {
		debug_print(
			"Needs at least Rebol v%i.%i.%i!\n",
			 MIN_REBOL_VER, MIN_REBOL_REV, MIN_REBOL_UPD);
		return 0;
	}
	if (!CHECK_STRUCT_ALIGN) {
		trace("CHECK_STRUCT_ALIGN failed!");
		return 0;
	}

	REBHSP spec;
	spec.mold = Common_mold;

	spec.size      = sizeof(void*);
	spec.flags     = HANDLE_REQUIRES_HOB_ON_FREE;
	spec.free      = ZstdEncHandle_free;
	spec.get_path  = ZstdEncHandle_get_path;
	spec.set_path  = ZstdEncHandle_set_path;
	Handle_ZstdEncoder = RL_REGISTER_HANDLE_SPEC((REBYTE*)"zstd-encoder", &spec);

	spec.size      = sizeof(void*);
	spec.flags     = HANDLE_REQUIRES_HOB_ON_FREE;
	spec.free      = ZstdDecHandle_free;
	spec.get_path  = ZstdDecHandle_get_path;
	//spec.set_path  = ZstdDecHandle_set_path;
	Handle_ZstdDecoder = RL_REGISTER_HANDLE_SPEC((REBYTE*)"zstd-decoder", &spec);


	RL_REGISTER_COMPRESS_METHOD(cb_cast("zstd"), CompressZstd, DecompressZstd);


	return init_block;
}

RXIEXT int RX_Call(int cmd, RXIFRM *frm, void *ctx) {
	return Command[cmd](frm, ctx);
}
