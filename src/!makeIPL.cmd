del cpm22.rom


sdasz80 -l -o sc3k_bios.s
sdasz80 -l -o cpm22_bios.s

sdcc -mz80 --no-std-crt0 --code-loc 0x0000 --data-loc 0xC000 --xram-loc 0xC000 cpm22_bios.rel sc3k_bios.rel

packihx cpm22.ihx > cpm22.hex
tools\hex2bin -p FF -s 0 -l 8000 -e rom cpm22.hex

pause

