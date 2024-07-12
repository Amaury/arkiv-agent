.PHONY: help all clean arkiv-agent lib src linux-x86_32 linux-x86_64 linux-arm_32 linux-arm_64 linux-riscv_64 macos-x86_64 macos-arm_64 distclean distallclean

help:
	@echo "$$(tput bold)General$$(tput sgr0)"
	@echo "make clean                $$(tput dim)Delete all compiled files.$$(tput sgr0)"
	@echo "make distclean            $$(tput dim)Delete created distribution files.$$(tput sgr0)"
	@echo "make distallclean         $$(tput dim)Delete created distribution files and downloaded rclone copies.$$(tput sgr0)"
	@echo
	@echo "$$(tput bold)Dynamic linking$$(tput sgr0)"
	@echo "make lib                  $$(tput dim)Compile libraries.$$(tput sgr0)"
	@echo "make src                  $$(tput dim)Compile agent.$$(tput sgr0)"
	@echo "make all                  $$(tput dim)Delete files and compile libraries and agent.$$(tput sgr0)"
	@echo
	@echo "$$(tput bold)Static linking$$(tput sgr0)"
	@if ! type zig > /dev/null 2>&1; then echo "$$(tput setab 1)Add to your \$$PATH environment variable the path to the zig compiler's directory.$$(tput sgr0)"; fi
	@echo "make linux-x86_32         $$(tput dim)Compile for Linux on x86_32.$$(tput sgr0)"
	@echo "make linux-x86_64         $$(tput dim)Compile for Linux on x86_64.$$(tput sgr0)"
	@echo "make linux-arm_32         $$(tput dim)Compile for Linux on ARM 32.$$(tput sgr0)"
	@echo "make linux-arm_64         $$(tput dim)Compile for Linux on ARM 64.$$(tput sgr0)"
	@echo "make macos-x86_64         $$(tput dim)Compile for MacOS on x86_64.$$(tput sgr0)"
	@echo "make macos-arm_64         $$(tput dim)Compile for MacOS on ARM 64.$$(tput sgr0)"
	@echo
	@echo "$$(tput bold)Creation of distribution$$(tput sgr0)"
	@echo "$$(tput setaf 2)For each distribution creation, the code is cleaned and compiled for the targeted architecture.$$(tput sgr0)"
	@echo "make dist-linux-x86_32    $$(tput dim)Create the distribution files for Linux x86 32 bits.$$(tput sgr0)"
	@echo "make dist-linux-x86_64    $$(tput dim)Create the distribution files for Linux x86 64 bits.$$(tput sgr0)"
	@echo "make dist-linux-arm_32    $$(tput dim)Create the distribution files for Linux ARM 32 bits.$$(tput sgr0)"
	@echo "make dist-linux-arm_64    $$(tput dim)Create the distribution files for Linux ARM 64 bits.$$(tput sgr0)"
	@echo "make dist-macos-x86_64    $$(tput dim)Create the distribution files for MacOS x86 64 bits.$$(tput sgr0)"
	@echo "make dist-macos-arm_64    $$(tput dim)Create the distribution files for MacOS ARM 64 bits.$$(tput sgr0)"

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
	cd src; make linux-x86_32

linux-x86_64: clean
	cd lib; make linux-x86_64
	cd src; make linux-x86_64

linux-arm_32: clean
	cd lib; make linux-arm_32
	cd src; make linux-arm_32

linux-arm_64: clean
	cd lib; make linux-arm_64
	cd src; make linux-arm_64

macos-x86_64: clean
	cd lib; make macos-x86_64
	cd src; make macos-x86_64

macos-arm_64: clean
	cd lib; make macos-arm_64
	cd src; make macos-arm_64

