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
    "stty 1:0:8bd:0:3:1c:7f:15:4:5:1:0:11:13:1a:0:12:f:17:16:0:0:73:0:0:0:0:0:0:0:0:0:0:0:0:0 < /dev/ttyS? > /dev/ttyS?";

  /* Set the serial port - uh uh - dirty isnt it? */
  acSttyCmd[100] = pcSerialName[0];
  acSttyCmd[113] = pcSerialName[0];
  return system(acSttyCmd);
}

/* Sends bytes to switch to upload menu */
int fSetUpload(char *pcSerialName)
{
  int iSetUpload;
  FILE *fpSerialPort;
  char acDevFile[] = "/dev/ttyS?";

  /* Set the serial port - uh uh - dirty isnt it? */
  acDevFile[9] = pcSerialName[0];
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
#ifdef USE_GTK
    message_box("Error", "Could not open serial device");
#else
    printf("An error occured: Could not open serial device file!\n");
#endif
    iSetUpload = -1;
  }
  return iSetUpload;
}

/* And upgrading ;-) */
#ifdef USE_GTK
int fUpgrade(char *pcSerialName, GtkLabel *label)
#else
int fUpgrade(char *pcSerialName)
#endif
{
  int icnt;
  int iNameLen;
  int iUpgrade;
  FILE *fpSerialPort;
  FILE *fpDummy;
  char acDevFile[] = "/dev/ttyS?";
#ifdef USE_GTK
  char str[100];
#endif

  iUpgrade = 0;
  /* Set the serial port - uh uh - dirty isnt it? */
  acDevFile[9] = pcSerialName[0];
  /* Open serial port */
  if ( (fpSerialPort = fopen(acDevFile, "rb+")) != 0)
  {
    /* Wait for G + 00 */
    while ( ( (fgetc(fpSerialPort) != 0x47) || (fgetc(fpSerialPort) != 0x00) ) )
    {
    }
#ifdef USE_GTK
    gtk_label_set_text(label, "Got G" );
    while (gtk_events_pending())
      gtk_main_iteration();
#else
    printf("\nGot G -> ");
#endif
    /* Get length of filename */
    iNameLen = fgetc(fpSerialPort);
#ifdef USE_GTK
    gtk_label_set_text(label, "Got Namelen" );
    while (gtk_events_pending())
      gtk_main_iteration();
#else
    printf("got NameLen %i -> ", iNameLen);
#endif
    {
      char acName[iNameLen + 40];

      /* We use the sx program to upload */
      strncpy(acName, "sx ", 4);
      /* sx needs the filename to upload so read it from the calc */
      for (icnt = 0; (acName[icnt+3] = (char)fgetc(fpSerialPort)) && 
        (icnt < iNameLen); icnt++) {}
      acName[icnt+3] = 0;
#ifdef USE_GTK
      sprintf(str, "got filename %s", acName + 3);
      gtk_label_set_text(label, str );
      while (gtk_events_pending())
        gtk_main_iteration();
#else
      printf("got filename %s -> ", acName + 3);
#endif
      fflush(fpSerialPort);
#ifdef USE_GTK
      gtk_label_set_text(label, "Port flushed");
      while (gtk_events_pending())
        gtk_main_iteration();
#else
      printf("Port flushed -> ");
#endif
      if ( (fpDummy = fopen(acName + 3, "rb")) != 0 )
      {
        fputc((char) 6, fpSerialPort);
#ifdef USE_GTK
        gtk_label_set_text(label, "Acknowledge written");
        while (gtk_events_pending())
          gtk_main_iteration();
#else
        printf("acknowledge written -> ");
#endif
        fclose(fpDummy);
        fclose(fpSerialPort);
#ifdef USE_GTK
	gtk_label_set_text(label, "port closed\n");
        while (gtk_events_pending())
          gtk_main_iteration();
#else
        printf("port closed\n");
#endif
        strncpy(acName + 3 + icnt, " < /dev/ttyS? > /dev/ttyS?", 28);
        acName[15 + icnt] = pcSerialName[0];
        acName[28 + icnt] = pcSerialName[0];
        printf("%s\n", acName);
#ifdef FUTURE_USE_GTK
	comm = popen( acName, r );
	fscanf( comm, "%s", &string );
	/* process, etc*/
#else
        system(acName);
#endif
      }
      else
      {
        fputc((char) 0, fpSerialPort);
#ifdef USE_GTK
        message_box("Error", "File not found");
#else
        printf("\nAn error occured: File not found!\n");
#endif
        fclose(fpSerialPort);
#ifdef USE_GTK
        gtk_label_set_text(label, "Port closed");
        while (gtk_events_pending())
          gtk_main_iteration();
#else
        printf("port closed\n");
#endif
        iUpgrade = -1;
      }
    }
  }
  else
  {
#ifdef USE_GTK
    message_box("Error", "Could not open serial device");
#else
    printf("\nAn error occured: Could not open serial device file!\n");
#endif
    iUpgrade = -2;
  }
  return iUpgrade;
}

/* End of hpserial.c */
