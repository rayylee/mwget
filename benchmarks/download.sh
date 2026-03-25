#!/usr/bin/env bash
#

cmd="mwget"
args=""
size="small"
declare -A urls=(
    ["small"]="http://127.0.0.1:8989/10MB.data"
    ["mid"]="http://127.0.0.1:8989/50MB.data"
    ["large"]="http://127.0.0.1:8989/200MB.data"
)
# declare -A urls=(
#     ["small"]="http://speedtest.tele2.net/5MB.zip"
#     ["mid"]="http://speedtest.tele2.net/50MB.zip"
#     ["large"]="http://speedtest.tele2.net/100MB.zip"
# )

result="result.data"

ARG1="$1"
ARG2="$2"

if [ -n "$ARG1" ]; then
    case "$ARG1" in
        mwget)
            cmd=mwget
            args="-O mwget.part"
            ;;
        wget)
            cmd="wget"
            args="-O wget.part"
            ;;
        curl)
            cmd="curl"
            args="-o curl.part"
            ;;
        *)
            ;;
    esac
fi

if [ -n "$ARG2" ]; then
    case "$ARG2" in
        small|mid|large)
            size=$ARG2
            ;;
        *)
            ;;
    esac

fi

t1=$(date +%s%3N)
$cmd $args "${urls[$size]}"
t2=$(date +%s%3N)

echo "$cmd $size $((t2-t1))" >> "$result"

exit 0
