/*
 * Gyatzee: Gnomified Yahtzee game.
 * (C) 1998 the Free Software Foundation
 *
 * File:   setup.c
 *
 * Author: Scott Heavner
 *
 *   Contains parser for command line arguments and 
 *   code for properties window.
 *
 *   Variables are exported in gyahtzee.h
 *
 *   This file is largely based upon GTT code by Eckehard Berns.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <config.h>
#include <gnome.h>
#include <gconf/gconf-client.h>

#include "yahtzee.h"
#include "gyahtzee.h"

static gint setupdialog_destroy(GtkWidget *widget, gint mode);
static GtkWidget *setupdialog = NULL;
static GtkWidget *HumanSpinner, *ComputerSpinner, *BonusEntry;
static GtkWidget *PlayerNames[MAX_NUMBER_OF_PLAYERS];
static GtkObject *HumanAdj, *ComputerAdj;

static int OriginalNumberOfComputers = -1;
static int OriginalNumberOfHumans    = -1;

static int tmpExtraYahtzeeBonus, tmpExtraYahtzeeJoker, tmpDoDelay, tmpDisplayComputerThoughts;

extern GtkWidget *window;

static void
parse_an_arg (poptContext ctx,
	      enum poptCallbackReason reason,
	      const struct poptOption *opt,
	      const char *arg, void *data)
{

	switch (opt->shortName)
	{
	case 'r':
                calc_random();
                GyahtzeeAbort = 1;
		break;
	default:
		break;
	}

	return;
}

static struct poptOption cb_options[] = {
  {NULL, '\0', POPT_ARG_CALLBACK, &parse_an_arg, 0},
  {NULL, 'r', POPT_ARG_NONE, &GyahtzeeAbort, 0, N_("Calculate random die throws (debug)"), NULL},
  {NULL, '\0', 0, NULL, 0}
};

const struct poptOption yahtzee_options[] = {
  {NULL, '\0', POPT_ARG_INCLUDE_TABLE, cb_options, 0, NULL, NULL},
  {NULL, 'd', POPT_ARG_NONE, &DoDelay, 0, N_("Delay computer moves"), NULL},
  {NULL, 's', POPT_ARG_NONE, &OnlyShowScores, 0, N_("Show high scores and exit"), NULL},
  {NULL, 't', POPT_ARG_NONE, &DisplayComputerThoughts, 0, N_("Display computer thoughts"), NULL},
  {NULL, 'n', POPT_ARG_INT, &NumberOfComputers, 0, N_("Number of computer opponents"), N_("NUMBER")},
  {NULL, 'p', POPT_ARG_INT, &NumberOfHumans, 0, N_("Number of human opponents"), N_("NUMBER")},
  {NULL, '\0', 0, NULL, 0}
};

static void
WarnNumPlayersChanged (void)
{
        GtkWidget *mb;

        mb = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    _("Current game will complete" 
                                      " with original number of players."));
        gtk_window_set_position(GTK_WINDOW(mb), GTK_WIN_POS_MOUSE);
        gtk_dialog_set_has_separator (GTK_DIALOG (mb), FALSE);
        gtk_dialog_run(GTK_DIALOG(mb));
        gtk_widget_destroy(mb);
}


static void
do_setup(GtkWidget *widget, gpointer data)
{
        GConfClient *client;
        GError *err = NULL;
        GSList *name_list = NULL;
        gchar *PrefLoc;
        int i;

        NumberOfComputers = 
                gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ComputerSpinner));
        NumberOfHumans = 
                gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(HumanSpinner));

        DoDelay = tmpDoDelay;
        DisplayComputerThoughts = tmpDisplayComputerThoughts;
        ExtraYahtzeeBonus = tmpExtraYahtzeeBonus;
        ExtraYahtzeeJoker = tmpExtraYahtzeeJoker;

        for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
                if (players[i].name != DefaultPlayerNames[i])
                        g_free(players[i].name);
                players[i].name = g_strdup(gtk_entry_get_text(GTK_ENTRY(PlayerNames[i])));

                if (i < NumberOfPlayers)
                        score_list_set_column_title(ScoreList, i+1,
                                                    players[i].name);
                name_list = g_slist_append(name_list, players[i].name);
        }

        if (sscanf(gtk_entry_get_text(GTK_ENTRY(BonusEntry)),"%d",&i)==1) {
                ExtraYahtzeeBonusVal = i;
        } 
                
	setupdialog_destroy(setupdialog, 1);

        client = gconf_client_get_default ();
        gconf_client_set_list (client, "/apps/gtali/PlayerNames",
                               GCONF_VALUE_STRING, name_list, &err);
        g_slist_free(name_list);
        if(err) {
                g_warning (G_STRLOC ": gconf error: %s\n", err->message);
                g_error_free(err);
                err = NULL;
        }

        gconf_client_set_int(client, "/apps/gtali/NumberOfComputerOpponents",
                             NumberOfComputers, &err);
        if(err) {
                g_warning (G_STRLOC ": gconf error: %s\n", err->message);
                g_error_free(err);
                err = NULL;
        }

        gconf_client_set_int(client, "/apps/gtali/NumberOfHumanOpponents",
                             NumberOfHumans, &err);
        if(err) {
                g_warning (G_STRLOC ": gconf error: %s\n", err->message);
                g_error_free(err);
                err = NULL;
        }

        gconf_client_set_int(client, "/apps/gtali/ExtraYahtzeeBonus",
                             ExtraYahtzeeBonus, &err);
        if(err) {
                g_warning (G_STRLOC ": gconf error: %s\n", err->message);
                g_error_free(err);
                err = NULL;
        }

        gconf_client_set_int(client, "/apps/gtali/ExtraYahtzeeBonusVal",
                             ExtraYahtzeeBonusVal, &err);
        if(err) {
                g_warning (G_STRLOC ": gconf error: %s\n", err->message);
                g_error_free(err);
                err = NULL;
        }

 	gconf_client_set_int(client, "/apps/gtali/ExtraYahtzeeJoker",
                             ExtraYahtzeeJoker, &err);
        if(err) {
                g_warning (G_STRLOC ": gconf error: %s\n", err->message);
                g_error_free(err);
                err = NULL;
        }

        if ( ( (NumberOfComputers!=OriginalNumberOfComputers)||
	       (NumberOfHumans!=OriginalNumberOfHumans) ) &&
	     !GameIsOver() )
                WarnNumPlayersChanged();
}

static gint
setupdialog_destroy(GtkWidget *widget, gint mode)
{
	if (mode == 1) {
		gtk_widget_destroy(setupdialog);
	}
	setupdialog = NULL;

	return FALSE;
}


static gint
set_as_int (GtkWidget *widget, gpointer *data)
{
        *((int *)data) = GTK_TOGGLE_BUTTON (widget)->active;
        if (data==(gpointer)&tmpExtraYahtzeeBonus) {
                gtk_entry_set_editable(GTK_ENTRY(BonusEntry),
                                       GTK_TOGGLE_BUTTON(widget)->active);
        }
        return FALSE;
}

static gint
MaxPlayersCheck (GtkWidget *widget, gpointer *data)
{
        int numc, numh;

        numc = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ComputerSpinner));
        numh = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(HumanSpinner));

        if ( (numc+numh) > MAX_NUMBER_OF_PLAYERS) {
                if (GTK_ADJUSTMENT(data)==GTK_ADJUSTMENT(HumanAdj)) {
                        gtk_adjustment_set_value(GTK_ADJUSTMENT(ComputerAdj),
                                                 (gfloat)(numc-1));
                } else {
                        gtk_adjustment_set_value(GTK_ADJUSTMENT(HumanAdj),
                                                 (gfloat)(numh-1));
                }
                        
        }

	return FALSE;
}

gint 
setup_game(GtkWidget *widget, gpointer data)
{
        GtkWidget *all_boxes, *box, *box2, *label, *button, *frame;
        gchar *ts;
        int i;

        if (setupdialog) {
                gtk_window_present (GTK_WINDOW(setupdialog));
                return FALSE;
        }

	setupdialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_transient_for (GTK_WINDOW (setupdialog), GTK_WINDOW (window));
	gtk_window_set_type_hint (GTK_WINDOW (setupdialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_position (GTK_WINDOW (setupdialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_container_set_border_width(GTK_CONTAINER(setupdialog), 10);
	gtk_window_set_title(GTK_WINDOW(setupdialog), _("GTali setup"));
        g_signal_connect(G_OBJECT(setupdialog), "delete_event",
                         G_CALLBACK(setupdialog_destroy), NULL);

	all_boxes = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(setupdialog), all_boxes);

	frame = gtk_frame_new(_("Computer Opponents"));
	gtk_box_pack_start(GTK_BOX(all_boxes), frame, TRUE, TRUE, 0);
	
	box = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(box), 8);
	gtk_container_add(GTK_CONTAINER(frame), box);

        /*--- Button ---*/
	button = gtk_check_button_new_with_label (
                         _("Delay between rolls") );
	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button),DoDelay);
        g_signal_connect(G_OBJECT(button), "clicked",
                         G_CALLBACK(set_as_int), &tmpDoDelay);
	gtk_widget_show (button);

        /*--- Button ---*/
	button = gtk_check_button_new_with_label (
                         _("Show thoughts during turn") );
	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button),
                                     DisplayComputerThoughts);
        g_signal_connect(G_OBJECT(button), "clicked",
                         G_CALLBACK(set_as_int), &tmpDisplayComputerThoughts);
	gtk_widget_show (button);


        /*--- Spinner (number of computers) ---*/
        OriginalNumberOfComputers = NumberOfComputers;
	box2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), box2, TRUE, TRUE, 0);
	label = gtk_label_new(_("Number of opponents:"));
	gtk_box_pack_start(GTK_BOX(box2), label, TRUE, TRUE, 0);
	gtk_widget_show(label);
        ComputerAdj = gtk_adjustment_new((gfloat)NumberOfComputers, 
                                         0.0, 6.0, 1.0, 6.0, 1.0);
	ComputerSpinner = gtk_spin_button_new(GTK_ADJUSTMENT(ComputerAdj),
                                              10, 0);
