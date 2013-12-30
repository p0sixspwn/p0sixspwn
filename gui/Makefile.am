AM_CFLAGS =	                   \
	$(GLOBAL_CFLAGS)           \
	$(libimobiledevice_CFLAGS) \
	$(libplist_CFLAGS)         \
	$(libzip_CFLAGS)           \
	$(libcurl_CFLAGS) $(GUI_CFLAGS)

AM_LDFLAGS =\
	$(AC_LDFLAGS) $(GUI_LDFLAGS)             
#\
#	$(libimobiledevice_LIBS)   \
#	$(libplist_LIBS)           \
#	$(libzip_LIBS)             \
#	$(libcurl_LIBS) 

bin_PROGRAMS = pris0nbarake

pris0nbarake_SOURCES = \
	main.cpp \
	JailbreakMainWindow.cpp \
	JailbreakWorker.cpp \
	Jailbreaker.cpp \
	iTunesKiller.cpp \
	../src/idevicebackup2.c \
	../src/jailbreak.c \
	../src/device.c \
	../src/lockdown.c \
	../src/afc.c \
	../src/backup.c \
	../src/backup_file.c \
	../src/mbdb.c \
	../src/mbdb_record.c \
	../src/file.c \
	../src/partial.c

pris0nbarake_CFLAGS = $(AM_CFLAGS) -D__GUI__
pris0nbarake_CXXFLAGS = $(AM_CFLAGS) -I../src
pris0nbarake_LDFLAGS = $(AM_LDFLAGS) $(GLOBAL_LIBS)
pris0nbarake_LDADD = $(AM_LDADD) $(GUI_LDADD)

