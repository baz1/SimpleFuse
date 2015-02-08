QDOC=qdoc

all: doc

doc: src/* config.qdocconf
	$(QDOC) config.qdocconf

clean:
	rm -r doc
