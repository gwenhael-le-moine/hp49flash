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

#ifdef USE_GTK
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#endif

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

#ifdef USE_GTK
int port = 0;
char *filename;

/* should handle this better: close connection, etc. By now, it just kills */
gint close_application(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  return(FALSE);
}

void set_com_port(GtkWidget *widget, gpointer data)
{
  port = GPOINTER_TO_INT(data);
}

void update_filename(GtkWidget *widget, GtkWidget *entry)
{
  free(filename);
  filename = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

void file_ok_sel(GtkWidget *w, GtkFileSelection *fs)
{
  free(filename);
  filename = strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs)));
}


void update_entry (GtkWidget *widget, GtkWidget *data)
{
  gtk_entry_set_text(GTK_ENTRY(data), filename);
}

/* a popup window, with a title, a message and an OK button. Rest of processing
will be frozen until OK is pressed. */
void message_box(char *title, char *message)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *button;

  dialog = gtk_dialog_new();

  /* 'freeze' */  
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

  gtk_window_set_title (GTK_WINDOW(dialog), title);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_widget_set_usize(dialog, 0, 0);

  label = gtk_label_new(message);
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
                  FALSE, FALSE, 10);
  gtk_widget_show(label);

  button = gtk_button_new_with_label("OK");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
		  GTK_SIGNAL_FUNC(gtk_main_quit), GTK_OBJECT(dialog));
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
                  GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT (dialog));
  gtk_box_pack_end(GTK_BOX (GTK_DIALOG (dialog)->action_area), button, 
                  FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  gtk_widget_show(dialog);

  gtk_main();
  return;
}

/* data is *entry, so we can update it */
void file_dialog (GtkWidget *widget, gpointer data)
{
  GtkWidget *filew;
  
  filew = gtk_file_selection_new("File selection");

/* is this needed?
  gtk_signal_connect (GTK_OBJECT (filew), "destroy",
        	      (GtkSignalFunc) destroy, &filew); */

  /* first update filename */
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
        	      "clicked", (GtkSignalFunc) file_ok_sel, filew );
  /* now update entry */
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
        	      "clicked", (GtkSignalFunc) update_entry, widget);
  /* ok, now you can kill it */
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION
					  (filew)->ok_button),
        		     "clicked", (GtkSignalFunc) gtk_widget_destroy,
        		     GTK_OBJECT (filew));

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION
					  (filew)->cancel_button),
        		     "clicked", (GtkSignalFunc) gtk_widget_destroy,
        		     GTK_OBJECT (filew));
  
  /* Lets set the filename, as if this were a save dialog, and we are giving
   a default filename */
  if (filename)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION(filew), 
        			   filename);

  gtk_widget_show(filew);
}

/* structure to pass the data to upload_step */
struct data_pass {
     int files;
     GtkWidget *bar, *label, *label2, *buttonlabel;
     char *pcserialname;
};

gint upload_step (gpointer data) { 
  static int icnt = 0;
  char str[100];
  struct data_pass *d = data;
  
  gtk_progress_set_value(GTK_PROGRESS(d->bar),
	   gtk_progress_get_value(GTK_PROGRESS(d->bar)) + 100/(d->files+1));

  sprintf(str, "%.1f%% complete", gtk_progress_get_value(GTK_PROGRESS(d->bar)));
  gtk_label_set_text(GTK_LABEL(d->label), str); 
  while (gtk_events_pending())
     gtk_main_iteration();

  fUpgrade(d->pcserialname, GTK_LABEL(d->label2));

  icnt++;
  if (icnt >= d->files) /* done */
  {
     gtk_label_set_text(GTK_LABEL(d->label2), "Done!"); 
     gtk_label_set_text(GTK_LABEL(d->buttonlabel), "Done!");
     gtk_main_quit(); 
     return FALSE;
  }
  return TRUE;
}

