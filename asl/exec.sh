input=$1
./asl $input > a.t
output_file=${input//.asl/.in}
comp=${input//.asl/.out}
../tvm/tvm-linux a.t < $output_file > visentada.t

diff visentada.t $comp > /dev/null

if [ $? -ne 0 ]; then
    echo "No"
    diff -y $comp visentada.t   
else 
    echo "OK"
    rm a.t
fi
rm visentada.t
