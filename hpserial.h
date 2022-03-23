/*
 * hpserial.h - Communicates with an HP 49G calc
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

/*
 * Set serial port to 9600 BAUD in raw mode
 * pcSerialName has to be "0" .. "3" which equals COM1 .. COM4
 * or /dev/ttyS0 .. /dev/ttyS3
 * ATTENTION: This may change in future releases!
 * The return value is 0 when finished without errors and negative other
 */
int fSetSerial(char *pcSerialName);

/*
 * Sends bytes to switch to upload menu
 * pcSerialName has to be "0" .. "3" which equals COM1 .. COM4
 * or /dev/ttyS0 .. /dev/ttyS3
 * ATTENTION: This may change in future releases!
 * The return value is 0 when finished without errors and negative other
 */
int fSetUpload(char *pcSerialName);

/*
 * And upgrading ;-)
 * pcSerialName has to be "0" .. "3" which equals COM1 .. COM4
 * or /dev/ttyS0 .. /dev/ttyS3
 * ATTENTION: This may change in future releases!
 * The return value is 0 when finished without errors and negative other
 */
int fUpgrade(char *pcSerialName);
/* End of hpserial.h */
