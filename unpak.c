/*
 * unpak.c - Extract an flash-ROM file for the HP 49G calc
 *
 * (C) 1999 by Matthias Bunte
 *
 * rev 0.0.1: Initial testing
 * rev 0.1: first released version
 * rev 0.2: bugfix and split up
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

/* Internal function prototypes */
/* Writes one updateable part of the flash-ROM from the *.fash-file */
int fWritePartFile(FILE *);

/* Extract files from flash-file */
int funpak(char *pcFlashFileName)
{
  int icnt;
  int iChar;
  int iFilesCount;
  /* Return value */
  int iunpak;
  /* The init string of the flash file */
  char acHPROM[6] = "HPROM";
  FILE *pFFlashFile;

  /* Initialize iunpak to zero */
  iunpak = 0;
  /* Open the flash-archiv */
  if ( (pFFlashFile = fopen(pcFlashFileName, "rb")) != 0 )
  {
    /* Check the first 5 chars: They must be "HPROM" */
    for (icnt = 0; (icnt < 5) &&
      ((iChar = getc(pFFlashFile)) != EOF) &&
      ((char) iChar == acHPROM[icnt]); icnt++) { }
    /* No problems? Let's do the next steps... */
    if ( (icnt == 5) && (iChar != EOF) )
    {
      /* The next number tells us how many files are in the flash-archiv */
      iFilesCount = getc(pFFlashFile);
      /* Again, hope this was not the end of the file... */
      if (iFilesCount != EOF)
      {
        /* We have to copy the archiv-contents to the suggested files */
        for (icnt = 0; icnt < iFilesCount; icnt++)
        {
          /* Any error will be stored to iunpak */
          iunpak |= fWritePartFile(pFFlashFile);
	}
        /* Did an error occured? No? Good :-) */
        if (!iunpak)
        {
          /* The count of unpaked files will be given back */
          iunpak = iFilesCount;
	}
      }
      else
      {
        printf("An error occured: unexpected end of .flash-file (no count of files)\n");
        iunpak = -1;
      }
    }
    else
    {
      printf("An error occured: Not a .flash-file for HP 49G upgrade\n");
      iunpak = -2;
    }
    /* And close the flash-archiv */
    fclose(pFFlashFile);
  }
  else
  {
    printf("An error occured: .flash-file does not exist\n");
    iunpak = -4;
  }
  return iunpak;
}

/* Writes one updateable part of the flash-ROM from the *.fash-file */
int fWritePartFile(FILE *pFFlashFile)
{
  int icnt;
  int iChar;
  int iNameLen;
  /* Return value */
  int iWritePartFile;
  unsigned long ulcnt;
  unsigned long ulDestFileLen;
  char acDestFileName[251];
  FILE *pFDestFile;

  /* Initialize iWritePartFile */
  iWritePartFile = 0;
  /* Initialize iChar */
  iChar = 0;
  if ( (iNameLen = getc(pFFlashFile)) != EOF )
  {
    /* Read length of the archiv file-name, max 250 chars */
    for (icnt = 0; (icnt < iNameLen) && (icnt < 250) &&
      ((iChar = getc(pFFlashFile)) != EOF); 
      icnt++)
    {
      acDestFileName[icnt] = (char)iChar;
    }
    /* Last char must be a zero - we program in c ;-) */
    acDestFileName[icnt] = 0;
    if ( (icnt == iNameLen) && (iChar != EOF) )
    {
      /* Read length of file */
      if ((iChar = getc(pFFlashFile)) != EOF)
      {
        ulDestFileLen = iChar;
        if ((iChar = getc(pFFlashFile)) != EOF)
        {
          ulDestFileLen |= (unsigned long) iChar << 8ul;
          if ((iChar = getc(pFFlashFile)) != EOF)
          {
            ulDestFileLen |= (unsigned long) iChar << 16ul;
            if ((iChar = getc(pFFlashFile)) != EOF)
            {
              ulDestFileLen |= (unsigned long) iChar << 24ul;
              if ( (pFDestFile = fopen(acDestFileName, "wb")) != 0 )
              {
		/* Finally copy ulDestFileLen bytes from the flash file to
                   the part file */
                for (ulcnt = 0; (ulcnt < ulDestFileLen) && 
                  ((iChar = getc(pFFlashFile)) != EOF); ulcnt++)
                {
                  putc(iChar, pFDestFile);
		}
                fclose(pFDestFile);
                if (iChar != EOF)
                {
                  /* No error occured */
                  iWritePartFile = 0;
	        }
                else
                {
                  printf("An error occured: Cannot write to destination file\n");
                  iWritePartFile = -10;
		}
	      }
              else
              {
                printf("An error occured: Cannot write to destination file\n");
                iWritePartFile = -20;
              }
	    }
            else
            {
              printf("An error occured: unexpected end of .flash-file (4th file-len byte)\n");
              iWritePartFile = -40;
            }
	  }
          else
          {
            printf("An error occured: unexpected end of .flash-file (3rd file-len byte)\n");
            iWritePartFile = -80;
          }
        }
        else
        {
          printf("An error occured: unexpected end of .flash-file (2nd file-len byte)\n");
          iWritePartFile = -160;
        }
      }
      else
      {
        printf("An error occured: unexpected end of .flash-file (1st file-len byte)\n");
        iWritePartFile = -320;
      }
    }
    else
    {
      printf("An error occured: Cannot determine destination file name\n");
      iWritePartFile = -640;
    }
  }
  else
  {
    printf("An error occured: unexpected end of .flash-file (name-len byte)\n");
    iWritePartFile = -1280;
  }
  return iWritePartFile;
}

/* End of unpak.c */
