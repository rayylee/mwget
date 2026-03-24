MWGET_SOURCE=https://github.com/rayylee/mwget
MWGET_OPTIONS="--timeout 300"
MWGET_BIN="mwget"

MWGET_BUILD() {
    echo "Ignore build..."
}

MWGET_VERSION() {
	${MWGET_BIN} --version | head -1 | cut -d' ' -f2
}
