//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// SPDX-License-Identifier: MIT
// =============================================================================
// Rebol/Zstd extension commands
// =============================================================================


#include "zstd-rebol-extension.h"
#include <stdio.h>
#include <stdlib.h> // malloc
#include <math.h>   // fmin, fmax

#define COMMAND int

#define FRM_IS_HANDLE(n, t)   (RXA_TYPE(frm,n) == RXT_HANDLE && RXA_HANDLE_TYPE(frm, n) == t)
#define ARG_Double(n)         RXA_DEC64(frm,n)
#define ARG_Float(n)          (float)RXA_DEC64(frm,n)
#define ARG_Int32(n)          RXA_INT32(frm,n)
#define ARG_Handle_Series(n)  RXA_HANDLE_CONTEXT(frm, n)->series;
#define OPT_SERIES(n)         (RXA_TYPE(frm,n) == RXT_NONE ? NULL : RXA_SERIES(frm, n))

#define RETURN_HANDLE(hob)                   \
	RXA_HANDLE(frm, 1)       = hob;          \
	RXA_HANDLE_TYPE(frm, 1)  = hob->sym;     \
	RXA_HANDLE_FLAGS(frm, 1) = hob->flags;   \
	RXA_TYPE(frm, 1) = RXT_HANDLE;           \
	return RXR_VALUE

#define RETURN_ERROR(err)  do {RXA_SERIES(frm, 1) = (REBSER*)err; return RXR_ERROR;} while(0)
static const REBYTE* ERR_INVALID_HANDLE = (const REBYTE*)"Invalid Zstd encoder or decoder handle!";
static const REBYTE* ERR_NO_DECODER     = (const REBYTE*)"Failed to create Zstd decoder!";
static const REBYTE* ERR_NO_ENCODER     = (const REBYTE*)"Failed to create Zstd encoder!";
static const REBYTE* ERR_NO_COMPRESS    = (const REBYTE*)"Failed to compress using the Zstd encoder!";
static const REBYTE* ERR_NO_DECOMPRESS  = (const REBYTE*)"Failed to decompress using the Zstd decoder!";
static const REBYTE* ERR_ENCODER_FINISHED = (const REBYTE*)"Zstd encoder is finished!";

#define APPEND_STRING(str, ...) \
	len = snprintf(NULL,0,__VA_ARGS__);\
	if (len > SERIES_REST(str)-SERIES_LEN(str)) {\
		RL_EXPAND_SERIES(str, SERIES_TAIL(str), len);\
		SERIES_TAIL(str) -= len;\
	}\
	len = snprintf( \
		SERIES_TEXT(str)+SERIES_TAIL(str),\
		SERIES_REST(str)-SERIES_TAIL(str),\
		__VA_ARGS__\
	);\
	SERIES_TAIL(str) += len;

int Common_mold(REBHOB *hob, REBSER *str) {
	size_t len;
	if (!str) return 0;
	SERIES_TAIL(str) = 0;
	APPEND_STRING(str, "0#%lx", (unsigned long)(uintptr_t)hob->handle);
	return len;
}


int ZstdEncHandle_free(void *hndl) {
	if (!hndl) return 0;
	REBHOB *hob = (REBHOB*)hndl;
	ZSTD_CStream *encoder = (ZSTD_CStream*)hob->handle;
	//debug_print("release encoder: %p ser: %p is referenced: %i\n", encoder, hob->series, IS_MARK_HOB(hob) != 0);
	if (encoder) {
		ZSTD_freeCStream(encoder);
		hob->handle = NULL;
	}
	UNMARK_HOB(hob);
	return 0;
}
int ZstdDecHandle_free(void* hndl) {
	if (!hndl) return 0;
	REBHOB *hob = (REBHOB*)hndl;
	ZSTD_DStream *decoder = (ZSTD_DStream*)hob->handle;
	//debug_print("release decoder: %p is referenced: %i\n", decoder, IS_MARK_HOB(hob) != 0);
	if (decoder) {
		ZSTD_freeDStream(decoder);
		hob->handle = NULL;
	}
	UNMARK_HOB(hob);
	return 0;
}
int ZstdEncHandle_get_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg) {
	ZSTD_CStream *encoder = (ZSTD_CStream*)hob->handle;
	word = RL_FIND_WORD(arg_words, word);
	switch (word) {
//	case W_ARG_FINISHED:
//		*type = RXT_LOGIC;
//		arg->int32a = ZstdEncoderIsFinished(encoder)?1:0;
//		break;
	default:
		return PE_BAD_SELECT;	
	}
	return PE_USE;
}
int ZstdEncHandle_set_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg) {
	ZSTD_CStream *encoder = (ZSTD_CStream*)hob->handle;
	word = RL_FIND_WORD(arg_words, word);
	switch (word) {
//	case W_ARG_MODE:
//		switch (*type) {
//		case RXT_INTEGER:
//			ZstdEncoderSetParameter(encoder, BROTLI_PARAM_MODE, (uint32_t)arg->uint64);
//			break;
//		default: 
//			return PE_BAD_SET_TYPE;
//		}
//		break;
//	case W_ARG_SIZE_HINT:
//		switch (*type) {
//		case RXT_INTEGER:
//			ZstdEncoderSetParameter(encoder, BROTLI_PARAM_SIZE_HINT, (uint32_t)arg->uint64);
//			break;
//		default: 
//			return PE_BAD_SET_TYPE;
//		}
//		break;
	default:
		return PE_BAD_SET;	
	}
	return PE_OK;
}

