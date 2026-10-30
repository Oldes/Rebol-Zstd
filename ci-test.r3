Rebol [
    title: "Rebol/Zstd extension CI test"
]

print ["Running test on Rebol build:" mold to-block system/build]
system/options/quiet: false
system/options/log/rebol: 4

;; make sure that we load a fresh extension
try [system/modules/zstd: none]

CI?: any [
    "true" = get-env "CI"
    "true" = get-env "GITHUB_ACTIONS"
    "true" = get-env "TRAVIS"
    "true" = get-env "CIRCLECI"
    "true" = get-env "GITLAB_CI"
]

if modules-dir: get-env 'REBOL_MODULES_DIR [
    system/options/modules: to-real-file modules-dir
    ls (system/options/modules)
]

;; import the extension
zstd: import 'zstd

print as-yellow "Content of the module..."
? zstd

print ["Zstd version:" zstd/version]


errors: 0
file: %src/rebol-extension.h ;; File to use as a source to compress.
data: read/binary file       ;; raw bytes, no deline, no decoding
sha1: checksum data 'sha1

;-----------------------------------------------------------------------
print-hline
print as-yellow "Basic de/compression using various qualities."
for quality 1 22 1 [
    unless all [
        time: dt [bin: zstd/compress/level data :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        equal? data zstd/decompress bin
    ][
        print [as-red "Failed to compress using quality:" quality]
        ++ errors
    ]
]

print as-yellow "Using short input..."
short-text: "X"
for quality 1 22 1 [
    unless all [
        time: dt [bin: zstd/compress/level short-text :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        str: to string! zstd/decompress bin
        equal? short-text str
    ][
        print [as-red "Failed to de/compress short text using quality:" quality]
        ++ errors
    ]
]


;-----------------------------------------------------------------------
print-hline
print as-yellow "Compress using streaming API."
unless all [
    handle? enc: zstd/make-encoder
    ? enc
    none?   zstd/read  :enc      ;; reading from encoder without data returns none
    handle? zstd/write :enc data ;; feed some data
    bin3:   zstd/write :enc none ;; finish using none value
    ? bin3
    print ["compressed size:" length? bin3]
    out:  decompress bin3 'zstd
    sha1 == checksum out 'sha1

][
    print as-red "Failed to compress using streaming API!"
    ++ errors
]
;-----------------------------------------------------------------------
print-hline
print as-yellow "Streaming using short inputs."
;; It is possible to reuse previous handle (when it was finished)
unless all [
    zstd/write :enc "Hello"  ;; Process some input data
    zstd/write :enc " "
    zstd/write :enc "Zstandard"
    ;; When there is enough data to compress,
    ;; use `read` to finish the current data block and get the encoded chunk
    bin1: zstd/read :enc
    ;; Continue with other data and use `/finish` to encode all remaining input
    ;; and mark the stream as complete.
    ? bin1
    bin2: zstd/write/finish :enc " from Rebol!"
    ;; Decompress both compressed blocks again (using extension's command this time):
    ? bin2
    str: to string! zstd/decompress join bin1 bin2
    ? str
    equal? str "Hello Zstandard from Rebol!"

    ;; Or using streaming API...
    dec: zstd/make-decoder
    zstd/write :dec bin1
    zstd/write :dec bin2
    str: to string! zstd/read :dec
    ? str
    equal? str "Hello Zstandard from Rebol!"

    ;; Or using streaming API with write/finish
    ;; NOTE: it is possible to reuse the decoder
    zstd/write :dec bin1
    str: to string! zstd/write/finish :dec bin2
    ? str
    equal? str "Hello Zstandard from Rebol!"
][
    print as-red "Failed to compress/decompress short inputs using streaming API!"
    ++ errors
]
;-----------------------------------------------------------------------
print-hline
print as-yellow "Compress and decompress files in chunks."

compress-file: function[file][
    src: open/read file                 ;; input file
    out: open/new/write join file %.zst ;; output file
    enc: zstd/make-encoder/level 6      ;; initialize Zstd encoder
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
    dec: zstd/make-decoder              ;; initialize Zstd decoder
    chunk-size: 65536
    while [not empty? chunk: copy/part src chunk-size][
        zstd/write :dec :chunk
    ]
    close src
    zstd/read :dec
]

unless all [
    compressed-file: compress-file :file
    print ["compressed size:" size? compressed-file]
    equal? data decompress-file :compressed-file
][
    print as-red "Failed to compress & decompress file."
    ++ errors
]
delete compressed-file

;-----------------------------------------------------------------------
if find system/schemes 'http [
    print-hline
    print as-yellow "Validate, that support is newly mentioned in HTTP requests."
    if none? attempt [
        ? system/schemes/http/headers/Accept-Encoding
        find system/schemes/http/headers/Accept-Encoding ",zstd"
    ][
        print as-red "Zstd not found in the HTTP's Accept-Encoding."
        ++ errors
    ]
]
;-----------------------------------------------------------------------
print-hline

prin as-yellow "TEST RESULT: "
print either errors == 0 [as-green "OK"][as-red "FAILED!"]
quit/return errors
