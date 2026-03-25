#!/usr/bin/env bash

SESSION_NAME="benchmark"
LOG_FILE="benchmark.log"
RESULT_FILE="result.data"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE" 2>&1
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
    log "  curl.download  : $(sha256sum curl.download 2>/dev/null | cut -f1)"
    log "  wget.download  : $(sha256sum wget.download 2>/dev/null | cut -f1)"
    log "  mwget.download : $(sha256sum mwget.download 2>/dev/null | cut -f1)"

    # Clean up downloaded files
    log "Cleaning up downloaded files..."
    rm -f curl.download wget.download mwget.download 2>/dev/null
}

start_nginx() {
    [ -d .www ] && rm -rf .www
    mkdir -p .www

    dd if=/dev/random of=.www/10MB.data bs=1M count=10 2>/dev/null
    dd if=/dev/random of=.www/50MB.data bs=1M count=50 2>/dev/null
    dd if=/dev/zero of=.www/200MB.data bs=1M count=0 seek=200 2>/dev/null

    if [ -e "$PWD/.www/nginx.pid" ]; then
        kill $(cat $PWD/.www/nginx.pid)
    fi

    cat > .www/nginx.conf << EOF
pid $PWD/.www/nginx.pid;
events {
    worker_connections 1024;
}
http {
    server {
        listen 8989;
        server_name localhost;

        location / {
            root $PWD/.www;
            autoindex on;
            limit_rate 1000k;
        }
    }
}
EOF

    nginx -c $PWD/.www/nginx.conf
}

stop_nginx() {
    if [ -e "$PWD/.www/nginx.pid" ]; then
        kill $(cat $PWD/.www/nginx.pid)
    fi

    [ -d .www ] && rm -rf .www
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

# tmux attach-session -t "$SESSION_NAME"

exit 0