int ZstdDecHandle_get_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg) {
	ZSTD_DStream *decoder = (ZSTD_DStream*)hob->handle;
	word = RL_FIND_WORD(arg_words, word);
	switch (word) {
//	case W_ARG_FINISHED:
//		*type = RXT_LOGIC;
//		arg->int32a = ZstdDecoderIsFinished(decoder)?1:0;
//		break;
	default:
		return PE_BAD_SELECT;	
	}
	return PE_USE;
}

/*
void* ZstdDefaultAllocFunc(void *opaque, size_t size) {
	debug_print("alloc: %zu\n", size);
	return RL_MEM_ALLOC(opaque, size);
}
void ZstdDefaultFreeFunc(void *opaque, void *address) {
	RL_MEM_FREE(opaque, address);
}
*/
ZSTD_DStream *decoder = NULL;

COMMAND cmd_init_words(RXIFRM *frm, void *ctx) {
	arg_words  = RL_MAP_WORDS(RXA_SERIES(frm,1));
	type_words = RL_MAP_WORDS(RXA_SERIES(frm,2));

	// custom initialization may be done here...

	return RXR_TRUE;
}

COMMAND cmd_version(RXIFRM *frm, void *ctx) {
	u32 encver = (i32)ZSTD_versionNumber();

	RXA_TYPE(frm, 1) = RXT_TUPLE;
	RXA_TUPLE(frm, 1)[0] = (encver >> 24) & 0xFF;
	RXA_TUPLE(frm, 1)[1] = (encver >> 12) & 0xFFF;
	RXA_TUPLE(frm, 1)[2] =  encver        & 0xFFF;
	RXA_TUPLE_LEN(frm, 1) = 3;

	return RXR_VALUE;
}


int CompressZstd(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error) {
	int quality = MAX(0, MIN((REBCNT)ZSTD_maxCLevel(), level));

	size_t result = ZSTD_compressBound(len);
	if (ZSTD_isError(result)) {
		*error = result;
		return FALSE;
	}
	if (result > MAX_I32) return FALSE;
	*output = RL_MAKE_BINARY((REBLEN)result);


	result = ZSTD_compress(BIN_HEAD(*output), SERIES_REST(*output), input, len, quality);
	if (ZSTD_isError(result)) {
		*error = (REBINT)result;
		return FALSE;
	}
	SERIES_TAIL(*output) = (REBLEN)result;
	return TRUE;
}

COMMAND cmd_compress(RXIFRM *frm, void *ctx) {
	REBSER *data    = RXA_SERIES(frm, 1);
	REBINT index    = RXA_INDEX(frm, 1);
	REBFLG ref_part = RXA_REF(frm, 2);
	REBLEN length   = SERIES_TAIL(data) - index;
	REBINT level    = RXA_REF(frm, 4) ? RXA_INT32(frm, 5) : 6;
	REBSER *output  = NULL;
	REBINT  error   = 0;

	if (ref_part) length = (REBLEN)MAX(0, MIN(length, RXA_INT64(frm, 3)));

	if (!CompressZstd((const REBYTE*)BIN_SKIP(data, index), length, (REBCNT)level, &output, &error)) {
		RETURN_ERROR(ERR_NO_COMPRESS);
	}

	RXA_SERIES(frm, 1) = output;
	RXA_TYPE(frm, 1) = RXT_BINARY;
	RXA_INDEX(frm, 1) = 0;
	return RXR_VALUE;
}

