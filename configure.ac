AC_INIT(gb-tools, 0.123)
AM_INIT_AUTOMAKE


AC_PROG_CC

AC_CHECK_HEADERS([zmq.h])
AC_CHECK_LIB(zmq, zmq_init)

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/gb/Makefile
])
AC_OUTPUT
