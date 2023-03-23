include ../../Makefile.header

CFLAGS	+= -I../../include

.c.s:
	@$(CC) $(CFLAGS) -S -o $*.s $<
.s.o:
	@$(AS) -o $*.o $<
.c.o:
	@$(CC) $(CFLAGS) -c -o $*.o $<

OBJS  = ll_rw_blk.o floppy.o hd.o

blk_drv.a: $(OBJS)
	$(AR) rcs blk_drv.a $(OBJS)
	sync

ll_rw_blk.s ll_rw_blk.o: ll_rw_blk.c

clean:
	rm -f *.a *.o