static ZSTD_DStream *g_decoder = NULL;
int DecompressZstd(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error) {
	size_t result;
	REBU64 out_len;

	// Using streaming API, because output size is unknown.
	// @@ But maybe it can be use when limit is available?

	// Keeping the decoder state in Rebol's handle, so it is released in case of Rebol exceptions.
	if (g_decoder == NULL) {
		g_decoder = ZSTD_createDStream();
	}
	if (!g_decoder) {
		trace("Failed to create the Zstd decoder!");
		return FALSE;
	}

	result = ZSTD_initDStream(g_decoder);
	if (ZSTD_isError(result)) {*error = (REBINT)result; return FALSE;}

	out_len = (limit != NO_LIMIT) ? limit : ZSTD_decompressBound(input, len);

	if (out_len == 0) {
		// Return empty binary.
		*output = RL_MAKE_BINARY(1);
		return TRUE;
	}
	if (out_len > MAX_I32) out_len = MAX_I32;
	*output = RL_MAKE_BINARY((REBLEN)out_len);

	ZSTD_inBuffer inBuffer = {input, len, 0};
	ZSTD_outBuffer outBuffer = {BIN_HEAD(*output), SERIES_AVAIL(*output), 0};

	while (1) {
		debug_print("output_size: %zu rec: %zu\n", outBuffer.size, ZSTD_DStreamOutSize());
		result = ZSTD_decompressStream(g_decoder, &outBuffer, &inBuffer);

		debug_print("DECODE result: %u available_in: %zu available_out: %zu\n", result, inBuffer.size - inBuffer.pos, outBuffer.size - outBuffer.pos);
		if (result == 0 || (limit != NO_LIMIT && outBuffer.pos > limit) ) break; // Decompression finished
		if (ZSTD_isError(result)) {*error = (REBINT)result; return FALSE;}

		// If the output buffer is full, resize it
		if (outBuffer.pos == outBuffer.size) {
			SERIES_TAIL(*output) = (REBLEN)outBuffer.pos;
			RL_EXPAND_SERIES(*output, AT_TAIL, SERIES_REST(*output));
			SERIES_TAIL(*output) = (REBLEN)outBuffer.pos;
			outBuffer.dst = BIN_HEAD(*output);
			outBuffer.size = SERIES_AVAIL(*output);
		}
	}

	if (limit != NO_LIMIT && outBuffer.pos > limit) outBuffer.pos = limit;

	SERIES_TAIL(*output) = (REBLEN)outBuffer.pos;
	return TRUE;

}

COMMAND cmd_decompress(RXIFRM *frm, void *ctx) {
	REBSER *data    = RXA_SERIES(frm, 1);
	REBINT index    = RXA_INDEX(frm, 1);
	REBFLG ref_part = RXA_REF(frm, 2);
	REBI64 length   = SERIES_TAIL(data) - index;
	REBI64 limit    = RXA_REF(frm, 4) ? RXA_INT64(frm, 5) : NO_LIMIT;
	REBSER *output  = NULL;
	REBINT  error   = 0;

	if (ref_part) length = MAX(0, MIN(length, RXA_INT64(frm, 3)));
	if (length < 0 || length > MAX_I32) {
		RETURN_ERROR(ERR_NO_DECOMPRESS);
	}

	if (!DecompressZstd((const REBYTE*)BIN_SKIP(data, index), (REBLEN)length, (REBCNT)limit, &output, &error)) {
		RETURN_ERROR(ERR_NO_DECOMPRESS);
	}

	RXA_SERIES(frm, 1) = output;
	RXA_TYPE(frm, 1) = RXT_BINARY;
	RXA_INDEX(frm, 1) = 0;
	return RXR_VALUE;
}


COMMAND cmd_make_encoder(RXIFRM *frm, void *ctx) {
	size_t result;
	REBINT quality = RXA_REF(frm, 1)
		? MAX(ZSTD_minCLevel(),
			  MIN(ZSTD_maxCLevel(), RXA_INT32(frm, 2)))
		: ZSTD_defaultCLevel();

	REBHOB *hob = RL_MAKE_HANDLE_CONTEXT(Handle_ZstdEncoder);
	if (hob == NULL) RETURN_ERROR(ERR_NO_ENCODER);

	ZSTD_CStream *encoder = ZSTD_createCStream();
	if (encoder == NULL) RETURN_ERROR(ERR_NO_ENCODER);

	hob->handle = encoder;

	result = ZSTD_initCStream(encoder, quality);
	if (ZSTD_isError(result)) RETURN_ERROR(ERR_NO_ENCODER);

	debug_print("enc: %p %u\n", encoder, hob->sym);

	RETURN_HANDLE(hob);
}

