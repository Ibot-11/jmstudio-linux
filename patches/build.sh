#!/usr/bin/env bash
# Build the patched Wine DLLs that make the scanner visible to JMStudio.
#
#   devenum.dll  0001: DirectShow reports a Windows-style USB DevicePath for each camera
#                      (Wine says "dummy_videoN", so JMStudio cannot read the vendor/product ID)
#                0003: the camera's display name is "@device:pnp:<DevicePath>" like on Windows
#                      (JMStudio's Moose module parses vid_/&mi_ out of it)
#   mf.dll       0002: Media Foundation lists the cameras (Qt's camera list, which JMStudio
#                      uses to find the scanner's serial number, is empty without it)
#
# Both are built from the Wine 11.0 release sources with clang; nothing is installed.
# Needs: clang, lld, flex, bison, gcc, make, patch, curl, tar, xz.
# Usage: build.sh [OUTPUT_DIR]      (default: ./camerafix-build-out)
set -euo pipefail

WINE_VERSION="11.0"
WINE_URL="https://dl.winehq.org/wine/source/$WINE_VERSION/wine-$WINE_VERSION.tar.xz"
# Release archive; its detached signature (…tar.xz.sign) verifies against Alexandre Julliard's key
# DA23 579A 74D4 AD9A F9D3  F945 CEFA C8EA AF17 519D.
WINE_SHA256="c07a6857933c1fc60dff5448d79f39c92481c1e9db5aa628db9d0358446e0701"
DLLS=(devenum mf)

HERE="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
OUT="$(realpath -m "${1:-./camerafix-build-out}")"
WORK="${XDG_CACHE_HOME:-$HOME/.cache}/jmstudio-linux/wine-build"

die() { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }
msg() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }

for c in clang lld-link flex bison gcc make patch curl tar xz sha256sum; do
	command -v "$c" >/dev/null || die "missing build tool: $c"
done

mkdir -p "$WORK" "$OUT"
TARBALL="$WORK/wine-$WINE_VERSION.tar.xz"
if [[ ! -f $TARBALL ]]; then
	msg "downloading $WINE_URL (33 MB)"
	curl -fsSL -o "$TARBALL.part" "$WINE_URL"
	mv "$TARBALL.part" "$TARBALL"
fi
echo "$WINE_SHA256  $TARBALL" | sha256sum -c --quiet - || die "checksum mismatch for $TARBALL (delete it and retry)"

SRC="$WORK/wine-$WINE_VERSION"
rm -rf "$SRC" "$WORK/build"
msg "unpacking and patching"
tar -C "$WORK" -xf "$TARBALL"
for p in "$HERE"/0*.patch; do patch -d "$SRC" -p1 --quiet < "$p"; done

msg "configuring (PE target via clang, only what the DLLs need)"
mkdir -p "$WORK/build"
cd "$WORK/build"
"$SRC/configure" --enable-archs=x86_64 --with-mingw=clang --disable-tests --without-x --without-wayland \
	--without-alsa --without-pulse --without-oss --without-cups --without-sane --without-gphoto \
	--without-pcap --without-krb5 --without-gstreamer --without-ffmpeg --without-vulkan \
	--without-opencl --without-usb --without-v4l2 --without-udev --without-sdl > configure.log 2>&1 \
	|| die "configure failed, see $WORK/build/configure.log"

msg "building ${DLLS[*]}"
targets=()
for d in "${DLLS[@]}"; do targets+=("dlls/$d/x86_64-windows/$d.dll"); done
make -j"$(nproc)" "${targets[@]}" > make.log 2>&1 || die "build failed, see $WORK/build/make.log"

for d in "${DLLS[@]}"; do cp "dlls/$d/x86_64-windows/$d.dll" "$OUT/$d.dll"; done
msg "built ${DLLS[*]} in $OUT"
