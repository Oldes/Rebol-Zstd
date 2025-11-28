[![rebol-zstd](https://github.com/user-attachments/assets/05c0560a-d610-4cbb-b2da-e748f4f6541e)](https://github.com/Oldes/Rebol-Zstd)

[![Rebol-Zstd CI](https://github.com/Oldes/Rebol-Zstd/actions/workflows/main.yml/badge.svg)](https://github.com/Oldes/Rebol-Zstd/actions/workflows/main.yml)
[![Gitter](https://badges.gitter.im/rebol3/community.svg)](https://app.gitter.im/#/room/#Rebol3:gitter.im)
[![Zulip](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://rebol.zulipchat.com/)

# Rebol/Zstd

[Zstandard](https://github.com/facebook/zstd) extension for [Rebol3](https://github.com/Oldes/Rebol3) (version 3.20.5 and higher)

## Usage
Basic usage is just using `import zstd` and then use `zstd` method with default Rebol `compress` and `decompress` functions. Like:
```rebol
import zstd
bin: compress "some data" 'zstd
txt: to string! decompress bin 'zstd
```

Additionally, this extension provides a streaming API, allowing data to be (de)compressed in chunks without requiring it to be fully loaded into memory.
```rebol
zstd: import zstd        ;; Import the module and assign it to a variable
enc: zstd/make-encoder   ;; Initialize the Zstandard encoder state handle
zstd/write :enc "Hello"  ;; Process some input data
zstd/write :enc " "
zstd/write :enc "Zstandard"
;; When there is enough data to compress,
;; use `read` to finish the current data block and get the encoded chunk
bin1: zstd/read :enc
;; Continue with other data and use `/finish` to encode all remaining input
;; and mark the stream as complete.
bin2: zstd/write/finish :enc " from Rebol!"
;; Decompress both compressed blocks again (using extension's command this time):
text: to string! zstd/decompress join bin1 bin2
;== "Hello Zstandard from Rebol!"
```

Using this streaming API, you can write functions like these:
```rebol
compress-file: function[file][
    src: open/read file                 ;; input file
    out: open/new/write join file %.br  ;; output file
    enc: zstd/make-encoder/level 6      ;; initialize Zstandard encoder
    enc/size-hint: size? src
    enc/mode: 1 ;= text input
    chunk-size: 65536
    while [not finish][
        chunk: copy/part src chunk-size
        ;; If length of the chunk is less than chunk-size,
        ;; it must be the last chunk and we can finish the stream.
        finish: chunk-size > length? chunk
        ;; Flush output after each chunk.
        write out zstd/write/flush/:finish :enc :chunk
    ]
    close src
    close out
]
decompress-file: function[file][
    src: open/read file                 ;; input file
    dec: zstd/make-decoder              ;; initialize Zstandard decoder
    chunk-size: 65536
    while [not empty? chunk: copy/part src chunk-size][
        zstd/write :dec :chunk
    ]
    close src
    zstd/read :dec
]
```

## Extension commands:


#### `version`
Native Zstd version

#### `compress` `:data`
Compress data using Zstandard
* `data` `[binary! any-string!]` Input data to compress.
* `/part` Limit the input data to a given length.
* `length` `[integer!]` Length of input data.
* `/level`
* `quality` `[integer!]` Compression level from 1 to 22.

#### `decompress` `:data`
Decompress data using Zstandard
* `data` `[binary! any-string!]` Input data to decompress.
* `/part` Limit the input data to a given length.
* `length` `[integer!]` Length of input data.
* `/size` Limit the output size.
* `bytes` `[integer!]` Maximum number of uncompressed bytes.

#### `make-encoder`
Create a new Zstandard encoder handle.
* `/level`
* `quality` `[integer!]` Compression level from 0 to 11.

#### `make-decoder`
Create a new Zstandard decoder handle.

#### `write` `:codec` `:data`
Feed data into a Zstandard streaming codec.
* `codec` `[handle!]` Zstandard encoder or decoder handle.
* `data` `[binary! any-string! none!]` Data to compress or decompress, or NONE to finish the stream.
* `/flush` Finish the current data block and return the encoded chunk.
* `/finish` Encode all remaining input and mark the stream as complete.

#### `read` `:codec`
Retrieve pending encoded or decoded data from the stream.
* `codec` `[handle!]` Zstandard encoder or decoder handle.

