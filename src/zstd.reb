REBOL [
	Title:   "Rebol Zstd Extension"
	Name:    zstd
	Version: 1.5.7
	Needs:   3.20.5
	Author:  @Oldes
	License: MIT
	Options: [delay]
	Url:     https://github.com/Oldes/Rebol-Zstd
	Exports: []
	Purpose: {
		Zstandard (de)compression - one-shot and streaming - plus
		registration of "zstd" as a compression method usable from
		Rebol's compress/decompress natives and from the HTTP scheme.

		Data only - never evaluated. The C header (gen-zstd.h) and the
		command table (gen-zstd.c) are generated from this file by
		tools/make-extension.r3.
	}
]

;; ---------------------------------------------------------------------------
;; Banner put on top of the generated files.
logo: {//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// Project: Rebol/Zstd extension
// SPDX-License-Identifier: MIT
// =============================================================================
// NOTE: auto-generated file, do not modify!}

;; ---------------------------------------------------------------------------
;; C-side configuration
;;
;; NOTE: the real Zstandard header is carried here rather than by
;; `c-include:`. The template only emits `c-include:` inside the
;; `#ifdef REB_EXT` branch, but ZSTD_CStream/ZSTD_DStream are needed in
;; both build modes (see sqlite.reb for the same reasoning).
c-header: {
#include "zstd.h"

#ifndef NO_LIMIT
#define NO_LIMIT ((REBCNT)-1)
#endif

extern REBCNT Handle_ZstdEncoder;
extern REBCNT Handle_ZstdDecoder;

int Common_mold(REBHOB *hob, REBSER *ser);

int ZstdEncHandle_free(void *hndl);
int ZstdDecHandle_free(void *hndl);

int CompressZstd(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error);
int DecompressZstd(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error);
}

;; ---------------------------------------------------------------------------
;; No `handles:` block: `zstd-encoder`/`zstd-decoder` are registered by hand
;; in zstd.c's Zstd_Init (spec.size/free/mold only, matching a raw-pointer
;; handle rather than an embedded struct). Neither type has a
;; path-accessible field, so there is nothing for this block to collect or
;; document - and an entry with no fields still rendered an empty
;; accessor table in the README. If a field is ever added, reinstate it here
;; as `NAME  GET-type  SET-type  "description"` (the way miniaudio/speak do)
;; plus a get_path/set_path pair wired up in Zstd_Init.

;; ---------------------------------------------------------------------------
;; Commands. Order is significant - it fixes the command indices.
;; The `_init` command is injected as the first one by the generator.
commands: [
	version: ["Native Zstd version"]
	compress: [
		"Compress data using Zstandard"
		data [binary! any-string!] "Input data to compress."
		/part                      "Limit the input data to a given length."
		 length [integer!]         "Length of input data."
		/level quality [integer!]  "Compression level from 1 to 22."
	]
	decompress: [
		"Decompress data using Zstandard"
		data [binary! any-string!] "Input data to decompress."
		/part                      "Limit the input data to a given length."
		 length [integer!]         "Length of input data."
		/size                      "Limit the output size."
		 bytes [integer!]          "Maximum number of uncompressed bytes."
	]

	;; Streaming API
	make-encoder: [
		"Create a new Zstandard encoder handle."
		/level quality [integer!] "Compression level from 0 to 11."
	]
	make-decoder: [
		"Create a new Zstandard decoder handle."
	]
	write: [
		"Feed data into a Zstandard streaming codec."
		codec [handle!] "Zstandard encoder or decoder handle."
		data  [binary! any-string! none!] "Data to compress or decompress, or NONE to finish the stream."
		/flush  "Finish the current data block and return the encoded chunk."
		/finish "Encode all remaining input and mark the stream as complete."
	]
	read: [
		"Retrieve pending encoded or decoded data from the stream."
		codec [handle!] "Zstandard encoder or decoder handle."
	]
]

;; ---------------------------------------------------------------------------
;; Module body. Registers "zstd" with the HTTP scheme's Accept-Encoding,
;; the same as the extension being converted did from its init code.
mezzanine: [
	attempt [
		unless find system/schemes/http/headers/Accept-Encoding "zstd" [
			append system/schemes/http/headers/Accept-Encoding ",zstd"
		]
	]
]
