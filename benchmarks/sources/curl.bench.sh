CURL_SOURCE=https://github.com/curl/curl.git
CURL_OPTIONS="-o curl.data"
CURL_BIN="curl"

CURL_BUILD() {
    echo "Ignore build..."
}

CURL_VERSION() {
	${CURL_BIN} --version | head -1 | cut -d' ' -f2
}
