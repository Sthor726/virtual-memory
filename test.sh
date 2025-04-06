
make clean
make
# use the comand line arguments but if there are none then defualt to rand, sort
./virtmem 10 7 ${1:-"rand"} ${2:-"scan"}  > log.txt