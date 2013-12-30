AM_CFLAGS =	                   \
	$(GLOBAL_CFLAGS)           \
	$(libimobiledevice_CFLAGS) \
	$(libplist_CFLAGS)         \
	$(libzip_CFLAGS)           \
	$(libcurl_CFLAGS)

AM_LDFLAGS =\
	$(AC_LDFLAGS)              \
	$(libimobiledevice_LIBS)   \
	$(libplist_LIBS)           \
	$(libzip_LIBS)             \
	$(libcurl_LIBS)

bin_PROGRAMS = pris0nbarake

pris0nbarake_SOURCES = \
	idevicebackup2.c \
	jailbreak.c \
	device.c \
	lockdown.c \
	afc.c \
	backup.c \
	backup_file.c \
	mbdb.c \
	mbdb_record.c \
	file.c \
	partial.c

pris0nbarake_CFLAGS = $(AM_CFLAGS)
pris0nbarake_LDFLAGS = $(AM_LDFLAGS) $(GLOBAL_LIBS)
pris0nbarake_LDADD = $(AM_LDADD) $(GUI_LDADD)