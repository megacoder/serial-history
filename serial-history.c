#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#include "serial-history.h"

static char *	me = "serial-history";
static char *	ofile;
static int	nonfatal;
static int	debug;
static char *	fn = "[stdin]";		/* Input file we're processing	 */
static int	multi;

static void
vmsg(
	int		e,		/* Errno in disguise		 */
	char * const	fmt,
	va_list		ap,
	...
)
{
	fprintf( stderr, "%s: ", me );
	vfprintf( stderr, fmt, ap );
	if( e )	{
		fprintf( stderr, "; %s (errno=%d)", strerror( e ), e );
	}
	fprintf( stderr, "\n" );
}

static void
msg(
	int		e,		/* Errno in disguise		 */
	char * const	fmt,		/* Printf-style message		 */
	...				/* Arguments as required	 */
)
{
	va_list		ap;

	va_start( ap, fmt );
	vmsg( e, fmt, ap );
	va_end( ap );
}

static void
panic(
	int		e,		/* Errno in disguise		 */
	char * const	fmt,		/* Printf-style message		 */
	...				/* Arguments as required	 */
)
{
	va_list		ap;

	va_start( ap, fmt );
	vmsg( e, fmt, ap );
	va_end( ap );
	exit( e ? e : 1 );
}

static void
usage(
	char * const	fmt,
	...
)
{
	if( fmt )	{
		va_list	ap;

		va_start( ap, fmt );
		vmsg( 0, fmt, ap );
		va_end( ap );
	}
	msg(
		0,
		"usage: "
		"%s "
		"[-D] "
		"[-o ofile] "
		"\n",
		me
	);
	exit( 1 );
	/*NOTREACHED*/
}

static void
doRegisterData(
	DebugEventKind const		kind,
	DebugEventLog const		port,
	DebugEventLog const		reg,
	DebugEventLog const		data
)
{
	switch( kind )	{
	default:
		break;
	case DebugEventKind_read:
	case DebugEventKind_write:
		if( reg == 0 )	{
			char	buf[ 8 ];

			buf[ 0 ] = '\0';
			do	{
				if( isalpha( data ) 	|| 
				isdigit( data ) 	||
				ispunct( data )		||
				(data == ' ') )	{
					sprintf( buf, "%c", (int) data );
					break;
				}
				if( iscntrl( data ) )	{
					switch( data )	{
					default:
						sprintf( 
							buf, 
							"^%c", 
							(int) data + '@'
						);
						break;
					case '\t':
						sprintf( buf, "\\t" );
						break;
					case '\n':
						sprintf( buf, "\\n" );
						break;
					case '\r':
						sprintf( buf, "\\r" );
						break;
					}
					break;
				}
			} while( 0 );
			if( buf[ 0 ] )	{
				printf( " '%s'", buf );
			}
		}
		break;
	}
}

static inline int
seeable(
	DebugEventLog const	item
)
{
	return(
		isalpha( (int) item )	|
		isdigit( (int) item )	|
		ispunct( (int) item )
	);
}

static void
doLiteralEvent( 
	DebugEventKind const		kind,
	DebugEventLog const		port,
	DebugEventLog const		reg,
	DebugEventLog const		data
)
{
	char		buf[ 32 ];
	char * const	lbp = buf + sizeof( buf );
	char *		bp;

	bp = buf;
	if( seeable( port ) )	{
		bp += snprintf( bp, lbp - bp, "%c", (int) port );
	}
	if( seeable( reg ) )	{
		bp += snprintf( bp, lbp - bp, "%c", (int) reg );
	}
	if( seeable( data ) )	{
		bp += snprintf( bp, lbp - bp, "%c", (int) data );
	}
	printf( "event" );
	if( bp != buf )	{
		printf( " '%s'", buf );
	}
}

