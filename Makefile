IDIR =include
CC=clang
CFLAGS=-I$(IDIR) -std=c99 -O3 -D_DEFAULT_SOURCE

ODIR=src/obj
SDIR=src

LIBS=-lSDL2 -lSDL2_image -lSDL2_ttf -lm -ldl

_DEPS = core.h event.h render.h comp.h tex.h logic.h display.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o core.o event.o render.o comp.o tex.o logic.o display.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

OUTPUT = logic

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
make: $(OBJ)
	$(CC) -o $(OUTPUT) $^ $(CFLAGS) $(LIBS)
		
debug: $(OBJ)
	$(CC) -g -o $(OUTPUT) $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~

-include $(OBJ:.o=.d)