COMMAND cmd_make_decoder(RXIFRM *frm, void *ctx) {
	REBHOB *hob = RL_MAKE_HANDLE_CONTEXT(Handle_ZstdDecoder);
	if (hob == NULL) RETURN_ERROR(ERR_NO_DECODER);

	ZSTD_DStream *decoder = ZSTD_createDStream();
	if (decoder == NULL) RETURN_ERROR(ERR_NO_DECODER);
	
	debug_print("dec: %p\n", decoder);

	hob->handle = decoder;
	RETURN_HANDLE(hob);
}

COMMAND cmd_write(RXIFRM *frm, void *ctx) {
	REBHOB *hob       = RXA_HANDLE(frm, 1);
	REBSER *data      = OPT_SERIES(2);
	REBINT index      = RXA_INDEX(frm, 2);
	REBOOL ref_flush  = RXA_REF(frm, 3);
	REBOOL ref_finish = RXA_REF(frm, 4);
	REBYTE *inp = NULL;
	REBYTE *out = NULL;
	size_t size = 0;
	size_t available_out = 0;
	size_t available_in = 0;
	REBLEN tail;
	REBSER *buffer;
	size_t result;

	ZSTD_inBuffer inBuffer = {0, 0, 0};
	ZSTD_outBuffer outBuffer = {0, 0, 0};

	if (hob->handle == NULL || !(hob->sym == Handle_ZstdEncoder || hob->sym == Handle_ZstdDecoder)) {
		RETURN_ERROR(ERR_INVALID_HANDLE);
	}

	buffer = hob->series;

	if (!data) {
		if (!buffer) return RXR_NONE;
		outBuffer.size = SERIES_REST(buffer);
		outBuffer.dst = BIN_HEAD(buffer);
		ref_finish = TRUE;
	}
	else {
		inBuffer.src = BIN_SKIP(data, index);
		inBuffer.size = SERIES_TAIL(data) - index;
		if (buffer == NULL) {
			buffer = hob->series = RL_MAKE_BINARY((REBLEN)inBuffer.size);
		}
		tail = SERIES_TAIL(buffer);
		outBuffer.size = SERIES_REST(buffer);

		if ((outBuffer.size - tail) < inBuffer.size) {
			RL_EXPAND_SERIES(buffer, tail, SERIES_REST(buffer));
			SERIES_TAIL(buffer) = tail;
			outBuffer.size = SERIES_REST(buffer);
		}
		outBuffer.dst = BIN_HEAD(buffer);
		debug_print("input length: %zu available_out: %zu \n", inBuffer.size - inBuffer.pos, outBuffer.size - outBuffer.pos);
	}
	
	outBuffer.pos = SERIES_TAIL(buffer);

	if (hob->sym == Handle_ZstdEncoder) {
		// compress..
		ZSTD_CStream *state = (ZSTD_CStream*)hob->handle;
		while (inBuffer.pos < inBuffer.size) {
			debug_print("inBuffer.pos: %zu inBuffer.size: %zu outBuffer.size: %zu\n", inBuffer.pos, inBuffer.size, outBuffer.size);
			result = ZSTD_compressStream(state, &outBuffer, &inBuffer);
			debug_print("zstd compress result: %zu %zu in: %zu %zu\n", result, outBuffer.pos, inBuffer.pos, inBuffer.size);
			SERIES_TAIL(buffer) = outBuffer.pos;
			if (ZSTD_isError(result)) RETURN_ERROR(ERR_NO_COMPRESS);
		}
		if (ref_flush) {
			while(1) {
				result = ZSTD_flushStream(state, &outBuffer);
				debug_print("zstd flush result: %zu %zu\n", result, outBuffer.pos);
				SERIES_TAIL(buffer) = outBuffer.pos;
				if (result == 0) break; // finished
				if (ZSTD_isError(result)) RETURN_ERROR(ERR_NO_COMPRESS);
				RL_EXPAND_SERIES(buffer, AT_TAIL, result);
				outBuffer.dst = BIN_HEAD(buffer);
				outBuffer.size = SERIES_REST(buffer);
			}
		}
		if (ref_finish) {
			while(1) {
				debug_print("inBuffer.pos: %zu inBuffer.size: %zu outBuffer.size: %zu pos: %zu\n", inBuffer.pos, inBuffer.size, outBuffer.size, outBuffer.pos);
				result = ZSTD_endStream(state, &outBuffer);
				debug_print("zstd end result: %zu out.pos: %zu in: %zu %zu\n", result, outBuffer.pos, inBuffer.pos, inBuffer.size);
				SERIES_TAIL(buffer) = outBuffer.pos;
				if (result == 0) break; // finished
				if (ZSTD_isError(result)) RETURN_ERROR(ERR_NO_COMPRESS);
				RL_EXPAND_SERIES(buffer, AT_TAIL, result);
				outBuffer.dst = BIN_HEAD(buffer);
				outBuffer.size = SERIES_REST(buffer);
			}
		}
	}
	else if (inBuffer.size > 0) {
		// decompress...
		ZSTD_DStream *state = (ZSTD_DStream*)hob->handle;
		while(1) {
			debug_print("inBuffer.pos: %zu inBuffer.size: %zu outBuffer.size: %zu pos: %zu\n", inBuffer.pos, inBuffer.size, outBuffer.size, outBuffer.pos);
			result = ZSTD_decompressStream(state, &outBuffer, &inBuffer);
			debug_print("zstd compress result: %zu out.pos: %zu in: %zu %zu\n", result, outBuffer.pos, inBuffer.size, inBuffer.pos);
			if (ZSTD_isError(result)) RETURN_ERROR(ERR_NO_DECOMPRESS);
			SERIES_TAIL(buffer) = (REBLEN)outBuffer.pos;
			if (inBuffer.pos == inBuffer.size) break;
			// If the output buffer is full, resize it
			if (outBuffer.pos == outBuffer.size) {
				RL_EXPAND_SERIES(buffer, AT_TAIL, outBuffer.pos);
				SERIES_TAIL(buffer) = (REBLEN)outBuffer.pos;
				outBuffer.dst = BIN_HEAD(buffer);
				outBuffer.size = SERIES_AVAIL(buffer);
			}
		}
	}


	if (ref_flush || ref_finish) {
		// Make copy of the buffer (for safety reasons).
		REBLEN tail = SERIES_TAIL(buffer);
		REBSER *output = RL_MAKE_BINARY(tail);
		COPY_MEM(BIN_HEAD(output), BIN_HEAD(buffer), tail);
		SERIES_TAIL(output) = tail;

		// Reset tail of the buffer, so it may be resused.
		SERIES_TAIL(buffer) = 0;

		// Return the new binary
		RXA_SERIES(frm, 1) = output;
		RXA_TYPE(frm, 1) = RXT_BINARY;
		RXA_INDEX(frm, 1) = 0;
	}
	return RXR_VALUE;
}

