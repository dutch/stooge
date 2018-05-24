bin_PROGRAMS = %D%/stooge
%C%_stooge_SOURCES = %D%/stooge.c
%C%_stooge_CPPFLAGS = $(LIBMICROHTTPD_CFLAGS)
%C%_stooge_LDADD = $(LIBMICROHTTPD_LIBS)
