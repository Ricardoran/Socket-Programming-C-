TARGETS=client aws serverA serverB serverC

CC=g++
CCOPTS=-Wall -Wextra

.PHONY: all clean pristine

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

pristine: clean

%: %.c[[]]
	$(CC) $(CCOPTS) -o $@ $<
