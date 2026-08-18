
MERLIN32DIR = ~/src//Merlin32_v1.1/MacOs

all: tcp_rdtime

# Download Merlin32.
#  https://www.brutaldeluxe.fr/products/crossdevtools/merlin/
# Change perms:
#  xattr -d com.apple.quarantine Merlin32

tcp_rdtime: tcp_rdtime.a65
	$MERLIN32DIR/Merlin32 -V . tcp_rdtime.a65

clean:
	rm tcp_rdtime

.PHONY: all clean
