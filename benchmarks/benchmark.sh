#!/usr/bin/env bash


set -e
set -o pipefail
set -u

# Early exit if we are running in a Bash shell older than v4.
# This script relies on Bashisms which were introduced only in v4.
if ((BASH_VERSINFO[0] < 4)); then
	echo "Sorry, you need at least Bash v4 to run this script"
	exit 1
fi

# Get the location of where the script exists on disk
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

readonly SOURCES_DIR="sources"
readonly BENCHES_DIR="benches"

# Declare arrays which are used within the sourced benchmark scripts
declare -A VERSIONS
declare -A test_OPTIONS

system_status() {
	# Global parameters defining state of test rig
	echo -e "Please wait, testing network connection ...\\n"
	readonly KERNEL="$(uname -srmo | cut -d'-' -f1)"
	readonly PROC="$(grep "model name" /proc/cpuinfo | uniq | cut -d':' -f2- | sed 's/\@/\\\\\@/g')"
	readonly CPUS="$(lscpu 2>/dev/null | grep "On-line CPU" | cut -d':' -f2 | sed 's/ //g')"
	readonly DATE=$(date +"%Y-%m-%d %H:%M:%S")
	readonly PING="RTT $(ping -c 5 speedtest.tele2.net  | tail -1 | awk '{print $4}' | cut -d'/' -f2)ms to speedtest.tele2.net"
}

# Declare pushd and popd functions to not be so verbose
pushd() {
	command pushd "$@" &> /dev/null
}

popd() {
	command popd &> /dev/null
}

# get_source <program-name>
#
# Clone the program repository or update an existing repository
get_source() {
    local program="$1"
    local PROG="${1^^}"
    if [[ ! -d "$prog" ]]; then
        local SRC_URL="${PROG}_SOURCE"
        git clone "${!SRC_URL}" "$program"
    else
        pushd "$program"
            git reset --hard HEAD
            git checkout master
            git pull origin master
        popd
    fi
}

# build_source <program-name>
#
# Call the <PROGRAM-NAME>_BUILD() function which is defined in the program
# specification to compile the program. Any configure options or CFLAGS should
# be added to the program specification file. CFLAGS may optionally be exported
# before the invocation of this script.
build_source() {
	local program="$1"
	local PROG="${1^^}"

	local BUILD_CMD="${PROG}_BUILD"
	${BUILD_CMD}
}

# Make sure we are in the directory where the script is located.
# From this point onwards, the script may make use of relative paths
cd "$SCRIPT_DIR"

# Global params that are set by the argparse code
NOSOURCE=false
NOBUILD=false

while getopts ":sb" opt; do
	case $opt in
		s) NOSOURCE=true;;
		b) NOBUILD=true;;
		:) echo "Missing argument for -$OPTARG" && exit 1;;
		\?) echo "Unknown option: $OPTARG" && exit 1;;
	esac
done

# Shift all the parsed options out. The next argument should be the name of the
# benchmark to execute
shift $((OPTIND-1))
readonly BENCH_NAME="${1:-}"

# Ensure that a valid benchmark is always available
if [[ -z $BENCH_NAME ]]; then
	echo "No benchmark specified. Exiting..."
	exit 1
elif [[ ! -f "${BENCHES_DIR}/${BENCH_NAME}.sh" ]]; then
	echo "Benchmark specification file ${BENCHES_DIR}/${BENCH_NAME}.sh not found"
	exit 1
else
	# shellcheck source=./benches/http2.sh
	source "./${BENCHES_DIR}/${BENCH_NAME}.sh"

fi

for prog in "${BENCHMARK_PROGRAMS[@]}"; do
	if [[ ! -f "${SOURCES_DIR}/${prog}.bench.sh" ]]; then
		echo "The benchmark config file for $prog not found. Exiting"
		exit 1
	fi
done

mkdir -p "$SOURCES_DIR"

system_status

echo -e "Kernel: $KERNEL\\nProcessor: $PROC\\nOn-line CPU(s): $CPUS\\nDate: $DATE\\nPing: $PING\\n"

for prog in "${BENCHMARK_PROGRAMS[@]}"; do
	echo "Running for: $prog"
	source "./${SOURCES_DIR}/$prog.bench.sh"

    pushd "$SOURCES_DIR"
        if [[ $NOSOURCE == false ]]; then
            get_source "$prog"
        fi

        if [[ $NOBUILD == false ]]; then
            pushd "$prog"
                build_source "$prog"
            popd
        fi

        VERSION_CMD="${prog^^}_VERSION"
        VERSIONS[$prog]=$(${VERSION_CMD})
        echo "Version: ${VERSIONS[$prog]}"

        run_bench "$prog" &
    popd
done
wait
finish_bench
