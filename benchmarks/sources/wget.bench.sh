WGET_SOURCE=https://git.savannah.gnu.org/git/wget.git
WGET_OPTIONS="--no-config -O wget.data"
WGET_BIN="wget"

WGET_BUILD() {
    echo "Ignore build..."
}

WGET_VERSION() {
	${WGET_BIN} --version | head -1 | cut -d' ' -f3
}
