input_path="/data/Users/motoya/20230709/20230212_nuall_00010-00019_p500/"
output_file="test.txt"

if [ $# == 1 ]; then
	if [ "$1" == "clean" ]; then
		rm $output_file && touch $output_file
		return
	fi
	input_path="$1"
fi


while read file; do
	if [[ "$file" =~ "evt_" ]]; then
		echo "${input_path}${file}/linked_tracks.root" >> $output_file
	fi
done < <(ls -A -1 $input_path)
