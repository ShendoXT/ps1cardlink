CDLIC_FILE = /usr/local/psxsdk/share/licenses/infoeur.dat

all:
	mkdir -p cd_root
	psx-gcc cardlink.c -o cardlink.elf
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
