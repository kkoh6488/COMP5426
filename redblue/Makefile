CFLAGS=-Wall -g

clean:
	rm redbluebuffered

redblue:
	mpicc redblue.c -o redblue

redbluebuffered:
	mpicc redblue_buffered.c redblueprocedure.c debuggrid.c -o redbluebuffered

debug:
	mpicc redblue.c debuggrid.c -o redblue
