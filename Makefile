.PHONY: help all clean arkiv-agent lib src linux-x86_32 linux-x86_64 linux-arm_32 linux-arm_64 linux-riscv_64 macos-x86_64 macos-arm_64 dist distclean

help:
	@echo "$$(tput bold)General$$(tput sgr0)"
	@echo "make clean            $$(tput dim)Delete all compiled files.$$(tput sgr0)"
	@echo
	@echo "$$(tput bold)Dynamic linking$$(tput sgr0)"
	@echo "make lib              $$(tput dim)Compile libraries.$$(tput sgr0)"
	@echo "make src              $$(tput dim)Compile agent.$$(tput sgr0)"
	@echo "make all              $$(tput dim)Delete files and compile libraries and agent.$$(tput sgr0)"
	@echo
	@echo "$$(tput bold)Static linking$$(tput sgr0)"
	@echo "make linux-x86_32     $$(tput dim)Compile for Linux on x86_32.$$(tput sgr0)"
	@echo "make linux-x86_64     $$(tput dim)Compile for Linux on x86_64.$$(tput sgr0)"
	@echo "make linux-arm_32     $$(tput dim)Compile for Linux on ARM 32.$$(tput sgr0)"
	@echo "make linux-arm_64     $$(tput dim)Compile for Linux on ARM 64.$$(tput sgr0)"
	@echo "make linux-riscv_64   $$(tput dim)Compile for Linux on RISC-V 64.$$(tput sgr0)"
	@echo "make macos-x86_64     $$(tput dim)Compile for MacOS on x86_64.$$(tput sgr0)"
	@echo "make macos-arm_64     $$(tput dim)Compile for MacOS on ARM 64.$$(tput sgr0)"

arkiv-agent: lib src dist

all:
	cd lib; make all
	cd src; make all

clean:
	cd lib; make clean
	cd src; make clean

lib:
	cd lib; make

src:
	cd src; make

linux-x86_32: clean
	cd lib; make linux-x86_32
	cd src; make agent-linux-i386

linux-x86_64: clean
	cd lib; make linux-x86_64
	cd src; make agent-linux-x86_64

linux-arm_32: clean
	cd lib; make linux-arm_32
	cd src; make agent-linux-arm_32

linux-arm_64: clean
	cd lib; make linux-arm_64
	cd src; make agent-linux-arm_64

linux-riscv_64: clean
	cd lib; make linux-riscv_64
	cd src; make agent-linux-riscv_64

macos-x86_64: clean
	cd lib; make macos-x86_64
	cd src; make agent-macos-x86_64

macos-arm_64: clean
	cd lib; make macos-arm_64
	cd src; make agent-macos-arm_64

dist:
	cp src/arkiv_agent dist/
	cp var/autoextract.sh dist/
	if [ ! -f var/rclone ]; then \
		wget https://downloads.rclone.org/rclone-current-linux-amd64.zip -O var/rclone-current-linux-amd64.zip; \
		cd var; \
		unzip rclone-current-linux-amd64.zip; \
		mv rclone-*/rclone ./; \
		rm -rf rclone-*; \
		cd -; \
	fi
	cp var/rclone dist/
	makeself --tar-extra "--exclude=.gitignore" dist/ arkiv_agent.run "Arkiv agent installation script" ./autoextract.sh
	rm dist/*
	mv arkiv_agent.run dist/
	cd dist; md5sum arkiv_agent.run > arkiv_agent.run.md5
	cd dist; sha512sum arkiv_agent.run > arkiv_agent.run.sha512

distclean:
	rm dist/*
	rm -rf var/rclone*

