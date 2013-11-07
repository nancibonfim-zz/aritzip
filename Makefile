compressors=./aritzip bzip2 gzip xz
gzip_ext=gz
bzip2_ext=bz2
xz_ext=xz
aritzip_ext=az

gzip_flags=-c
bzip2_flags=-c
xz_flags=--stdout
aritzip_flags=s

gzip_decompress_flags=-d -c
bzip2_decompress_flags=-d -c
xz_decompress_flags=--stdout --decompress
aritzip_decompress_flags=d


folder=arquivos_teste
files=$(wildcard $(folder)/*)

SHELL=bash

#security issue
PATH:=$(PATH):.
CFLAGS?=-Wall -Wextra -ggdb3 -O0

all: aritzip test

aritzip:

compressor_targets=$(foreach compressor,$(compressors),$(compressor)-c)
decompressor_targets=$(foreach compressor,$(compressors),$(compressor)-d)
test: $(compressor_targets) $(decompressor_targets) md5

# Compressor, flags, file, extension
define proccess
	@echo "$5 file $(notdir $3) with method $1"
	@$(SHELL) -x -c "time  2> /dev/null $1 $2 $3 | cat > $6/$(notdir $3).$4" 2> time_spent/$5_$1_$(notdir $3)_time.txt
endef


# the cat is a zip hack
define compress
	$(call proccess,$1,$2,$3,$4,compress,compressed_files)
endef

define decompress
	$(call proccess,$1,$2,compressed_files/$(notdir $3).$4,decompressed,decompress,decompressed_files)
endef


dirs:
	@mkdir -p compressed_files time_spent

%-c: dirs
	$(foreach file,$(files),$(call compress,$(*),$($(*)_flags),$(file),$($(*)_ext)))

%-d:
	@mkdir -p decompressed_files
	$(foreach file,$(files),$(call decompress,$(*),$($(*)_decompress_flags),$(file),$($(*)_ext)))

define compare

	./compare.sh $2/$1 $3/$1.az.decompressed
endef

md5:
	@mkdir -p md5sum
	$(foreach file,$(files),$(call compare,$(notdir $(file)),arquivos_teste,decompressed_files))

.PHONY: test %-c %-d
