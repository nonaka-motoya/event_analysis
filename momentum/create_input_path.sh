# You need to create `env.sh` on the same directory.
# And you should declare shell variables `INPUT_PATH` and `OUTPUT_FILE`.
#
# Example: 
# ``` env.sh
# INPUT_PATH="/data/Users/motoya/test_20230707_100091_klong/"
# OUTPUT_FILE="./input_files/debug/LTList.txt.debug"
# ```

source env.sh

if [ $# == 1 ]; then
	if [ "$1" == "clean" ]; then
		rm $OUTPUT_FILE && touch $OUTPUT_FILE
		return
	fi
	INPUT_PATH="$1"
fi


while read file; do
	if [[ "$file" =~ "evt_" ]]; then
		echo "${INPUT_PATH}${file}/linked_tracks.root" >> $OUTPUT_FILE
	fi
done < <(ls -A -1 $INPUT_PATH)
