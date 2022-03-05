del z80p2dos.bin


sdasz80 -l -o z80p2dos.s

sdcc -mz80 --no-std-crt0 --code-loc 0xDC00 --data-loc 0xC000 z80p2dos.rel

packihx z80p2dos.ihx > z80p2dos.hex
tools\hex2bin -p 0 -s 0xDC00 -e bin z80p2dos.hex

pause

