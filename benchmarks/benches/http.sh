readonly BENCHMARK_PROGRAMS=("curl" "wget" "mwget")
readonly BENCHM_NAME="http"
test_OPTIONS["WGET"]=""
test_OPTIONS["CURL"]=""
test_OPTIONS["MWGET"]=""

readonly NITER=3 # Number of iterations
readonly NFILE=3

time_cmd() {
	local cmd="$1"
	local size="$2"
	local data_file="$3"
	echo "$cmd"
	for ((i=0;i<NITER;i++)); do
		t1=$(date +%s%3N)
		$cmd &>/dev/null
		t2=$(date +%s%3N)
		echo "$size" $((t2-t1)) >> "$data_file"
		echo -n "." >&2
	done
	echo ""
}

run_bench() {
    local prog="$1"
    local PROG="${1^^}"
    local binary="${PROG}_BIN"
    local prog_options="${PROG}_OPTIONS"
    local prog_test_options="${test_OPTIONS[$PROG]}"
    local cmdline="${!binary} ${!prog_options:-} ${prog_test_options} https://archive.kernel.org/centos-vault/6.10/isos/i386/README.txt"

    rm -f "../${prog}_${BENCHM_NAME}.data"

    # Warm-up Run
    echo "Warm-up ..."
    $cmdline &>/dev/null

    local files=("" "i386/CentOS-6.10-i386-netinstall.iso" "i386/CentOS-6.10-i386-netinstall.iso" "i386/CentOS-6.10-i386-bin-DVD2.iso")
    local sizes=("" "1" "2" "3")
    for ((n=1; n<=NFILE; n++)); do
        local url="https://archive.kernel.org/centos-vault/6.10/isos/${files[$n]}"

        local size=${sizes[$n]}
        time_cmd "${!binary} ${!prog_options} ${prog_test_options:-} $url" $size "../${prog}_${BENCHM_NAME}.data"
    done

}

finish_bench() {
	local plot_title=""
	local plot_cmd
	local plot_title_left

	pushd "$SCRIPT_DIR"

	plot_title_left="$KERNEL\\n\
		$PROC\\n\
		ping $PING"
	plot_cmd="plot"
	local colornum=1
	for prog in "${BENCHMARK_PROGRAMS[@]}"; do
		gnuplot -c "${BENCHES_DIR}/convert.gp" "${prog}_${BENCHM_NAME}.data" "$NFILE"
		local prog_options="${prog^^}_OPTIONS"
		local prog_test_options="${test_OPTIONS[${prog^^}]}"
		plot_title="$plot_title\\n\
			$prog ${VERSIONS[$prog]} ${!prog_options} ${prog_test_options:-}"
		if [[ $plot_cmd == "plot" ]]; then
			plot_cmd="${plot_cmd} \"processed_${prog}_${BENCHM_NAME}.data\" using 1:2 with linespoints title \"$prog\" lt $colornum"
		else
			plot_cmd="${plot_cmd}, \"processed_${prog}_${BENCHM_NAME}.data\" using 1:2 with linespoints title \"$prog\" lt $colornum"
		fi
		plot_cmd="${plot_cmd}, \"processed_${prog}_${BENCHM_NAME}.data\" using 1:2:3:4 with yerrorbars notitle lt $colornum"
		((colornum=colornum+1))
	done
	cat <<EOF | gnuplot
	set terminal svg
	set output "${BENCHM_NAME}.svg"

	set label 1 "$plot_title_left"
	set label 2 "$plot_title"
	set label 1 at character 3, 25
	set label 2 at character 37, 26

	# aspect ratio, for image size
	set size 1,0.8

	set grid y
	set xtics 1
    set xlabel "Number of trials"
	set ylabel "Time (ms)"

	$plot_cmd
EOF
	popd
}
