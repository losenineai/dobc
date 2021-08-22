
include toolchain.mk

FORCE:

md5: FORCE
	cd md5; \
	$(ndkbuild); \
	cd -

md5_clean: FORCE
	cd md5; \
	$(ndkbuild) clean; \
	cd -

base64: FORCE
	cd base64 ; \
	$(ndkbuild) ; \
	cp libs/armeabi-v7a/libbase64.so  libs/armeabi-v7a/libbase64.so.decode; \
	cd -

base64_clean: FORCE
	cd base64; \
	$(ndkbuild) clean; \
	cd -

crc32: FORCE
	cd crc32 ; \
	$(ndkbuild) ; \
	cp libs/armeabi-v7a/libcrc32.so  libs/armeabi-v7a/libcrc32.so.decode; \
	cd -

crc32_clean: FORCE
	cd crc32; \
	$(ndkbuild) clean; \
	cd -