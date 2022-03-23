/*
 *
 upgrade.c - Extract an flash-ROM file for the HP 49G calc and
 *             transfer it via serial port
 *
 * (C) 1999 by Matthias Bunte
 *
 * rev 0.0.1: Initial testing
 * rev 0.1: first released version
 * rev 0.2: bugfix and split up
 * rev 0.2.1: added GTK GUI - by Bruno Barberi Gnecco
 * rev 0.2.2: BBG: fixed bug that prevented opening the .flash
 *            BBG: corrected Progressbar.
 *            BBG: now extracted files are cleaned (a better way to go would be
 *                   create a temp directory, do stuff there and erase it)
 * rev 0.2.3: BBG: updates are now shown in screen.
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

#include "unpak.h"
#include "hpserial.h"

void display_help (void) {
    printf("Usage:\n");
    printf("  upgrade \"name of flash file\" \"serial device number\"\n\n");
    printf("  upgrade \"name of flash file\" \"serial device number\" [ -p | -s | -d | -u ]\n\n");
    printf("The serial device number is \"0\" for ttyS0 aka COM1, \"1\" for ttyS1 aka COM2 etc.\n");
    printf("Use first type for interactive upgrade. Follow instructions given.\n\n");
    printf("Use second type for controling upgrade process\n");
    printf("e.g. when another program calls upgrade.\n\n");
    printf("  -p  unpaks the flash file\n");
    printf("  -s  sets the mode of the serial device\n");
    printf("  -d  switchs calc to download menu when in terminal mode\n");
    printf("  -u  waits for an upload request by the calc and\n");
    printf("      loads the requested file up by the sx-xmodem program.\n");
    printf("\n(C) 1999 by M. Bunte  This program is licenced under the GNU public licence.\n");
}

/* Do the update step by step - show the user what to do */
void fStepByStep(int iFileCount, char *pcFileName, char *pcSerialName)
{
  char cDummy;
  int icnt;

  printf("\nFlash update: file %s at /dev/ttyS%s\n", pcFileName, pcSerialName);
  printf("Connect the HP 49G with /dev/ttyS%s,\n", pcSerialName);
  printf("press \"ON\" and \"D\" to get the tests-menu,\n");
  printf("press \"+\" and \"ENTER\", hold them and press \"ON\" to get the no-system-menu,\n");
  printf("press \"4\" to enter terminal mode,\n");
  printf("now press \"RETURN\" on the computer to continue this program...\n");
  scanf("%c", &cDummy);
  fSetUpload(pcSerialName);
  printf("\nNow press \"1\" to start upgrading the system,\n");
  printf("when the calc shows \"Prompting for file\" \n");
  printf("  press \"RETURN\" on the computer...\n");
  scanf("%c", &cDummy);
  /* Download all system files */
  for ( icnt = 0; icnt < iFileCount; icnt++ )
  {
    fUpgrade(pcSerialName);
  }
}

int main(int argc, char *argv[])
{
  int imain;
  int iFileCount;

  imain = 0;

  printf("upgrade v0.2.2, Copyright (C) 1999 Matthias Bunte <MBunte@gmx.net>\n");
  printf("upgrade comes with ABSOLUTELY NO WARRANTY; for details see `GPL'.\n");
  printf("This is free software, and you are welcome to redistribute it\n");
  printf("under certain conditions; see `GPL' for details.\n\n");

  /* How many arguments? Two? */
  if (argc == 3)
  {
    /* Is the serial port number valid? */
    if ((argv[2][0] >= 0x30) && (argv[2][0] < 0x34) && (argv[2][1] == 0))
    {
      /* Is the serial port accessible? */
      if (!fSetSerial(argv[2]))
      {
        /* After error-free unpaking the flash file start the
           step-by-step upgrade */
        if ( (iFileCount = funpak(argv[1])) > 0 )
        {
          fStepByStep(iFileCount, argv[1], argv[2]);
        }
        else
        {
          printf("An error occurred: Could not unpak correctly.\n");
          imain = -1;
  }
      }
      else
      {
        printf("Serial port number %s has an error.\n", argv[2]);
        imain = -2;
      }
    }
    else
    {
      printf("Serial port number %s has to be 0 ... 3.\n", argv[2]);
      imain = -4;
    }
  }
  else if ((argc == 4) && (argv[3][0] == '-'))
  {
    if ((argv[2][0] >= 0x30) && (argv[2][0] < 0x34) && (argv[2][1] == 0))
    {
      switch (argv[3][1])
      {
      case 'p': imain = funpak(argv[1]); break;
      case 's': imain = fSetSerial(argv[2]); break;
      case 'd': imain = fSetUpload(argv[2]); break;
      case 'u': imain = fUpgrade(argv[2]); break;
      default: imain = -1; printf("Wrong parameter\n");
      }
    }
    else
    {
      printf("Serial port number %s has to be 0 ... 3.\n", argv[2]);
      imain = -1;
    }
  }
  else
  {
    display_help();
    imain = 0;
  }

  /* clean */
  system("rm -f Part* System");

  return imain;
}
