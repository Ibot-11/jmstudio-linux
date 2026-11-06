#!/usr/bin/env bash
# One-step installer for jmstudio-linux (3DMakerpro JMStudio on Linux, via Proton).
#
#   curl -fsSL https://raw.githubusercontent.com/Ibot-11/jmstudio-linux/main/install.sh | bash
#
# or, from a clone of the repo:   ./install.sh
#
# It fetches the repo into ~/.local/lib/jmstudio-linux (or uses the clone it runs from), puts
# `jmstudio-linux` into ~/.local/bin and starts the guided installer (`jmstudio-linux install`).
# Options for the guided installer go after `--`, for example to skip all questions:
#   curl -fsSL .../install.sh | bash -s -- --yes
#
# Environment: JMSTUDIO_REF (branch or tag to fetch, default main), JMSTUDIO_APP_DIR, JMSTUDIO_BIN_DIR.

# Everything lives in a function so a half-downloaded script never runs partially.
main() {
	set -euo pipefail

	local repo="Ibot-11/jmstudio-linux"
	local ref="${JMSTUDIO_REF:-main}"
	local app="${JMSTUDIO_APP_DIR:-$HOME/.local/lib/jmstudio-linux}"
	local bin="${JMSTUDIO_BIN_DIR:-$HOME/.local/bin}"
	local here=""

	die() { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }
	msg() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }

	[[ $EUID -ne 0 ]] || die "do not run this as root; it installs into your home directory and asks for a password only where needed."
	command -v bash >/dev/null && command -v curl >/dev/null || die "curl is required"

	# Running from a clone? Use it in place.
	if [[ -n ${BASH_SOURCE[0]:-} && -f ${BASH_SOURCE[0]} ]]; then
		here="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
	fi
	if [[ -n $here && -x $here/jmstudio-linux && -d $here/patches ]]; then
		app="$here"
		msg "using $app"
	elif command -v git >/dev/null; then
		if [[ -d $app/.git ]]; then
			msg "updating $app"
			git -C "$app" fetch --quiet --depth 1 origin "$ref"
			git -C "$app" checkout --quiet --force FETCH_HEAD
		else
			[[ ! -e $app ]] || die "$app exists and is not a jmstudio-linux checkout; move it away or set JMSTUDIO_APP_DIR"
			msg "downloading jmstudio-linux ($ref) into $app"
			mkdir -p "$(dirname "$app")"
			git clone --quiet --depth 1 --branch "$ref" "https://github.com/$repo.git" "$app"
		fi
	else
		msg "downloading jmstudio-linux ($ref) into $app"
		command -v tar >/dev/null || die "tar is required"
		mkdir -p "$app"
		curl -fsSL "https://codeload.github.com/$repo/tar.gz/$ref" | tar -xz --strip-components=1 -C "$app"
	fi

	mkdir -p "$bin"
	if [[ ! -e $bin/jmstudio-linux || -L $bin/jmstudio-linux ]]; then
		ln -sf "$app/jmstudio-linux" "$bin/jmstudio-linux"
	fi

	exec "$app/jmstudio-linux" install "$@"
}

main "$@"
