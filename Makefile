CC = gcc
CFLAGS = -O3 -fopenmp -march=native -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs gtk+-3.0)
SRCDIR = src
INCDIR = include
BUILDDIR = build
TARGET = $(BUILDDIR)/file_compressor

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(LIBS) -I$(INCDIR) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/file_compressor

uninstall:
	rm -f /usr/local/bin/file_compressor
