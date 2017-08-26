COURSE = cs551-17s

PROJECT = prj2

INCLUDE_DIR = $(HOME)/$(COURSE)/include

CFLAGS = -g -Wall -std=gnu11
CPPFLAGS=	-I$(INCLUDE_DIR)

LIBS = -L $(HOME)/$(COURSE)/lib -lcs551

H_FILES = \
  matrix_mul.h \
  matrix_test_data.h

C_FILES = \
  main.c \
  matrix_mul.c \
  matrix_test_data.c

SRC_FILES = \
  $(C_FILES) \
  $(H_FILES) \
  Makefile \
  README

OBJS = $(C_FILES:.c=.o)
DEPENDS = $(C_FILES:.c=.depends)

TARGET = $(PROJECT)

.PHONY:		clean all

all:		$(TARGET)

$(TARGET):	$(OBJS)
		$(CC) $(OBJS) $(LIBS) -o $@

test_matrix_test_data: matrix_test_data.c matrix_test_data.h matrix_mul.h
		$(CC) $(CFLAGS) $(CPPFLAGS)  \
		   -DTEST_MATRIX_TEST_DATA $<  $(LIBS) -o $@

clean:
		rm -f *.o  *~ $(DEPENDS) $(TARGET) $(PROJECT).tar.gz

submit:
		tar -cvzf $(PROJECT).tar.gz $(SRC_FILES)

##This Perl filter is used by the .depends rule below
PERL_FILTER_INCLUDE = '$$line .= $$_; \
		       if (!/\\$$/) { \
		         @a = split(/\s+/, $$line); \
		         @b = grep { $$_ !~ "$(COURSE)" } @a; \
		         print "@b\n"; \
		         $$line = ""; \
		       } \
		       else { \
		         $$line =~ s/\\$$//; \
		       }'

#This rule creates a .depends file for the .c prerequisite.  The perl
#program removes dependencies on the course library files.
%.depends:	%.c
		@$(CC) $(CPPFLAGS) -MM $< | \
		perl -ne $(PERL_FILTER_INCLUDE) > $@

-include $(DEPENDS)
