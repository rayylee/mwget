#!/usr/bin/env bash

SESSION_NAME="benchmark"
LOG_FILE="benchmark.log"
RESULT_FILE="result.data"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE" 2>&1
}

format() {
    local input_file="$1"

    echo "| Size | mwget | wget | curl |"
    echo "|------|-------|------|------|"

    awk '
    {
        tool = $1
        size = $2
        value = $3

        data[size, tool] = value

        sizes[size] = 1
        tools[tool] = 1
    }

    END {
        tool_order[1] = "mwget"
        tool_order[2] = "wget"
        tool_order[3] = "curl"

        size_order[1] = "small"
        size_order[2] = "mid"
        size_order[3] = "large"

        for (i = 1; i <= 3; i++) {
            size = size_order[i]
            printf "| %s ", size

            for (j = 1; j <= 3; j++) {
                tool = tool_order[j]
                printf "| %s ", data[size, tool]
            }
            print "|"
        }
    }' "$input_file"
}

do_benchmark() {
    local level="$1"
    # Create tmux session with three vertical panes
    tmux new-session -d -s "$SESSION_NAME" \
        "bash download.sh mwget $level" \; \
        split-window -v \
        "bash download.sh wget $level" \; \
        split-window -v \
        "bash download.sh curl $level" \; \
        select-layout even-vertical \; \
            attach-session

    # Log final file sizes
    log "$level results:"
    log "  curl.part  : $(sha256sum curl.part 2>/dev/null | cut -f1)"
    log "  wget.part  : $(sha256sum wget.part 2>/dev/null | cut -f1)"
    log "  mwget.part : $(sha256sum mwget.part 2>/dev/null | cut -f1)"

    # Clean up downloaded files
    log "Cleaning up downloaded files..."
    rm -f curl.part wget.part mwget.part 2>/dev/null
}

start_nginx() {
    [ -d .tmp ] && rm -rf .tmp
    mkdir -p .tmp

    dd if=/dev/random of=.tmp/10MB.data bs=1M count=10 2>/dev/null
    dd if=/dev/random of=.tmp/50MB.data bs=1M count=50 2>/dev/null
    dd if=/dev/zero of=.tmp/200MB.data bs=1M count=0 seek=200 2>/dev/null

    if [ -e "$PWD/.tmp/nginx.pid" ]; then
        kill $(cat $PWD/.tmp/nginx.pid)
    fi

    cat > .tmp/nginx.conf << EOF
pid $PWD/.tmp/nginx.pid;
events {
    worker_connections 1024;
}
http {
    server {
        listen 8989;
        server_name localhost;

        location / {
            root $PWD/.tmp;
            autoindex on;
            limit_rate 1000k;
        }
    }
}
EOF

    nginx -c $PWD/.tmp/nginx.conf
}

stop_nginx() {
    if [ -e "$PWD/.tmp/nginx.pid" ]; then
        kill $(cat $PWD/.tmp/nginx.pid)
    fi

    [ -d .tmp ] && rm -rf .tmp
}

[ -e $RESULT_FILE ] && rm -f $RESULT_FILE
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Starting tmux download test with mwget, wget and curl" > "$LOG_FILE"

start_nginx

for s in \
    small \
    mid \
    large
do
    do_benchmark $s
done

log "Cleanup completed"
log "Test finished successfully"

stop_nginx

format $RESULT_FILE

# tmux attach-session -t "$SESSION_NAME"

exit 0
