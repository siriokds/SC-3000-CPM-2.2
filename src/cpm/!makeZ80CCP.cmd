del z80ccp.bin


sdasz80 -l -o z80ccp.s

sdcc -mz80 --no-std-crt0 --code-loc 0xDC00 --data-loc 0xC000 z80ccp.rel

packihx z80ccp.ihx > z80ccp.hex
tools\hex2bin -p 0 -s 0xD300 -m 0x100 -e bin z80ccp.hex

pause

