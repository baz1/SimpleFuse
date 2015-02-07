QDOC=qdoc

all: doc

doc: src/qsimplefuse.h src/qsimplefuse.cpp config.qdocconf docstyle.css
	$(QDOC) config.qdocconf

clean:
	rm -r doc
