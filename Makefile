CFLAGS := -O3 -Wall

.PHONY: run clean

find_rels:

run:
	./find_rels

clean:
	rm ./find_rels
