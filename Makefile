.PHONY: help all clean arkiv-agent lib src linux-x86_32 linux-x86_64 linux-arm_32 linux-arm_64 linux-riscv_64 macos-x86_64 macos-arm_64 dist

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
	rm -f dist/*
	tar --transform "s|src/agent|arkiv-agent/agent|" \
	    -czf dist/arkiv-agent.tgz
	cd dist; md5sum arkiv-agent.tgz > arkiv-agent.md5
	cd dist; sha256sum arkiv-agent.tgz > arkiv-agent.sha256sum

dummy:
	rm -rf dist/*
	cp src/agent dist/
	mkdir dist/lib
	cp lib/*.so dist/lib/

