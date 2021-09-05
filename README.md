# dobc

1. dobc now only work success in Windows10 and VS2017.
2. dobc now only work success in thumb mode so.

## Build

```
mkdir build
cd build
cmake ..
msbuild dobc.sln
```

## Usage

### show help
```
dobc -help
```
### de-ollvm one function in one so library
```
dobc -d [funcaddr] [soname] -o output.so 
```

### de-ollvm one so library
```
dobc soname -o output.so
```

## APK be de-obfusation

Some APK test by manual, becuase auto-test hard to work close-source so。

| APK desc       | ollvm-so      | so-md5  | decode-so | func | 
| ------------ |:-------------:| -----:|-----:| -----:| 
| 斗鱼         | [libmakeurl](https://github.com/baikaishiuc/dobc/blob/main/data/%E6%96%97%E9%B1%BC/edfc5f175821c4605ebb11399953054/libmakeurl2.4.9.so)    | 2f0a73a9509eb0599aca9c298016ee9 | libmakeurl.so.d | 0x342d 0x407d 0x367d 0x15521 0x366f5 | 
| 抖音         | [libcms](https://github.com/baikaishiuc/dobc/blob/main/data/%E6%8A%96%E9%9F%B3/1d8f011a1a53b2b7bd73aed5ebe62142/libcms.so)        | 1d8f011a1a53b2b7bd73aed5ebe62142 | libcms.so.d |
| 快手         | [libqsmain](https://github.com/baikaishiuc/dobc/blob/main/data/%E5%BF%AB%E6%89%8B/2f0a73a9509eb0599aca9c298016ee9/libkwsgmain.so)     | 2f0a73a9509eb0599aca9c298016ee9 | libqsmain.so.d | 0xcb59 | 
|             | [libqsmain](https://github.com/baikaishiuc/dobc/blob/main/data/%E5%BF%AB%E6%89%8B/bf8035a0f4c9680a9b53eb225bbe12fd/libkwsgmain.so)     | bf8035a0f4c9680a9b53eb225bbe12fd | [libqsmain.so.d] (https://github.com/baikaishiuc/dobc/blob/main/data/%E5%BF%AB%E6%89%8B/bf8035a0f4c9680a9b53eb225bbe12fd/libkwsgmain.so.decode) | 0xc061 0x3e551 | 
| liblazarus  | [liblazarus](https://github.com/baikaishiuc/dobc/blob/main/data/liblazarus/liblazarus.so)     |       |  [liblazarus.so.d ](https://github.com/baikaishiuc/dobc/blob/main/data/liblazarus/test.so) | 0x15f09 0x132ed |


## Unit Tests
|        | ollvm-so      | so-md5  | decode-so | func  | 
| ------------ |:-------------:| -----:|-----:| -----:| 
| md5         | [libmd5.so](https://github.com/baikaishiuc/dobc/blob/main/unittests/md5/libs/armeabi-v7a/libmd5.so)    | 2f0a73a9509eb0599aca9c298016ee9 | [libmd5.so.d](https://github.com/baikaishiuc/dobc/blob/main/unittests/md5/libs/armeabi-v7a/libmd5.so.decode) | md5Update, md5Final | 
| md4         | libmd4.so    | 1d8f011a1a53b2b7bd73aed5ebe62142 | libmd4.so.d | md4Update, md4Final
| sha256         | libsha256.so     | 2f0a73a9509eb0599aca9c298016ee9 | libsha256.so.d | encrypt | 
| sha512         | libsha512.so     | 2f0a73a9509eb0599aca9c298016ee9 | libsha512.so.d | encrypt | 
| base64 | [libbase64.so](https://github.com/baikaishiuc/dobc/blob/main/unittests/base64/libs/armeabi-v7a/libbase64.so) | | [libbase64.so.d](https://github.com/baikaishiuc/dobc/blob/main/unittests/base64/libs/armeabi-v7a/libbase64.so.decode) | base64Encode, base64Decode |
| fft | libfft.so | | libfft.so.d | encode |
| yuv2gb | libyuv2rgb.so | | libyuv2rgb.so.d | encode |
| insertsort | libinsertsort.so | | libinsert.so | |
| quicksort | libquicksort.so | | libquicksort.so | |
| twosum | libtwosum.so | | libtwosum.so.d | leetcode | 
| avl | libavl.so | | libavl.so.d | |
| astar | libastar.so | | libastar.so.d | |
| binsearch | libbinsearch.so | | libbinsearch.so.d | |




## Wiki
