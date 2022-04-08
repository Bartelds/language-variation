
.SUFFIXES:

DST = ../bin

CC = gcc

OPT = -O2 -s -Wall

LEX = flex

MATHLIB = -lm

LEXLIB = -lfl

BIN =	$(DST)/bugtest \
	$(DST)/clgroup \
	$(DST)/clnewick \
	$(DST)/cluster \
	$(DST)/den \
	$(DST)/dif2tab \
	$(DST)/diffix \
	$(DST)/difinfo \
	$(DST)/difmod \
	$(DST)/difsum \
	$(DST)/features \
	$(DST)/giw \
	$(DST)/legend \
	$(DST)/leven \
	$(DST)/leven-r \
	$(DST)/leven-s \
	$(DST)/linc \
	$(DST)/ll2dst \
	$(DST)/ll2grid \
	$(DST)/locmod \
	$(DST)/mapclust \
	$(DST)/mapdiff \
	$(DST)/maplink \
	$(DST)/mapmap \
	$(DST)/maprgb \
	$(DST)/mapsetup \
	$(DST)/mapvec \
	$(DST)/mds \
	$(DST)/perfiles \
	$(DST)/sssplit \
	$(DST)/vec2tab \
	$(DST)/xstokens

all: $(BIN)

$(DST)/bugtest: bugtest.c
	$(CC) $(OPT) -o $(DST)/bugtest bugtest.c

$(DST)/clgroup: clgroup.c
	$(CC) $(OPT) -o $(DST)/clgroup clgroup.c

$(DST)/clnewick: clnewick.c
	$(CC) $(OPT) -o $(DST)/clnewick clnewick.c

$(DST)/cluster: cluster.c
	$(CC) $(OPT) -o $(DST)/cluster cluster.c $(MATHLIB)

$(DST)/den: den.c
	$(CC) $(OPT) -o $(DST)/den den.c $(MATHLIB)

$(DST)/dif2tab: dif2tab.c
	$(CC) $(OPT) -o $(DST)/dif2tab dif2tab.c

$(DST)/diffix: diffix.c
	$(CC) $(OPT) -o $(DST)/diffix diffix.c $(MATHLIB)

$(DST)/difinfo: difinfo.c
	$(CC) $(OPT) -o $(DST)/difinfo difinfo.c $(MATHLIB)

$(DST)/difmod: difmod.c
	$(CC) $(OPT) -o $(DST)/difmod difmod.c

$(DST)/difsum: difsum.c
	$(CC) $(OPT) -o $(DST)/difsum difsum.c $(MATHLIB)

$(DST)/features: features.l
	$(LEX) $(LEXOPT) features.l
	$(CC) $(OPT) -o $(DST)/features lex.yy.c $(LEXLIB) $(MATHLIB)
	rm -f lex.yy.c

$(DST)/giw: giw.c
	$(CC) $(OPT) -o $(DST)/giw giw.c $(MATHLIB)

$(DST)/legend: legend.c
	$(CC) $(OPT) -o $(DST)/legend legend.c

$(DST)/leven: leven.c
	$(CC) $(OPT) -o $(DST)/leven leven.c $(MATHLIB)

$(DST)/leven-r: leven.c
	$(CC) -DLEVEN_REAL $(OPT) -o $(DST)/leven-r leven.c $(MATHLIB)

$(DST)/leven-s: leven.c
	$(CC) -DLEVEN_SMALL $(OPT) -o $(DST)/leven-s leven.c $(MATHLIB)

$(DST)/linc: linc.c
	$(CC) $(OPT) -o $(DST)/linc linc.c $(MATHLIB)

$(DST)/ll2dst: ll2dst.c
	$(CC) $(OPT) -o $(DST)/ll2dst ll2dst.c $(MATHLIB)

$(DST)/ll2grid: ll2grid.c
	$(CC) $(OPT) -o $(DST)/ll2grid ll2grid.c $(MATHLIB)

$(DST)/locmod: locmod.c
	$(CC) $(OPT) -o $(DST)/locmod locmod.c $(MATHLIB)

$(DST)/mapclust: mapclust.c
	$(CC) $(OPT) -o $(DST)/mapclust mapclust.c $(MATHLIB)

$(DST)/mapdiff: mapdiff.c
	$(CC) $(OPT) -o $(DST)/mapdiff mapdiff.c $(MATHLIB)

$(DST)/maplink: maplink.c
	$(CC) $(OPT) -o $(DST)/maplink maplink.c $(MATHLIB)

$(DST)/mapmap: mapmap.c
	$(CC) $(OPT) -o $(DST)/mapmap mapmap.c $(MATHLIB)

$(DST)/maprgb: maprgb.c
	$(CC) $(OPT) -o $(DST)/maprgb maprgb.c $(MATHLIB)

$(DST)/mapsetup: mapsetup.c
	$(CC) $(OPT) -o $(DST)/mapsetup mapsetup.c $(MATHLIB)

$(DST)/mapvec: mapvec.c
	$(CC) $(OPT) -o $(DST)/mapvec mapvec.c $(MATHLIB)

$(DST)/mds: mds.c
	$(CC) $(OPT) -o $(DST)/mds mds.c $(MATHLIB)

$(DST)/perfiles: perfiles.c
	$(CC) $(OPT) -o $(DST)/perfiles perfiles.c

$(DST)/sssplit: sssplit.l
	$(LEX) $(LEXOPT) sssplit.l
	$(CC) $(OPT) -o $(DST)/sssplit lex.yy.c $(LEXLIB) $(MATHLIB)
	rm -f lex.yy.c

$(DST)/vec2tab: vec2tab.c
	$(CC) $(OPT) -o $(DST)/vec2tab vec2tab.c

$(DST)/xstokens: xstokens.c
	$(CC) $(OPT) -o $(DST)/xstokens xstokens.c