#if 0
 ifndef HAVE_GTK_SPIN_BUTTON_SET_SNAP_TO_TICKS
        gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ComputerSpinner),
					  GTK_UPDATE_ALWAYS |
					  GTK_UPDATE_SNAP_TO_TICKS);
 else
        gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ComputerSpinner),
					  GTK_UPDATE_ALWAYS );
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ComputerSpinner),
					  TRUE);
 endif
#endif
         g_signal_connect(G_OBJECT(ComputerAdj), "value_changed",
                          G_CALLBACK(MaxPlayersCheck), ComputerAdj);
         gtk_box_pack_start(GTK_BOX(box2), ComputerSpinner, FALSE, TRUE, 0);
	gtk_widget_show(ComputerSpinner);
	gtk_widget_show(box2);

	gtk_widget_show(box);
	gtk_widget_show(frame);


	frame = gtk_frame_new(_("Human Players"));
	gtk_box_pack_start(GTK_BOX(all_boxes), frame, TRUE, TRUE, 0);
	
	box = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(box), 8);
	gtk_container_add(GTK_CONTAINER(frame), box);

        /*--- Spinner (number of humans) ---*/
        OriginalNumberOfHumans = NumberOfHumans;
	box2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), box2, TRUE, TRUE, 0);
	label = gtk_label_new(_("Number of players:"));
	gtk_box_pack_start(GTK_BOX(box2), label, TRUE, TRUE, 0);
	gtk_widget_show(label);
        HumanAdj = gtk_adjustment_new((gfloat)NumberOfHumans, 0.0,
                                      6.0, 1.0, 6.0, 1.0);
	HumanSpinner = gtk_spin_button_new(GTK_ADJUSTMENT(HumanAdj), 10, 0);
