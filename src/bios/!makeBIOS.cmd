del cpm22_bios.bin

sdasz80 -l -o cpm22_bios.s

sdcc -mz80 --no-std-crt0 --code-loc 0xEA00 --data-loc 0xF000 cpm22_bios.rel

packihx cpm22_bios.ihx > cpm22_bios.hex
tools\hex2bin -p 00 -s 0xEA00 -e rom cpm22_bios.hex

pause

