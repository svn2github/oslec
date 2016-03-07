/*
  sample.c
  David Rowe 14 December 2006

  User mode side of zaptel echo sampling system.

  Compile:
  
    gcc sample.c -o sample -Wall
*/

/*
  Copyright (C) 2007 David Rowe
 
  All rights reserved.
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2, as
  published by the Free Software Foundation.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SAMPLE_BUF_SZ 1000
#define FS            8000
#define MAX_STR       256
#define ZT_CHUNKSIZE  8

/* ioctls for sample */

#define SAMPLE_SET_CHANNEL 0
#define SAMPLE_TX_IMPULSE  1

int main(int argc, char *argv[]) {
	int    fd, len, i, j, frames;
	short  buf[3*SAMPLE_BUF_SZ];
	short  txbuf[SAMPLE_BUF_SZ];
	short  rxbuf[SAMPLE_BUF_SZ];
	short  ecbuf[SAMPLE_BUF_SZ];
	FILE  *ftx, *frx, *fec;
	float  secs;
        int    sample_ch;
        char   filename[MAX_STR];
	short  *pbuf, *ptxbuf, *prxbuf, *pecbuf;

	if (argc < 4) {
	  printf("usage: %s SampleName channel(1|2|.....) length(secs)\n"
		 " [-i(impulse mode)]", 
		 argv[0]);
	  exit(0);
	}

	sprintf(filename, "%s_tx.raw", argv[1]);
	ftx = fopen(filename,"wb");
	if (ftx == NULL) {
	  printf("Can't open tx sample file: %s\n", filename);
	  exit(1);
	}

	sprintf(filename, "%s_rx.raw", argv[1]);
	frx = fopen(filename,"wb");
	if (frx == NULL) {
	  printf("Can't open rx sample file: %s\n", filename);
	  exit(1);
	}

	sprintf(filename, "%s_ec.raw", argv[1]);
	fec = fopen(filename,"wb");
	if (frx == NULL) {
	  printf("Can't open ec sample file: %s\n", filename);
	  exit(1);
	}

	sample_ch = atoi(argv[2]);
	if (sample_ch < 1) {
	  printf("Invalid channel: %d must be > 0\n", sample_ch);
	  exit(1);
	}

	secs = atof(argv[3]);
	if ((secs < 0.0) || (secs > 100.0)) {
	  printf("Invalid secs %f, must be between 0 and 100\n", secs);
	  exit(1);
	}
	frames = (secs*FS)/SAMPLE_BUF_SZ;

	fd = open("/dev/sample", O_RDWR);
	if( fd == -1) {
		printf("open error...\n");
		exit(0);
	}
	
	ioctl(fd, SAMPLE_SET_CHANNEL, &sample_ch);

	if (argc == 5) {
	  if(!strcmp(argv[4], "-i")) {
	    printf("Impulse mode enabled\n");
	    ioctl(fd, SAMPLE_TX_IMPULSE, &sample_ch);
	  }
	}

	printf("sampling Zap/%d...\n", sample_ch);
	for(i=0; i<frames; i++) {
	  len = read(fd, buf, 3*sizeof(short)*SAMPLE_BUF_SZ);
	  if( len == -1 ) {
	    printf("read error...\n");
	    exit(1);
	  }

	  /* demultiplex and write to disk */

	  pbuf = buf;
	  ptxbuf = txbuf;
	  prxbuf = rxbuf;
	  pecbuf = ecbuf;
	  for(j=0; j<SAMPLE_BUF_SZ; j++) {
	      *ptxbuf++ = *pbuf++;
	      *prxbuf++ = *pbuf++;
	      *pecbuf++ = *pbuf++;
	  }
	  fwrite(txbuf, sizeof(short), SAMPLE_BUF_SZ, ftx);
	  fwrite(rxbuf, sizeof(short), SAMPLE_BUF_SZ, frx);
	  fwrite(ecbuf, sizeof(short), SAMPLE_BUF_SZ, fec);
	}

	close(fd);
	fclose(ftx);
	fclose(frx);
	fclose(fec);

	return 0;
}