#if 0
 ifndef HAVE_GTK_SPIN_BUTTON_SET_SNAP_TO_TICKS
        gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(HumanSpinner),
					  GTK_UPDATE_ALWAYS |
					  GTK_UPDATE_SNAP_TO_TICKS);
 else
        gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(HumanSpinner),
					  GTK_UPDATE_ALWAYS );
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(HumanSpinner),
					  TRUE);
 endif
#endif
         g_signal_connect(G_OBJECT(HumanAdj), "value_changed",
                          G_CALLBACK(MaxPlayersCheck), HumanAdj);
 
        gtk_box_pack_start(GTK_BOX(box2), HumanSpinner, FALSE, TRUE, 0);
	gtk_widget_show(HumanSpinner);
	gtk_widget_show(box2);

	gtk_widget_show(box);
	gtk_widget_show(frame);



        /*--- OPTIONAL RULES FRAME ----*/
 	frame = gtk_frame_new(_("Optional Rules"));
 	gtk_box_pack_start(GTK_BOX(all_boxes), frame, TRUE, TRUE, 0);
 	
 	box = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(box), 8);
 	gtk_container_add(GTK_CONTAINER(frame), box);
 
         /*--- Button ---*/
 	box2 = gtk_hbox_new(FALSE, 0);
 	gtk_box_pack_start(GTK_BOX(box), box2, TRUE, TRUE, 0);
 
 	button = gtk_check_button_new_with_label (_("Extra Yahtzee Bonus") );
 	/*gtk_box_pack_start(GTK_BOX(box2), button, FALSE, FALSE, 0);*/
 	gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
        tmpExtraYahtzeeBonus = ExtraYahtzeeBonus;
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button),ExtraYahtzeeBonus);
        g_signal_connect(G_OBJECT(button), "clicked",
                         G_CALLBACK(set_as_int), &tmpExtraYahtzeeBonus);
 	gtk_widget_show (button);

        BonusEntry = gtk_entry_new();
        gtk_entry_set_max_length(GTK_ENTRY(BonusEntry),3);
        /* Why is it so damn big by default? */
        gtk_widget_set_usize(BonusEntry, 50, -1);
        ts = g_strdup_printf("%3d",ExtraYahtzeeBonusVal);
        gtk_entry_set_text(GTK_ENTRY(BonusEntry),ts);
        g_free(ts);
        if (!ExtraYahtzeeBonus) {
                gtk_entry_set_editable(GTK_ENTRY(BonusEntry),FALSE);
        }
        gtk_box_pack_start (GTK_BOX (box2), BonusEntry, TRUE, TRUE, 0);
        gtk_widget_show (BonusEntry);
 	gtk_widget_show(box2);
        
        
        /*--- Button ---*/
 	button = gtk_check_button_new_with_label (_("Enforce Joker Rules"));
        tmpExtraYahtzeeJoker = ExtraYahtzeeJoker;
 	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button),
                                      ExtraYahtzeeJoker);
        g_signal_connect(G_OBJECT(button), "clicked",
                         G_CALLBACK(set_as_int), &tmpExtraYahtzeeJoker);
        gtk_widget_set_sensitive(button,FALSE); /* NOT READY YET */
 	gtk_widget_show (button);
 	gtk_widget_show(box);
 	gtk_widget_show(frame);
        
        
        /*--- PLAYER NAMES FRAME ----*/
	frame = gtk_frame_new(_("Player Names"));
	gtk_box_pack_start(GTK_BOX(all_boxes), frame, TRUE, TRUE, 0);
	
	box = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(box), 8);
	gtk_container_add(GTK_CONTAINER(frame), box);

        for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
                box2 = gtk_hbox_new(FALSE, 3);

                gtk_box_pack_start(GTK_BOX(box), box2, TRUE, TRUE, 0);
                ts = g_strdup_printf(" %1d:",i+1);
                label = gtk_label_new(ts);
                g_free(ts);
                gtk_box_pack_start(GTK_BOX(box2), label, TRUE, TRUE, 0);
                gtk_widget_show(label);

                PlayerNames[i] = gtk_entry_new ();
                ts = g_strdup_printf("PlayerName%1d",i+1);
                gtk_object_set_data (GTK_OBJECT(setupdialog), ts, PlayerNames[i]);
                g_free(ts);
                gtk_entry_set_text(GTK_ENTRY(PlayerNames[i]),players[i].name);
                gtk_box_pack_start (GTK_BOX (box2), PlayerNames[i], TRUE, TRUE, 0);
                gtk_widget_show(PlayerNames[i]);

                gtk_widget_show(box2);
        }

	gtk_widget_show(box);
	gtk_widget_show(frame);


	/*--- OK/CANCEL into "box" ---*/
	box = gtk_hbox_new(TRUE, 5);
	gtk_box_pack_start(GTK_BOX(all_boxes), box, TRUE, TRUE, 0);

	button = gtk_button_new_from_stock (GTK_STOCK_OK);
        g_signal_connect(G_OBJECT(button), "clicked",
                         G_CALLBACK(do_setup), NULL);
	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 5);
        gtk_widget_show(button);

	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
        g_signal_connect(G_OBJECT(button), "clicked",
                         G_CALLBACK(setupdialog_destroy), GINT_TO_POINTER(1));
	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 5);
        gtk_widget_show(button);
        gtk_widget_show(box);
	
	gtk_widget_show(all_boxes);

	gtk_widget_show(setupdialog);

        return FALSE;
}


/* Arrgh - lets all use the same tabs under emacs: 
Local Variables:
tab-width: 8
c-basic-offset: 8
indent-tabs-mode: nil
*/   
