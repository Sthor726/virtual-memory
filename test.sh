
make clean
make
# use the comand line arguments but if there are none then defualt to rand, sort
./virtmem 10 5 custom ${2:-"focus"} # > log.txt