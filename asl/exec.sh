input=$1
./asl $input > a.t
output_file=${input//.asl/.in}
../tvm/tvm-linux a.t < $output_file
