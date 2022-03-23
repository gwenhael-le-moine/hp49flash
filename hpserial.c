/*
 * hpserial.c - Communicates with an HP 49G calc
 *
 * (C) 1999 by Matthias Bunte
 *
 * rev 0.0.1: Initial testing
 * rev 0.1: first released version
 * rev 0.2: bugfix and split up
 * rev 0.2.2: BBG: added GUI support
 * rev 0.2.3: BBG: labels are now updated
 */

/*
    Copyright note:

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hpserial.h"

/* Set serial port to 9600 raw
   Should be done with ioctl's... */
int fSetSerial(char *pcSerialName)
{
  char acSttyCmd[] =
    "stty 1:0:8bd:0:3:1c:7f:15:4:5:1:0:11:13:1a:0:12:f:17:16:0:0:73:0:0:0:0:0:0:0:0:0:0:0:0:0 < /dev/ttyUSB1 > /dev/ttyUSB1";

  /* Set the serial port - uh uh - dirty isnt it? */
  /* acSttyCmd[100] = pcSerialName[0]; */
  /* acSttyCmd[113] = pcSerialName[0]; */
  return system(acSttyCmd);
}

/* Sends bytes to switch to upload menu */
int fSetUpload(char *pcSerialName)
{
  int iSetUpload;
  FILE *fpSerialPort;
  char acDevFile[] = "/dev/ttyUSB1";

  /* Set the serial port - uh uh - dirty isnt it? */
  /* acDevFile[9] = pcSerialName[0]; */
  /* Open serial port */
  if ((fpSerialPort = fopen(acDevFile, "rb+")) != 0)
  {
    /* Write upload menu sequence */
    fputc((char) 0x02, fpSerialPort);
    fputc((char) 0xb8, fpSerialPort);
    fputc((char) 0x03, fpSerialPort);
    fputc((char) 0x00, fpSerialPort);
    fclose(fpSerialPort);
    iSetUpload = 0;
  }
  else
  {
    printf("An error occured: Could not open serial device file!\n");
    iSetUpload = -1;
  }
  return iSetUpload;
}

/* And upgrading ;-) */
int fUpgrade(char *pcSerialName)
{
  int icnt;
  int iNameLen;
  int iUpgrade;
  FILE *fpSerialPort;
  FILE *fpDummy;
  char acDevFile[] = "/dev/ttyUSB1";

  iUpgrade = 0;
  /* Set the serial port - uh uh - dirty isnt it? */
  /* acDevFile[9] = pcSerialName[0]; */
  /* Open serial port */
  if ( (fpSerialPort = fopen(acDevFile, "rb+")) != 0)
  {
    /* Wait for G + 00 */
    while ( ( (fgetc(fpSerialPort) != 0x47) || (fgetc(fpSerialPort) != 0x00) ) )
    {
    }
    printf("\nGot G -> ");

    /* Get length of filename */
    iNameLen = fgetc(fpSerialPort);
    printf("got NameLen %i -> ", iNameLen);
    {
      char acName[iNameLen + 40];

      /* We use the sx program to upload */
      strncpy(acName, "sx ", 4);
      /* sx needs the filename to upload so read it from the calc */
      for (icnt = 0; (acName[icnt+3] = (char)fgetc(fpSerialPort)) &&
        (icnt < iNameLen); icnt++) {}
      acName[icnt+3] = 0;

      printf("got filename %s -> ", acName + 3);

      fflush(fpSerialPort);
      printf("Port flushed -> ");
      if ( (fpDummy = fopen(acName + 3, "rb")) != 0 )
      {
        fputc((char) 6, fpSerialPort);
        printf("acknowledge written -> ");
        fclose(fpDummy);
        fclose(fpSerialPort);
        printf("port closed\n");
        strncpy(acName + 3 + icnt, " < /dev/ttyUSB1 > /dev/ttyUSB1", 31);
        /* acName[15 + icnt] = pcSerialName[0]; */
        /* acName[28 + icnt] = pcSerialName[0]; */
        printf("%s\n", acName);
        system(acName);
      }
      else
      {
        fputc((char) 0, fpSerialPort);
        printf("\nAn error occured: File not found!\n");
        fclose(fpSerialPort);
        printf("port closed\n");
        iUpgrade = -1;
      }
    }
  }
  else
  {
    printf("\nAn error occured: Could not open serial device file!\n");
    iUpgrade = -2;
  }
  return iUpgrade;
}

/* End of hpserial.c */