/* Do the update step by step - show the user what to do */
void fStepByStep(int iFileCount, char *pcFileName, char *pcSerialName)
{
  char str[1024];
  GtkWidget *dialog;
  GtkWidget *label, *label2, *buttonlabel;
  GtkWidget *box;
  GtkWidget *separator;
  GtkWidget *button;
  GtkWidget *bar;
  GtkAdjustment *adj;
  struct data_pass data;

  sprintf(str, "\nFlash update: file %s at /dev/ttyS%s\n"
		"Connect the HP 49G with /dev/ttyS%s,\n"
		"press \"ON\" and \"D\" to get the tests-menu,\n"
		"press \"+\" and \"ENTER\", hold them and press \"ON\" to get the no-system-menu,\n"
		"press \"4\" to enter terminal mode,\n"
		"now click OK on the computer to continue this program...",
		pcFileName, pcSerialName, pcSerialName);
  message_box("Connect", str);
  if (fSetUpload(pcSerialName))
      return;
  message_box("Connect", "Now press \"1\" to start upgrading the system,\n"
		"when the calc shows \"Attempting to download file\"\n"
		"click OK on the computer...");

  /* Download all system files, and show a nifty progress bar */
  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Uploading...");
  gtk_container_set_border_width(GTK_CONTAINER (dialog), 5);

  adj = (GtkAdjustment *)gtk_adjustment_new(0, 0, 100, 0, 0, 0);
  bar = gtk_progress_bar_new_with_adjustment(adj);
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->vbox), bar,
                  FALSE, FALSE, 10);
  gtk_widget_show(bar);

  /* show % complete */
  label = gtk_label_new("???% complete");
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
                  FALSE, FALSE, 10);
  gtk_widget_show(label);
  
  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->vbox), separator,
		  FALSE, FALSE, 0);
  gtk_widget_show(separator);

  /* show status */
  box = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(box), 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG (dialog)->vbox), box, TRUE, TRUE, 0);
  gtk_widget_show(box);
  label2 = gtk_label_new("Status: ");
  gtk_label_set_justify(GTK_LABEL(label2), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(box), label2, FALSE, FALSE, 0);
  gtk_widget_show(label2);
  label2 = gtk_label_new("starting...");
  gtk_label_set_justify(GTK_LABEL(label2), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(box), label2, FALSE, FALSE, 0);
  gtk_widget_show(label2);
  
  /* this will KILL the app. that's not the way to go, but, by now... */
  buttonlabel = gtk_label_new("Cancel");
  button = gtk_button_new();
  gtk_widget_show(buttonlabel);
  gtk_container_add(GTK_CONTAINER(button), buttonlabel);
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_main_quit,
                             GTK_OBJECT (dialog));
  gtk_signal_connect_object (GTK_OBJECT (button),
        		     "clicked", (GtkSignalFunc) gtk_widget_destroy,
        		     GTK_OBJECT (dialog));
  gtk_box_pack_end(GTK_BOX (GTK_DIALOG (dialog)->action_area), button, 
                  FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  
  data.files = iFileCount;
  data.bar = bar;
  data.label = label;
  data.label2 = label2;
  data.buttonlabel = buttonlabel;
  data.pcserialname = pcSerialName;

  gtk_timeout_add( 50, (GtkFunction) upload_step, (gpointer) &data ); 
  
  gtk_widget_show(dialog);
  gtk_main();
}

/* this is the function responsible for the settings and upload */
void start (GtkWidget *widget, gpointer data)
{
  int iFileCount, a;
  char portName[2];
  struct stat st;

  if (!filename || filename[0] == '\0' )
  {
    message_box("No filename", "No filename given!\nYou must provide one." );
    return;
  }
  else
  {
    /* test file access */  
    a = access( filename, R_OK | F_OK );
    stat(filename, &st);
    if (a != 0)
    {
      switch (errno)
      {
        case ENOENT:
          message_box("File not found", "File not found");
          return;
        case EACCES:
          message_box("Forbidden", "You cannot access this file.\nTrying to hack, eh?");
          return;
      }
    }
    else if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
      message_box("Not a file", "Not a valid file");
      return;
    }
  }
  
  /* this sprintf is a quick and ugly fix. I don't want to mess with the io functions
  by now... :) */
  sprintf( portName, "%d", port );
  if (!fSetSerial(portName))
  {
    if ( (iFileCount = funpak(filename)) > 0 )
      fStepByStep(iFileCount, filename, portName);
  }
  else
    message_box("Port error", "Serial port has an error.");
  
  /* clean */
  system("rm -f Part* System");

  gtk_main_quit();
}

/* process arguments */
void args(int argc, char *argv[])
{
  int i;
  for (i = 1; i < argc; i++ ) {
    if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0 )
    {
    	port = atoi(argv[++i]);
        if (port < 1 || port > 4)
          port = 1;
    }
    else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filename") == 0 )
    {
    	filename = strdup(argv[++i]);
    }

    else switch (argv[i][1])
      {
      case 'p': 
        if (filename != NULL) 
        {
          funpak(argv[1]); 
        }
        else 
        {
          display_help();          exit(1);
        }
        break;
      case 's': 
        if (filename != NULL) 
        {     
          fSetSerial(argv[2]);
        }
        else 
        {
          display_help();          exit(1);
        }
	break;
      case 'd':
        if (filename != NULL) 
        {
          fSetUpload(argv[2]);
        }
        else 
        {
          display_help();          exit(1);
        }
	break;
      case 'u': 
        if (filename != NULL) 
        {
          fUpgrade(argv[2], NULL);
        }
        else 
        {
          display_help();          exit(1);
        }
	break;
      default:
          message_box("Wrong parameter\n", 
            "One or the arguments you passed is not valid");
          exit(1);
      }
  }
}

