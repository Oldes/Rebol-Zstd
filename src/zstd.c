//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// Project: Rebol/Zstd extension
// SPDX-License-Identifier: MIT
// ===========================================================================
// Entry points for the Rebol/Zstd extension module.
//
//   REB_EXT defined ... standalone zstd-x64.rebx
//   REB_EXT absent .... compiled into the host
//
// One-time setup lives in Zstd_Init(), called from the generated `_init`
// command when the module body evaluates - not from the entry points, so
// that `Options: [delay]` can postpone it until the module is imported.
//

#include "gen-zstd.h"

#ifdef REB_EXT
// Standalone builds are their own binary and must supply the storage.
// Embedded builds use host-lib.c's definition, declared extern by reb-lib.h.
RL_LIB *RL;
#endif

static char *init_block = ZSTD_EXT_INIT_CODE;

// Symbols of the registered handle types. Declared in the generated header
// (from the spec's `c-header:`), defined here.
REBCNT Handle_ZstdEncoder = 0;
REBCNT Handle_ZstdDecoder = 0;


// Registers the encoder/decoder handle types and the "zstd" de/compression
// method. Runs when the module body evaluates, so it happens at the same
// point in both build modes - and only on first import under `delay`.
//
// Returns plain TRUE/FALSE, NOT an RXR_* code: RXR_FALSE is 3, which is
// truthy in C. The generated handler maps the result onto RXR_TRUE/RXR_FALSE.
int Zstd_Init(void) {
	REBHSP spec;

	// Neither handle has any path-accessible field, so get_path/set_path
	// are left unset (NULL) rather than pointed at callbacks that would
	// only ever return PE_BAD_SELECT/PE_BAD_SET - same as sqlite-stmt
	// omitting set_path in sqlite.c.
	CLEARS(&spec);
	spec.size     = sizeof(void*);
	spec.flags    = HANDLE_REQUIRES_HOB_ON_FREE;
	spec.free     = ZstdEncHandle_free;
	spec.mold     = Common_mold;
	Handle_ZstdEncoder = RL_REGISTER_HANDLE_SPEC((REBYTE*)"zstd-encoder", &spec);

	CLEARS(&spec);
	spec.size     = sizeof(void*);
	spec.flags    = HANDLE_REQUIRES_HOB_ON_FREE;
	spec.free     = ZstdDecHandle_free;
	spec.mold     = Common_mold;
	Handle_ZstdDecoder = RL_REGISTER_HANDLE_SPEC((REBYTE*)"zstd-decoder", &spec);

	RL_REGISTER_COMPRESS_METHOD(cb_cast("zstd"), CompressZstd, DecompressZstd);

	return TRUE;
}


#ifdef REB_EXT

/***********************************************************************
**  Standalone extension library
***********************************************************************/

RXIEXT const char *RX_Init(int opts, RL_LIB *lib) {
	REBYTE ver[8];
	RL = lib;
	RL_VERSION(ver);
	debug_print("RX_Init zstd-extension; Rebol v%i.%i.%i\n", ver[1], ver[2], ver[3]);

	if (MIN_REBOL_VERSION > VERSION(ver[1], ver[2], ver[3])) return 0;
	if (!CHECK_STRUCT_ALIGN) {
		trace("CHECK_STRUCT_ALIGN failed!");
		return 0;
	}
	return init_block;
}

RXIEXT int RX_Quit(int opts) {
	return 0;
}

// Reports the RL_API ABI this was built against, so `load-extension`
// can refuse an incompatible host. An absent symbol means ABI 0.
RXIEXT int RX_Abi(void) {
	return RL_ABI_VERSION;
}

// Resolved by name, so the spelling is fixed. The bounds-checked
// dispatcher is generated into gen-zstd.c.
RXIEXT int RX_Call(int cmd, RXIFRM *frm, void *ctx) {
	return Zstd_RX_Call(cmd, frm, ctx);
}

#endif
