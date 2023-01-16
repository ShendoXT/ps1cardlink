CDLIC_FILE = /usr/local/psxsdk/share/licenses/infoeur.dat
NOPS_CMD = mono ~/Tools/nops.exe
#COM_PORT = /dev/cu.usbserial-10
COM_PORT = 192.168.220.33:23
BIN2H = python3 ~/Tools/bin2header.py

all:
	psx-gcc ports.c cardlink.c -o cardlink.elf
	elf2exe cardlink.elf cardlink.exe -mark_eur
	upx -9 cardlink.exe --force

image:
	mkdir -p cd_root
	psx-gcc ports.c cardlink.c -o cardlink.elf
	elf2exe cardlink.elf cardlink.exe -mark_eur
	cp cardlink.exe cd_root
	systemcnf cardlink.exe > cd_root/system.cnf
	mkisofs -o cardlink.hsf -V cardlink -sysid PLAYSTATION cd_root
	mkpsxiso cardlink.hsf cardlink.bin $(CDLIC_FILE)

clean:
	rm cardlink.elf
	rm cardlink.exe
	rm cardlink.hsf
	rm cardlink.cue
	rm cardlink.bin

run:
	$(NOPS_CMD) /exe cardlink.exe $(COM_PORT) /m
	