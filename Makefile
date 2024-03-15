.PHONY: all clean arkiv-agent lib src dist

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