COMMAND cmd_read(RXIFRM *frm, void *ctx) {
	REBHOB *hob  = RXA_HANDLE(frm, 1);
	REBSER *buffer;
	size_t result;

	if (hob->handle == NULL || !(hob->sym == Handle_ZstdEncoder || hob->sym == Handle_ZstdDecoder)) {
		RETURN_ERROR(ERR_INVALID_HANDLE);
	}

	if (!(buffer = hob->series)) return RXR_NONE;

	ZSTD_inBuffer inBuffer = {0, 0, 0};
	ZSTD_outBuffer outBuffer = {0, 0, 0};

	outBuffer.dst = BIN_HEAD(buffer);
	outBuffer.size = SERIES_REST(buffer);
	outBuffer.pos = SERIES_TAIL(buffer);

	// Make sure to flush all input data when compressing.
	// For decompression, all data should be already available in the buffer.
	if (hob->sym == Handle_ZstdEncoder) {
		ZSTD_CStream *state = (ZSTD_CStream*)hob->handle;
		while(1) {
			result = ZSTD_flushStream(state, &outBuffer);
			printf("zstd flush result: %zu %zu\n", result, outBuffer.pos);
			SERIES_TAIL(buffer) = outBuffer.pos;
			if (result == 0) break; // finished
			if (ZSTD_isError(result)) RETURN_ERROR(ERR_NO_COMPRESS);
			RL_EXPAND_SERIES(buffer, AT_TAIL, result);
			outBuffer.dst = BIN_HEAD(buffer);
			outBuffer.size = SERIES_REST(buffer);
		}
	}

	// Make copy of the buffer (for safety reasons).
	REBLEN tail = SERIES_TAIL(buffer);
	REBSER *output = RL_MAKE_BINARY(tail);
	COPY_MEM(BIN_HEAD(output), BIN_HEAD(buffer), tail);
	SERIES_TAIL(output) = tail;

	// Reset tail of the buffer, so it may be resused.
	SERIES_TAIL(buffer) = 0;

	// Return the new binary
	RXA_SERIES(frm, 1) = output;
	RXA_TYPE(frm, 1) = RXT_BINARY;
	RXA_INDEX(frm, 1) = 0;
	return RXR_VALUE;
}
