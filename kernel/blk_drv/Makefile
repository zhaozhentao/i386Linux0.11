include ../../Makefile.header

CFLAGS	+= -I../../include
CPP	+= -I../../include

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	$(AS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

OBJS = ll_rw_blk.o hd.o

blk_drv.a: $(OBJS)
	$(AR) rcs blk_drv.a $(OBJS)
	sync

hd.o: hd.c

ll_rw_blk.o: ll_rw_blk.c

clean:
	rm *.a *.o

