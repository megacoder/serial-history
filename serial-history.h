#ifndef	serial_history_h
#define	serial_history_h

#include <linux/types.h>

#define	TARGET_PAGE_SIZE	4086

/*
 *------------------------------------------------------------------------
 * addDebugEvent: append an event to the debug log
 *------------------------------------------------------------------------
 */

typedef enum	{
	DebugEventKind_read	= 0,
	DebugEventKind_write	= 1,
	DebugEventKind_ignore	= 2,
	DebugEventKind_event	= 3
} DebugEventKind;

typedef __u16	DebugEventCnt;
typedef __u32	DebugEventLog;

typedef struct	{
	DebugEventCnt	qty;
	DebugEventCnt	next;
	DebugEventLog	events[ (TARGET_PAGE_SIZE-(sizeof(DebugEventCnt)*2)) / 
				sizeof( DebugEventLog ) ];
} DebugEventPage;

#endif	/* serial_history_h */
