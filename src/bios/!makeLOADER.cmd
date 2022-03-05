del boot_loader.bin


sdasz80 -l -o boot_loader.s

sdcc -mz80 --no-std-crt0 --code-loc 0x0000 --data-loc 0xC000 boot_loader.rel

packihx boot_loader.ihx > boot_loader.hex
tools\hex2bin -p FF -s 0xFF00 -e bin boot_loader.hex

pause

