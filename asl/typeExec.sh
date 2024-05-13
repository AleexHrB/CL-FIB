input=$1
./asl $input > a.t
output_file=${input//.asl/.err}
diff -y a.t $output_file 
rm a.t