dist-linux-x86_32: distclean linux-x86_32
	cp src/arkiv_agent-linux-x86_32 dist/arkiv_agent
	cp var/autoextract.sh dist/
	if [ ! -f var/rclone_linux-386 ]; then \
		wget https://downloads.rclone.org/rclone-current-linux-386.zip -O var/rclone-current-linux-386.zip; \
		cd var; \
		unzip rclone-current-linux-386.zip; \
		mv rclone-*/rclone ./rclone_linux-386; \
		rm -rf rclone-*; \
		cd -; \
	fi
	cp var/rclone_linux-386 dist/rclone
	makeself --tar-extra "--exclude=.gitignore" dist/ arkiv_agent-linux-x86_32.run "Arkiv agent installation script" ./autoextract.sh
	rm dist/*
	mv arkiv_agent-linux-x86_32.run dist/
	cd dist; md5sum arkiv_agent-linux-x86_32.run > arkiv_agent-linux-x86_32.run.md5
	cd dist; sha512sum arkiv_agent-linux-x86_32.run > arkiv_agent-linux-x86_32.run.sha512

dist-linux-x86_64: distclean linux-x86_64
	cp src/arkiv_agent-linux-x86_64 dist/arkiv_agent
	cp var/autoextract.sh dist/
	if [ ! -f var/rclone_linux-amd64 ]; then \
		wget https://downloads.rclone.org/rclone-current-linux-amd64.zip -O var/rclone-current-linux-amd64.zip; \
		cd var; \
		unzip rclone-current-linux-amd64.zip; \
		mv rclone-*/rclone ./rclone_linux-amd64; \
		rm -rf rclone-*; \
		cd -; \
	fi
	cp var/rclone_linux-amd64 dist/rclone
	makeself --tar-extra "--exclude=.gitignore" dist/ arkiv_agent-linux-x86_64.run "Arkiv agent installation script" ./autoextract.sh
	rm dist/*
	mv arkiv_agent-linux-x86_64.run dist/
	cd dist; md5sum arkiv_agent-linux-x86_64.run > arkiv_agent-linux-x86_64.run.md5
	cd dist; sha512sum arkiv_agent-linux-x86_64.run > arkiv_agent-linux-x86_64.run.sha512

dist-linux-arm_32: distclean linux-arm_32
	cp src/arkiv_agent-linux-arm_32 dist/arkiv_agent
	cp var/autoextract.sh dist/
	if [ ! -f var/rclone_linux-arm-v7 ]; then \
		wget https://downloads.rclone.org/rclone-current-linux-arm-v7.zip -O var/rclone-current-linux-arm-v7.zip; \
		cd var; \
		unzip rclone-current-linux-arm-v7.zip; \
		mv rclone-*/rclone ./rclone_linux-arm-v7; \
		rm -rf rclone-*; \
		cd -; \
	fi
	cp var/rclone_linux-arm-v7 dist/rclone
	makeself --tar-extra "--exclude=.gitignore" dist/ arkiv_agent-linux-arm_32.run "Arkiv agent installation script" ./autoextract.sh
	rm dist/*
	mv arkiv_agent-linux-arm_32.run dist/
	cd dist; md5sum arkiv_agent-linux-arm_32.run > arkiv_agent-linux-arm_32.run.md5
	cd dist; sha512sum arkiv_agent-linux-arm_32.run > arkiv_agent-linux-arm_32.run.sha512

dist-linux-arm_64: distclean linux-arm_64
	cp src/arkiv_agent-linux-arm_64 dist/arkiv_agent
	cp var/autoextract.sh dist/
	if [ ! -f var/rclone_linux-arm64 ]; then \
		wget https://downloads.rclone.org/rclone-current-linux-arm64.zip -O var/rclone-current-linux-arm64.zip; \
		cd var; \
		unzip rclone-current-linux-arm64.zip; \
		mv rclone-*/rclone ./rclone_linux-arm64; \
		rm -rf rclone-*; \
		cd -; \
	fi
	cp var/rclone_linux-arm64 dist/rclone
	makeself --tar-extra "--exclude=.gitignore" dist/ arkiv_agent-linux-arm_64.run "Arkiv agent installation script" ./autoextract.sh
	rm dist/*
	mv arkiv_agent-linux-arm_64.run dist/
	cd dist; md5sum arkiv_agent-linux-arm_64.run > arkiv_agent-linux-arm_64.run.md5
	cd dist; sha512sum arkiv_agent-linux-arm_64.run > arkiv_agent-linux-arm_64.run.sha512

dist-macos-x86_64: distclean macos-x86_64
	cp src/arkiv_agent-macos-x86_64 dist/arkiv_agent
	cp var/autoextract.sh dist/
	if [ ! -f var/rclone_osx-amd64 ]; then \
		wget https://downloads.rclone.org/rclone-current-osx-amd64.zip -O var/rclone-current-osx-amd64.zip; \
		cd var; \
		unzip rclone-current-osx-amd64.zip; \
		mv rclone-*/rclone ./rclone_osx-amd64; \
		rm -rf rclone-*; \
		cd -; \
	fi
	cp var/rclone_osx-amd64 dist/rclone
	makeself --tar-extra "--exclude=.gitignore" dist/ arkiv_agent-macos-x86_64.run "Arkiv agent installation script" ./autoextract.sh
	rm dist/*
	mv arkiv_agent-macos-x86_64.run dist/
	cd dist; md5sum arkiv_agent-macos-x86_64.run > arkiv_agent-macos-x86_64.run.md5
	cd dist; sha512sum arkiv_agent-macos-x86_64.run > arkiv_agent-macos-x86_64.run.sha512

dist-macos-arm_64: distclean macos-arm_64
	cp src/arkiv_agent-macos-arm_64 dist/arkiv_agent
	cp var/autoextract.sh dist/
	if [ ! -f var/rclone_osx-arm64 ]; then \
		wget https://downloads.rclone.org/rclone-current-osx-arm64.zip -O var/rclone-current-osx-arm64.zip; \
		cd var; \
		unzip rclone-current-osx-arm64.zip; \
		mv rclone-*/rclone ./rclone_osx-arm64; \
		rm -rf rclone-*; \
		cd -; \
	fi
	cp var/rclone_osx-arm64 dist/rclone
	makeself --tar-extra "--exclude=.gitignore" dist/ arkiv_agent-macos-arm_64.run "Arkiv agent installation script" ./autoextract.sh
	rm dist/*
	mv arkiv_agent-macos-arm_64.run dist/
	cd dist; md5sum arkiv_agent-macos-arm_64.run > arkiv_agent-macos-arm_64.run.md5
	cd dist; sha512sum arkiv_agent-macos-arm_64.run > arkiv_agent-macos-arm_64.run.sha512

distclean:
	rm -f dist/*

distallclean: distclean
	rm -rf var/rclone*

