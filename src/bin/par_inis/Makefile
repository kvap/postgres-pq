PGFILEDESC = "par_inis - PargreSQL message passing daemon"
subdir = src/bin/par_inis
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global
par_inis_includedir = "$(top_builddir)/src/include/par_inis"

override CPPFLAGS := -I$(par_inis_includedir) $(CPPFLAGS)

#MPICC=/share/mpi/openmpi/bin/mpicc
MPICC=mpicc
DAEMON_LIBS= -lrt
DAEMON_OBJS=	_pargresql_communicator.o _pargresql_memory_manager.o $(WIN32RES)
LIB_OBJS=""
#LIB_OBJS=	_pargresql_memory_manager.o _pargresql_library.o

all: par_inis_daemon

par_inis_daemon: $(DAEMON_OBJS)
	$(MPICC) $(CFLAGS) $(DAEMON_OBJS) $(LDFLAGS) $(DAEMON_LIBS) -o $@$(X)

_pargresql_communicator.o:
	$(MPICC) -c $(CPPFLAGS) _pargresql_communicator.c $(LDFLAGS) $(DAEMON_LIBS)

#par_inis_lib: $(LIB_OBJS)
#	$(CC) $(CFLAGS) $(LIB_OBJS) $(LDFLAGS) $(LIBS) -o $@$(X)

install: all installdirs
	$(INSTALL_PROGRAM) par_inis_daemon$(X) '$(DESTDIR)$(bindir)/par_inis_daemon$(X)'

installdirs:
	$(mkinstalldirs) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/par_inis_daemon$(X)'

clean distclean maintainer-clean:
	rm -f par_inis_daemon$(X) $(DAEMON_OBJS) $(LIB_OBJS)
