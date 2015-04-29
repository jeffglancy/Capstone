TARGET = capstoned
CC = gcc
CFLAGS = -I$(IDIR) -O2

ODIR = obj
IDIR = inc
SDIR = src

LIBS=-lbcm2835 -lpthread

_DEPS = capstone.h LPD8806.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = capstone.o LPD8806.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 