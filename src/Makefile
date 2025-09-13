# Makefile for src2build

ifeq ($(notdir $(CURDIR)),src)
  $(error do not execute make in src directory.)
endif

CC = gcc2
CFLAGS = -funsigned-char -O2 -Wall -W -Werror -DUSE_DOSCALL -D__DOS_INLINE__
AS = has060
ASFLAGS = -c3 -w2
LD = $(CC)
LDFLAGS =
U8TOSJ = u8tosj


TARGET = src2build.x
ARCHIVE_ZIP = src2b.zip

SRCS = src2build.c u8tosj.s table.s
OBJS = src2build.o u8tosj.o table.o
DOCS = README.txt CHANGELOG.txt LICENSE

.PHONY: all archive clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.s
	$(AS) $(ASFLAGS) -o$@ $<


ifdef SRC_DIR
archive: $(ARCHIVE_ZIP)
else
archive: ../srcdir.txt
	$(if $(SRC_DIR_IS_DEFINED),$(error SRC_DIR is empty.))
	make $@ SRC_DIR_IS_DEFINED=yes SRC_DIR=$(shell cat ../srcdir.txt)
../srcdir.txt:
	$(error file 'srcdir.txt' created by src2build is required.)
endif

$(ARCHIVE_ZIP): $(DOCS) $(TARGET)
	rm -f $@
	strip -p $(TARGET)
	zip -9 $@ $^

%.txt: $(SRC_DIR)/../%.md
	rm -f $@
	$(U8TOSJ) < $^ > $@

LICENSE: $(SRC_DIR)/../LICENSE
	rm -f $@
	cp $< $@


clean:
	rm -f $(ARCHIVE_ZIP) $(DOCS) $(TARGET) $(OBJS)