int main(int argc, char *argv[])
{
  GtkWidget *window = NULL;
  GtkWidget *box1, *box2, *box3;
  GtkWidget *button;
  GtkWidget *separator;
  GtkWidget *label;
  GtkWidget *entry;

  /* init GTK, create window, etc */
  gtk_init(&argc,&argv);
  args(argc, argv);
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
		        GTK_SIGNAL_FUNC(close_application),
                        NULL);
  
  gtk_window_set_title(GTK_WINDOW (window), "Program Name");
  gtk_container_set_border_width(GTK_CONTAINER (window), 5);

  /* here's our ad */
  box1 = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER (window), box1);
  gtk_widget_show(box1);
  label = gtk_label_new(
                "HPFLASH v0.2.2, Copyright (C) 1999 Matthias Bunte <MBunte@gmx.net>\n"
                "GUI by Bruno Barberi Gnecco <brunobg@geocities.com>\n"
		"HPFLASH comes with ABSOLUTELY NO WARRANTY; for details see `GPL'.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; see `GPL' for details.\n");
  gtk_box_pack_start(GTK_BOX (box1), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start(GTK_BOX (box1), separator, FALSE, TRUE, 0);
  gtk_widget_show(separator);

  box2 = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER (box2), 10);
  gtk_box_pack_start(GTK_BOX (box1), box2, TRUE, TRUE, 0);
  gtk_widget_show(box2);

  /* Radio buttons to set serial port */
  label = gtk_label_new("Set COM port:");
  gtk_box_pack_start(GTK_BOX (box2), label, TRUE, FALSE, 0);
  gtk_widget_show(label);

  box3 = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER (box3), 10);
  gtk_box_pack_start(GTK_BOX (box2), box3, TRUE, TRUE, 0);
  gtk_widget_show(box3);

  button = gtk_radio_button_new_with_label(NULL, "COM 1");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                GTK_SIGNAL_FUNC (set_com_port), GINT_TO_POINTER(1) );
  gtk_box_pack_start(GTK_BOX (box3), button, TRUE, TRUE, 0);
  if (port == 1 )
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show(button);
  button = gtk_radio_button_new_with_label(
		gtk_radio_button_group (GTK_RADIO_BUTTON (button)), "COM 2");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                GTK_SIGNAL_FUNC (set_com_port), GINT_TO_POINTER(2) );
  gtk_box_pack_start(GTK_BOX (box3), button, TRUE, TRUE, 0);
  if (port == 2 ) 
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show(button);
  button = gtk_radio_button_new_with_label(
		gtk_radio_button_group (GTK_RADIO_BUTTON (button)), "COM 3");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                GTK_SIGNAL_FUNC (set_com_port), GINT_TO_POINTER(3) );
  gtk_box_pack_start(GTK_BOX (box3), button, TRUE, TRUE, 0);
  if (port == 3 )
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show(button);
  button = gtk_radio_button_new_with_label(
		gtk_radio_button_group (GTK_RADIO_BUTTON (button)), "COM 4");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                GTK_SIGNAL_FUNC (set_com_port), GINT_TO_POINTER(4) );
  gtk_box_pack_start(GTK_BOX (box3), button, TRUE, TRUE, 0);
  if (port == 4 )
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show(button);
  
  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 0);
  gtk_widget_show(separator);
  
  /* ask filename. */
  box2 = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER (box2), 10);
  gtk_box_pack_start(GTK_BOX (box1), box2, TRUE, TRUE, 0);
  gtk_widget_show(box2);
  label = gtk_label_new("Filename:");
  gtk_box_pack_start(GTK_BOX (box2), label, FALSE, FALSE, 10);
  gtk_widget_show(label);
  
  entry = gtk_entry_new_with_max_length(80);
  if (filename) 
  {
    gtk_entry_set_text (GTK_ENTRY (entry), filename); 
    gtk_entry_select_region (GTK_ENTRY (entry),
			   0, GTK_ENTRY(entry)->text_length);
  }
  else
  {
    filename = (char *)malloc(100);
    getcwd(filename, 100);
    strcat(filename, "/");
    gtk_entry_set_text (GTK_ENTRY (entry), filename); 
    gtk_entry_select_region (GTK_ENTRY (entry),
			   0, GTK_ENTRY(entry)->text_length);
  }
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
                  GTK_SIGNAL_FUNC(update_filename), entry);
  gtk_box_pack_start (GTK_BOX (box2), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  /* file selector */
  button = gtk_button_new_with_label("Choose");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			     GTK_SIGNAL_FUNC(file_dialog), (gpointer)entry );
  gtk_box_pack_start(GTK_BOX (box2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  separator = gtk_hseparator_new();
  gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);
  gtk_widget_show (separator);

  /* start & quit buttons */
  box2 = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER (box2), 10);
  gtk_box_pack_start(GTK_BOX (box1), box2, TRUE, TRUE, 0);
  gtk_widget_show(box2);

  button = gtk_button_new_with_label("Start");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			     GTK_SIGNAL_FUNC(start), NULL );
  gtk_box_pack_start(GTK_BOX (box2), button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT); 
  gtk_widget_grab_default(button);
  gtk_widget_show(button);
  button = gtk_button_new_with_label("Quit");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			     GTK_SIGNAL_FUNC(gtk_main_quit), NULL );
  gtk_box_pack_start(GTK_BOX (box2), button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  gtk_widget_show(window);

  gtk_main();
  return 0;
}

#else /* do_not_use_Gtk!:) */

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
#endif /* USE_GTK */
