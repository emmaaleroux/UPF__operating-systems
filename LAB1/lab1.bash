gcc -Wall -Wextra -Werror main.c circularBuffer.c -o main
time ./main binary ../Data/test_small.dat 128 #changed the last number from 18,64,128,1024 (buffer size)
#changed the name of the file to change the inputted file
#time command: to measure the time the execution lasted
hexdump -v -e '10/4 "%d " "\n"' ../Data/test_big.dat #prints 10 integers
ls -lh ../Data/test_big.dat #since I'm working from the src file I use this, however the last part is only the path to the desired files
ls -lh ../Data/int_text_big.txt #second file
./main text ../Data/test_big.dat 128 #read a binary file as text
./main binary ../Data/int_text_big.txt 128 #read a text file as binary
