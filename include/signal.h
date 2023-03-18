#ifndef _SIGNAL_H
#define _SIGNAL_H

typedef unsigned int sigset_t;		/* 32 bits */

struct sigaction {
	void (*sa_handler)(int);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};

#endif /* _SIGNAL_H */

