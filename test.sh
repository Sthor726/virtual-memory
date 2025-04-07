make clean
make
# use the comand line arguments but if there are none then defualt to rand, sort
# ./virtmem 10 5 ${1:-"rand"} ${2:-"scan"}  > log.txt
./virtmem batch > log.txt