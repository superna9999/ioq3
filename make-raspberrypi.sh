#!/bin/bash

git_hash=$(git describe --always)

make \
	BR=build \
	BD=build \
	USE_CODEC_VORBIS=1 \
	USE_CURL=1 \
	USE_CURL_DLOPEN=0 \
	USE_OPENAL=1 \
	USE_OPENAL_DLOPEN=0 \
	USE_RENDERER_DLOPEN=0 \
	USE_VOIP=1 \
	USE_LOCAL_HEADERS=0 \
	USE_INTERNAL_JPEG=1 \
	USE_INTERNAL_SPEEX=1 \
	USE_INTERNAL_ZLIB=1 \
	BUILD_CLIENT_SMP=0 \
	BUILD_GAME_SO=1 \
	BUILD_GAME_QVM=1 \
	BUILD_RENDERER_OPENGL2=0 \
	ARCH=arm \
	PLATFORM=linux \
	PLATFORM_HACK=raspberrypi \
	COMPILE_ARCH=arm \
	COMPILE_PLATFORM=linux \
	VERSION=1.36+${git_hash} \
	COPYDIR=/opt/quake3/ \
	release \
	-j4
