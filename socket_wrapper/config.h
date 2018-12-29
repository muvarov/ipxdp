/* Name of package */
#define PACKAGE "socket_wrapper"

/* Version number of package */
#define VERSION "1.2.1"

/* #undef LOCALEDIR */
#define DATADIR "/usr/local/share/socket_wrapper"
#define LIBDIR "/usr/local/lib"
#define PLUGINDIR "/usr/local/lib/socket_wrapper-0"
#define SYSCONFDIR "/usr/local/etc"

/************************** HEADER FILES *************************/

/* #undef HAVE_SYS_FILIO_H */
#define HAVE_SYS_SIGNALFD_H 1
#define HAVE_SYS_EVENTFD_H 1
#define HAVE_SYS_TIMERFD_H 1
#define HAVE_GNU_LIB_NAMES_H 1
#define HAVE_RPC_RPC_H 1

/**************************** STRUCTS ****************************/

#define HAVE_STRUCT_IN_PKTINFO 1
#define HAVE_STRUCT_IN6_PKTINFO 1

/************************ STRUCT MEMBERS *************************/

/* #undef HAVE_STRUCT_SOCKADDR_SA_LEN */
#define HAVE_STRUCT_MSGHDR_MSG_CONTROL 1

/**************************** SYMBOLS ****************************/

#define HAVE_PROGRAM_INVOCATION_SHORT_NAME 1

/*************************** FUNCTIONS ***************************/

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1
#define HAVE_SIGNALFD 1
#define HAVE_EVENTFD 1
#define HAVE_TIMERFD_CREATE 1
#define HAVE_BINDRESVPORT 1
#define HAVE_ACCEPT4 1
#define HAVE_OPEN64 1
#define HAVE_FOPEN64 1
/* #undef HAVE_GETPROGNAME */
/* #undef HAVE_GETEXECNAME */
/* #undef HAVE_PLEDGE */

/* #undef HAVE_ACCEPT_PSOCKLEN_T */
/* #undef HAVE_IOCTL_INT */
#define HAVE_EVENTFD_UNSIGNED_INT 1

/*************************** LIBRARIES ***************************/

#define HAVE_GETTIMEOFDAY_TZ 1
/* #undef HAVE_GETTIMEOFDAY_TZ_VOID */

/*************************** DATA TYPES***************************/

#define SIZEOF_PID_T 4

/**************************** OPTIONS ****************************/

#define HAVE_GCC_THREAD_LOCAL_STORAGE 1
#define HAVE_CONSTRUCTOR_ATTRIBUTE 1
#define HAVE_DESTRUCTOR_ATTRIBUTE 1
/* #undef HAVE_FALLTHROUGH_ATTRIBUTE */
#define HAVE_ADDRESS_SANITIZER_ATTRIBUTE 1
#define HAVE_SOCKADDR_STORAGE 1
#define HAVE_IPV6 1
#define HAVE_FUNCTION_ATTRIBUTE_FORMAT 1

/* #undef HAVE_APPLE */
/* #undef HAVE_LIBSOCKET */

/*************************** ENDIAN *****************************/

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
/* #undef WORDS_BIGENDIAN */
