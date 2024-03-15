# Installation and configuration Makefile script
# Needs GNU Make (version >= 3.82), because of the .ONESHELL option.
# @copyright	© Amaury Bouchard
# @link		http://wiki.bash-hackers.org/scripting/terminalcodes
# @link		http://aionica.computerlink.ro/2011/08/multiple-domain-selfsigned-ssltls-certificates-for-apache-namebased-ssltls-vhosts/
# @link		https://serversforhackers.com/c/mysqldump-with-modern-mysql
# @link		https://www.gnu.org/software/make/manual/html_node/One-Shell.html

.ONESHELL:
SHELL := /bin/bash
.PHONY: y clean all

y:
	@# Ylib compilation
	pushd y > /dev/null
	make liby.a
	cp liby.a ../
	#cp liby.so ../
	cp *.h ../../include/
	popd > /dev/null

clean:
	@# cleaning
	pushd y > /dev/null
	make clean
	popd > /dev/null
	rm -f *.a *.so *~
	rm -rf ../include/*.h

all: clean y

