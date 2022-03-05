del test.com


sdasz80 -l -o test.s

sdcc -mz80 --no-std-crt0 --code-loc 0x0100 --data-loc 0xC000 test.rel

packihx test.ihx > test.hex
tools\hex2bin -p FF -s 0x100 -e com test.hex

pause

