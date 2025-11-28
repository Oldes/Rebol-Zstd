//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// Project: Rebol/Zstd extension
// SPDX-License-Identifier: MIT
// =============================================================================
// NOTE: auto-generated file, do not modify!


#include "zstd.h"
#include "rebol-extension.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4267)   /* conversion from 'size_t' to 'int', possible loss of data */
#endif

#define SERIES_TEXT(s)   ((char*)SERIES_DATA(s))

#define MIN_REBOL_VER 3
#define MIN_REBOL_REV 20
#define MIN_REBOL_UPD 5
#define VERSION(a, b, c) (a << 16) + (b << 8) + c
#define MIN_REBOL_VERSION VERSION(MIN_REBOL_VER, MIN_REBOL_REV, MIN_REBOL_UPD)

#ifndef NO_LIMIT
#define NO_LIMIT ((REBCNT)-1)
#endif


extern REBCNT Handle_ZstdEncoder;
extern REBCNT Handle_ZstdDecoder;

extern u32* arg_words;
extern u32* type_words;


enum ext_commands {
	CMD_ZSTD_INIT_WORDS,
	CMD_ZSTD_VERSION,
	CMD_ZSTD_COMPRESS,
	CMD_ZSTD_DECOMPRESS,
	CMD_ZSTD_MAKE_ENCODER,
	CMD_ZSTD_MAKE_DECODER,
	CMD_ZSTD_WRITE,
	CMD_ZSTD_READ,
};


int cmd_init_words(RXIFRM *frm, void *ctx);
int cmd_version(RXIFRM *frm, void *ctx);
int cmd_compress(RXIFRM *frm, void *ctx);
int cmd_decompress(RXIFRM *frm, void *ctx);
int cmd_make_encoder(RXIFRM *frm, void *ctx);
int cmd_make_decoder(RXIFRM *frm, void *ctx);
int cmd_write(RXIFRM *frm, void *ctx);
int cmd_read(RXIFRM *frm, void *ctx);

enum ma_arg_words {W_ARG_0
};
enum ma_type_words {W_TYPE_0
};

typedef int (*MyCommandPointer)(RXIFRM *frm, void *ctx);

#define ZSTD_EXT_INIT_CODE \
	"REBOL [Title: \"Rebol Zstd Extension\" Name: zstd Type: module Version: 0.1.0 Needs: 3.20.5 Author: Oldes Date: 28-Nov-2025/13:50:15 License: MIT Url: https://github.com/Oldes/Rebol-Zstd]\n"\
	"init-words: command [args [block!] type [block!]]\n"\
	"version: command [\"Native Zstd version\"]\n"\
	"compress: command [\"Compress data using Zstandard\" data [binary! any-string!] \"Input data to compress.\" /part \"Limit the input data to a given length.\" length [integer!] \"Length of input data.\" /level quality [integer!] \"Compression level from 1 to 22.\"]\n"\
	"decompress: command [\"Decompress data using Zstandard\" data [binary! any-string!] \"Input data to decompress.\" /part \"Limit the input data to a given length.\" length [integer!] \"Length of input data.\" /size \"Limit the output size.\" bytes [integer!] \"Maximum number of uncompressed bytes.\"]\n"\
	"make-encoder: command [\"Create a new Zstandard encoder handle.\" /level quality [integer!] \"Compression level from 0 to 11.\"]\n"\
	"make-decoder: command [\"Create a new Zstandard decoder handle.\"]\n"\
	"write: command [\"Feed data into a Zstandard streaming codec.\" codec [handle!] \"Zstandard encoder or decoder handle.\" data [binary! any-string! none!] {Data to compress or decompress, or NONE to finish the stream.} /flush {Finish the current data block and return the encoded chunk.} /finish {Encode all remaining input and mark the stream as complete.}]\n"\
	"read: command [{Retrieve pending encoded or decoded data from the stream.} codec [handle!] \"Zstandard encoder or decoder handle.\"]\n"\
	"init-words [][]\n"\
	"protect/hide 'init-words\n"\
	"\n"\
	";; Update HTTP scheme that it's able to use the Zstd compression\n"\
	"attempt [\n"\
	"	unless find system/schemes/http/headers/Accept-Encoding \"zstd\" [\n"\
	"		append system/schemes/http/headers/Accept-Encoding \",zstd\"\n"\
	"	]\n"\
	"]\n"

#ifdef  USE_TRACES
#include <stdio.h>
#define debug_print(fmt, ...) do { printf(fmt, __VA_ARGS__); } while (0)
#define trace(str) puts(str)
#else
#define debug_print(fmt, ...)
#define trace(str) 
#endif