static int
process(
	void
)
{
	DebugEventPage	debugEventPage;
	size_t		n;
	int		i;

	/* Read in the debug event page (it's binary)			 */
	n = fread( 
		&debugEventPage, 
		sizeof( char ), 
		sizeof( debugEventPage ),
		stdin
	);
	if( n > TARGET_PAGE_SIZE )	{
		msg( 0, "n=%u, TARGET_PAGE_SIZE=%u", n, TARGET_PAGE_SIZE );
		n = TARGET_PAGE_SIZE;
	}
	/* Basic sanity checking					 */
	if( n > sizeof( debugEventPage ) )	{
		msg( 0, "File '%s' contains more than %u bytes",
			fn, sizeof( debugEventPage ) );
		++nonfatal;
		return( -1 );
	}
	n -= (sizeof( DebugEventCnt ) * 2);
	n /= sizeof( DebugEventLog );
	if( n <= 0 )	{
		msg( 0, "File '%s' too short to be a real file.", fn );
		++nonfatal;
		return( -1 );
	}
	if( n != debugEventPage.next )	{
		msg( 
			0, 
			"Warning! File '%s', n=%u but next=%u",
			fn, 
			(unsigned) n, 
			(unsigned) debugEventPage.next 
		);
	}
	if( multi )	{
		printf( "%s:\n", fn );
	}
	if( debug )	{
		printf( "max = %u, ", (unsigned) debugEventPage.qty );
		printf( "used = %u\n", (unsigned) debugEventPage.next );
	}
	for( i = 0; i < n; ++i )	{
		DebugEventLog const	e = debugEventPage.events[ i ];
		DebugEventLog const	kind = (e >> 30) & 0x3;
		DebugEventLog const	port = (e >> 16) & 0x3FFF;
		DebugEventLog const	reg  = (e >>  8) & 0xFF;
		DebugEventLog const	data = (e >>  0) & 0xFF;

		printf( 
			"%02X %03X %02X %02X   ",
			(unsigned) kind,
			(unsigned) port,
			(unsigned) reg,
			(unsigned) data
		);
		switch( kind )	{
		default:
			break;
		case DebugEventKind_read:
			printf( 
				"inb(%03X,%d):%02X", 
				(unsigned) port, 
				(unsigned) reg, 
				(unsigned) data
			);
			doRegisterData( kind, port, reg, data );
			break;
		case DebugEventKind_write:
			printf( 
				"out(%03X,%d):%02X", 
				(unsigned) port, 
				(unsigned) reg, 
				(unsigned) data 
			);
			doRegisterData( kind, port, reg, data );
			break;
		case DebugEventKind_ignore:
			printf( "ignore(%02X)\n", (unsigned) data );
			break;
		case DebugEventKind_event:
			doLiteralEvent( kind, port, reg, data );
			break;
		}
		printf( "\n" );
	}
	return( 0 );
}

int
main(
	int		argc,
	char * *	argv
)
{
	int		c;

	/* Parse command line						 */
	me = argv[ 0 ];
	opterr = 1;
	while( (c = getopt( argc, argv, "Do:" )) != EOF )	{
		switch( c )	{
		default:
			fprintf( stderr, "%s: no -%c switch yet!\n", me, c );
			/*FALLTHROUGH*/
		case '?':
			++nonfatal;
			break;
		case 'D':
			++debug;
			break;
		case 'o':
			ofile = optarg;
			break;
		}
	}
	if( nonfatal )	{
		usage( "illegal switch(es)" );
	}
	/* Redirect output file if we were asked to			 */
	if( ofile )	{
		(void) unlink( ofile );
		if( freopen( ofile, "wt", stdout ) != stdout )	{
			panic( errno, "cannot open '%s' for writing", ofile );
		}
	}
	/* Any command line arguments are treated as files to process	 */
	if( optind < argc )	{
		multi = ((argc - optind) > 1);
		while( optind < argc )	{

			fn = argv[ optind++ ];
			if( freopen( fn, "rt", stdin ) != stdin )	{
				msg(
					errno,
					"cannot open '%s' for reading",
					fn
				);
				continue;
			}
			if( process() )	{
				++nonfatal;
			}
			(void) fclose( stdin );
		}
	} else	{
		/* No command line arguments, so process "stdin"	 */
		if( process() )	{
			++nonfatal;
		}
	}
	exit( nonfatal ? 1 : 0 );
}
