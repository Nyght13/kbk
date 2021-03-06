/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@pacinfo.com)				   *
*	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
*	    Brian Moore (rom@rom.efn.org)				   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Tartarus/doc/rom.license                  *
***************************************************************************/

/***************************************************************************
*       Tartarus code is copyright (C) 1997-1998 by Daniel Graham          *
*	In using this code you agree to comply with the Tartarus license   *
*       found in the file /Tartarus/doc/tartarus.doc                       *
***************************************************************************/

#include "include.h"

typedef struct multdata MULTDATA;
int flag_lookup args( ( const char *name, const struct flag_type *flag_table) );
struct multdata {
   DESCRIPTOR_DATA *des;
};

void log_naughty args(( CHAR_DATA *ch, char *argument, int logtype));
bool arena;
bool wizlock;
bool newlock;
bool    write_to_descriptor     args( ( int desc, char *txt, int length ) );
int     close           args( ( int fd ) );
void	print_obj_types( CHAR_DATA *ch );
int 	compare_area	args( (const void *v1, const void *v2) );

/*
 * Local functions.
 */
ROOM_INDEX_DATA *	find_location	args( ( CHAR_DATA *ch, char *arg ) );

char * weapon_name_lookup args((int type));

void do_wiznet( CHAR_DATA *ch, char *argument )
{
    int flag;
    char buf[MAX_STRING_LENGTH];

    if ( argument[0] == '\0' )
    {
      	if (IS_SET(ch->wiznet,WIZ_ON))
      	{
            send_to_char("Signing off of Wiznet.\n\r",ch);
            REMOVE_BIT(ch->wiznet,WIZ_ON);
      	}
      	else
      	{
            send_to_char("Welcome to Wiznet!\n\r",ch);
            SET_BIT(ch->wiznet,WIZ_ON);
      	}
      	return;
    }

    if (!str_prefix(argument,"on"))
    {
	send_to_char("Welcome to Wiznet!\n\r",ch);
	SET_BIT(ch->wiznet,WIZ_ON);
	return;
    }

    if (!str_prefix(argument,"off"))
    {
	send_to_char("Signing off of Wiznet.\n\r",ch);
	REMOVE_BIT(ch->wiznet,WIZ_ON);
	return;
    }

    /* show wiznet status */
    if (!str_prefix(argument,"status"))
    {
	buf[0] = '\0';

	if (!IS_SET(ch->wiznet,WIZ_ON))
	    strcat(buf,"off ");

	for (flag = 0; wiznet_table[flag].name != NULL; flag++)
	    if (IS_SET(ch->wiznet,wiznet_table[flag].flag))
	    {
		strcat(buf,wiznet_table[flag].name);
		strcat(buf," ");
	    }

	strcat(buf,"\n\r");

	send_to_char("Wiznet status:\n\r",ch);
	send_to_char(buf,ch);
	return;
    }

    if (!str_prefix(argument,"show"))
    /* list of all wiznet options */
    {
	buf[0] = '\0';

	for (flag = 0; wiznet_table[flag].name != NULL; flag++)
	{
	    char wiznet_show[MSL];
	    if (wiznet_table[flag].level <= get_trust(ch))
	    {
		if (IS_SET(ch->wiznet,wiznet_table[flag].flag))
		{
			sprintf(wiznet_show,
			"[%-2d] {R%-10s{x ",
			wiznet_table[flag].level,wiznet_table[flag].name);
		}
		else
		{
			sprintf(wiznet_show,
			"[%-2d] %-10s ",
			wiznet_table[flag].level,wiznet_table[flag].name);
		}
	
                if (flag % 5 == 0)
                        strcat(buf,"\n\r");

	    	strcat(buf,wiznet_show);
	    }
	}

	strcat(buf,"\n\r");

	send_to_char("Wiznet options available to you are:\n\r",ch);
	send_to_char(buf,ch);
	return;
    }

    flag = wiznet_lookup(argument);

    if (flag == -1 || get_trust(ch) < wiznet_table[flag].level)
    {
	send_to_char("No such option.\n\r",ch);
	return;
    }

    if (IS_SET(ch->wiznet,wiznet_table[flag].flag))
    {
	sprintf(buf,"You will no longer see %s on wiznet.\n\r",
	        wiznet_table[flag].name);
	send_to_char(buf,ch);
	REMOVE_BIT(ch->wiznet,wiznet_table[flag].flag);
    	return;
    }
    else
    {
    	sprintf(buf,"You will now see %s on wiznet.\n\r",
		wiznet_table[flag].name);
	send_to_char(buf,ch);
    	SET_BIT(ch->wiznet,wiznet_table[flag].flag);
	return;
    }

}

void wiznet(char *string, CHAR_DATA *ch, OBJ_DATA *obj,
	    long flag, long flag_skip, int min_level)
{
    DESCRIPTOR_DATA *d;

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if (d->connected == CON_PLAYING
	&&  IS_IMMORTAL(d->character)
	&&  IS_SET(d->character->wiznet,WIZ_ON)
	&&  (!flag || IS_SET(d->character->wiznet,flag))
	&&  (!flag_skip || !IS_SET(d->character->wiznet,flag_skip))
	&&  get_trust(d->character) >= min_level
	&&  d->character != ch)
        {
	    if (IS_SET(d->character->wiznet,WIZ_PREFIX))
	  	send_to_char("--> ",d->character);
            act_new(string,d->character,obj,ch,TO_CHAR,POS_DEAD);
        }
    }

    return;
}

void do_leader( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    if (ch->level < 54 || IS_NPC(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        send_to_char( "Syntax: leader <char>\n\r",ch);
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char("They aren't playing.\n\r",ch);
        return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Can't make mobs leaders.\n\r",ch);
	return;
    }

    if (victim->pcdata->induct == CABAL_LEADER)
    {
        send_to_char("Your power to INDUCT has been taken away!\n\r",victim);
        sprintf(buf, "You have taken away %s's power to induct.\n\r",victim->name);
        send_to_char(buf, ch);
        victim->pcdata->induct = 0;
        sprintf(buf,"AUTO: Cabal leader status revoked by %s.\n\r", ch->original_name);
        if (!IS_IMMORTAL(victim))
                add_history(NULL,victim,buf);
        return;
    }
    else
    {
        send_to_char("You have been given the power to INDUCT!\n\r",victim);
        sprintf(buf, "You have given %s the power to induct.\n\r",victim->name);
        send_to_char(buf, ch);
        victim->pcdata->induct = CABAL_LEADER;
        sprintf(buf,"AUTO: Made cabal leader by %s.\n\r", ch->original_name);
        if (!IS_IMMORTAL(victim))
                add_history(NULL,victim,buf);
        return;
    }
    return;
}

char *const pos_table[]=
{
	"dead", "mortal", "incap", "stun", "sleep", "rest", "sit", "fight",
	"stand",
NULL
};

char *const eq_table[]=
{
	"light", "finger_l", "finger_r", "neck_1", "neck_2", "torso", "head",
	"legs",
	"feet", "hands", "arms", "shield", "body", "waist", "wrist_l", "wrist_r",
	"wield", "hold", "dual_wield","float", "trinal_wield","brand","strap",NULL
};

void do_smite( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];  /* Lot of arguments */
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    char arg4 [MAX_INPUT_LENGTH];
    char arg5 [MAX_INPUT_LENGTH];
    char arg6 [MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *SmittenEQ;
    const int MAX_SMITE_FRACTION = 99;  /* You can change this if you want */
    int hp_percent = 0;
    int mana_percent = 0;
    int move_percent = 0;
    int pos = 0;
    int eq = 0;

    argument = one_argument( argument, arg1 );  /* Combine the arguments */
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );
    argument = one_argument( argument, arg5 );
    argument = one_argument( argument, arg6 );

    if ( IS_NPC(ch) )  /* NPCs may get to be smited, but switch immortals can't */
        return;

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Syntax:\n\r",ch);
        send_to_char( "Smite <victim> <hp> <mana> <move> <position> <equipment>\n\r",ch);
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They are saved only through their abscence.\n\r",ch);
	return;
    }

    if (!IS_NPC(victim) && victim->level >= get_trust(ch))
    {
        send_to_char("Your reach exceeds your grasp.\n\r",ch);
        return;
    }

    if ( arg2[0] != '\0' )
        hp_percent = atoi( arg2 );
    else
        hp_percent = 50;

    if ( hp_percent > MAX_SMITE_FRACTION )
    {
        send_to_char("Hp percent must be between 0 and 95.\n\r", ch );
        return;
    }

    if ( arg3[0] != '\0' )
        mana_percent = atoi( arg3 );
    else
        mana_percent = 0;

    if ( mana_percent > MAX_SMITE_FRACTION || mana_percent < 0 )
    {
        send_to_char("Mana percent must be between 0 and 95.\n\r", ch );
        return;
    }

    if ( arg4[0] != '\0' )
        move_percent = atoi( arg4 );
    else
        move_percent = 0;

    if ( move_percent > MAX_SMITE_FRACTION || move_percent < 0 )
    {
        send_to_char("Move percent must be between 0 and 95.\n\r", ch );
        return;
    }

    /* Customize stuff by alignment */

    if (ch->alignment > 300)
    {
        act_new("Your actions have brought the holy power of $n upon you!",ch,NULL,victim,TO_VICT,POS_DEAD);
        act_new("$N has brought the holy power of $n upon themselves!",ch,NULL,victim,TO_NOTVICT,POS_DEAD);
    }
    if (ch->alignment > -301 && ch->alignment < 301)
    {
        act_new("Your actions have called the divine fury of $n upon you!",ch,NULL,victim,TO_VICT,POS_DEAD);
        act_new("$N has called the divine fury of $n upon themselves!",ch,NULL,victim,TO_NOTVICT,POS_DEAD);
    }
    if (ch->alignment < -300)
    {
        act_new("You are struck down by the infernal power of $n!!",ch,NULL,victim,TO_VICT,POS_DEAD);
        act_new("The hellspawned, infernal power of $n has struck down $N!!",ch,NULL,victim,TO_NOTVICT,POS_DEAD);
    }

    /* This is where the thing we did in retribution is used */

    if ( ch->pcdata->smite[0] != '\0' )
    {
        send_to_char( ch->pcdata->smite, victim );
    }

    /* If it REALLY hurt */

    if ( hp_percent > 75 && victim->hit > victim->max_hit / 4 )
        send_to_char( "That really did HURT!\n\r", victim );

    /* Let's see if equipment needs to be 'blown away' */

    for ( eq = 0; eq_table[eq] != NULL; eq++ )
    {
        if ( !str_prefix( eq_table[eq], arg6 ) )
        {
            if ( ( SmittenEQ = get_eq_char( victim, eq ) ) != NULL )
            {
                sprintf( buf, "$n's %s is blown away!", SmittenEQ->short_descr);
                act( buf, victim, NULL, NULL, TO_ROOM);
                sprintf( buf, "Your %s is blown away!\n\r", SmittenEQ->short_descr);
                send_to_char( buf, victim );

                unequip_char( victim, SmittenEQ );
                obj_from_char( SmittenEQ );
                obj_to_room( SmittenEQ, victim->in_room );
            }
            break;
        }
    }

    /* Let's see what position to put the victim in */

    for ( pos = 0; pos_table[pos] != NULL; pos++ )
    {
        if ( !str_prefix( pos_table[pos], arg5 ) )
        {
            victim->position = pos;                 /* This only works because of the way the pos_table is arranged. */
            if ( victim->position == POS_FIGHTING )
                victim->position = POS_STANDING;     /* POS_FIGHTING is bad */

            if ( victim->position < POS_STUNNED )
                victim->position = POS_STUNNED;

            if ( victim->position == POS_STUNNED )
            {
                act( "$n is stunned, but will probably recover.",victim, NULL, NULL, TO_ROOM );
                send_to_char("You are stunned, but will probably recover.\n\r",victim );
            }

            if ( victim->position == POS_RESTING || victim->position == POS_SITTING )
            {
                act("$n is knocked onto $s butt!", victim, NULL, NULL, TO_ROOM );
                send_to_char("You are knocked onto your butt!\n\r", victim );
            }
            break;
        }
    }

    /* Calculate total hp loss */
    victim->hit -= ( ( victim->hit * hp_percent ) / 100 );
    if ( victim->hit < 1 )
        victim->hit = 1;

    /* Calculate total mana loss */
    victim->mana -= ( ( victim->mana * mana_percent ) / 100 );
    if ( victim->mana < 1 )
       victim->mana = 1;

    /* Calculate total move loss */
    victim->move -= ( ( victim->move * move_percent ) / 100 );
    if ( victim->move < 1 )
        victim->move = 1;

    send_to_char("Your will is done, your power felt.\n\r",ch);
    return;
}

void do_oldsmite( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg1[MAX_INPUT_LENGTH];

    if ( IS_NPC(ch) )
    {
	send_to_char( "Mobs can't smite.\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        send_to_char("Syntax: smite <char>\n\r", ch);
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't playing.\n\r", ch );
        return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Trying to smite a mob?\n\r", ch );
	return;
    }

    if (ch == victim)
    {
        send_to_char( "Trying to smite yourself?\n\r", ch );
        return;
    }

    act( "A bolt from the heavens smites $N!", ch, NULL, victim, TO_NOTVICT );
    act( "A bolt from the heavens smites you!", ch, NULL, victim, TO_VICT );
    act( "You smite $N!", ch, NULL, victim, TO_CHAR );
    victim->hit /= 2;
    return;
}

void do_induct( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH], tbuf[MAX_STRING_LENGTH];
    char tstr[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int cabal;
    int i, iAlign, iEthos;
    int gn, gns, sn;

    if (IS_NPC(ch))
	return;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ((ch->level < 56 && ch->pcdata->induct != CABAL_LEADER)
	|| IS_NPC(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	if(get_trust(ch)>55)
	{
	send_to_char("INDUCT:   Members / Max:\n\r",ch);
	send_to_char("--------------------------\n\r",ch);
        for(i=1; i < MAX_CABAL; i++)
        {       
		sprintf(tbuf,"%d",cabal_max[i]);
                sprintf(buf, "%-12s %-4d / %s\n\r",
			capitalize(cabal_table[i].name),
			cabal_members[i],cabal_table[i].max_members != 1 ? tbuf : "none");
		send_to_char(buf,ch);
	}
	send_to_char("--------------------------\n\r",ch);
        send_to_char( "Syntax: induct <char> <cabal name>\n\r",ch);
	return;
	}
        buf[0] = '\0';
        send_to_char( "Syntax: induct <char> <cabal name>\n\r",ch);
        send_to_char("Valid Cabals are: ",ch);
        for (i = 1; i < (MAX_CABAL -1); i++)
        {
        sprintf(buf,"%s, ", cabal_table[i].name);
        send_to_char(buf,ch);
        }
        sprintf(buf,"%s.\n\r",cabal_table[MAX_CABAL -1].name);
        send_to_char(buf,ch);
	if ((ch->cabal) && (ch->pcdata->induct == CABAL_LEADER))
	{
		sprintf(buf,"There are %d people in your cabal.\n\r",cabal_members[ch->cabal]);
		send_to_char(buf,ch);
	}
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't playing.\n\r", ch );
        return;
    }

    if (IS_NPC(victim))
        return send_to_char( "Mobs can't be inducted into Cabals.\n\r", ch );


    if (!str_prefix(arg2,"empire"))
    {
        send_to_char( "Use the EMPIRE command.\n\r", ch );
	return;
    }

    if (!str_prefix(arg2,"none"))
    {
        if (ch->cabal != victim->cabal && ch->level < 56)
        {
            send_to_char("You have no power over that person's affiliations!\n\r",ch);
            return;
        }
        else
        {
        if (!victim->cabal)
        {
                send_to_char("They are already not in a cabal.\n\r",ch);
                return;
        }       
	    if (victim != ch)
	    {
	        send_to_char("They are now homeless!\n\r",ch);
            	send_to_char("You are now homeless!\n\r",victim);
	    }
	    else
            	send_to_char("You are now homeless!\n\r",victim);
            cabal = victim->cabal;
        /* take away cabal skills */
	cabal_members[victim->cabal]--;
        gn = group_lookup(cabal_table[victim->cabal].name);
        for (gns = 0; gns < MAX_SKILL; gns++)
        {
                if (group_table[gn].spells[gns] == NULL )
                break;
     
                sn = skill_lookup(group_table[gn].spells[gns]);
        
                if (skill_table[sn].skill_level[victim->class] < LEVEL_HERO + 1
                &&  victim->pcdata->learned[sn] > 0)
                        victim->pcdata->learned[sn] = -2;
        }
        group_remove(victim,cabal_table[victim->cabal].name);
        victim->cabal = 0;
	sprintf(buf,"AUTO: Uninducted from %s by %s.\n\r",cabal ? capitalize(cabal_table[cabal].name) : "-",ch->original_name);
	if (!IS_IMMORTAL(victim))
		add_history(NULL,victim,buf);
        return;
        }
    }

    if ( victim->cabal != 0 )
    {
        send_to_char("That person is already in a cabal!\n\r", ch);
        return;
    }

    if ((cabal = cabal_lookup(arg2)) == 0)
    {
        send_to_char("No such cabal exists.\n\r",ch);
	return;
    }

    if ((cabal = cabal_lookup(arg2)) != ch->cabal &&
        get_trust(ch)<56)
    {
        send_to_char("You may only induct into the cabal which you belong.\n\r", ch);
        return;
    }

    if (IS_SET(victim->act,PLR_MORON) && !IS_IMMORTAL(ch))
    {
        send_to_char( "Are you sure?\n\r", ch );
	return;
    }

    if (!cabal_table[cabal].induct && get_trust(ch) < 60)
    {
	send_to_char("That cabal is CLOSED.\n\r",ch);
	return;
    }

    if (victim->alignment<0)       	iAlign=ALIGN_E;
    else if (victim->alignment==0)	iAlign=ALIGN_N;
	else                  		iAlign=ALIGN_G;

    if (!(cabal_restr_table[cabal].acc_align & iAlign) && !IS_IMMORTAL(victim))
    {
	switch (iAlign)
    	{
		case ALIGN_E: send_to_char("That cabal does not accept evil characters.\n\r",ch); break;
           	case ALIGN_N: send_to_char("That cabal does not accept neutral characters.\n\r",ch); break;
           	case ALIGN_G: send_to_char("That cabal does not accept good characters.\n\r",ch); break;
           	default: send_to_char("ERROR: SEEK AN IMPLEMENTOR!\n\r",ch);  
	}
	return;
    }

    if (victim->pcdata->ethos < 0)     	iEthos=ETHOS_C;
    else if (victim->pcdata->ethos==0)	iEthos=ETHOS_N;
	else                           	iEthos=ETHOS_L;

    if (!(cabal_restr_table[cabal].acc_ethos & iEthos) && !IS_IMMORTAL(victim))
    {
	switch (iEthos)
	{
		case ETHOS_C: send_to_char("That cabal does not accept chaotic characters.\n\r",ch); break;
           	case ETHOS_N: send_to_char("That cabal does not accept neutral characters.\n\r",ch); break;
           	case ETHOS_L: send_to_char("That cabal does not accept lawful characters.\n\r",ch); break;
           	default: send_to_char("ERROR: SEEK AN IMPLEMENTOR!\n\r",ch);  
	}
	return;
    }

    sprintf(buf,"%s has been inducted into the %s.",
        victim->name, cabal_table[cabal].long_name);
    act(buf,victim,0,ch,TO_NOTVICT);
    strcat(buf,"\n\r");
    send_to_char(buf,ch);
    sprintf(buf,"You have been inducted into the %s.\n\r", cabal_table[cabal].long_name);
    send_to_char(buf, victim);
    group_remove(victim,cabal_table[victim->cabal].name);
    victim->cabal = cabal;
    sprintf(tstr,"%s into %s.",victim->name, capitalize(cabal_table[cabal].name));
    log_naughty(ch,tstr,5);

    sprintf(buf,"AUTO: Inducted into %s by %s.\n\r",capitalize(cabal_table[cabal].name),ch->original_name);
    if (!IS_IMMORTAL(victim))
	add_history(NULL,victim,buf);

    group_add(victim,cabal_table[cabal].name,FALSE);
    cabal_members[cabal]++;
    gn = group_lookup(cabal_table[victim->cabal].name);
    for (gns = 0; gns < MAX_SKILL; gns++)
    {
	if (gn < 1 || group_table[gn].spells[gns] == NULL )
	    break;
     
	sn = skill_lookup(group_table[gn].spells[gns]);
        
	if (sn > 0 && skill_table[sn].skill_level[victim->class] < LEVEL_HERO + 1)
	    victim->pcdata->learned[sn] = 70;
    }

    if(victim->cabal == CABAL_KNIGHT)
	set_extitle(victim,", Knight of Thera");
    if(victim->cabal == CABAL_BOUNTY)
	set_extitle(victim,", Neophyte Hunter");
}



void do_leaderpeek( CHAR_DATA *ch, char *argument )
{
    	char arg1 [MAX_INPUT_LENGTH];
    	char buf[MAX_STRING_LENGTH];
    	int sn;
    	CHAR_DATA *victim;
    	int col=0;
    	argument = one_argument( argument, arg1 );

    	if ( IS_NPC(ch))
		return;

    	if ((ch->level < 56 
		&& ch->pcdata->induct != CABAL_LEADER
		&& ch->pcdata->empire != EMPIRE_EMPEROR))
    	{
        	send_to_char("Huh?\n\r", ch);
        	return;
    	}

    	if ((( victim = get_char_room( ch, arg1 )) != NULL) && (victim->cabal != 0) && (victim->cabal != ch->cabal))
    	{
        	send_to_char("You may not do that to people in another cabal.\n\r", ch);
        	return;
    	}

    	if ( arg1[0] == '\0')
    	{
		send_to_char( "Who is it you wish to know more about?\n\r", ch );
		return;
    	}
    	else if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
    	{
		send_to_char("They must be in the room for you to find out that much.\n\r",ch);
		return;
    	}

    	if ( IS_NPC(victim) )
		return send_to_char("You cannot use this command on NPCs.\n\r",ch);

        if ( IS_IMMORTAL(victim))
        {
                act("A bolt from the heavens comes down and smites your ass!",ch,0,0,TO_CHAR);
                ch->hit/=2;
		return;
        }

    	sprintf(buf,"%s has the following abilities at 90 percent or higher:\n\r", victim->original_name );
		send_to_char( buf, ch );

    	for ( sn = 0; sn < MAX_SKILL; sn++ )
    	{
		if ( skill_table[sn].name == NULL )
			break;

		if (skill_table[sn].skill_level[victim->class] > 52)
			continue;
	    
		if ( victim->level < skill_table[sn].skill_level[victim->class] || victim->pcdata->learned[sn] < 90)
			continue;

	    	sprintf( buf, "%-20s %3d%%  ", skill_table[sn].name, victim->pcdata->learned[sn] );
	    	send_to_char( buf, ch );

	    	if ( ++col % 3 == 0 )
			send_to_char( "\n\r", ch );
    	}

	send_to_char( "\n\r", ch );
	sprintf(buf,"%s has a PK record of %d/%d",victim->name,victim->pcdata->kills[PK_KILLS], victim->pcdata->killed[PK_KILLED]);
	send_to_char(buf,ch);
	send_to_char("\n\r",ch);
	sprintf(buf,"%s is ", victim->original_name );
	send_to_char( buf, ch );
	if ( victim->alignment == 1000 ) send_to_char( "good,", ch );
	else if ( victim->alignment == 0 ) send_to_char( "neutral,", ch );
	else send_to_char( "evil,", ch );
	send_to_char( " and has a ", ch );
	if ( victim->pcdata->ethos == 1000 ) send_to_char( "lawful ethos.\n\r", ch );
	else if ( victim->pcdata->ethos == 0 ) send_to_char( "neutral ethos.\n\r", ch );
	else send_to_char( "chaotic ethos.\n\r", ch );

    	sprintf( buf, "%s has %d hours played.\n\r", victim->original_name,
    	(int) (victim->played + current_time - victim->logon) / 3600 );
    	send_to_char(buf,ch);

	if ( col % 3 != 0 )
	    send_to_char( "\n\r", ch );

  	return;
}

void easy_induct( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int cabal;

    if ((cabal = cabal_lookup(argument)) == 0)
    {
        send_to_char("No such cabal exists.\n\r",ch);
	return;
    }

    ch->cabal = cabal;
    group_add(ch,cabal_table[cabal].name,FALSE);
    if (cabal == CABAL_ANCIENT)
    {
        ch->pcdata->learned[gsn_eye_of_the_predator] = 75;
        ch->pcdata->learned[gsn_shadowgate] = 75;
        ch->pcdata->learned[skill_lookup("eyes of intrigue")] = 75;
        ch->pcdata->learned[skill_lookup("shroud")] = 75;
        ch->pcdata->learned[skill_lookup("cloak")] = 75;
        ch->pcdata->learned[skill_lookup("mantle of maehslin")] = 75;
        ch->pcdata->learned[skill_lookup("eavesdrop")] = 75;
    }
    else if (cabal == CABAL_BOUNTY)
    {
    }
    else if (cabal == CABAL_KNIGHT)
    {
	ch->pcdata->learned[skill_lookup("lightwalk")] = 75;
	ch->pcdata->learned[skill_lookup("dragonplate")] = 75;
	ch->pcdata->learned[skill_lookup("dragonweapon")] = 75;
	ch->pcdata->learned[skill_lookup("blademaster")] = 75;
	ch->pcdata->learned[skill_lookup("truestrike")] = 75;
    }
    else if (cabal == CABAL_ARCANA)
    {
        ch->pcdata->learned[skill_lookup("channel")] = 75;
        ch->pcdata->learned[skill_lookup("venueport")] = 75;
        ch->pcdata->learned[skill_lookup("spiritblade")] = 75;
        ch->pcdata->learned[skill_lookup("scourge")] = 75;
        ch->pcdata->learned[skill_lookup("scrying")] = 75;
        ch->pcdata->learned[skill_lookup("counterspell")] = 75;
    }
    else if (cabal == CABAL_RAGER)
    {
	ch->pcdata->learned[gsn_bandage] = 75;
	ch->pcdata->learned[gsn_spellbane] = 75;
	ch->pcdata->learned[gsn_trophy] = 75;
	ch->pcdata->learned[gsn_resistance] = 75;
	ch->pcdata->learned[gsn_blitz] = 75;
	ch->pcdata->learned[gsn_bloodthirst] = 75;
 	ch->pcdata->learned[gsn_true_sight] = 75;
    }
    else if (cabal == CABAL_OUTLAW)
    {
	ch->pcdata->learned[skill_lookup("decoy")] = 75;
	ch->pcdata->learned[skill_lookup("isperse")] = 75;
	ch->pcdata->learned[skill_lookup("chaos blade")] = 75;
	ch->pcdata->learned[skill_lookup("call slaves")] = 75;
	ch->pcdata->learned[skill_lookup("randomizer")] = 75;
	ch->pcdata->learned[skill_lookup("garble")] = 75;
	ch->pcdata->learned[skill_lookup("confuse")] = 75;
	ch->pcdata->learned[skill_lookup("chromatic fire")] = 75;
    }

    else if (cabal == CABAL_EMPIRE)
    {
    	ch->pcdata->learned[skill_lookup("nightfist")] = 75;
    	ch->pcdata->learned[skill_lookup("shove")] = 75;
    	ch->pcdata->learned[skill_lookup("centurions")] = 75;
    	ch->pcdata->learned[skill_lookup("black shroud")] = 75;
    	ch->pcdata->learned[skill_lookup("imperial training")] = 75;
        ch->pcdata->learned[skill_lookup("devour")] = 75;
        ch->pcdata->learned[skill_lookup("sigil of pain")] = 75;
    }

    else if (cabal == CABAL_SYLVAN)
    {
	ch->pcdata->learned[skill_lookup("chameleon")] = 75;
	ch->pcdata->learned[skill_lookup("pathfinding")] = 75;
	ch->pcdata->learned[skill_lookup("beast call")] = 75;
	ch->pcdata->learned[skill_lookup("wall of thorns")] = 75;
	ch->pcdata->learned[skill_lookup("insectswarm")] = 75;
	ch->pcdata->learned[skill_lookup("spiderhands")] = 75;
	}

    sprintf(buf,"%s has been inducted into %s.\n\r",ch->name, capitalize(cabal_table[cabal].long_name));
    log_string( buf );
}


/* equips a character */

void do_outfit ( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    AFFECT_DATA af;

    if (IS_NPC(ch) || (ch->pcdata->newbie == FALSE && ch->ghost == 0))
    {
	send_to_char("Find it yourself!\n\r",ch);
	return;
    }

    if (is_affected(ch,skill_lookup("outfit")))
    {
	send_to_char("You cannot yet reequip yourself.\n\r",ch);
	return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_BODY ) ) == NULL )
    {
	obj = create_object( get_obj_index(1210), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_FINGER_L ) ) == NULL )
    {
	obj = create_object( get_obj_index(1214), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }


    if ( ( obj = get_eq_char( ch, WEAR_FINGER_R ) ) == NULL )
    {
	obj = create_object( get_obj_index(1214), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_NECK_1 ) ) == NULL )
    {
	obj = create_object( get_obj_index(1213), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_NECK_2 ) ) == NULL )
    {
	obj = create_object( get_obj_index(1213), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_LEGS ) ) == NULL )
    {
	obj = create_object( get_obj_index(1217), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_FEET ) ) == NULL )
    {
	obj = create_object( get_obj_index(1218), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_HANDS ) ) == NULL )
    {
	obj = create_object( get_obj_index(1212), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_ARMS ) ) == NULL )
    {
	obj = create_object( get_obj_index(1211), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_ABOUT ) ) == NULL )
    {
	obj = create_object( get_obj_index(1221), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_WAIST ) ) == NULL )
    {
	obj = create_object( get_obj_index(1216), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_WRIST_L ) ) == NULL )
    {
	obj = create_object( get_obj_index(1215), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if ( ( obj = get_eq_char( ch, WEAR_WRIST_R ) ) == NULL )
    {
	obj = create_object( get_obj_index(1215), 0 );
	obj->cost = 0;
      	obj_to_char( obj, ch );
      	wear_obj( ch, obj, FALSE );
    }

    if (((obj = get_eq_char(ch,WEAR_WIELD)) == NULL 
    ||   !IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS)) 
    &&  (obj = get_eq_char( ch, WEAR_LIGHT ) ) == NULL
    &&  (obj = get_eq_char( ch, WEAR_SHIELD ) ) == NULL )
    {
	obj = create_object( get_obj_index(1220), 0 );
	obj->cost = 0;
        obj_to_char( obj, ch );
        wear_obj( ch, obj, FALSE );
    }

    send_to_char("You have been equipped by the gods.\n\r",ch);
    send_to_char("You may buy weapons and instruments one south and one up from the recall pit in Midgaard.\n\r",ch);

    init_affect(&af);
    af.where		= TO_AFFECTS;
    af.type		= skill_lookup("outfit");
    af.location		= 0;
    af.duration		= 80;
    af.aftype		= AFT_SKILL;
    af.bitvector	= AFF_INFRARED;
    af.modifier		= 0;
    af.level		= ch->level;
	af.affect_list_msg = str_dup("grants newbie gear");
    affect_to_char(ch,&af);
}

/* RT nochannels command, for those spammers */
void do_nochannels( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Nochannel whom?", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }

    if ( IS_SET(victim->comm, COMM_NOCHANNELS) )
    {
        REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char( "The gods have restored your channel priviliges.\n\r",
		      victim );
        send_to_char( "NOCHANNELS removed.\n\r", ch );
	sprintf(buf,"$N restores channels to %s",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char( "The gods have revoked your channel priviliges.\n\r",
		       victim );
        send_to_char( "NOCHANNELS set.\n\r", ch );
	sprintf(buf,"$N revokes %s's channels.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}



void do_smote(CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    char *letter,*name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }

    if (strstr(argument,ch->name) == NULL)
    {
	send_to_char("You must include your name in an smote.\n\r",ch);
	return;
    }

    send_to_char(argument,ch);
    send_to_char("\n\r",ch);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr(argument,vch->name)) == NULL)
        {
	    send_to_char(argument,vch);
	    send_to_char("\n\r",vch);
            continue;
        }

        strcpy(temp,argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        name = vch->name;

        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == strlen(vch->name))
            {
                strcat(temp,"r");
                continue;
            }

            if (*letter == 's' && matches == strlen(vch->name))
            {
                matches = 0;
                continue;
            }

            if (matches == strlen(vch->name))
            {
                matches = 0;
            }

            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == strlen(vch->name))
                {
                    strcat(temp,"you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat(last,letter,1);
                continue;
            }

            matches = 0;
            strcat(temp,last);
            strncat(temp,letter,1);
            last[0] = '\0';
            name = vch->name;
        }

	send_to_char(temp,vch);
	send_to_char("\n\r",vch);
    }

    return;
}

void do_bamfin( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( !IS_NPC(ch) )
    {
	smash_tilde( argument );

	if (argument[0] == '\0')
	{
	    sprintf(buf,"Your poofin is %s\n\r",ch->pcdata->bamfin);
	    send_to_char(buf,ch);
	    return;
	    
	}

	free_string( ch->pcdata->bamfin );
	ch->pcdata->bamfin = str_dup( argument );

        sprintf(buf,"Your poofin is now %s\n\r",ch->pcdata->bamfin);
        send_to_char(buf,ch);
    }
    return;
}



void do_bamfout( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( !IS_NPC(ch) )
    {
        smash_tilde( argument );

        if (argument[0] == '\0')
        {
            sprintf(buf,"Your poofout is %s\n\r",ch->pcdata->bamfout);
            send_to_char(buf,ch);
            return;
      
        }

        free_string( ch->pcdata->bamfout );
        ch->pcdata->bamfout = str_dup( argument );

        sprintf(buf,"Your poofout is now %s\n\r",ch->pcdata->bamfout);
        send_to_char(buf,ch);
    }
    return;
}


void do_deny( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH], *cname;
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Deny whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    SET_BIT(victim->act, PLR_DENY);
    send_to_char( "You are denied access!\n\r", victim );
    sprintf(buf,"$N denies access to %s",victim->name);
    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    send_to_char( "OK.\n\r", ch );
    save_char_obj(victim);
    stop_fighting(victim,TRUE);
    victim->pause = 0;

    sprintf(buf,"AUTO: Denied by %s.\n\r",ch->original_name);
    add_history(NULL,victim,buf);
    cname	= str_dup(victim->original_name);
    do_quit_new(victim,"", TRUE);
    delete_char(cname,TRUE);
    return;
}

void do_undeny( CHAR_DATA *ch, char *argument)
{
    if (argument[0] == '\0')
    {
    	send_to_char("Load who?\n\r", ch);
    	return;
    }

    FILE *fp;
    char findFile[MIL];
    char buf[MIL];
    

    sprintf( findFile, "%s/dead_char/%s%s", PLAYER_DIR, capitalize(argument),".plr");
    if ( ( fp = fopen( findFile, "r" ) ) == NULL )
    {
	return send_to_char("That player file does not exist.\n\r",ch);
    }

    sprintf(buf,"mv %s/dead_char/%s.plr %s/%s.plr",PLAYER_DIR,capitalize(argument),PLAYER_DIR,capitalize(argument));
    system(buf);
    
    sprintf(buf,"%s has been allowed access.\n\r",capitalize(argument));
    return send_to_char(buf, ch);
}	

void do_disconnect( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Disconnect whom?\n\r", ch );
	return;
    }

    if (is_number(arg))
    {
	int desc;

	desc = atoi(arg);
    	for ( d = descriptor_list; d != NULL; d = d->next )
    	{
            if ( d->descriptor == desc )
            {
            	close_socket( d );
            	send_to_char( "Ok.\n\r", ch );
            	return;
            }
	}
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim->desc == NULL )
    {
	act( "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
	return;
    }

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	if ( d == victim->desc )
	{
	    close_socket( d );
	    send_to_char( "Ok.\n\r", ch );
	    return;
	}
    }

    bug( "Do_disconnect: desc not found.", 0 );
    send_to_char( "Descriptor not found!\n\r", ch );
    return;
}



void do_pardon( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Syntax: pardon <character> <killer|thief>.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( !str_cmp( arg2, "killer" ) )
    {
	if ( IS_SET(victim->act, PLR_KILLER) )
	{
	    REMOVE_BIT( victim->act, PLR_KILLER );
	    send_to_char( "Killer flag removed.\n\r", ch );
	    send_to_char( "You are no longer a KILLER.\n\r", victim );
	}
	return;
    }

    if ( !str_cmp( arg2, "thief" ) )
    {
	if ( IS_SET(victim->act, PLR_THIEF) )
	{
	    REMOVE_BIT( victim->act, PLR_THIEF );
	    send_to_char( "Thief flag removed.\n\r", ch );
	    send_to_char( "You are no longer a THIEF.\n\r", victim );
	}
	return;
    }

    send_to_char( "Syntax: pardon <character> <killer|thief>.\n\r", ch );
    return;
}



void do_echo( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    char buffer[MAX_STRING_LENGTH*2];

    if ( argument[0] == '\0' )
    {
	send_to_char( "Global echo what?\n\r", ch );
	return;
    }

	/*if (arena) {
		do_arena_echo(argument);
		return;
	}*/
    for ( d = descriptor_list; d; d = d->next )
    {
	if ( d->connected == CON_PLAYING )
	{
	    colorconv(buffer, argument, d->character);
	    if (get_trust(d->character) >= get_trust(ch))
		send_to_char( "global> ",d->character);
	    send_to_char( buffer, d->character );
	    send_to_char( "\n\r",   d->character );
	}
    }

    return;
}



void do_recho( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Local echo what?\n\r", ch );

	return;
    }

    for ( d = descriptor_list; d; d = d->next )
    {
	if ( d->connected == CON_PLAYING
	&&   d->character->in_room == ch->in_room )
	{
            if (get_trust(d->character) >= get_trust(ch))
                send_to_char( "local> ",d->character);
	    send_to_char( argument, d->character );
	    send_to_char( "\n\r",   d->character );
	}
    }

    return;
}

void do_zecho(CHAR_DATA *ch, char *argument)
{
	char buffer[MAX_STRING_LENGTH*2];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
	send_to_char("Zone echo what?\n\r",ch);
	return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
	if (d->connected == CON_PLAYING
	&&  d->character->in_room != NULL && ch->in_room != NULL
	&&  d->character->in_room->area == ch->in_room->area)
	{
		colorconv(buffer, argument, d->character);
	    if (get_trust(d->character) >= get_trust(ch) && IS_IMMORTAL(d->character) && !IS_NPC(ch))
		send_to_char("zone> ",d->character);
	    send_to_char(buffer,d->character);
	    send_to_char("\n\r",d->character);
	}
    }
}

void do_pecho( CHAR_DATA *ch, char *argument )
{
	char buffer[MAX_STRING_LENGTH*2];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if ( argument[0] == '\0' || arg[0] == '\0' )
    {
	send_to_char("Personal echo what?\n\r", ch);
	return;
    }

    if  ( (victim = get_char_world(ch, arg) ) == NULL )
    {
	send_to_char("Target not found.\n\r",ch);
	return;
    }


    if (get_trust(victim) >= get_trust(ch) && get_trust(ch) != MAX_LEVEL)
        send_to_char( "personal> ",victim);
	colorconv(buffer, argument, victim);
    send_to_char(buffer,victim);
    send_to_char("\n\r",victim);
	colorconv(buffer, argument, ch);
    send_to_char( "personal> ",ch);
    send_to_char(buffer,ch);
    send_to_char("\n\r",ch);
}


ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if ( is_number(arg) )
	return get_room_index( atoi( arg ) );

    if ( ( victim = get_char_world( ch, arg ) ) != NULL )
	return victim->in_room;

    if ( ( obj = get_obj_world( ch, arg ) ) != NULL )
	return obj->in_room;

    return NULL;
}



void do_transfer( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Transfer whom (and where)?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg1, "all" ) )
    {
	for ( d = descriptor_list; d != NULL; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    &&   d->character != ch
	    &&   d->character->in_room != NULL
	    &&   can_see( ch, d->character ) )
	    {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "%s %s", d->character->name, arg2 );
		do_transfer( ch, buf );
	    }
	}
	return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
	location = ch->in_room;
    }
    else
    {
	if ( ( location = find_location( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "No such location.\n\r", ch );
	    return;
	}

	if ( !is_room_owner(ch,location) && room_is_private( location )
	&&  get_trust(ch) < MAX_LEVEL)
	{
	    send_to_char( "That room is private right now.\n\r", ch );
	    return;
	}
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim->in_room == NULL )
    {
	send_to_char( "They are in limbo.\n\r", ch );
	return;
    }

    if (victim->level >= ch->level && !IS_NPC(victim))
    {
	send_to_char( "They are too high for you to mess with.\n\r", ch );
	return;
    }

    if ( victim->fighting != NULL )
	stop_fighting( victim, TRUE );
    act( "$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, location );
    act( "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
    if ( ch != victim )
	act( "$n has transferred you.", ch, NULL, victim, TO_VICT );
    do_look( victim, "auto" );
    send_to_char( "Ok.\n\r", ch );

}



void do_at( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    OBJ_DATA *on;
    CHAR_DATA *wch;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "At where what?\n\r", ch );
	return;
    }

    if ( ( location = find_location( ch, arg ) ) == NULL )
    {
	send_to_char( "No such location.\n\r", ch );
	return;
    }

    if (!is_room_owner(ch,location) && room_is_private( location )
    &&  get_trust(ch) < MAX_LEVEL)
    {
	send_to_char( "That room is private right now.\n\r", ch );
	return;
    }

    original = ch->in_room;
    on = ch->on;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
	if ( wch == ch )
	{
	    char_from_room( ch );
	    char_to_room( ch, original );
	    ch->on = on;
	    break;
	}
    }

    return;
}



void do_goto( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    int count = 0;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Goto where?\n\r", ch );
	return;
    }

    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
	send_to_char( "No such location.\n\r", ch );
	return;
    }

    count = 0;
    for ( rch = location->people; rch != NULL; rch = rch->next_in_room )
        count++;

    if (!is_room_owner(ch,location) && room_is_private(location)
    &&  (count > 1 || get_trust(ch) < MAX_LEVEL))
    {
	send_to_char( "That room is private right now.\n\r", ch );
	return;
    }

    if ( ch->fighting != NULL )
	stop_fighting( ch, TRUE );

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
	if (get_trust(rch) >= ch->invis_level)
	{
	    if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
		act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
	    else
		act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
	}
    }

    char_from_room( ch );
    char_to_room( ch, location );


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
            else
                act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }

    do_look( ch, "auto" );
    return;
}

void do_violate( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Goto where?\n\r", ch );
        return;
    }

    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }

    if (!room_is_private( location ))
    {
        send_to_char( "That room isn't private, use goto.\n\r", ch );
        return;
    }

    if ( ch->fighting != NULL )
        stop_fighting( ch, TRUE );

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
            else
                act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }

    char_from_room( ch );
    char_to_room( ch, location );


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
            else
                act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }

    do_look( ch, "auto" );
    return;
}

/* RT to replace the 3 stat commands */

void do_stat ( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char *string;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim;

   string = one_argument(argument, arg);
   if ( arg[0] == '\0')
   {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  stat <name>\n\r",ch);
	send_to_char("  stat obj <name>\n\r",ch);
	send_to_char("  stat mob/char <name>\n\r",ch);
 	send_to_char("  stat room <number>\n\r",ch);
	return;
   }

   if (!str_cmp(arg,"room"))
   {
	do_rstat(ch,string);
	return;
   }

   if (!str_cmp(arg,"obj"))
   {
	do_ostat(ch,string);
	return;
   }

   if(!str_cmp(arg,"char")  || !str_cmp(arg,"mob"))
   {
	do_mstat(ch,string);
	return;
   }

   /* do it the old way */

   obj = get_obj_world(ch,argument);
   if (obj != NULL)
   {
     do_ostat(ch,argument);
     return;
   }

  victim = get_char_world(ch,argument);
  if (victim != NULL)
  {
    do_mstat(ch,argument);
    return;
  }

  location = find_location(ch,argument);
  if (location != NULL)
  {
    do_rstat(ch,argument);
    return;
  }

  send_to_char("Nothing by that name found anywhere.\n\r",ch);
}





void do_rstat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    int door;

    one_argument( argument, arg );
    location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
    if ( location == NULL )
    {
	send_to_char( "No such location.\n\r", ch );
	return;
    }

    if (!is_room_owner(ch,location) && ch->in_room != location
    &&  room_is_private( location ) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
	send_to_char( "That room is private right now.\n\r", ch );
	return;
    }

    sprintf( buf, "Name: '%s'\n\rArea: '%s' %s\n\r",
	location->name,
	location->area->name,
	IS_EXPLORE(location) ? "(AREA_EXPLORE)" : "" );
    send_to_char( buf, ch );

    sprintf( buf, "Credit: '%s'\n\r",
	location->area->credits);
    send_to_char_bw( buf, ch) ;

    sprintf( buf,
    "Vnum: %ld  Sector: %s  Light: %d  Healing: %d  mana: %d\n\r",
	location->vnum,
	sector_bit_name(location->sector_type),
	location->light,
	location->heal_rate,
    location->mana_rate );
    send_to_char( buf, ch );

    sprintf( buf,
	"Room flags: %s.\n\rDescription:\n\r%s",
	//flag_string(location->room_flags),
	flag_string(room_flags,ch->in_room->room_flags),
	location->description );
    send_to_char( buf, ch );

    if ( location->extra_descr != NULL )
    {
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Extra description keywords: '", ch );
	for ( ed = location->extra_descr; ed; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next != NULL )
		send_to_char( " ", ch );
	}
	send_to_char( "'.\n\r", ch );
    }

    send_to_char( "Characters:", ch );
    for ( rch = location->people; rch; rch = rch->next_in_room )
    {
	if (can_see(ch,rch))
        {
	    send_to_char( " ", ch );
	    one_argument( rch->name, buf );
	    send_to_char( buf, ch );
	}
    }

    send_to_char( ".\n\rObjects:   ", ch );
    for ( obj = location->contents; obj; obj = obj->next_content )
    {
	send_to_char( " ", ch );
	one_argument( obj->name, buf );
	send_to_char( buf, ch );
    }
    send_to_char( ".\n\r", ch );

    for ( door = 0; door <= 5; door++ )
    {
	EXIT_DATA *pexit;

	if ( ( pexit = location->exit[door] ) != NULL )
	{
	    sprintf( buf,
		"Door: %d.  To: %ld.  Key: %ld.  Exit flags: %ld.\n\rKeyword: '%s'.  Description: %s",

		door,
		(pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
	    	pexit->key,
	    	pexit->exit_info,
	    	pexit->keyword,
	    	pexit->description[0] != '\0'
		    ? pexit->description : "(none).\n\r" );
	    send_to_char( buf, ch );
	}
    }

    if ( location->progtypes != 0 )  
    {
    	if ( IS_SET(location->progtypes, IPROG_SPEECH ) ) 
	{
            sprintf(buf, "Room has speech prog: %s\n\r", location->rprogs->speech_name);
            send_to_char( buf, ch );
    	}
   }

    do_raffects(ch,"");

    return;
}



void do_ostat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Stat what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_world( ch, argument ) ) == NULL )
    {
	send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
	return;
    }

    sprintf( buf, "Name(s): %s\n\r",
	obj->name );
    send_to_char( buf, ch );

    sprintf( buf, "Vnum: %ld  Format: %s  Type: %s  Resets: %d\n\r",
	obj->pIndexData->vnum, obj->pIndexData->new_format ? "new" : "old",
        item_name(obj->item_type), obj->pIndexData->reset_num );
    send_to_char( buf, ch );

    sprintf( buf, "Short description: %s\n\rLong description: %s\n\r",
	obj->short_descr, obj->description );
    send_to_char( buf, ch );

	sprintf( buf, "Owner: %s\n\r", obj->owner);
    send_to_char(buf,ch);

    sprintf( buf, "Wear bits: %s\n\rExtra bits: %s\n\r",
	wear_bit_name(obj->wear_flags), bitmask_string( &obj->extra_flags, extra_flags ) );
    send_to_char( buf, ch );

    sprintf( buf, "Restrict_flags: %s\n\r",
	restrict_bit_name(obj->pIndexData->restrict_flags));
    send_to_char(buf,ch);

    sprintf( buf, "Number: %d/%d  Weight: %d/%d/%d\n\r",
	1,           get_obj_number( obj ),
	obj->weight, get_obj_weight( obj ),get_true_weight(obj) );
    send_to_char( buf, ch );

    sprintf( buf, "Material: %s\n\r", obj->material->name);
    send_to_char( buf, ch );

    sprintf( buf, "Level: %d  Cost: %d  Condition: %d  Timer: %d\n\r",
	obj->level, obj->cost, obj->condition, obj->timer );
    send_to_char( buf, ch );

    sprintf( buf,
	"In room: %ld  In object: %s  Carried by: %s  Wear_loc: %d\n\r",
	obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
	obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
	obj->carried_by == NULL    ? "(none)" :
	    can_see(ch,obj->carried_by) ? obj->carried_by->name
				 	: "someone",
	obj->wear_loc );
    send_to_char( buf, ch );

    sprintf( buf, "Values: %ld %ld %ld %ld %ld\n\r",
	obj->value[0], obj->value[1], obj->value[2], obj->value[3],
	obj->value[4] );
    send_to_char( buf, ch );

/* Report object limit and count and say if maxxed */
    sprintf( buf, "COUNT_DATA: Limit is %d, Count is %d -->%s.\n\r",obj->pIndexData->limtotal,obj->pIndexData->limcount,
(obj->pIndexData->limcount < obj->pIndexData->limtotal) ? "Not maxxed" : (obj->pIndexData->limtotal == 0) ? "Non-limited" : "Maxxed");
     send_to_char(buf,ch);

    /* now give out vital statistics as per identify */

    switch ( obj->item_type )
    {
    	case ITEM_SCROLL:
    	case ITEM_POTION:
    	case ITEM_PILL:
	    sprintf( buf, "Level %ld spells of:", obj->value[0] );
	    send_to_char( buf, ch );

	    if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
	    {
	    	send_to_char( " '", ch );
	    	send_to_char( skill_table[obj->value[1]].name, ch );
	    	send_to_char( "'", ch );
	    }

	    if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
	    {
	    	send_to_char( " '", ch );
	    	send_to_char( skill_table[obj->value[2]].name, ch );
	    	send_to_char( "'", ch );
	    }

	    if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
	    {
	    	send_to_char( " '", ch );
	    	send_to_char( skill_table[obj->value[3]].name, ch );
	    	send_to_char( "'", ch );
	    }

	    if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
	    {
		send_to_char(" '",ch);
		send_to_char(skill_table[obj->value[4]].name,ch);
		send_to_char("'",ch);
	    }

	    send_to_char( ".\n\r", ch );
	break;

    	case ITEM_WAND:
    	case ITEM_STAFF:
	    sprintf( buf, "Has %ld(%ld) charges of level %ld",
	    	obj->value[1], obj->value[2], obj->value[0] );
	    send_to_char( buf, ch );

	    if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
	    {
	    	send_to_char( " '", ch );
	    	send_to_char( skill_table[obj->value[3]].name, ch );
	    	send_to_char( "'", ch );
	    }

	    send_to_char( ".\n\r", ch );
	break;

	case ITEM_DRINK_CON:
	    sprintf(buf,"It holds %s-colored %s.\n\r",
		liq_table[obj->value[2]].liq_color,
		liq_table[obj->value[2]].liq_name);
	    send_to_char(buf,ch);
	    break;


    	case ITEM_WEAPON:
 	    send_to_char("Weapon type is ",ch);
	    switch (obj->value[0])
	    {
	    	case(WEAPON_EXOTIC):
		    send_to_char("exotic\n\r",ch);
		    break;
	    	case(WEAPON_SWORD):
		    send_to_char("sword\n\r",ch);
		    break;
	    	case(WEAPON_DAGGER):
		    send_to_char("dagger\n\r",ch);
		    break;
	    	case(WEAPON_SPEAR):
		    send_to_char("spear/staff\n\r",ch);
		    break;
	    	case(WEAPON_MACE):
		    send_to_char("mace/club\n\r",ch);
		    break;
	   	case(WEAPON_AXE):
		    send_to_char("axe\n\r",ch);
		    break;
	    	case(WEAPON_FLAIL):
		    send_to_char("flail\n\r",ch);
		    break;
	    	case(WEAPON_WHIP):
		    send_to_char("whip\n\r",ch);
		    break;
	    	case(WEAPON_POLEARM):
		    send_to_char("polearm\n\r",ch);
		    break;
		case (WEAPON_STAFF):
		send_to_char("staff.\n\r",ch);
		break;
	    	default:
		    send_to_char("unknown\n\r",ch);
		    break;
 	    }
	    if (obj->pIndexData->new_format)
	    	sprintf(buf,"Damage is %ldd%ld (average %ld)\n\r",
		    obj->value[1],obj->value[2],
		    (1 + obj->value[2]) * obj->value[1] / 2);
	    else
	    	sprintf( buf, "Damage is %ld to %ld (average %ld)\n\r",
	    	    obj->value[1], obj->value[2],
	    	    ( obj->value[1] + obj->value[2] ) / 2 );
	    send_to_char( buf, ch );

	    sprintf(buf,"Damage noun is %s.\n\r",
		(obj->value[3] > 0 && obj->value[3] < MAX_DAMAGE_MESSAGE) ?
		    attack_table[obj->value[3]].noun : "undefined");
	    send_to_char(buf,ch);

	    if (obj->value[4])  /* weapon flags */
	    {
	        sprintf(buf,"Weapons flags: %s\n\r",
		    weapon_bit_name(obj->value[4]));
	        send_to_char(buf,ch);
            }
	break;

    	case ITEM_ARMOR:
	    sprintf( buf,
	    "Armor class is %ld pierce, %ld bash, %ld slash, and %ld vs. magic\n\r",
	        obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
	    send_to_char( buf, ch );
	break;

	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    sprintf(buf,"Original HP: %d  Owner: %s  Killed by: %s\n\r",
		obj->ohp,
		obj->owner,
		obj->talked);
	    send_to_char(buf,ch);
	break;
        case ITEM_CONTAINER:
            sprintf(buf,"Capacity: %ld#  Maximum weight: %ld#  flags: %s\n\r",
                obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
            send_to_char(buf,ch);
        break;
    }


    if ( obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL )
    {
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Extra description keywords: '", ch );

	for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next != NULL )
	    	send_to_char( " ", ch );
	}

	for ( ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next != NULL )
		send_to_char( " ", ch );
	}

	send_to_char( "'\n\r", ch );
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
	sprintf( buf, "Affects %s by %d, level %d",
	    affect_loc_name( paf->location ), paf->modifier,paf->level );
	send_to_char(buf,ch);
	if ( paf->duration > -1)
	    sprintf(buf,", %d hours.\n\r",paf->duration);
	else
	    sprintf(buf,".\n\r");
	send_to_char( buf, ch );
	if (paf->bitvector)
	{
	    switch(paf->where)
	    {
		case TO_AFFECTS:
		    sprintf(buf,"Adds %s affect.\n",
			affect_bit_name(paf->bitvector));
		    break;
                case TO_WEAPON:
                    sprintf(buf,"Adds %s weapon flags.\n",
                        weapon_bit_name(paf->bitvector));
		    break;
		case TO_OBJECT:
		    sprintf(buf,"Adds %s object flag.\n",
			extra_bit_name(paf->bitvector));
		    break;
		case TO_IMMUNE:
		    sprintf(buf,"Adds immunity to %s.\n",
			imm_bit_name(paf->bitvector));
		    break;
		case TO_RESIST:
		    sprintf(buf,"Adds resistance to %s.\n\r",
			imm_bit_name(paf->bitvector));
		    break;
		case TO_VULN:
		    sprintf(buf,"Adds vulnerability to %s.\n\r",
			imm_bit_name(paf->bitvector));
		    break;
		default:
		    sprintf(buf,"Unknown bit %d: %d\n\r",
			paf->where,paf->bitvector);
		    break;
	    }
	    send_to_char(buf,ch);
	}
    }

    if (!obj->enchanted)
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
	sprintf( buf, "Affects %s by %d, level %d.\n\r",
	    affect_loc_name( paf->location ), paf->modifier,paf->level );
	send_to_char( buf, ch );
        if (paf->bitvector)
        {
            switch(paf->where)
            {
                case TO_AFFECTS:
                    sprintf(buf,"Adds %s affect.\n",
                        affect_bit_name(paf->bitvector));
                    break;
                case TO_OBJECT:
                    sprintf(buf,"Adds %s object flag.\n",
                        extra_bit_name(paf->bitvector));
                    break;
                case TO_IMMUNE:
                    sprintf(buf,"Adds immunity to %s.\n",
                        imm_bit_name(paf->bitvector));
                    break;
                case TO_RESIST:
                    sprintf(buf,"Adds resistance to %s.\n\r",
                        imm_bit_name(paf->bitvector));
                    break;
                case TO_VULN:
                    sprintf(buf,"Adds vulnerability to %s.\n\r",
                        imm_bit_name(paf->bitvector));
                    break;
                default:
                    sprintf(buf,"Unknown bit %d: %d\n\r",
                        paf->where,paf->bitvector);
                    break;
            }
            send_to_char(buf,ch);
        }
    }
// test with iprog stuff begins here
if ( obj->pIndexData->progtypes != 0 )  {
    if ( IS_SET(obj->progtypes, IPROG_WEAR ) ) {
	sprintf(buf, "Item has wear prog: %s\n\r", obj->pIndexData->iprogs->wear_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_REMOVE ) ) {
	sprintf(buf, "Item has remove prog: %s\n\r", obj->pIndexData->iprogs->remove_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_GET ) ) {
	sprintf(buf, "Item has get prog: %s\n\r", obj->pIndexData->iprogs->get_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_DROP ) ) {
	sprintf(buf, "Item has drop prog: %s\n\r", obj->pIndexData->iprogs->drop_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_SAC ) ) {
	sprintf(buf, "Item has sacrifice prog: %s\n\r", obj->pIndexData->iprogs->sac_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_GIVE ) ) {
	sprintf(buf, "Item has give prog: %s\n\r", obj->pIndexData->iprogs->give_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_FIGHT ) ) {
	sprintf(buf, "Item has fight prog: %s\n\r", obj->pIndexData->iprogs->fight_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_DEATH ) ) {
	sprintf(buf, "Item has death prog: %s\n\r", obj->pIndexData->iprogs->death_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_SPEECH ) ) {
	sprintf(buf, "Item has speech prog: %s\n\r", obj->pIndexData->iprogs->speech_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_PULSE ) ) {
	sprintf(buf, "Item has pulse prog: %s\n\r", obj->pIndexData->iprogs->pulse_name);
	send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_ENTRY ) ) {
        sprintf(buf, "Item has entry prog: %s\n\r", obj->pIndexData->iprogs->entry_name);
        send_to_char( buf, ch );
    }    
    if ( IS_SET(obj->progtypes, IPROG_INVOKE) ) {
        sprintf(buf, "Item has invoke prog: %s\n\r", obj->pIndexData->iprogs->invoke_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(obj->progtypes, IPROG_GREET) ) {
        sprintf(buf, "Item has greet prog: %s\n\r", obj->pIndexData->iprogs->greet_name);
        send_to_char( buf, ch );
    }
}
/* Will add later for wear and remove echo..
if(obj->pIndexData->wear_echo[0]!=NULL)
{
	sprintf(buf,"Item echos to wearer: %s\n\r", obj->pIndexData->wear_echo[0]);
	send_to_char(buf,ch);
}
if(obj->pIndexData->wear_echo[1]!=NULL)
{
	sprintf(buf,"Item echos to room on wear: %s\n\r", obj->pIndexData->wear_echo[1]);
	send_to_char(buf,ch);
}
if(obj->pIndexData->remove_echo[0]!=NULL) 
{   
        sprintf(buf,"Item echos to remover: %s\n\r", obj->pIndexData->remove_echo[0]);
        send_to_char(buf,ch);
}   
if(obj->pIndexData->remove_echo[1]!=NULL)
{   
        sprintf(buf,"Item echos to room on remove: %s\n\r", obj->pIndexData->remove_echo[1]);
        send_to_char(buf,ch);
}
*/
// test with iprogs ends here.
    return;
}

void do_mstat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MSL];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    CHAR_DATA *victim;
    int gn, i;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Stat whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }


    sprintf( buf, "Name: %s\n\r",
	victim->name);
    send_to_char( buf, ch );


    if(victim->original_name) 
    {
        sprintf(buf, "OName: %s\n\r", victim->original_name);
	send_to_char(buf, ch);
    }

    if (checkScrollsAcumen(victim))
	send_to_char("PLAYER HAS ACUMEN SCROLLS!\n\r", ch);

    sprintf( buf,
        "Vnum: %ld Format: %s Race: %s Group: %d Sex: %s Room: %ld\n\r",
	IS_NPC(victim) ? victim->pIndexData->vnum : 0,
	IS_NPC(victim) ? victim->pIndexData->new_format ? "new" : "old" : "pc",
	race_table[victim->race].name,
	IS_NPC(victim) ? victim->group : 0, sex_table[victim->sex].name,
	victim->in_room == NULL    ?        0 : victim->in_room->vnum
	);
    send_to_char( buf, ch );

    if (IS_NPC(victim))
    {
	sprintf(buf,"Count: %d  Killed: %d\n\r",
	    victim->pIndexData->count,victim->pIndexData->killed);
	send_to_char(buf,ch);
    }


    sprintf( buf,
        "Str: %d(%d) Int: %d(%d) Wis: %d(%d) Dex: %d(%d) Con: %d(%d)\n\r",
	victim->perm_stat[STAT_STR],
	get_curr_stat(victim,STAT_STR),
	victim->perm_stat[STAT_INT],
	get_curr_stat(victim,STAT_INT),
	victim->perm_stat[STAT_WIS],
	get_curr_stat(victim,STAT_WIS),
	victim->perm_stat[STAT_DEX],
	get_curr_stat(victim,STAT_DEX),
	victim->perm_stat[STAT_CON],
	get_curr_stat(victim,STAT_CON) );
    send_to_char( buf, ch );


    sprintf( buf, "Hp: %d/%d  mana: %d/%d  Move: %d/%d Practices: %d\n\r",
	victim->hit,         victim->max_hit,
    victim->mana,        victim->max_mana,
	victim->move,        victim->max_move,
	IS_NPC(ch) ? 0 : victim->practice );
    send_to_char( buf, ch );


    sprintf( buf,
        "Lv: %d (%d) Class: %s  Align: %d Gold: %ld Quest Credits: %ld\n\r",
	victim->level,victim->level + victim->drain_level,
	IS_NPC(victim) ? "mobile" : class_table[victim->class].name,
	victim->alignment,
	victim->gold, victim->quest_credits );
    send_to_char( buf, ch );


        if (!IS_NPC(victim))
        {
		printf_to_char(ch, "Bounty Worth: %ld  Bounty Credits: %d  Wanteds %d\n\r",
			victim->pcdata->bounty, victim->pcdata->bounty_credits, victim->pcdata->wanteds);
        }

    if (!IS_NPC(victim)) {
	sprintf(buf,"Hometown: %s, ",hometown_table[victim->hometown].name);
    	send_to_char(buf,ch);
    }

    if (!IS_NPC(victim))
    {

        sprintf(buf, "Ethos: %d ", victim->pcdata->ethos);
	send_to_char(buf, ch);
    }
    if (!IS_NPC(victim))
    {

        sprintf(buf,"Death count: %d\n\r", victim->pcdata->death_count);
	send_to_char(buf,ch);
    }

	if (!IS_NPC(victim) && victim->pcdata->dedication > 0)
	{
		if (victim->pcdata->dedication == DED_TWOHAND)
			send_to_char("Dedication: two-handed sword.\n\r",ch);
		else if (victim->pcdata->dedication == DED_SHIELD)
			send_to_char("Dedication: shield.\n\r",ch);
	}

	if (!IS_NPC(victim))
	{
	sprintf(buf,"%d Specialization(s): ",victim->pcdata->special);
	send_to_char(buf,ch);
	gn=0;
        for (i = 0; i <= MAX_SPECS; i++) {
		if (victim->pcdata->warrior_specs[i] > 0 && victim->pcdata->warrior_specs[i] <= MAX_WEAPON) {
			send_to_char(weapon_name_lookup(victim->pcdata->warrior_specs[i]),ch);
			send_to_char(". ",ch);
			gn++;
		}
	}

	if (gn==0) {
		send_to_char("(none)",ch);
	}
	send_to_char("\n\r",ch);
}

    sprintf(buf,"Armor: pierce: %d (%.1f%%)  bash: %d (%.1f%%) slash: %d (%.1f%%) magic: %d (%.1f%%) Dam_mod: {R%d%% (%d%%){x\n\r",
	    GET_AC(victim,AC_PIERCE),
		calculate_ac_redux(GET_AC(victim,AC_PIERCE)) * 100,
		GET_AC(victim,AC_BASH),
		calculate_ac_redux(GET_AC(victim,AC_BASH)) * 100,
	    GET_AC(victim,AC_SLASH),  
		calculate_ac_redux(GET_AC(victim,AC_SLASH)) * 100,
		GET_AC(victim,AC_EXOTIC), 
		calculate_ac_redux(GET_AC(victim,AC_EXOTIC)) * 100,
		victim->base_dam_mod, victim->dam_mod);
    send_to_char(buf,ch);

    	sprintf( buf,
        "Hit: {R%d(%d)  {xDam: {R%d(%d){x Attack_mod: {R%d(%d){x EnhancedDam Mod: {R%.0f%%(%.0f%%){x\n\rRegen: {R%d(%d){x Saves: {R%d{x\n\rSize: %s (%d) Position: %s Wimpy: %d\n\r",
	GET_HITROLL(victim), victim->hitroll, GET_DAMROLL(victim), victim->damroll, victim->base_numAttacks, victim->numAttacks, victim->base_enhancedDamMod, victim->enhancedDamMod, victim->base_regen, victim->regen_rate, victim->saving_throw,
	size_table[victim->size].name, victim->size, position_table[victim->position].name,
	victim->wimpy );

    send_to_char( buf, ch );
	
	sprintf( buf, "Material: %s\n\r", victim->material ? victim->material : "not specified" );
	send_to_char( buf, ch );

    if (!IS_NPC(victim) && victim->cabal)
    {
	sprintf(buf,"Cabal: %s  Leader: %s\n\r",str_dup(capitalize(cabal_table[victim->cabal].name)),
		victim->pcdata->induct == CABAL_LEADER ? "yes" : "no");
	send_to_char(buf,ch);
    }

    if (IS_NPC(victim) && victim->pIndexData->new_format)
    {
	sprintf(buf, "Damage: %dd%d  Message:  %s\n\r",
	    victim->damage[DICE_NUMBER],victim->damage[DICE_TYPE],
	    attack_table[victim->dam_type].noun);
	send_to_char(buf,ch);
    }

    sprintf( buf, "Fighting: %s  Hunting: %s  Carry number: %d  Carry weight: %ld\n\r",
	victim->fighting ? victim->fighting->name : "(none)",
	(IS_NPC(victim) && victim->hunting) ? victim->hunting->name : "(none)",
	victim->carry_number, get_carry_weight(victim) / 10 );
    send_to_char( buf, ch );


    if (!IS_NPC(victim))
    {

    	sprintf( buf,
            "Age: %d  Played: %d  Ghost: %d  Drain Level: %d\n\r",
	    get_age(victim),
	    (int) (victim->played + current_time - victim->logon) / 3600,
	    victim->ghost,
	    victim->drain_level );
    	send_to_char( buf, ch );

	sprintf(buf,
"New age: %d (%s), Death_age: %d (age_mod is %d).\n\r",
get_age(victim),get_age_name(victim),get_death_age(victim),
victim->pcdata->age_mod);
    	send_to_char( buf, ch );
    }

    sprintf(buf, "Act: %s\n\r",act_bit_name(victim->act));
    send_to_char(buf,ch);

    if (victim->comm)
    {

        sprintf(buf,"Comm: %s\n\r",comm_bit_name(victim->comm));
    	send_to_char(buf,ch);
    }

    if (IS_NPC(victim) && victim->off_flags)
    {

        sprintf(buf, "Offense: %s\n\r",off_bit_name(victim->off_flags));
	send_to_char(buf,ch);
    }

    if (IS_NPC(victim) && victim->extended_flags)
    {
	sprintf(buf, "Extended: %s\n\r", flag_string(extended_flags, victim->extended_flags));
	send_to_char(buf, ch);
    }

    if (victim->imm_flags)
    {

        sprintf(buf, "Immune: %s\n\r",imm_bit_name(victim->imm_flags));
	send_to_char(buf,ch);
    }

    if (victim->res_flags)
    {

        sprintf(buf, "Resist: %s\n\r", imm_bit_name(victim->res_flags));
	send_to_char(buf,ch);
    }

    if (victim->vuln_flags)
    {

        sprintf(buf, "Vulnerable: %s\n\r", imm_bit_name(victim->vuln_flags));
	send_to_char(buf,ch);
    }


    sprintf(buf, "Form: %s\n\rParts: %s\n\r",
	form_bit_name(victim->form), part_bit_name(victim->parts));
    send_to_char(buf,ch);

        sprintf(buf, "Affected by: %s\n\r", affect_bit_name(victim->affected_by));
	send_to_char(buf,ch);

    sprintf( buf, "Master: %s  Leader: %s\n\r",
	victim->master      ? victim->master->name   : "(none)",
	victim->leader      ? victim->leader->name   : "(none)");
    send_to_char( buf, ch );

	if (!IS_NPC(victim))
	{
	sprintf(buf,"{RPKills:{x {W%d{x  {RPKDeaths:{x {W%d{x  {RMobDeaths:{x {W%d{x\n\r",
	victim->pcdata->kills[PK_KILLS],
	victim->pcdata->killed[PK_KILLED],
	victim->pcdata->killed[MOB_KILLED]);
	send_to_char( buf, ch );
	}

	if (!IS_NPC(victim))
    	{
    	sprintf( buf, "Security: %d.\n\r", victim->pcdata->security );
    	send_to_char( buf, ch );
    	}

	if (!IS_NPC(victim))
	{
		sprintf(buf, "Pfile Version: %d.\n\r",victim->version );
		send_to_char( buf, ch);
	}

	if (!IS_NPC(victim))
	{
    	sprintf(buf, "{BLast PC fought: {W%s{B,{W %d {Bseconds ago.{x\n\r",
	victim->last_fight_name != NULL ? victim->last_fight_name :"(none)",
	victim->last_fight_time ? (int)(current_time -
	victim->last_fight_time) : -1);
	send_to_char(buf,ch);
	}

    sprintf( buf, "Short description: %s\n\rLong description: %s",
	victim->short_descr,
	victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\n\r" );
    send_to_char( buf, ch );

    if ( IS_NPC(victim) && victim->spec_fun != 0 )
    {
	sprintf(buf,"Mobile has special procedure %s.\n\r",
		spec_name(victim->spec_fun));
	send_to_char(buf,ch);
    }

    if (IS_NPC(victim) && victim->pIndexData->progtypes != 0 )  {
    if ( IS_SET(victim->progtypes, MPROG_ATTACK ) ) {
        sprintf(buf, "Mobile has attack prog: %s\n\r", victim->pIndexData->mprogs->attack_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_BRIBE ) ) {
        sprintf(buf, "Mobile has bribe prog: %s\n\r", victim->pIndexData->mprogs->bribe_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_ENTRY ) ) {
        sprintf(buf, "Mobile has entry prog: %s\n\r", victim->pIndexData->mprogs->entry_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_GREET ) ) {
        sprintf(buf, "Mobile has greet prog: %s\n\r", victim->pIndexData->mprogs->greet_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_GIVE ) ) {
        sprintf(buf, "Mobile has give prog: %s\n\r", victim->pIndexData->mprogs->give_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_FIGHT ) ) {
        sprintf(buf, "Mobile has fight prog: %s\n\r", victim->pIndexData->mprogs->fight_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_DEATH ) ) {
        sprintf(buf, "Mobile has death prog: %s\n\r", victim->pIndexData->mprogs->death_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_PULSE ) ) {
        sprintf(buf, "Mobile has pulse prog: %s\n\r", victim->pIndexData->mprogs->pulse_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_SPEECH ) ) {
        sprintf(buf, "Mobile has speech prog: %s\n\r", victim->pIndexData->mprogs->speech_name);
        send_to_char( buf, ch );
    }
    if ( IS_SET(victim->progtypes, MPROG_MOVE) ) {
        sprintf(buf, "Mobile has movement prog: %s\n\r", victim->pIndexData->mprogs->move_name);
        send_to_char( buf, ch );
    }
    }

/* tracking stuff */
    if ( IS_NPC(victim) && victim->last_fought != NULL)

        sprintf(buf,"TRACKING: Player %s.\n\r",victim->last_fought->name);
    else

        sprintf(buf,"TRACKING: Not tracking.\n\r");

    send_to_char(buf,ch);


    for ( paf = victim->affected; paf != NULL; paf = paf->next )
    {
	if (paf->aftype == AFT_SPELL)
		sprintf( buf2, "Spell");
	else if (paf->aftype == AFT_SKILL)
	    	sprintf( buf2, "Skill");
	else if (paf->aftype == AFT_POWER)
	    	sprintf( buf2, "Power");
	else if (paf->aftype == AFT_MALADY)
	    	sprintf( buf2, "Malady");
	else if (paf->aftype == AFT_COMMUNE)
	    	sprintf( buf2, "Commune");
	else if (paf->aftype == AFT_INVIS)
		sprintf( buf2, "Invis");
	else
 		sprintf( buf2, "Uknown");
	sprintf( buf, "%s: '%s' %s%s%s",buf2,
	skill_table[(int) paf->type].name,
	paf->name ? "(" : "", paf->name ? paf->name : "", paf->name ? ") " : "");
	send_to_char(buf,ch);
	if (paf->affect_list_msg != NULL && str_cmp(paf->affect_list_msg,"(null)"))
	{
			sprintf( buf, " %s ", paf->affect_list_msg);
			send_to_char(buf,ch);
	}
	else if (paf->where == TO_IMMUNE || paf->where == TO_RESIST || paf->where == TO_VULN)
	{
		sprintf( buf, ": %s %s attacks ", paf->where == TO_IMMUNE ? "grants immunity to" : paf->where == TO_RESIST ? "grants resistance to" : "induces a vulnerability to", paf->where == TO_IMMUNE || paf->where == TO_RESIST || paf->where == TO_VULN ? imm_bit_name(paf->bitvector) :
			affect_bit_name( paf->bitvector ));
		send_to_char(buf,ch);
	}
	else
	{	
		sprintf( buf,
		 ": modifies %s by %d ",
		 affect_loc_name( paf->location ),
		 paf->type == gsn_ward_diminution ? 0 : paf->modifier);
		send_to_char(buf,ch);
	}

	sprintf( buf,
	"for %d hours with %s-bits %s, owner %s, level %d.\n\r",
	paf->duration, paf->where == TO_IMMUNE ? "imm" : paf->where == TO_RESIST ? "res" : paf->where == TO_VULN ? "vuln" : "aff",
	paf->where == TO_IMMUNE || paf->where == TO_RESIST || paf->where == TO_VULN ? imm_bit_name(paf->bitvector) :
	affect_bit_name( paf->bitvector ),
	paf->owner_name != NULL ? paf->owner_name : "none",
	paf->level
	);
	send_to_char( buf, ch );
    }
    return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;

    string = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  vnum obj <name>\n\r",ch);
	send_to_char("  vnum mob <name>\n\r",ch);
	send_to_char("  vnum skill <skill or spell>\n\r",ch);
	return;
    }

    if (!str_cmp(arg,"obj"))
    {
	do_ofind(ch,string);
 	return;
    }

    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    {
	do_mfind(ch,string);
	return;
    }

    if (!str_cmp(arg,"skill") || !str_cmp(arg,"spell"))
    {
	do_slookup(ch,string);
	return;
    }
    /* do both */
    do_mfind(ch,argument);
    do_ofind(ch,argument);
}


void do_mfind( CHAR_DATA *ch, char *argument )
{
    extern long top_mob_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    long vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Find whom?\n\r", ch );
	return;
    }

    fAll	= FALSE; /* !str_cmp( arg, "all" ); */
    found	= FALSE;
    nMatch	= 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_mob_index; vnum++ )
    {
	if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
	{
	    nMatch++;
	    if ( fAll || is_name( argument, pMobIndex->player_name ) )
	    {
		found = TRUE;
		sprintf( buf, "[%5ld] - %s\n\r",
		    pMobIndex->vnum, pMobIndex->short_descr );
		send_to_char( buf, ch );
	    }
	}
    }

    if ( !found )
	send_to_char( "No mobiles by that name.\n\r", ch );

    return;
}



void do_ofind( CHAR_DATA *ch, char *argument )
{
    extern long top_obj_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    long vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Find what?\n\r", ch );
	return;
    }

    fAll	= FALSE; /* !str_cmp( arg, "all" ); */
    found	= FALSE;
    nMatch	= 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
    {
	if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
	{
	    nMatch++;
	    if ( fAll || is_name( argument, pObjIndex->name ) )
	    {
		found = TRUE;
		sprintf( buf, "[%5ld] - %s %s\n\r",
		    pObjIndex->vnum, pObjIndex->short_descr ,
		    is_set(&pObjIndex->extra_flags,ITEM_HIDDEN) ? "(HIDDEN)" : "" );
		send_to_char( buf, ch );
	    }
	}
    }

    if ( !found )
	send_to_char( "No objects by that name.\n\r", ch );

    return;
}


void do_owhere(CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 200;

    buffer = new_buf();

    if (argument[0] == '\0')
    {
	send_to_char("Find what?\n\r",ch);
	return;
    }

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( argument, obj->name )
        ||   ch->level < obj->level)
            continue;

        found = TRUE;
        number++;

        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;

        if ( in_obj->carried_by != NULL && can_see(ch,in_obj->carried_by)
	&&   in_obj->carried_by->in_room != NULL)
            sprintf( buf, "%3d) %s is carried by %s [Room %ld]\n\r",
                number, obj->short_descr,PERS(in_obj->carried_by, ch),
		in_obj->carried_by->in_room->vnum );
        else if (in_obj->in_room != NULL && can_see_room(ch,in_obj->in_room))
            sprintf( buf, "%3d) %s is in %s [Room %ld]\n\r",
                number, obj->short_descr,in_obj->in_room->name,
	   	in_obj->in_room->vnum);
	else
            sprintf( buf, "%3d) %s is somewhere\n\r",number, obj->short_descr);

        buf[0] = UPPER(buf[0]);
        add_buf(buffer,buf);

        if (number >= max_found)
            break;
    }

    if ( !found )
        send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
    else
        page_to_char(buf_string(buffer),ch);

    free_buf(buffer);
}


void do_mwhere( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    CHAR_DATA *victim;
    bool found;
    int count = 0;

    if ( argument[0] == '\0' )
    {
	DESCRIPTOR_DATA *d;

	/* show characters logged */

	buffer = new_buf();
	for (d = descriptor_list; d != NULL; d = d->next)
	{
	    if (d->character != NULL && d->connected == CON_PLAYING
	    &&  d->character->in_room != NULL && can_see(ch,d->character)
	    &&  can_see_room(ch,d->character->in_room))
	    {
		victim = d->character;
		count++;
		if (d->original != NULL)
		    sprintf(buf,"%3d) %s (in the body of %s) is in %s [%ld]\n\r",
			count, d->original->name,victim->short_descr,
			victim->in_room->name,victim->in_room->vnum);
		else
		    sprintf(buf,"%3d) %s is in %s [%ld]\n\r",
			count, victim->original_name,victim->in_room->name,
			victim->in_room->vnum);
		add_buf(buffer,buf);
	    }
	}

        page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
	return;
    }

    found = FALSE;
    buffer = new_buf();
    for ( victim = char_list; victim != NULL; victim = victim->next )
    {
	if ( victim->in_room != NULL
	&&   can_see(ch,victim)
	&&   is_name( argument, IS_NPC(victim) ? victim->name : victim->original_name ) )
	{
	    found = TRUE;
	    count++;
	    sprintf( buf, "%3d) [%5ld] %-28s [%5ld] %s\n\r", count,
		IS_NPC(victim) ? victim->pIndexData->vnum : 0,
		IS_NPC(victim) ? victim->short_descr : victim->name,
		victim->in_room->vnum,
		victim->in_room->name );
	    add_buf(buffer,buf);
	}
    }

    if ( !found )
	act( "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
    else
    	page_to_char(buf_string(buffer),ch);

    free_buf(buffer);

    return;
}



void do_reboo( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to REBOOT, spell it out.\n\r", ch );
    send_to_char( "Use: 'reboot nosave',if you don't want players saved at reboot.\n\r",ch);
    return;
}



void do_reboot( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    CHAR_DATA *vch;
    char arg[MAX_INPUT_LENGTH];
    one_argument(arg,argument);

    if (str_cmp(arg,"nosave"))
    {
	sprintf( buf, "Rebooting now!");
	do_echo( ch, buf );
    }

    merc_down = TRUE;
    do_asave(ch,"world");
    do_restore(ch,"all");
    do_force(ch, "all save");
    do_save(ch, "");
    for ( d = descriptor_list; d != NULL; d = d_next )
    {
	d_next = d->next;
	vch = d->original ? d->original : d->character;
	if (vch != NULL && d->connected == 0) 
	{
	    if (is_affected(vch,skill_lookup("channel"))){
		affect_strip(vch,skill_lookup("channel"));
            }

            if (is_affected(vch,skill_lookup("defiance"))){
            	affect_strip(vch,skill_lookup("defiance"));
            }

	    save_char_obj(vch);
	}
    	close_socket(d);
    }
     
    close_db();
    merc_down = TRUE;
    return;
}



void do_shutdow( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SHUTDOWN, spell it out.\n\r", ch );
    return;
}



void do_shutdown( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    CHAR_DATA *vch;

    sprintf( buf, "Shutting down now!");
    append_file( ch, SHUTDOWN_FILE, buf );
    strcat( buf, "\n\r" );
    if (ch->invis_level < LEVEL_HERO)
    	do_echo( ch, buf );
    merc_down = TRUE;
    for ( d = descriptor_list; d != NULL; d = d_next)
    {
	d_next = d->next;
	vch = d->original ? d->original : d->character;
	if (vch != NULL && d->connected == 0)
	    save_char_obj(vch);
		free_char(vch);
	close_socket(d);
    }
	
	if ( fpReserve )
		fclose( fpReserve );

    close_db();
    return;
}

void do_protect( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;

    if (argument[0] == '\0')
    {
	send_to_char("Protect whom from snooping?\n\r",ch);
	return;
    }

    if ((victim = get_char_world(ch,argument)) == NULL)
    {
	send_to_char("You can't find them.\n\r",ch);
	return;
    }

    if (IS_SET(victim->comm,COMM_SNOOP_PROOF))
    {
	act_new("$N is no longer snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
	send_to_char("Your snoop-proofing was just removed.\n\r",victim);
	REMOVE_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
    else
    {
	act_new("$N is now snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
	send_to_char("You are now immune to snooping.\n\r",victim);
	SET_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
}



void do_snoop( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Snoop whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim->desc == NULL )
    {
	send_to_char( "No descriptor to snoop.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Cancelling all snoops.\n\r", ch );
	wiznet("$N stops being such a snoop.",
		ch,NULL,WIZ_SNOOPS,WIZ_SECURE,get_trust(ch));
	for ( d = descriptor_list; d != NULL; d = d->next )
	{
	    if ( d->snoop_by == ch->desc )
		d->snoop_by = NULL;
	}
	return;
    }

    if ( victim->desc->snoop_by != NULL )
    {
	send_to_char( "Busy already.\n\r", ch );
	return;
    }

    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r",ch);
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch )
    ||   IS_SET(victim->comm,COMM_SNOOP_PROOF))
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( ch->desc != NULL )
    {
	for ( d = ch->desc->snoop_by; d != NULL; d = d->snoop_by )
	{
	    if ( d->character == victim || d->original == victim )
	    {
		send_to_char( "No snoop loops.\n\r", ch );
		return;
	    }
	if (strcmp(arg, "imm"))
	continue;
	}
    }

    victim->desc->snoop_by = ch->desc;
    sprintf(buf,"$N starts snooping on %s",
	(IS_NPC(ch) ? victim->short_descr : victim->name));
    wiznet(buf,ch,NULL,WIZ_SNOOPS,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}



void do_switch( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Switch into whom?\n\r", ch );
	return;
    }

    if ( ch->desc == NULL )
	return;

    if ( ch->desc->original != NULL )
    {
	send_to_char( "You are already switched.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Ok.\n\r", ch );
	return;
    }

    if (!IS_NPC(victim))
    {
	send_to_char("You can only switch into mobiles.\n\r",ch);
	return;
    }

    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
	send_to_char("That character is in a private room.\n\r",ch);
	return;
    }

    if ( victim->desc != NULL )
    {
	send_to_char( "Character in use.\n\r", ch );
	return;
    }

    sprintf(buf,"$N switches into %s",victim->short_descr);
    wiznet(buf,ch,NULL,WIZ_SWITCHES,WIZ_SECURE,get_trust(ch));

    ch->desc->character = victim;
    ch->desc->original  = ch;
    victim->desc        = ch->desc;
    ch->desc            = NULL;
    /* change communications to match */
    if (ch->prompt != NULL)
        victim->prompt = str_dup(ch->prompt);
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    send_to_char( "Ok.\n\r", victim );
    return;
}



void do_return( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( ch->desc == NULL )
	return;

    if ( ch->desc->original == NULL )
    {
	send_to_char( "You aren't switched.\n\r", ch );
	return;
    }

    send_to_char(
"You return to your original body. Type replay to see any missed tells.\n\r",
	ch );
    if (ch->prompt != NULL)
    {
	free_string(ch->prompt);
	ch->prompt = NULL;
    }

    sprintf(buf,"$N returns from %s.",ch->short_descr);
    wiznet(buf,ch->desc->original,0,WIZ_SWITCHES,WIZ_SECURE,get_trust(ch));
    ch->desc->character       = ch->desc->original;
    ch->desc->original        = NULL;
    ch->desc->character->desc = ch->desc;
    ch->desc                  = NULL;
    return;
}

/* trust levels for load and clone */
bool obj_check (CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (IS_TRUSTED(ch,GOD)
	|| (IS_TRUSTED(ch,IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
	|| (IS_TRUSTED(ch,DEMI)	    && obj->level <= 10 && obj->cost <= 500)
	|| (IS_TRUSTED(ch,ANGEL)    && obj->level <=  5 && obj->cost <= 250)
	|| (IS_TRUSTED(ch,AVATAR)   && obj->level ==  0 && obj->cost <= 100))
	return TRUE;
    else
	return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone)
{
    OBJ_DATA *c_obj, *t_obj;


    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
    {
	if (obj_check(ch,c_obj))
	{
	    t_obj = create_object(c_obj->pIndexData,0);
	    clone_object(c_obj,t_obj);
	    obj_to_obj(t_obj,clone);
	    recursive_clone(ch,c_obj,t_obj);
	}
    }
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char *rest;
    CHAR_DATA *mob;
    OBJ_DATA  *obj;

    rest = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Clone what?\n\r",ch);
	return;
    }

    if (!str_prefix(arg,"object"))
    {
	mob = NULL;
	obj = get_obj_here(ch,rest);
	if (obj == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }
    else if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
	obj = NULL;
	mob = get_char_room(ch,rest);
	if (mob == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }
    else /* find both */
    {
	mob = get_char_room(ch,argument);
	obj = get_obj_here(ch,argument);
	if (mob == NULL && obj == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }

    /* clone an object */
    if (obj != NULL)
    {
	OBJ_DATA *clone;

	if (!obj_check(ch,obj))
	{
	    send_to_char(
		"Your powers are not great enough for such a task.\n\r",ch);
	    return;
	}

	clone = create_object(obj->pIndexData,0);
	clone_object(obj,clone);
	if (obj->carried_by != NULL)
	    obj_to_char(clone,ch);
	else
	    obj_to_room(clone,ch->in_room);
 	recursive_clone(ch,obj,clone);

	act("$n has created $p.",ch,clone,NULL,TO_ROOM);
	act("You clone $p.",ch,clone,NULL,TO_CHAR);
	wiznet("$N clones $p.",ch,clone,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
	return;
    }
    else if (mob != NULL)
    {
	CHAR_DATA *clone;
	OBJ_DATA *new_obj;
	char buf[MAX_STRING_LENGTH];

	if (!IS_NPC(mob))
	{
	    send_to_char("You can only clone mobiles.\n\r",ch);
	    return;
	}

	if ((mob->level > 20 && !IS_TRUSTED(ch,GOD))
	||  (mob->level > 10 && !IS_TRUSTED(ch,IMMORTAL))
	||  (mob->level >  5 && !IS_TRUSTED(ch,DEMI))
	||  (mob->level >  0 && !IS_TRUSTED(ch,ANGEL))
	||  !IS_TRUSTED(ch,AVATAR))
	{
	    send_to_char(
		"Your powers are not great enough for such a task.\n\r",ch);
	    return;
	}

	clone = create_mobile(mob->pIndexData);
	clone_mobile(mob,clone);

	for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
	{
	    if (obj_check(ch,obj))
	    {
		new_obj = create_object(obj->pIndexData,0);
		clone_object(obj,new_obj);
		recursive_clone(ch,obj,new_obj);
		obj_to_char(new_obj,clone);
		new_obj->wear_loc = obj->wear_loc;
	    }
	}
	char_to_room(clone,ch->in_room);
        act("$n has created $N.",ch,NULL,clone,TO_ROOM);
        act("You clone $N.",ch,NULL,clone,TO_CHAR);
	sprintf(buf,"$N clones %s.",clone->short_descr);
	wiznet(buf,ch,NULL,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
        return;
    }
}

/* RT to replace the two load commands */

void do_load(CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
    log_naughty(ch,argument,1);
    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  load mob <vnum>\n\r",ch);
	send_to_char("  load obj <vnum> <level>\n\r",ch);
	return;
    }

    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    {
	do_mload(ch,argument);
	return;
    }

    if (!str_cmp(arg,"obj"))
    {
	do_oload(ch,argument);
	return;
    }
    /* echo syntax */
    do_load(ch,"");
}


void do_mload( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg );

    if ( arg[0] == '\0' || !is_number(arg) )
    {
	send_to_char( "Syntax: load mob <vnum>.\n\r", ch );
	return;
    }

    if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL )
    {
	send_to_char( "No mob has that vnum.\n\r", ch );
	return;
    }

    victim = create_mobile( pMobIndex );
    char_to_room( victim, ch->in_room );
    act( "$n has created $N!", ch, NULL, victim, TO_ROOM );
    sprintf(buf,"$N loads %s.",victim->short_descr);
    wiznet(buf,ch,NULL,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}



void do_oload( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    int level;

    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || !is_number(arg1))
    {
	send_to_char( "Syntax: load obj <vnum> <level>.\n\r", ch );
	return;
    }

    level = get_trust(ch); /* default */

    if ( arg2[0] != '\0')  /* load with a level */
    {
	if (!is_number(arg2))
        {
	  send_to_char( "Syntax: oload <vnum> <level>.\n\r", ch );
	  return;
	}
        level = atoi(arg2);
        if (level < 0 || level > get_trust(ch))
	{
	  send_to_char( "Level must be be between 0 and your level.\n\r",ch);
  	  return;
	}
    }

    if ( ( pObjIndex = get_obj_index( atoi( arg1 ) ) ) == NULL )
    {
	send_to_char( "No object has that vnum.\n\r", ch );
	return;
    }

    obj = create_object( pObjIndex, level );
    if ( CAN_WEAR(obj, ITEM_TAKE) )
	obj_to_char( obj, ch );
    else
	obj_to_room( obj, ch->in_room );
    act( "$n has created $p!", ch, obj, NULL, TO_ROOM );
    wiznet("$N loads $p.",ch,obj,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}



void do_purge( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    DESCRIPTOR_DATA *d;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	/* 'purge' */
	CHAR_DATA *vnext;
	OBJ_DATA  *obj_next;

	for ( victim = ch->in_room->people; victim != NULL; victim = vnext )
	{
	    vnext = victim->next_in_room;
	    if ( IS_NPC(victim) && !IS_SET(victim->act,ACT_NOPURGE)
	    &&   victim != ch /* safety precaution */ && !IS_AFFECTED(victim,AFF_CHARM) )
		extract_char( victim, TRUE );
	}

	for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if (!IS_OBJ_STAT(obj,ITEM_NOPURGE))
	      extract_obj( obj );
	}

	act( "$n purges the room!", ch, NULL, NULL, TO_ROOM);
	send_to_char( "Ok.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( !IS_NPC(victim) )
    {

	if (ch == victim)
	{
	  send_to_char("Ho ho ho.\n\r",ch);
	  return;
	}

	if (get_trust(ch) <= get_trust(victim))
	{
	  send_to_char("Maybe that wasn't a good idea...\n\r",ch);
	  sprintf(buf,"%s tried to purge you!\n\r",ch->name);
	  send_to_char(buf,victim);
	  return;
	}

	act("$n disintegrates $N.",ch,0,victim,TO_NOTVICT);

    	if (victim->level > 1)
	    save_char_obj( victim );
    	d = victim->desc;
    	extract_char( victim, TRUE );
    	if ( d != NULL )
          close_socket( d );

	return;
    }

    act( "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    extract_char( victim, TRUE );
    return;
}



void do_advance( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;
    int iLevel;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
	send_to_char( "Syntax: advance <char> <level>.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( ( level = atoi( arg2 ) ) < 1 || level > 60 )
    {
	send_to_char( "Level must be 1 to 60.\n\r", ch );
	return;
    }

    if ( level > get_trust( ch ) )
    {
	send_to_char( "Limited to your trust level.\n\r", ch );
	return;
    }

    /*
     * Lower level:
     *   Reset to level 1.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if ( level <= victim->level )
    {
        int temp_prac;

	send_to_char( "You lower their level!\n\r", ch );
	send_to_char( "{b****{x OOOOHHHHHHHHHH  NNNNOOOO {b****{x\n\r", victim );
	temp_prac = victim->practice;
	victim->level    = 1;
	victim->exp_total = 1;
        victim->exp      = exp_per_level(victim);
	victim->max_hit  = 10;
    victim->max_mana = 100;
	victim->max_move = 100;
	victim->pcdata->perm_hit = 10;
    victim->pcdata->perm_mana = 100;
	victim->pcdata->perm_move = 100;
	victim->practice = 0;
	victim->train 	 = 0;
	victim->hit      = victim->max_hit;
    victim->mana     = victim->max_mana;
	victim->move     = victim->max_move;
	advance_level( victim, TRUE );
    }
    else
    {
	send_to_char( "You raise their level!\n\r", ch );
	send_to_char( "{r****{x OOOOHHHHHHHHHH  YYYYEEEESSS {r****{x\n\r", victim );
    }

    for ( iLevel = victim->level ; iLevel < level; iLevel++ )
    {
	victim->level += 1;
	advance_level( victim,TRUE);
    }
    sprintf(buf,"You are now level %d.\n\r",victim->level);
    send_to_char(buf,victim);
    victim->exp   = exp_per_level(victim)
		  * UMAX( 1, victim->level );
    save_char_obj(victim);
    return;
}



void do_trust( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
	send_to_char( "Syntax: trust <char> <level>.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( ( level = atoi( arg2 ) ) < 0 || level > 60 )
    {
	send_to_char( "Level must be 0 (reset) or 1 to 60.\n\r", ch );
	return;
    }

    if ( level > get_trust( ch ) )
    {
	send_to_char( "Limited to your trust.\n\r", ch );
	return;
    }

    victim->trust = level;
    return;
}



void do_restore( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    DESCRIPTOR_DATA *d;

    one_argument( argument, arg );
    if (arg[0] == '\0' || !str_cmp(arg,"room"))
    {
    /* cure room */

        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            affect_strip(vch,gsn_plague);
            affect_strip(vch,gsn_poison);
            affect_strip(vch,gsn_blindness);
            affect_strip(vch,gsn_sleep);
            affect_strip(vch,gsn_curse);

            vch->hit 	= vch->max_hit;
            vch->mana   = vch->max_mana;
            vch->move	= vch->max_move;
            update_pos( vch);
            act("$n has restored you.",ch,NULL,vch,TO_VICT);
        }

        sprintf(buf,"$N restored room %ld.",ch->in_room->vnum);
        wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));

        send_to_char("Room restored.\n\r",ch);
        return;

    }

    if ( get_trust(ch) >=  MAX_LEVEL - 2 && !str_cmp(arg,"all"))
    {
    /* cure all */

        for (d = descriptor_list; d != NULL; d = d->next)
        {
	    victim = d->character;

	    if (victim == NULL || IS_NPC(victim))
		continue;

            affect_strip(victim,gsn_plague);
            affect_strip(victim,gsn_poison);
            affect_strip(victim,gsn_blindness);
            affect_strip(victim,gsn_sleep);
            affect_strip(victim,gsn_curse);

            victim->hit 	= victim->max_hit;
            victim->mana    = victim->max_mana;
            victim->move	= victim->max_move;
            update_pos( victim);
	    if (victim->in_room != NULL)
                act("$n has restored you.",ch,NULL,victim,TO_VICT);
        }
	send_to_char("All active players restored.\n\r",ch);
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    affect_strip(victim,gsn_plague);
    affect_strip(victim,gsn_poison);
    affect_strip(victim,gsn_blindness);
    affect_strip(victim,gsn_sleep);
    affect_strip(victim,gsn_curse);
    victim->hit  = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    update_pos( victim );
    act( "$n has restored you.", ch, NULL, victim, TO_VICT );
    sprintf(buf,"$N restored %s",
	IS_NPC(victim) ? victim->short_descr : victim->name);
    wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}


void do_freeze( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Freeze whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->act, PLR_FREEZE) )
    {
	REMOVE_BIT(victim->act, PLR_FREEZE);
	send_to_char( "You can play again.\n\r", victim );
	send_to_char( "FREEZE removed.\n\r", ch );
	sprintf(buf,"$N thaws %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->act, PLR_FREEZE);
	send_to_char( "You can't do ANYthing!\n\r", victim );
	send_to_char( "FREEZE set.\n\r", ch );
	sprintf(buf,"$N puts %s in the deep freeze.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    save_char_obj( victim );

    return;
}



void do_log( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Log whom?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	if ( fLogAll )
	{
	    fLogAll = FALSE;
	    send_to_char( "Log ALL off.\n\r", ch );
	}
	else
	{
	    fLogAll = TRUE;
	    send_to_char( "Log ALL on.\n\r", ch );
	}
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if ( IS_SET(victim->act, PLR_LOG) )
    {
	REMOVE_BIT(victim->act, PLR_LOG);
	send_to_char( "LOG removed.\n\r", ch );
    }
    else
    {
	SET_BIT(victim->act, PLR_LOG);
	send_to_char( "LOG set.\n\r", ch );
    }

    return;
}



void do_noemote( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Noemote whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }


    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->comm, COMM_NOEMOTE) )
    {
	REMOVE_BIT(victim->comm, COMM_NOEMOTE);
	send_to_char( "You can emote again.\n\r", victim );
	send_to_char( "NOEMOTE removed.\n\r", ch );
	sprintf(buf,"$N restores emotes to %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->comm, COMM_NOEMOTE);
	send_to_char( "You can't emote!\n\r", victim );
	send_to_char( "NOEMOTE set.\n\r", ch );
	sprintf(buf,"$N revokes %s's emotes.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}



void do_noshout( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Noshout whom?\n\r",ch);
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->comm, COMM_NOSHOUT) )
    {
	REMOVE_BIT(victim->comm, COMM_NOSHOUT);
	send_to_char( "You can shout again.\n\r", victim );
	send_to_char( "NOSHOUT removed.\n\r", ch );
	sprintf(buf,"$N restores shouts to %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->comm, COMM_NOSHOUT);
	send_to_char( "You can't shout!\n\r", victim );
	send_to_char( "NOSHOUT set.\n\r", ch );
	sprintf(buf,"$N revokes %s's shouts.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}



void do_notell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Notell whom?", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->comm, COMM_NOTELL) )
    {
	REMOVE_BIT(victim->comm, COMM_NOTELL);
	send_to_char( "You can tell again.\n\r", victim );
	send_to_char( "NOTELL removed.\n\r", ch );
	sprintf(buf,"$N restores tells to %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->comm, COMM_NOTELL);
	send_to_char( "You can't tell!\n\r", victim );
	send_to_char( "NOTELL set.\n\r", ch );
	sprintf(buf,"$N revokes %s's tells.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}



void do_peace( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *rch;

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
	if ( rch->fighting != NULL )
	    stop_fighting( rch, TRUE );
	if (IS_NPC(rch) && IS_SET(rch->act,ACT_AGGRESSIVE))
	    REMOVE_BIT(rch->act,ACT_AGGRESSIVE);
	if (IS_NPC(rch))
	    rch->last_fought = NULL;
    }

    send_to_char( "Ok.\n\r", ch );
    return;
}

void do_wizlock( CHAR_DATA *ch, char *argument )
{
    extern bool wizlock;
    wizlock = !wizlock;

    if ( wizlock )
    {
	wiznet("$N has wizlocked the game.",ch,NULL,0,0,0);
	send_to_char( "Game wizlocked.\n\r", ch );
    }
    else
    {
	wiznet("$N removes wizlock.",ch,NULL,0,0,0);
	send_to_char( "Game un-wizlocked.\n\r", ch );
    }

    return;
}

/* RT anti-newbie code */

void do_newlock( CHAR_DATA *ch, char *argument )
{
    extern bool newlock;
    newlock = !newlock;

    if ( newlock )
    {
	wiznet("$N locks out new characters.",ch,NULL,0,0,0);
        send_to_char( "New characters have been locked out.\n\r", ch );
    }
    else
    {
	wiznet("$N allows new characters back in.",ch,NULL,0,0,0);
        send_to_char( "Newlock removed.\n\r", ch );
    }

    return;
}


void do_slookup( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int sn;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Lookup which skill or spell?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].name == NULL )
		break;
	    sprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
		sn, skill_table[sn].slot, skill_table[sn].name );
	    send_to_char( buf, ch );
	}
    }
    else
    {
	if ( ( sn = skill_lookup( arg ) ) < 0 )
	{
	    send_to_char( "No such skill or spell.\n\r", ch );
	    return;
	}

	sprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
	    sn, skill_table[sn].slot, skill_table[sn].name );
	send_to_char( buf, ch );
    }

    return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    log_naughty(ch,argument,2);
    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  set mob/char   <name> <field> <value>\n\r",ch);
	send_to_char("  set obj   <name> <field> <value>\n\r",ch);
	send_to_char("  set room  <room> <field> <value>\n\r",ch);
        send_to_char("  set skill <name> <spell or skill> <value>\n\r",ch);
	return;
    }

    if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
	do_mset(ch,argument);
	return;
    }

    if (!str_prefix(arg,"skill") || !str_prefix(arg,"spell"))
    {
	do_sset(ch,argument);
	return;
    }

    if (!str_prefix(arg,"object"))
    {
	do_oset(ch,argument);
	return;
    }

    if (!str_prefix(arg,"room"))
    {
	do_rset(ch,argument);
	return;
    }

    /* echo syntax */
    do_set(ch,"");
}

void do_sset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    int sn;
    bool fAll;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char( "Syntax:\n\r",ch);
	send_to_char( "  set skill <name> <spell or skill> <value>\n\r", ch);
	send_to_char( "  set skill <name> all <value>\n\r",ch);
	send_to_char("   (use the name of the skill, not the number)\n\r",ch);
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    fAll = !str_cmp( arg2, "all" );
    sn   = 0;
    if ( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
    {
	send_to_char( "No such skill or spell.\n\r", ch );
	return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
	send_to_char( "Value must be numeric.\n\r", ch );
	return;
    }

    value = atoi( arg3 );
    if ( (value < 0 || value > 100) && (value != -2) )
    {
	send_to_char( "Value range is 0 to 100 (-2 for permanent strip).\n\r", ch );
	return;
    }

    if ( fAll )
    {
	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].name != NULL )
		victim->pcdata->learned[sn]	= value;
	}
    }
    else
    {
	victim->pcdata->learned[sn] = value;
    }

    return;
}



void do_mset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    int value;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  set char <name> <field> <value>\n\r",ch);
	send_to_char("  Field being one of:\n\r",			ch );
	send_to_char("    str int wis dex con sex class level\n\r",	ch );
    	send_to_char("    race group gold qc hp mana move prac\n\r",ch);
	send_to_char("    align train thirst hunger drunk full\n\r",	ch );
	send_to_char("    home xptotal specialization security \n\r",ch);
	send_to_char("    pause hours attack_mod dam_mod ghost\n\r", ch );
	send_to_char("    enhanced_mod regen dedication\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    /* clear zones for mobs */
    victim->zone = NULL;

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number( arg3 ) ? atoi( arg3 ) : -1;

    /*
     * Set something.
     */

   if ( !str_cmp( arg2, "xptotal"))
    {
	if ( value < 0)
	{
	send_to_char("Xp total must be greater than 0.\n\r",ch);
	return;
	}
	victim->exp_total = value;
	return;
    }
    if ( !str_cmp (arg2, "pause"))
    {
        if ( value < 0 || value > 12 )
	{
	    send_to_char("Select a number between 0-12.\n\r",ch);
	    return;
	}
	victim->pause = value;
	return;
    }

    if ( !str_cmp (arg2, "hours"))
    {
    	if (IS_NPC(victim))
    	{
    	send_to_char("Not on NPC's.\n\r",ch);
    	return;
    	}
    	
    	if (!is_number(arg3))
    	{
    	send_to_char("Value must be numeric.\n\r",ch);
    	return;
    	}

    	value = atoi(arg3);
    	
    	if (value < 0)
    	{
    	send_to_char("Value must be numeric.\n\r",ch);
    	return;
    	}

	value *= 3600;
	victim->played = value;
    	return;
    }

    if (!str_cmp( arg2, "home"))
    {
	if (hometown_lookup(arg3))
	{
	victim->hometown = hometown_lookup(arg3);
	return;
	}
	else
	{
	send_to_char("No such hometown.\n\r",ch);
	return;
	}
    }

    if(!str_prefix(arg2,"attack_mod"))
    {
		if(!is_number(arg3))
			return send_to_char("Invalid attack_mod: must be a number.\n\r",ch);
		if(is_number(arg3) && atoi(arg3) < 0)
			return send_to_char("Invalid attack_mod: must be a positive number.\n\r",ch);
		if(is_number(arg3) && atoi(arg3) > 20)
			return send_to_char("Invalid attack_mod: must be a value less than or equal to 20.\n\r",ch);
		
		victim->numAttacks = (victim->numAttacks + atoi(arg3)) - victim->base_numAttacks;
		victim->base_numAttacks = atoi(arg3);
		sprintf(buf,"%s's base number of attacks set to %d.\n\r",IS_NPC(victim) ? victim->short_descr : victim->name, victim->base_numAttacks);
		send_to_char(buf,ch);
		return;
    }

    if(!str_prefix(arg2,"enhanced_mod"))
    {
		if(!is_number(arg3))
			return send_to_char("Invalid enhanced_mod: must be a number.\n\r",ch);
		
		if(is_number(arg3) && atoi(arg3) < 0)
			return send_to_char("Invalid enhanced_mod: must be a positive number.\n\r",ch);
		
		victim->enhancedDamMod = (victim->enhancedDamMod + atoi(arg3)) - victim->base_enhancedDamMod;
		victim->base_enhancedDamMod = atoi(arg3);
		sprintf(buf,"%s's base enhanced damage modifier set to %.0f.\n\r",IS_NPC(victim) ? victim->short_descr : victim->name, victim->base_enhancedDamMod);
		send_to_char(buf,ch);
		return;
    }
	
	if(!str_prefix(arg2,"regen"))
    {
		if(!is_number(arg3))
			return send_to_char("Invalid regen: must be a number.\n\r",ch);
		
		if(is_number(arg3) && atoi(arg3) < 0)
			return send_to_char("Invalid regen: must be a positive number.\n\r",ch);
		
		victim->regen_rate = (victim->regen_rate + atoi(arg3)) - victim->base_regen;
		victim->base_regen = atoi(arg3);
		sprintf(buf,"%s's base regeneration modifier set to %d.\n\r",IS_NPC(victim) ? victim->short_descr : victim->original_name, victim->base_regen);
		send_to_char(buf,ch);
		return;
    }
	
   
    if(!str_prefix(arg2,"dedication"))
    {
		if(IS_NPC(victim))
			return send_to_char("You can only set a PC's dedication.\n\r",ch);
		if(!str_prefix(arg3,"two-handed"))
			victim->pcdata->dedication = DED_TWOHAND;
		else if (!str_prefix(arg3,"shield"))
			victim->pcdata->dedication = DED_SHIELD;
		else
			return send_to_char("Valid dedications are: two-hand, shield.\n\r",ch);

		sprintf(buf,"%s's dedication has been changed to %s.\n\r",IS_NPC(victim) ? victim->short_descr : victim->name,!str_prefix(arg3,"two-handed") ? "Two-Handed Sword" : "Shield");
		send_to_char(buf,ch);
		return;
    }

    if(!str_prefix(arg2,"dam_mod"))
    {
		if(!is_number(arg3))
			return send_to_char("Invalid dam_mod.\n\r",ch);
			
		victim->dam_mod = (victim->dam_mod + atoi(arg3)) - victim->base_dam_mod;
		victim->base_dam_mod = atoi(arg3);
		sprintf(buf,"%s's base dam_mod set to %d%%.\n\r",IS_NPC(victim) ? victim->short_descr : victim->name, victim->base_dam_mod);
		send_to_char(buf,ch);
		return;
    }

    if(!str_prefix(arg2,"ghost"))
    {
        if(!is_number(arg3))
                return send_to_char("Ghost must be a number.\n\r",ch);
        victim->ghost = atoi(arg3);
	printf_to_char(ch, "Ghost timer for %s reset to %d.\n\r", victim->ghost);
        return;
    }

    /* No longer needed specialization */
    if ( !str_cmp( arg2,"specialization" ) )
    {
	if (value < 0 || value > 20000)
	{
	send_to_char("Specialization points are between 0 and 20000.\n\r",ch);
	return;
	}
	victim->pcdata->special = value;
	return;
    }

    if ( !str_cmp( arg2, "str" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_STR) )
	{
	    sprintf(buf,
		"Strength range is 3 to %d\n\r.",
		get_max_train(victim,STAT_STR));
	    send_to_char(buf,ch);
	    return;
	}

	victim->perm_stat[STAT_STR] = value;
	return;
    }

    if ( !str_cmp( arg2, "security" ) )	/* OLC */
    {
	if ( IS_NPC(ch) )
	{
		send_to_char( "NPC's cannot set security.\n\r", ch );
		return;
	}

        if ( IS_NPC( victim ) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            return;
        }

	if ( (value > ch->pcdata->security || value < 0) && (IS_IMMORTAL(ch) && (ch->level=60)) )
	{
	    if ( ch->pcdata->security != 0 )
	    {
		sprintf( buf, "Valid security is 0-%d.\n\r", ch->pcdata->security );
		send_to_char( buf, ch );
	    }
	    else
	    {
		send_to_char( "Valid security is 0 only.\n\r", ch );
	    }
	    return;
	}
	victim->pcdata->security = value;
	return;
    }

    if ( !str_cmp( arg2, "int" ) )
    {
        if ( value < 3 || value > get_max_train(victim,STAT_INT) )
        {
            sprintf(buf,
		"Intelligence range is 3 to %d.\n\r",
		get_max_train(victim,STAT_INT));
            send_to_char(buf,ch);
            return;
        }

        victim->perm_stat[STAT_INT] = value;
        return;
    }

    if ( !str_cmp( arg2, "wis" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_WIS) )
	{
	    sprintf(buf,
		"Wisdom range is 3 to %d.\n\r",get_max_train(victim,STAT_WIS));
	    send_to_char( buf, ch );
	    return;
	}

	victim->perm_stat[STAT_WIS] = value;
	return;
    }

    if ( !str_cmp( arg2, "dex" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_DEX) )
	{
	    sprintf(buf,
		"Dexterity ranges is 3 to %d.\n\r",
		get_max_train(victim,STAT_DEX));
	    send_to_char( buf, ch );
	    return;
	}

	victim->perm_stat[STAT_DEX] = value;
	return;
    }

    if ( !str_cmp( arg2, "con" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_CON) )
	{
	    sprintf(buf,
		"Constitution range is 3 to %d.\n\r",
		get_max_train(victim,STAT_CON));
	    send_to_char( buf, ch );
	    return;
	}

	victim->perm_stat[STAT_CON] = value;
	return;
    }

    if ( !str_prefix( arg2, "sex" ) )
    {
	if ( value < 0 || value > 2 )
	{
	    send_to_char( "Sex range is 0 to 2.\n\r", ch );
	    return;
	}
	victim->sex = value;
	if (!IS_NPC(victim))
	    victim->pcdata->true_sex = value;
	return;
    }

    if ( !str_prefix( arg2, "class" ) )
    {
	int class;

	if (IS_NPC(victim))
	{
	    send_to_char("Mobiles have no class.\n\r",ch);
	    return;
	}

	class = class_lookup(arg3);
	if ( class == -1 )
	{
	    char buf[MAX_STRING_LENGTH];

        	strcpy( buf, "Possible classes are: " );
        	for ( class = 0; class < MAX_CLASS; class++ )
        	{
            	    if ( class > 0 )
                    	strcat( buf, " " );
            	    strcat( buf, class_table[class].name );
        	}
            strcat( buf, ".\n\r" );

	    send_to_char(buf,ch);
	    return;
	}

	victim->class = class;
	victim->pcdata->special = SPEC_NONE;
	return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
	if ( !IS_NPC(victim) )
	{
	    send_to_char( "Not on PC's.\n\r", ch );
	    return;
	}

	if ( value < 0 || value > 60 )
	{
	    send_to_char( "Level range is 0 to 60.\n\r", ch );
	    return;
	}
	victim->level = value;
	return;
    }

    if ( !str_prefix( arg2, "gold" ) )
    {
	victim->gold = value;
	return;
    }

    if ( !str_prefix( arg2, "qc" ) )
    {
	victim->quest_credits = value;
	return;
    }
    if ( !str_prefix( arg2, "hp" ) )
    {
	if ( value < -10 || value > 30000 )
	{
	    send_to_char( "Hp range is -10 to 30,000 hit points.\n\r", ch );
	    return;
	}
	victim->max_hit = value;
        if (!IS_NPC(victim))
            victim->pcdata->perm_hit = value;
	return;
    }

    if ( !str_prefix( arg2, "mana" ) )
    {
	if ( value < 0 || value > 30000 )
	{
        send_to_char( "mana range is 0 to 30,000 mana points.\n\r", ch );
	    return;
	}
    victim->max_mana = value;
        if (!IS_NPC(victim))
            victim->pcdata->perm_mana = value;
	return;
    }

    if ( !str_prefix( arg2, "move" ) )
    {
	if ( value < 0 || value > 30000 )
	{
	    send_to_char( "Move range is 0 to 30,000 move points.\n\r", ch );
	    return;
	}
	victim->max_move = value;
        if (!IS_NPC(victim))
            victim->pcdata->perm_move = value;
	return;
    }

    if ( !str_prefix( arg2, "practice" ) )
    {
	if ( value < 0 || value > 250 )
	{
	    send_to_char( "Practice range is 0 to 250 sessions.\n\r", ch );
	    return;
	}
	victim->practice = value;
	return;
    }

    if ( !str_prefix( arg2, "train" ))
    {
	if (value < 0 || value > 50 )
	{
	    send_to_char("Training session range is 0 to 50 sessions.\n\r",ch);
	    return;
	}
	victim->train = value;
	return;
    }

    if ( !str_prefix( arg2, "align" ) )
    {
	if ( value < -1000 || value > 1000 )
	{
	    send_to_char( "Alignment range is -1000 to 1000.\n\r", ch );
	    return;
	}
	victim->alignment = value;
	return;
    }

    if ( !str_prefix( arg2, "ethos" ) )
    {
	if ( value < -1000 || value > 1000 )
	{
	    send_to_char( "Ethos range is -1000 to 1000.\n\r", ch );
	    return;
	}
	if (!IS_NPC(victim))
	    victim->pcdata->ethos = value;
	return;
    }

    if ( !str_prefix( arg2, "thirst" ) )
    {
	if ( IS_NPC(victim) )
	{
	    send_to_char( "Not on NPC's.\n\r", ch );
	    return;
	}

	if ( value < -1 || value > 100 )
	{
	    send_to_char( "Thirst range is -1 to 100.\n\r", ch );
	    return;
	}

	victim->pcdata->condition[COND_THIRST] = value;
	return;
    }

    if ( !str_prefix( arg2, "drunk" ) )
    {
	if ( IS_NPC(victim) )
	{
	    send_to_char( "Not on NPC's.\n\r", ch );
	    return;
	}

	if ( value < -1 || value > 100 )
	{
	    send_to_char( "Drunk range is -1 to 100.\n\r", ch );
	    return;
	}

	victim->pcdata->condition[COND_DRUNK] = value;
	return;
    }

    if ( !str_prefix( arg2, "full" ) )
    {
	if ( IS_NPC(victim) )
	{
	    send_to_char( "Not on NPC's.\n\r", ch );
	    return;
	}

	if ( value < -1 || value > 100 )
	{
	    send_to_char( "Full range is -1 to 100.\n\r", ch );
	    return;
	}

	victim->pcdata->condition[COND_FULL] = value;
	return;
    }

    if ( !str_prefix( arg2, "hunger" ) )
    {
        if ( IS_NPC(victim) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            return;
        }

        if ( value < -1 || value > 100 )
        {
            send_to_char( "Full range is -1 to 100.\n\r", ch );
            return;
        }

        victim->pcdata->condition[COND_HUNGER] = value;
        return;
    }

    if (!str_prefix( arg2, "race" ) )
    {
	int race;

	race = race_lookup(arg3);

	if ( race == 0)
	{
	    send_to_char("That is not a valid race.\n\r",ch);
	    return;
	}

	if (!IS_NPC(victim) && !race_table[race].pc_race)
	{
	    send_to_char("That is not a valid player race.\n\r",ch);
	    return;
	}

	victim->race = race;
	return;
    }

    if (!str_prefix(arg2,"group"))
    {
	if (!IS_NPC(victim))
	{
	    send_to_char("Only on NPCs.\n\r",ch);
	    return;
	}
	victim->group = value;
	return;
    }


    /*
     * Generate usage message.
     */
    do_mset( ch, "" );
    return;
}

void do_string( CHAR_DATA *ch, char *argument )
{
    char type [MAX_INPUT_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    log_naughty(ch,argument,3);
    smash_tilde( argument );
    argument = one_argument( argument, type );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  string char <name> <field> <string>\n\r",ch);
	send_to_char("    fields: name short long desc title spec\n\r",ch);
	send_to_char("  string obj  <name> <field> <string>\n\r",ch);
	send_to_char("    fields: name short long extended owner\n\r",ch);
	return;
    }

    if (!str_prefix(type,"character") || !str_prefix(type,"mobile"))
    {
    	if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
    	}

	/* clear zone for mobs */
	victim->zone = NULL;

	/* string something */

     	if ( !str_prefix( arg2, "name" ) )
    	{
	    free_string( victim->name );
	    victim->name = str_dup( arg3 );
	    return;
    	}

    	if ( !str_prefix( arg2, "description" ) )
    	{
    	    free_string(victim->description);
    	    victim->description = str_dup(arg3);
    	    return;
    	}

    	if ( !str_prefix( arg2, "short" ) )
    	{
	    free_string( victim->short_descr );
	    victim->short_descr = str_dup( arg3 );
	    return;
    	}

    	if ( !str_prefix( arg2, "long" ) )
    	{
	    free_string( victim->long_descr );
	    strcat(arg3,"\n\r");
	    victim->long_descr = str_dup( arg3 );
	    return;
    	}

    	if ( !str_prefix( arg2, "title" ) )
    	{
	    if ( IS_NPC(victim) )
	    {
	    	send_to_char( "Not on NPC's.\n\r", ch );
	    	return;
	    }

	    set_title( victim, arg3 );
	    return;
    	}

    	if ( !str_prefix( arg2, "spec" ) )
    	{
	    if ( !IS_NPC(victim) )
	    {
	    	send_to_char( "Not on PC's.\n\r", ch );
	    	return;
	    }

	    if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 )
	    {
	    	send_to_char( "No such spec fun.\n\r", ch );
	    	return;
	    }

	    return;
    	}
    }

    if (!str_prefix(type,"object"))
    {
    	/* string an obj */

   	if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    	{
	    send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
	    return;
    	}

        if ( !str_prefix( arg2, "name" ) )
    	{
	    free_string( obj->name );
	    obj->name = str_dup( arg3 );
	    return;
    	}

        if ( !str_prefix( arg2, "owner" ) )
        {
            free_string( obj->owner );
            obj->owner = str_dup( arg3 );
            return;
        }

    	if ( !str_prefix( arg2, "short" ) )
    	{
	    free_string( obj->short_descr );
	    obj->short_descr = str_dup( arg3 );
	    return;
    	}

    	if ( !str_prefix( arg2, "long" ) )
    	{
	    free_string( obj->description );
	    obj->description = str_dup( arg3 );
	    return;
    	}

    	if ( !str_prefix( arg2, "ed" ) || !str_prefix( arg2, "extended"))
    	{
	    EXTRA_DESCR_DATA *ed;

	    argument = one_argument( argument, arg3 );
	    if ( argument == NULL )
	    {
	    	send_to_char( "Syntax: oset <object> ed <keyword> <string>\n\r",
		    ch );
	    	return;
	    }

 	    strcat(argument,"\n\r");

	    ed = new_extra_descr();

	    ed->keyword		= str_dup( arg3     );
	    ed->description	= str_dup( argument );
	    ed->next		= obj->extra_descr;
	    obj->extra_descr	= ed;
	    return;
    	}
    }


    /* echo bad use message */
    do_string(ch,"");
}



void do_oset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int value;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  set obj <object> <field> <value>\n\r",ch);
	send_to_char("  Field being one of:\n\r",				ch );
	send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\n\r",	ch );
	send_to_char("    extra wear level weight cost timer\n\r",	ch );
	return;
    }

    if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
	return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
    {
/*	obj->value[0] = UMIN(50,value); */
	obj->value[0] = value;
	return;
    }

    if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
    {
	obj->value[1] = value;
	return;
    }

    if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
    {
	obj->value[2] = value;
	return;
    }

    if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
    {
	obj->value[3] = value;
	return;
    }

    if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
    {
	obj->value[4] = value;
	return;
    }

    if ( !str_prefix( arg2, "wear" ) )
    {
	obj->wear_flags = value;
	return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
	obj->level = value;
	return;
    }

    if ( !str_prefix( arg2, "weight" ) )
    {
	obj->weight = value;
	return;
    }

    if ( !str_prefix( arg2, "cost" ) )
    {
	obj->cost = value;
	return;
    }

    if ( !str_prefix( arg2, "timer" ) )
    {
	obj->timer = value;
	return;
    }

    /*
     * Generate usage message.
     */
    do_oset( ch, "" );
    return;
}

void do_rset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char( "Syntax:\n\r",ch);
	send_to_char( "  set room <location> <field> <value>\n\r",ch);
	send_to_char( "  Field being one of:\n\r",			ch );
	send_to_char( "    flags sector\n\r",				ch );
	return;
    }

    if ( ( location = find_location( ch, arg1 ) ) == NULL )
    {
	send_to_char( "No such location.\n\r", ch );
	return;
    }

    if (!is_room_owner(ch,location) && ch->in_room != location
    &&  room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That room is private right now.\n\r",ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
	send_to_char( "Value must be numeric.\n\r", ch );
	return;
    }
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_prefix( arg2, "flags" ) )
    {
	location->room_flags	= value;
	return;
    }

    if ( !str_prefix( arg2, "sector" ) )
    {
	location->sector_type	= value;
	return;
    }

    /*
     * Generate usage message.
     */
    do_rset( ch, "" );
    return;
}



void do_sockets( CHAR_DATA *ch, char *argument )
{
    char buf[2 * MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    int count;

    count	= 0;
    buf[0]	= '\0';

    one_argument(argument,arg);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	if ( d->character != NULL && can_see( ch, d->character )
	&& (arg[0] == '\0' ||
		is_name(arg,d->character->name)
		|| (d->original &&
	 	is_name(arg,d->original->name))))
	{
	    count++;

	    sprintf( buf + strlen(buf), "[%3d %18s %4d %5s %s] %-15s %s@%s\n\r",
		d->descriptor,
		d->connected == (CON_PLAYING) ? "Playing" :
		d->connected == (CON_GET_NAME) ? "Getting name" :
		d->connected == (CON_GET_OLD_PASSWORD) ? "Getting password" :
		d->connected == (CON_CONFIRM_NEW_NAME) ? "Confirming name" :
		d->connected == (CON_GET_NEW_PASSWORD) ? "Getting new pwd" :
		d->connected == (CON_CONFIRM_NEW_PASSWORD) ? "Confirm new pwd" :
		d->connected == (CON_GET_NEW_RACE) ? "Getting race" :
		d->connected == (CON_GET_NEW_SEX) ? "Getting sex" :
		d->connected == (CON_GET_NEW_CLASS) ? "Getting class" :
		d->connected == (CON_ROLLING_STATS) ? "Rolling stats" :
    		d->connected == (CON_DEFAULT_CHOICE) ? "Get hometown" :
		d->connected == (CON_GET_ALIGNMENT) ? "Getting alignment" :
		d->connected == (CON_GET_ETHOS) ? "Getting ethos" :
		d->connected == (CON_READ_IMOTD) ? "Getting Imotd":
		d->connected == (CON_READ_MOTD) ? "Getting motd" :
		d->connected == (CON_GET_CABAL) ? "Getting cabal" : "null",
        	(int)((current_time - d->character->logon) / 60 ),
                d->connected == (CON_PLAYING) ? pc_race_table[d->character->race].who_name : "     ",
                d->connected == (CON_PLAYING) ? class_table[d->character->class].who_name : "   ",
		(ch->level == 60) ? d->ip : "unknown",
		d->original  ? (d->original->original_name) ? d->original->original_name : d->original->name  :
		d->character ? (d->character->original_name) ?
		d->character->original_name : d->character->name : "(none)",
		(ch->level == 60) ? d->host : "unknown"
		);
	}
    }
    if (count == 0)
    {
	send_to_char("No one by that name is connected.\n\r",ch);
	return;
    }

    sprintf( buf2, "Total of %d user%s online.\n\r",
count, count == 1 ? "" : "s" );
    strcat(buf,buf2);
    page_to_char( buf, ch );
    return;
}

char* get_end_host(char* host) {
   int i=0;
   while(host[i]!='.'&&host[i]!='\0') {
      i++;
   }
   if(host[i]!='\0')
   i++;
   return &host[i];
}

int host_comp(MULTDATA* d1, MULTDATA* d2) {
    return strcmp(get_end_host(d1->des->host),get_end_host(d2->des->host));
}

void do_multicheck( CHAR_DATA *ch, char *argument) {
    char buf[2 * MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int count=0,i=0,j=0;
    MULTDATA CHARLIST[80];

    buf[0]      = '\0';

      for(d=descriptor_list;d!=NULL;d=d->next) {
	if(d->character!=NULL&&can_see(ch,d->character)) {
	   CHARLIST[i].des=d;
	   i++;
	}
	if(i>80) {
	   send_to_char("Max 80 characters, have coder fix.\n\r",ch);
	   return;
	}
      }
      qsort(CHARLIST, i, sizeof(MULTDATA), (void*)&host_comp);

    if(i<2) {
	send_to_char("This command is not usable with only 1 player.\n\r",ch);
	return;
    }

    for(j=0;j<i-1;j++) {
      if((!strcmp(get_end_host(CHARLIST[j].des->host),
         	get_end_host(CHARLIST[j+1].des->host))) ||
		(j!=0&&!strcmp(get_end_host(CHARLIST[j].des->host),
		get_end_host(CHARLIST[j-1].des->host)))) {
          count++;
          sprintf( buf + strlen(buf), "[%3d %2d] %s@%s\n\r",
                CHARLIST[j].des->descriptor,
                CHARLIST[j].des->connected,
                CHARLIST[j].des->original  ?
		CHARLIST[j].des->original->original_name ?
		CHARLIST[j].des->original->original_name :
		CHARLIST[j].des->original->name  :
                CHARLIST[j].des->character ?
		CHARLIST[j].des->character->original_name ?
		CHARLIST[j].des->character->original_name :
		CHARLIST[j].des->character->name : "(none)",
                CHARLIST[j].des->host
                );
      }
    }

    if(!strcmp(get_end_host(CHARLIST[j].des->host),
		get_end_host(CHARLIST[j-1].des->host))) {
	count++;
	sprintf(buf+strlen(buf), "[%3d %2d] %s@%s\n\r",
		CHARLIST[j].des->descriptor, CHARLIST[j].des->connected,
		CHARLIST[j].des->original ? CHARLIST[j].des->original->name :
		CHARLIST[j].des->character ? CHARLIST[j].des->character->name : "(none)",
		CHARLIST[j].des->host);
    }

    if (count == 0) {
       send_to_char("No matches were found.\n\r",ch);
       return;
    }

    sprintf( buf2, "%d user%s\n\r", count, count == 1 ? "" : "s" );
    strcat(buf,buf2);
    page_to_char( buf, ch );
    return;
}


/* Use this to lag out spammers and force them to stop moving */
void do_lagout(CHAR_DATA *ch,char *argument)
{
	char arg[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

    one_argument(argument,arg);
    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	return;
    }
    	victim = get_char_world(ch,arg);
	if (victim == NULL)
	{
	return;
 	}
	WAIT_STATE(victim,24);
	return;
}


/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Force whom to do what?\n\r", ch );
	return;
    }

    one_argument(argument,arg2);

    if (!str_cmp(arg2,"delete") || !str_prefix(arg2,"mob"))
    {
	send_to_char("That will NOT be done.\n\r",ch);
	return;
    }

    sprintf( buf, "$n forces you to '%s'.", argument );


    if ( !str_cmp( arg, "all" ) )
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if (get_trust(ch) < MAX_LEVEL - 3)
	{
	    send_to_char("Not at your level!\n\r",ch);
	    return;
	}

	for ( vch = char_list; vch != NULL; vch = vch_next )
	{
	    vch_next = vch->next;

	    if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch ) )
	    {
		act( buf, ch, NULL, vch, TO_VICT );
		interpret( vch, argument );
	    }
	}
    }
    else if (!str_cmp(arg,"players"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (get_trust(ch) < MAX_LEVEL - 2)
        {
            send_to_char("Not at your level!\n\r",ch);
            return;
        }

        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;

            if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
	    &&	 vch->level < LEVEL_HERO)
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else if (!str_cmp(arg,"gods"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (get_trust(ch) < MAX_LEVEL - 2)
        {
            send_to_char("Not at your level!\n\r",ch);
            return;
        }

        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;

            if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
            &&   vch->level >= LEVEL_HERO)
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else
    {
	CHAR_DATA *victim;

	if ( ( victim = get_char_world( ch, arg ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}

	if ( victim == ch )
	{
	    send_to_char( "Aye aye, right away!\n\r", ch );
	    return;
	}

    	if (!is_room_owner(ch,victim->in_room)
	&&  ch->in_room != victim->in_room
        &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    	{
            send_to_char("That character is in a private room.\n\r",ch);
            return;
        }

	if ( get_trust( victim ) >= get_trust( ch ) )
	{
	    send_to_char( "Do it yourself!\n\r", ch );
	    return;
	}

	if ( !IS_NPC(victim) && get_trust(ch) < MAX_LEVEL -3)
	{
	    send_to_char("Not at your level!\n\r",ch);
	    return;
	}

	act( buf, ch, NULL, victim, TO_VICT );
	interpret( victim, argument );
    }

    send_to_char( "Ok.\n\r", ch );
    return;
}



/*
 * New routines by Dionysos.
 */
void do_invis( CHAR_DATA *ch, char *argument )
{
    int level;
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    /* take the default path */

      if ( ch->invis_level)
      {
	  ch->invis_level = 0;
	  act( "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
	  send_to_char( "You slowly fade back into existence.\n\r", ch );
      }
      else
      {
	  ch->invis_level = get_trust(ch);
	  act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
	  send_to_char( "You slowly vanish into thin air.\n\r", ch );
      }
    else
    /* do the level thing */
    {
      level = atoi(arg);
      if (level < 2 || level > get_trust(ch))
      {
	send_to_char("Invis level must be between 2 and your level.\n\r",ch);
        return;
      }
      else
      {
	  ch->reply = NULL;
          ch->invis_level = level;
          act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You slowly vanish into thin air.\n\r", ch );
      }
    }

    return;
}


void do_incognito( CHAR_DATA *ch, char *argument )
{
    int level;
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    /* take the default path */

      if ( ch->incog_level)
      {
          ch->incog_level = 0;
          act( "$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You are no longer cloaked.\n\r", ch );
      }
      else
      {
          ch->incog_level = get_trust(ch);
          act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You cloak your presence.\n\r", ch );
      }
    else
    /* do the level thing */
    {
      level = atoi(arg);
      if (level < 2 || level > get_trust(ch))
      {
        send_to_char("Incog level must be between 2 and your level.\n\r",ch);
        return;
      }
      else
      {
          ch->reply = NULL;
          ch->incog_level = level;
          act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You cloak your presence.\n\r", ch );
      }
    }

    return;
}



void do_holylight( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;

    if ( IS_SET(ch->act, PLR_HOLYLIGHT) )
    {
	REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
	send_to_char( "Holy light mode off.\n\r", ch );
    }
    else
    {
	SET_BIT(ch->act, PLR_HOLYLIGHT);
	send_to_char( "Holy light mode on.\n\r", ch );
    }

    return;
}

/* prefix command: it will put the string typed on each line typed */

void do_prefi (CHAR_DATA *ch, char *argument)
{
    send_to_char("You cannot abbreviate the prefix command.\r\n",ch);
    return;
}

void do_prefix (CHAR_DATA *ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
    {
	if (ch->prefix[0] == '\0')
	{
	    send_to_char("You have no prefix to clear.\r\n",ch);
	    return;
	}

	send_to_char("Prefix removed.\r\n",ch);
	free_string(ch->prefix);
	ch->prefix = str_dup("");
	return;
    }

    if (ch->prefix[0] != '\0')
    {
	sprintf(buf,"Prefix changed to %s.\r\n",argument);
	free_string(ch->prefix);
    }
    else
    {
	sprintf(buf,"Prefix set to %s.\r\n",argument);
    }

    ch->prefix = str_dup(argument);
}

void do_astrip(CHAR_DATA *ch,char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_STRING_LENGTH];
	AFFECT_DATA *af, *af_next;

   	one_argument(argument,arg);
	if (arg[0] == '\0')
		victim = ch;
	else
		victim = get_char_world(ch,arg);

	if (victim == NULL)
	{
	send_to_char("They aren't here.\n\r",ch);
	return;
	}

	for ( af = victim->affected; af != NULL; af = af_next )
        {
		af_next=af->next;
  		if (!af->strippable)
  			continue;
  		affect_remove(victim,af);
 	}

	if (victim != ch)
		act("All strippable affects removed from $N.",ch,0,victim,TO_CHAR);
	else
		send_to_char("All strippable affects removed from yourself.\n\r",ch);

	return;
}


void do_limcounter(CHAR_DATA *ch,char *argument)
{
    OBJ_INDEX_DATA *pObjIndex;
    char buf[200];
    long vnum;

    if (!is_number(argument))
    {
	send_to_char("Only limstat by vnums.\n\r",ch);
	return;
    }

    vnum = atoi(argument);

    pObjIndex = get_obj_index(vnum);
    if (pObjIndex == NULL)
    {
	send_to_char("Not found.\n\r",ch);
	return;
    }

    sprintf(buf,"Obj vnum %ld,Max: %d, Count %d.",vnum, 
    pObjIndex->limtotal,
pObjIndex->limcount);
    send_to_char(buf,ch);
    send_to_char("\n\r",ch);
    return;
}


void do_classes(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int iRace;
    int iClass;

    for (iRace = 1;  race_table[iRace].name != NULL; iRace++)
	{
	if (!race_table[iRace].pc_race)
		break;
	sprintf(buf,"\n\r%s:\n\r",race_table[iRace].name);
	send_to_char(buf,ch);
	for (iClass = 0; iClass < MAX_CLASS; iClass++)
	  {
	  if (pc_race_table[iRace].classes[iClass] == 1)
		{
		sprintf(buf,"%s ",class_table[iClass].name);
		send_to_char(buf,ch);
		}
	  }
	}
  	return;
}


void do_access(CHAR_DATA *ch,char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument,arg);
    if (arg[0] == '\0')
    {
	send_to_char("Allow who to access Builder channel?\n\r",ch);
	return;
    }

    if ( (victim = get_char_world(ch,arg)) == NULL)
    {
	send_to_char("They aren't here.\n\r",ch);
	return;
    }

   if (IS_NPC(victim))
	{
	send_to_char("Not on mobiles.\n\r",ch);
	return;
   	}

   if (IS_SET(victim->comm, COMM_BUILDER))
   {
	REMOVE_BIT(victim->comm,COMM_BUILDER);
	act("Builder channel removed from $N.",ch,0,victim,TO_CHAR);
	return;
   }

  SET_BIT(victim->comm, COMM_BUILDER);
  act("Builder channel given to $N.",ch,0,victim,TO_CHAR);
  send_to_char("You now have Builder channel access.\n\r",victim);
  return;
}


void do_deathmessage(CHAR_DATA *ch,char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if ( !IS_NPC(ch) )
    {
	smash_tilde( argument );

	if (argument[0] == '\0')
	{
	    sprintf(buf,"Your death message is %s\n\r",ch->pcdata->imm_death);
	    send_to_char(buf,ch);
	    return;
	}

	if ( strstr(argument,ch->name) == NULL)
	{
	    send_to_char("You must include your name.\n\r",ch);
	    return;
	}

	free_string( ch->pcdata->imm_death );
	ch->pcdata->imm_death = str_dup( argument );

        sprintf(buf,"Your death message is now %s\n\r",ch->pcdata->imm_death);
        send_to_char(buf,ch);
    }
    return;
}


void do_max_limits(CHAR_DATA *ch,char *argument)
{
/*
    OBJ_DATA *obj;
    OBJ_DATA *o_next;
    int count; */
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pobj;
    long vnum;
    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Max limit on which items?\n\r",ch);
	return;
    }
    vnum = 0;

    if (is_number(arg))
    vnum = atoi(arg);

    if (vnum != 0)
    {
    pobj = get_obj_index(vnum);
    if (pobj == NULL)
	{
	send_to_char("No object by that vnum exists.\n\r",ch);
	return;
	}
	if (pobj->limtotal == 0)
	{
	send_to_char("That item is not limited.\n\r",ch);
	return;
	}
/*
    count = 0;
    for (obj = object_list; obj != NULL; obj = o_next)
	{
	o_next = obj->next;
	if (obj->pIndexData->vnum == pobj->vnum)
		count++;
	}
*/

    if (pobj->limcount >= pobj->limtotal )
	{
send_to_char("That item is already at it's max count.\n\r",ch);
	return;
	}

	pobj->limcount = pobj->limtotal;
	send_to_char("Item is now maxxed.\n\r",ch);
	return;
    }
    else
	{
	send_to_char("You must give the vnum of the object to be maxxed.\n\r",ch);
	return;
	}

    return;
}

/* Add Apply */
void do_addapply(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	AFFECT_DATA paf;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	int affect_modify = 0, bit = 0, enchant_type, pos, i;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );
	argument = one_argument( argument, arg3 );

	if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
	{
		send_to_char("Syntax for applies: addapply <object> <apply type> <value>\n\r",ch);
		send_to_char("Apply Types: hp str dex int wis con mana\n\r", ch);
		send_to_char("             ac move hitroll damroll saves\n\r\n\r", ch);
		send_to_char("Syntax for affects: addapply <object> affect <affect name>\n\r",ch);
		send_to_char("Affect Names: blind invisible detect_evil detect_invis detect_magic\n\r",ch);
		send_to_char("              detect_hidden detect_good sanctuary faerie_fire infrared\n\r",ch);
		send_to_char("              curse poison protect_evil protect_good sneak hide sleep charm\n\r", ch);
		send_to_char("              flying pass_door haste calm plague weaken dark_vision berserk\n\r", ch);
		send_to_char("              swim regeneration slow\n\r", ch);
		return;
	}

	if ((obj = get_obj_here(ch,arg1)) == NULL)
	{
		send_to_char("No such object exists!\n\r",ch);
		return;
	}

	if (!str_prefix(arg2,"hp"))
		enchant_type=APPLY_HIT;
	else if (!str_prefix(arg2,"str"))
		enchant_type=APPLY_STR;
	else if (!str_prefix(arg2,"dex"))
		enchant_type=APPLY_DEX;
	else if (!str_prefix(arg2,"int"))
		enchant_type=APPLY_INT;
	else if (!str_prefix(arg2,"wis"))
		enchant_type=APPLY_WIS;
	else if (!str_prefix(arg2,"con"))
		enchant_type=APPLY_CON;
	else if (!str_prefix(arg2,"mana"))
		enchant_type=APPLY_MANA;
	else if (!str_prefix(arg2,"move"))
		enchant_type=APPLY_MOVE;
	else if (!str_prefix(arg2,"ac"))
		enchant_type=APPLY_AC;
	else if (!str_prefix(arg2,"hitroll"))
		enchant_type=APPLY_HITROLL;
	else if (!str_prefix(arg2,"damroll"))
		enchant_type=APPLY_DAMROLL;
	else if (!str_prefix(arg2,"saves"))
		enchant_type=APPLY_SAVING_SPELL;
	else if (!str_prefix(arg2,"affect"))
		enchant_type=APPLY_SPELL_AFFECT;
	else
	{
		send_to_char("That apply is not possible!\n\r",ch);
		return;
	}

	if (enchant_type==APPLY_SPELL_AFFECT)
	{
		for (pos = 0; affect_flags[pos].name != NULL; pos++)
			if (!str_cmp(affect_flags[pos].name,arg3))
				bit = affect_flags[pos].bit;
	}
	else
	{
		if ( is_number(arg3) )
			affect_modify=atoi(arg3);
		else
		{
			send_to_char("Applies require a value.\n\r", ch);
			return;
		}
	}

	affect_enchant(obj);

	/* create the affect */
        init_affect(&paf);
      	paf.where	= TO_AFFECTS;
        paf.aftype    = AFT_SKILL;
    	paf.type	= 0;
	paf.level	= ch->level;
	paf.duration	= -1;
	paf.location	= enchant_type;
	paf.modifier	= affect_modify;
	paf.bitvector	= bit;

	if ( enchant_type == APPLY_SPELL_AFFECT )
	{
		/* make table work with skill_lookup */
        	for ( i=0 ; arg3[i] != '\0'; i++ )
        	{
            		if ( arg3[i] == '_' )
                		arg3[i] = ' ';
        	}

        	paf.type      = skill_lookup(arg3);
	}

	affect_to_obj(obj,&paf);

	send_to_char("Ok.\n\r", ch);
}

void do_arena_echo(const char *txt)
{
	DESCRIPTOR_DATA *d;
	char buffer[MAX_STRING_LENGTH*2];

	for ( d = descriptor_list; d; d = d->next )
	{
		if ( d->connected == CON_PLAYING )
		{
			colorconv(buffer, txt, d->character);
			send_to_char( "{RARENA>{x ",d->character);
			send_to_char( buffer, d->character );
			send_to_char( "\n\r",   d->character );
		}
	}

    return;
}

void do_arena( CHAR_DATA *ch, char *argument )
{
	DESCRIPTOR_DATA *d;
	ROOM_INDEX_DATA *mortlocation;
	ROOM_INDEX_DATA *immlocation;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	int trophycounter;
	char buffer[MAX_STRING_LENGTH];

	extern bool arena;
	arena = !arena;

	if (arena)
	{
		do_arena_echo("ARENA MODE STARTING UP...");
		mortlocation = get_room_index(26000);
		immlocation = get_room_index(26060);

		for ( d = descriptor_list; d; d = d->next )
		{
			if ( d->connected == CON_PLAYING )
			{
				if (d->character->pcdata->newbie==TRUE)
					continue;

				act( "$n disappears to the Arena.", d->character, NULL, NULL, TO_ROOM );
				char_from_room( d->character);
				if (d->character->level > LEVEL_HERO) {
					char_to_room( d->character, immlocation);
				} else {
					char_to_room( d->character, mortlocation);
					d->character->ghost = 2;
				}
 				act( "$n arrives suddenly into the Arena.", d->character, NULL, NULL, TO_ROOM );
				do_look( d->character, "auto");
			}
		}
		send_to_char( "Game enters arena mode.\n\r", ch );
		do_arena_echo("WELCOME TO THE COLISEUM.");
		do_arena_echo("You have entered the elite fighting grounds of this realm.");
		do_arena_echo("You will fight others and attempt to reap as many kills as you can.");
		do_arena_echo("Here you are not bound to cabal, class, or alignment politics.");
		do_arena_echo("When you are slain, you will return to the altar with your equipment.");
		do_arena_echo("A prize may be awarded to the winner of the most kills.");
		do_arena_echo("You have a moment to move out. Good luck.");
		wizlock = TRUE;
	} else {
		mortlocation = get_room_index(3014);
		immlocation = get_room_index(1200);

		do_arena_echo("ARENA MODE SHUTTING DOWN...");
		do_arena_echo("Results for Arena:");
		do_arena_echo("------------------");
		for ( d = descriptor_list; d; d = d->next )
		{
			if ( d->connected == CON_PLAYING )
			{
				/* TRANSFER OUT */
				act( "$n disappears out of the Arena.", d->character, NULL, NULL, TO_ROOM );
				char_from_room( d->character);
				if (d->character->level > LEVEL_HERO) {
					char_to_room( d->character, immlocation);
				} else {
					char_to_room( d->character, mortlocation);
					d->character->ghost = 2;
				}
 				act( "$n arrives suddenly from the Arena.", d->character, NULL, NULL, TO_ROOM );

				/* COUNT TROPHIES */

				trophycounter = 0;
				for (obj = d->character->carrying; obj != NULL; obj = obj_next )
				{
					obj_next = obj->next_content;

					if (obj->pIndexData->vnum == OBJ_VNUM_ARENA_TROPHY)
					{
						trophycounter++;
						obj_from_char(obj);
						extract_obj(obj);
					}
				}
				if (!(IS_IMMORTAL(d->character))) {
				sprintf(buffer,"%s: %d kill(s)",d->character->name,trophycounter);
				do_arena_echo(buffer);
				}
				do_look(d->character, "auto");
			}
		}

		send_to_char( "Game exits arena mode.\n\r", ch );
		wizlock = FALSE;
	}

	return;
}
void log_naughty( CHAR_DATA *ch, char *argument, int logtype )
{
	FILE *fp;

   if ( ( fp = fopen( GOD_LOG_FILE, "a" ) ) != NULL )
    {
		if(logtype==1)
			fprintf(fp,"LOAD: %s is loading %s.\n",ch->name,argument);
		if(logtype==2)
			fprintf(fp,"SET: %s is setting %s.\n",ch->name,argument);
		if(logtype==3)
			fprintf(fp,"STRING: %s is stringing %s.\n",ch->name,argument);
    }
   fclose(fp);
}

void do_history(CHAR_DATA *ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char obuf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int len;
	bool found = FALSE;

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );

	if (arg1[0] == '\0')
	{
		send_to_char("Syntax: history <character>          = read a character's history\n\r", ch);
		send_to_char("        history <character> + <info> = add to the temporary buffer\n\r", ch);
		send_to_char("        history <character> -        = remove the last line from the temporary buffer\n\r", ch);
		send_to_char("        history <character> show     = shows the temporary buffer\n\r", ch);
		send_to_char("        history <character> add      = write the temporary buffer to their pfile\n\r", ch);
		send_to_char("        history <character> clear    = clear a player's history (IMP ONLY)\n\r",ch);
		return;
	}

	if ((victim = get_char_world(ch,arg1)) == NULL)
	{
		send_to_char("They aren't here, attempting offline history...\n\r",ch);
		sprintf(buf,"finger %s history",arg1);
		interpret(ch,buf);
		return;
	}

	if (IS_NPC(victim))
	{
		send_to_char("You can't history a mob.\n\r",ch);
		return;
	}

	if(victim->level>=ch->level && get_trust(ch)<60)
	{
		send_to_char("Access denied.\n\r",ch);
		return;
	}

	if (arg2[0] == '\0')
	{
		show_history(ch,victim);
		return;
	}
	if (!strcmp(arg2,"clear"))
	{
		if(ch->level<60)
		{
			send_to_char("You are not permitted to complete that operation.\n\r",ch);
			return;
		}
		victim->pcdata->history_buffer=NULL;
	}
	if (!strcmp(arg2,"add")){
		if (victim->pcdata->temp_history_buffer == NULL || victim->pcdata->temp_history_buffer[0] == '\0')
		{
			send_to_char("The temporary buffer for that character is empty.\n\r",ch);
			return;
		} else {
			add_history(ch,victim,victim->pcdata->temp_history_buffer);
			free_string(victim->pcdata->temp_history_buffer);
			victim->pcdata->temp_history_buffer = NULL;
			show_history(ch,victim);
		}
		return;
	}

	if (!strcmp(arg2,"show"))
	{
		show_temp_history(ch,victim);
		return;
	}

	if (!strcmp(arg2,"+"))
	{
		if (argument[0] != '\0')
		{
			obuf[0] = (int)NULL;
			if (victim->pcdata->temp_history_buffer) {
			strcat(obuf,victim->pcdata->temp_history_buffer);
			}
			smash_tilde(argument);
			strcat(obuf,argument);
			strcat(obuf,"\n\r");

			free_string(victim->pcdata->temp_history_buffer);
			victim->pcdata->temp_history_buffer = str_dup(obuf);
		} else {
			send_to_char("Add what?\n\r",ch);
			return;
		}
		show_temp_history(ch,victim);
	}


	if (!strcmp(arg2,"-"))
	{

		if (victim->pcdata->temp_history_buffer == NULL || victim->pcdata->temp_history_buffer[0] == '\0')
		{
			send_to_char("No lines left to remove.\n\r",ch);
			return;
		}

		strcpy(obuf,victim->pcdata->temp_history_buffer);

		for (len = strlen(obuf); len > 0; len--)
		{
			if (obuf[len] == '\r')
			{
				if (!found)
				{
					if (len > 0)
					{
						len--;
					}
					found = TRUE;
				}
				else
				{
					obuf[len + 1] = '\0';
					free_string(victim->pcdata->temp_history_buffer);
					victim->pcdata->temp_history_buffer = str_dup(obuf);
					show_temp_history(ch,victim);
					return;
				}
			}
		}
		obuf[0] = (int)NULL;
		free_string(victim->pcdata->temp_history_buffer);
		victim->pcdata->temp_history_buffer = str_dup(obuf);
		show_temp_history(ch,victim);
		return;
	}
	return;
}

void add_history(CHAR_DATA *ch, CHAR_DATA *victim, char *string)
{
	char buf[MAX_STRING_LENGTH];
	char obuf[MAX_STRING_LENGTH];
	char *strtime;

	strtime = ctime( &current_time );
	chomp(strtime);

	if(IS_NPC(victim))
		return;
	if (victim->pcdata->history_buffer)
		sprintf(buf,victim->pcdata->history_buffer);
	else
		strcpy(buf,"");

	sprintf(obuf,"Added by %s (%s at %d hours)\n\r",
		ch ? ch->name : "AUTO", strtime, (int)((victim->played + current_time - victim->logon) / 3600));
	strcat(buf,obuf);
	strcat(buf,string);
	strcat(buf,"\n\r");

	free_string(victim->pcdata->history_buffer);
	victim->pcdata->history_buffer = str_dup(buf);

	return;
}

void show_temp_history(CHAR_DATA *ch, CHAR_DATA *victim)
{
	BUFFER *output;

	output = new_buf();
	send_to_char(victim->name,ch);
	send_to_char("'s temporary history buffer:\n\r",ch);

	if (victim->pcdata->temp_history_buffer == NULL || victim->pcdata->temp_history_buffer[0] == '\0')
	{
		send_to_char("Temporary buffer is clear.\n\r", ch );
	} else {
		add_buf(output,victim->pcdata->temp_history_buffer);
		page_to_char(buf_string(output),ch);
		free_buf(output);
	}
	return;
}

void show_history(CHAR_DATA *ch, CHAR_DATA *victim)
{
	BUFFER *output;

	output = new_buf();
	send_to_char(victim->name,ch);
	send_to_char("'s player history:\n\r",ch);
	if (victim->pcdata->history_buffer == NULL || victim->pcdata->history_buffer[0] == '\0') {
		send_to_char("No pfile history available.\n\r",ch);
	} else {
		add_buf(output,victim->pcdata->history_buffer);
		page_to_char(buf_string(output),ch);
		free_buf(output);
	}
	return;
}

void do_rastrip(CHAR_DATA *ch,char *argument)
{
	ROOM_AFFECT_DATA *af, *af_next;
	ROOM_INDEX_DATA *location = NULL;

	if (argument[0] == '\0')
	{
		location = ch->in_room;
	}

	if (location == NULL && (location = find_location(ch, argument)) == NULL)
	{
		send_to_char("No such room exists.\n\r", ch);
		return;
	}


	for (af = location->affected; af != NULL; af = af_next)
	{
		af_next = af->next;
		affect_remove_room(location,af);
	}

	act("All affects stripped from '$t'.",ch,location->name,0,TO_CHAR);

	return;
}

void do_raffects(CHAR_DATA *ch, char *argument)
{
	ROOM_AFFECT_DATA *paf;
	char buf[MAX_STRING_LENGTH];
	bool found = FALSE;
		
	if (ch->in_room->affected != NULL)
	{
		if (!found) {
			send_to_char("The room is affected by:\n\r", ch);
			found = TRUE;
		}
		
		for (paf = ch->in_room->affected; paf != NULL; paf = paf->next)
		{
			CHAR_DATA *owner = NULL;
			if (paf->owner_name)
				owner = find_char_by_name(paf->owner_name);
					
			if (paf->aftype == AFT_SPELL)
				sprintf( buf, "Spell: '%s' ", skill_table[paf->type].name);
			if (paf->aftype == AFT_SKILL)
				sprintf( buf, "Skill: '%s' ", skill_table[paf->type].name);
			if (paf->aftype == AFT_POWER)
				sprintf( buf, "Power: '%s' ", skill_table[paf->type].name);
			if (paf->aftype == AFT_COMMUNE)
				sprintf( buf, "Commune: '%s' ", skill_table[paf->type].name);
			if (paf->aftype != AFT_SPELL && paf->aftype!=AFT_SKILL && paf->aftype!=AFT_POWER && paf->aftype!=AFT_MALADY && paf->aftype!=AFT_COMMUNE)
				sprintf( buf, "Spell: '%s' ", skill_table[paf->type].name);
			send_to_char(buf, ch);

			sprintf(buf,"modifies %s by %d for %d hours with %s-bits %s, owner %s, level %d.\n\r",
			raffect_loc_name(paf->location),
			paf->modifier,
			paf->duration,
			paf->where == TO_ROOM_FLAGS ? "flag" : paf->where == TO_ROOM_CONST? "const" : "aff",
			"NULL",
        		owner != NULL ? owner->original_name : "none",
			paf->level
			);
			send_to_char(buf,ch);
		}
	}

	if(!found)
		send_to_char("The room is not affected by anything.\n\r",ch);
	return;
}

void do_vmstat( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg );

    if ( arg[0] == '\0' || !is_number(arg) )
    {
        send_to_char( "VMSTAT Mobiles:\n\r", ch );
	send_to_char( "Syntax: vmstat [vnum] or vmstat [name]\n\r", ch );
        return;
    }

    if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL )
    {
        send_to_char( "No mob has that vnum.\n\r", ch );
        return;
    }

    victim = create_mobile( pMobIndex );
    char_to_room( victim, ch->in_room );
    sprintf( buf, "%s", victim->name );
    do_mstat( ch, buf );
    extract_char( victim, TRUE);
    return;
}

void do_vostat( CHAR_DATA *ch, char *argument )
{
    extern long top_obj_index;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex=NULL;
    OBJ_DATA *obj;
    bool found = FALSE;
    long vnum, nMatch=0;
    char *blah;
    blah = one_argument( argument, arg1 );

    if ( arg1[0] == '\0')
    {
        send_to_char( "VOSTAT Objects:\n\r", ch );
	send_to_char( "Syntax: vostat [vnum] or vostat [name]\n\r", ch );
        return;
    }
    if(is_number(arg1))
    {
    if ( ( pObjIndex = get_obj_index( atoi( arg1 ) ) ) == NULL )
    {
        send_to_char( "No object has that vnum.\n\r", ch );
        return;
    }
	found=TRUE;
    }
    if(!is_number(arg1))
	{
	    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
	    {
        	if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
	        {
        	nMatch++;
	            if ( is_name( argument, pObjIndex->name ) )
        	    {
                	found=TRUE;
	                break;
        	    }
            }
    	}
    }
    if(!found || !pObjIndex)
    {
	send_to_char("No objects by that name were found.\n\r",ch);
	return;
    }
    pObjIndex->limcount--;
    obj = create_object( pObjIndex, 0 );
    obj_to_room( obj, ch->in_room );
    sprintf( buf, "%s", obj->name );
    do_ostat( ch, buf );
    extract_obj( obj );
    pObjIndex->limcount++;
return;
}

void do_afk(CHAR_DATA *ch, char *argument)
{

	if (IS_SET(ch->comm, COMM_AFK))
	{
		REMOVE_BIT(ch->comm, COMM_AFK);
		send_to_char("AFK removed.\n\r",ch);
		wiznet("$N is no longer AFK.",ch,NULL,WIZ_LINKS,0,0);
		do_replay(ch,"");
	} else {
		SET_BIT(ch->comm, COMM_AFK);
		send_to_char("AFK set.\n\r",ch);
		wiznet("$N is now AFK.",ch,NULL,WIZ_LINKS,0,0);
 	}

	return;
}

void do_teleport(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *pRoomIndex;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Teleport who?\n\r", ch );
        return;
    }

    if (( victim = get_char_world( ch, arg )) == NULL)
    {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    pRoomIndex = get_random_room(victim);

    if (victim != ch)
        send_to_char("{WWhoosh, The world spins around you and you are somewhere else!{x\n\r",victim);
    act( "$n vanishes!", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    act( "$n slams into the ground from the sky!", victim, NULL, NULL, TO_ROOM );
    do_look( victim, "auto" );
    return;
}

void do_qcrumble( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Quietly crumble what from whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( victim, arg1, ch ) ) == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }

    extract_obj( obj );
    act( "$p quietly crumbles into dust.", ch, obj, NULL, TO_CHAR );
    return;
}

void do_ghost( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  ghost <name> <yes/no>\n\r",ch);
	send_to_char("  ghost all <yes/no>\n\r",ch);
	send_to_char("  ghost zone <yes/no>\n\r",ch);
	send_to_char("Ghosting persons YES makes them a ghost, out of PK range.\n\r",ch);
	send_to_char("Ghosting persons NO unghosts them, and puts them back into range.\n\r",ch);
	send_to_char("Ghosting ALL changes EVERYONE to/from a ghost.\n\r",ch);
	send_to_char("Ghosting ZONE changes everyone in your AREA to/from a ghost.\n\r",ch);
	send_to_char("NOTE: Ghosting TO a ghost lasts for 15 ticks, then wears off.\n\r",ch);
	return;
    }

    if ( !str_cmp( arg1, "all" ) )
    {
	for (d = descriptor_list; d; d = d->next)
	{
	    if ( d->connected == CON_PLAYING
	    &&   d->character != ch
	    &&   d->character->in_room != NULL
	    &&   can_see( ch, d->character ) )
	    {
          	if (!str_cmp(arg2,"yes"))
          	{
			d->character->ghost = 15;
			send_to_char("You have turned into an invincible ghost for a several minutes.\n\r",d->character);
        	}
        	if (!str_cmp(arg2,"no"))
      	{
			d->character->ghost = 0;
			send_to_char("You are no longer an invincible ghost.\n\r",d->character);
		}
          }
	}
	return;
    }

    if ( !str_cmp( arg1, "zone" ) )
    {
	for ( d = descriptor_list; d; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    &&   d->character != ch
	    &&   d->character->in_room != NULL
	    &&   can_see( ch, d->character )
          &&  d->character->in_room->area == ch->in_room->area )
	    {
          	if (!str_cmp(arg2,"yes"))
          	{
			d->character->ghost = 15;
			send_to_char("You have turned into an invincible ghost for a several minutes.\n\r",d->character);
  	      }
   	      if (!str_cmp(arg2,"no"))
		{
			d->character->ghost = 0;
			REMOVE_BIT(d->character->affected_by,AFF_INFRARED);
			send_to_char("You are no longer an invincible ghost.\n\r",d->character);
		}
          }
	}
	return;
    }

    victim = get_char_world( ch, arg1 );

    if ( victim == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }


    if (!str_cmp(arg2,"merth"))
    {
	victim->ghost = 3;
	}
    if (!str_cmp(arg2,"yes"))
    {
	victim->ghost = 15;
	send_to_char("You have turned into an invincible ghost for a several minutes.\n\r",victim);
	}
    if (!str_cmp(arg2,"no")) {
	victim->ghost = 0;
	send_to_char("You are no longer an invincible ghost.\n\r",victim);
    }
    return;
}

void do_otype(CHAR_DATA *ch, char *argument)
{
    int type;
    int type2;
    long vnum=1;
    char buf[MAX_STRING_LENGTH];
    char buffer[12 * MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *item;
    OBJ_INDEX_DATA *obj;
    bool found;

    item = one_argument(argument, arg1);
    one_argument ( item , arg2);

    found = FALSE;
    buffer [0] = '\0';

    if (arg1[0] == '\0')
        {
        send_to_char("Type 'help otype' for usage.\n\r",ch);
        return;
        }

    type2 = 0;

    if  ((!str_cmp(arg1,"armor")
	&& !str_cmp(arg1,"weapon")
	&& arg2[0] != '\0' ))
        {
        send_to_char("Type 'help otype' for proper usage.\n\r",ch);
        return;
        }
    else if (!str_cmp(arg1,"armor"))
        {
        type = flag_value(type_flags,arg1);
        if ((type2 = flag_value(wear_flags,arg2)) == NO_FLAG)
          {
                send_to_char("No such armor type.\n\r",ch);
                return;
          }
        }
    else if (!str_cmp(arg1,"weapon"))
        {
        type = flag_value(type_flags,arg1);
        if ((type2 = flag_value(weapon_class,arg2)) == NO_FLAG)
          {
                send_to_char("No such weapon type.\n\r",ch);
                return;
          }
        }
    else
        {
            if((type = flag_value(type_flags,arg1)) == NO_FLAG)
                {
                send_to_char("Unknown Type.\n\r", ch);
                return;
                }
        }

    for(;vnum <= top_vnum_obj; vnum++)
        {
        if((obj=get_obj_index(vnum)) != NULL)
            {
            if((obj->item_type == type && type2 == 0
            && str_cmp(arg1,"weapon") && str_cmp(arg1,"armor"))
            || (obj->item_type == type && obj->value[0] == type2
            && str_cmp(arg1,"armor"))
            || (obj->item_type == type && IS_SET(obj->wear_flags,type2)
            && str_cmp(arg1,"weapon")))
		{
		sprintf(buf, "Area [%-25s] %5ld (%3d/%3d) %-12s L%2d - %s\n\r", obj->area->name, vnum, obj->limcount, obj->limtotal, obj->material->name, obj->level, obj->short_descr);
		found = TRUE;
		strcat(buffer,buf);
                }
            }
        }

    if (!found)
        send_to_char("No objects of that type exist.\n\r",ch);
    else
        if (ch->lines)
            page_to_char(buffer,ch);
        else
            send_to_char(buffer,ch);
}

#define CH(descriptor)  ((descriptor)->original ? \
(descriptor)->original : (descriptor)->character)

/* This file holds the copyover data */
#define COPYOVER_FILE "copyover.data"

/* This is the executable file */
#define EXE_FILE	  "../area/pos2"

/*  Copyover - Original idea: Fusion of MUD++
 *  Adapted to Diku by Erwin S. Andreasen, <erwin@andreasen.org>
 *  http://www.andreasen.org
 *  Changed into a ROM patch after seeing the 100th request for it :)
 */

void do_copyover (CHAR_DATA *ch, char * argument)
{
	FILE *fp;
	DESCRIPTOR_DATA *d, *d_next;
	char buf [MSL], buf2[MSL], immbuf[MSL];
	extern int port,control; /* db.c */

	fp = fopen (COPYOVER_FILE, "w");

	if (!fp)
	{
		send_to_char ("Copyover file not writeable, aborted.\n\r",ch);
		n_logf("Could not write to copyover file: %s", COPYOVER_FILE);
		perror ("do_copyover:fopen");
		return;
	}

	

	/* Consider changing all saved areas here, if you use OLC */
	do_asave (ch,"world");
    	do_restore(ch,"all");
   	do_force(ch, "all save");

	/* For each playing descriptor, save its state */
	for (d = descriptor_list; d ; d = d_next)
	{
		CHAR_DATA * och = CH (d);
		d_next = d->next; /* We delete from the list , so need to save this */

		if (!d->character || d->connected > CON_PLAYING) /* drop those logging on */
		{
			write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0);
			close_socket (d); /* throw'em out */
		}
		else
		{
			if (is_affected(och,skill_lookup("channel")))
			{
				affect_strip(och,skill_lookup("channel"));
				sprintf(buf,"Your mind loses its mental strength and you feel less healthy.\n\r");
				write_to_descriptor (d->descriptor, buf, 0);
			}

			if (is_affected(och,gsn_trip_wire))
			{
				affect_strip(och,gsn_trip_wire);
				sprintf(buf,"Your tripwire crumbles.\n\r");
				write_to_descriptor (d->descriptor, buf, 0);
			}

			if (och->level >= ch->level && IS_IMMORTAL(och))
			{
				sprintf(buf,"(%s) IMM: %s initiating copyover!\n\r", timestamp(), ch->original_name);
				
				write_to_descriptor(d->descriptor, buf, 0);
			}
			
			fprintf (fp, "%d %s %s %s\n", d->descriptor, och->original_name, d->host, d->ip);
			sprintf(buf, "\n\rYour mind is suddenly overcome with a piercing screech and the world about you shatters into countless shards of reality!\n\r");

			save_char_obj (och);
			write_to_descriptor (d->descriptor, buf, 0);
		}	
	}
	
	sprintf(immbuf,"(%s) IMM: %s initiating copyover!\n\r", timestamp(), ch->original_name);
	log_string( immbuf );
	fprintf (fp, "-1\n");
	fclose (fp);

	/* Close reserve and other always-open files and release other resources */
	fclose (fpReserve);

 	close_db();

	/* exec - descriptors are inherited */
	sprintf (buf, "%d", port);
	sprintf (buf2, "%d", control);
	execl (EXE_FILE, "pos2", buf, "copyover", buf2, (char *) NULL);

	/* Failed - sucessful exec will not return */

	perror ("do_copyover: execl");
	send_to_char ("Copyover FAILED!\n\r",ch);

	/* Here you might want to reopen fpReserve */
	fpReserve = fopen (NULL_FILE, "r");
}

/* Recover from a copyover - load players */
void copyover_recover ()
{
	DESCRIPTOR_DATA *d;
	FILE *fp;
	char name [100];
	char host[MSL];
	int desc;
	bool fOld;
	char ip[100];
	

	n_logf ("Copyover recovery initiated");

	fp = fopen (COPYOVER_FILE, "r");

	if (!fp) /* there are some descriptors open which will hang forever then ? */
	{
		perror ("copyover_recover:fopen");
		n_logf ("Copyover file not found. Exitting.\n\r");
		exit (1);
	}

	unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading	*/

	for (;;)
	{
		fscanf (fp, "%d %s %s %s\n", &desc, name, host, ip);
		if (desc == -1)
			break;

		/* Write something, and check if it goes error-free */
		if (!write_to_descriptor (desc, "\n\rYou struggle to fight off the soul-wrenching sound...\n\r",0))
		{
			close (desc); /* nope */
			continue;
		}

		d = new_descriptor();
		d->descriptor = desc;

		d->host = str_dup (host);
		d->ip = str_dup(ip);
		d->next = descriptor_list;
		descriptor_list = d;
		d->connected = CON_COPYOVER_RECOVER; /* -15, so close_socket frees the char */


		/* Now, find the pfile */

		fOld = load_char_obj (d, name);

		if (!fOld) /* Player file not found?! */
		{
			write_to_descriptor (desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r", 0);
			close_socket (d);
		}
		else /* ok! */
		{
			write_to_descriptor (desc, "\n\rThe lifeless and broken void about you blinks white and the penetrating noise vanishes, leaving only the calmness of reality.\n\r",0);
			
				if (!IS_IMMORTAL(d->character) && !IS_NPC(d->character))
					act("$n struggles and regains his composure.",d->character,0,0,TO_ROOM);
				else if (IS_IMMORTAL(d->character) && !IS_NPC(d->character))
					act("$n blurs into your view from the broken shards of existence.",d->character,0,0,TO_ROOM);
			
			if (d->character->cloaked) {
				CHAR_DATA *ch = d->character;
				AFFECT_DATA af;
				char cloakbuf[MSL];
				init_affect(&af);
			  	af.where	= TO_AFFECTS;
				af.aftype	= AFT_POWER;
			  	af.type		= gsn_cloak_form;
			  	af.level	= ch->level;
			  	af.location	= APPLY_NONE;
			  	af.modifier	= 0;
			  	af.bitvector	= AFF_SNEAK;
			  	af.duration 	= -1;
				af.affect_list_msg = str_dup("grants silent movement and a shrouded disguise");
			  	affect_to_char( ch, &af );
				af.affect_list_msg = NULL;
			  	af.location 	= APPLY_HIT;
			  	af.modifier 	= ch->level*5;
			  	affect_to_char(ch,&af);
			  	af.location 	= APPLY_MOVE;
			  	af.modifier 	= ch->level*2;
			  	affect_to_char(ch,&af); 
			  	af.location 	= APPLY_MANA;
			  	af.modifier 	= ch->level*2;
			  	affect_to_char(ch,&af);
				af.location	= APPLY_DAMROLL;
				af.modifier	= 5;
				affect_to_char(ch,&af);
				af.location	= APPLY_HITROLL;
				af.modifier	= 5;
				affect_to_char(ch,&af);
			  	check_improve(ch,gsn_cloak_form,TRUE,6);
			  	send_to_char("You cloak your presence.\n\r",ch);
				free_string(ch->name);
				sprintf(cloakbuf, "cloaked figure");
				ch->name=str_dup(cloakbuf);
				ch->cloaked = 1;
				free_string(cloakbuf);
			}
			
			/* Just In Case */
			if (!d->character->in_room)
				d->character->in_room = get_room_index (ROOM_VNUM_TEMPLE);

			/* Insert in the char_list */
			d->character->next = char_list;
			char_list = d->character;

			char_to_room (d->character, d->character->in_room);
			do_look (d->character, "auto");

			d->connected = CON_PLAYING;
			reset_char(d->character);
		}
	}
   fclose (fp);


}

void do_wflag(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int bitvector;
    int flag;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char("Syntax: wflag <object> <flag number>\n",ch);
        send_to_char("1: Flaming 2: Frost 3: Vampiric 4: Sharp\n\r",ch);
	send_to_char("5: Vorpal 6: Two Hands 7: Shocking 8: Poison\n\r",ch);
	send_to_char("9: Leader weapon\n\r",ch);
        return;
    }

    if ((obj = get_obj_world(ch, arg1)) == NULL)
    {
    	send_to_char("Nothing like that in Heaven, Hell, or Earth.\n\r",ch);
    	return;
    }

    if(obj->item_type != ITEM_WEAPON)
    {
    	send_to_char("That object is not a weapon.\n\r",ch);
    	return;
    }

    flag = atoi( arg2 );

    switch ( flag )
    {
    	case 1: bitvector = WEAPON_FLAMING; break;
    	case 2: bitvector = WEAPON_FROST; break;
    	case 3: bitvector = WEAPON_VAMPIRIC; break;
    	case 4: bitvector = WEAPON_SHARP; break;
    	case 5: bitvector = WEAPON_VORPAL; break;
    	case 6: bitvector = WEAPON_TWO_HANDS; break;
    	case 7: bitvector = WEAPON_SHOCKING; break;
    	case 8: bitvector = WEAPON_POISON; break;
	case 9: bitvector = WEAPON_LEADER; break;
    	default: bitvector = 0;
    }

    if (bitvector == 0)
    {
    	send_to_char("There are no weapon flags like that.\n\r",ch);
    	return;
    }

    if (IS_SET(obj->value[4], bitvector))
        REMOVE_BIT(obj->value[4], bitvector);
    else
        SET_BIT(obj->value[4], bitvector);

    send_to_char("Ok.\n\r",ch);

    return;

}

void do_empower(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char( "Syntax: empower <char> <yes/no>\n\r", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL || IS_NPC(victim))
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }


    if (!str_prefix(arg2,"no"))
    {
	if (IS_SET(victim->act, PLR_EMPOWERED))
	{
        REMOVE_BIT(victim->act, PLR_EMPOWERED);
        send_to_char( "The Immortals have revoked your empowerment!\n\r",victim );
	send_to_char( "OK.\n\r",ch);
	sprintf(buf,"$N revokes %s's empowerment.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,0,0);
	sprintf(buf,"AUTO: Unempowered by %s.\n\r",ch->name);
	add_history(NULL,victim,buf);
	return;
	} else {
	send_to_char( "They are not empowered in the first place.\n\r", ch );
	return;
	}
    }
    else if (!str_prefix(arg2,"yes"))
    {
	if (!IS_SET(victim->act, PLR_EMPOWERED))
	{
        SET_BIT(victim->act, PLR_EMPOWERED);
        send_to_char( "You have been empowered by the Immortals.\n\r", victim );
	send_to_char( "OK.\n\r",ch);
	sprintf(buf,"$N empowers %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,0,0);
	sprintf(buf,"AUTO: Empowered by %s.\n\r",ch->name);
	add_history(NULL,victim,buf);
	return;
	} else {
	send_to_char( "They are already empowered.\n\r", ch );
	return;
	}
    }
    else
    {
	send_to_char( "Syntax: empower <char> <yes/no>\n\r", ch );
	return;
    }

    return;
}

void do_lockstat( CHAR_DATA *ch, char *argument )
{
  char buf[64];

  sprintf(buf,"{WNewlock is:{x %s\n\r",newlock ? "{GON{x" : "{ROFF{x");
  send_to_char(buf, ch);
  sprintf(buf,"{WWizlock is:{x %s\n\r",wizlock ? "{GON{x" : "{ROFF{x");
  send_to_char(buf, ch);
}

void do_flag(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH],arg3[MAX_INPUT_LENGTH],arg4[MAX_INPUT_LENGTH];
    char word[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    long *flag, old = 0, new = 0, marked = 0, pos;
    char type;
    const struct flag_type *flag_table;

    argument = one_argument(argument,arg1);
    argument = one_argument(argument,arg2);
    argument = one_argument(argument,arg3);
    argument = one_argument(argument,arg4);

    type = argument[0];

    if (type == '=' || type == '-' || type == '+')
        argument = one_argument(argument,word);

    if (arg1[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  flag mob  <name> <field> <+/-/=> <flags>\n\r",ch);
	send_to_char("  flag char <name> <field> <+/-/=> <flags>\n\r",ch);
	send_to_char("  flag obj  <name> <field> <+/-/=> <flags>\n\r",ch);
	send_to_char("  mob  flags: act,aff,off,imm,res,vuln,form,part\n\r",ch);
	send_to_char("  char flags: plr,comm,aff,imm,res,vuln\n\r",ch);
	send_to_char("  obj flags:  wear,extra,weaponflags,restricts\n\r",ch);
	send_to_char("  +: add flag, -: remove flag, = set equal to\n\r",ch);
	send_to_char("  otherwise flag toggles the flags listed.\n\r",ch);
	return;
    }

    if (arg2[0] == '\0')
    {
	send_to_char("What do you wish to set flags on?\n\r",ch);
	return;
    }

    if (arg3[0] == '\0')
    {
	send_to_char("You need to specify a flag to set.\n\r",ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Which flags do you wish to change?\n\r",ch);
	return;
    }

    if (!str_prefix(arg1,"mob") || !str_prefix(arg1,"char"))
    {
	victim = get_char_world(ch,arg2);
	if (victim == NULL)
	{
	    send_to_char("You can't find them.\n\r",ch);
	    return;
	}
	victim->zone = NULL;

        /* select a flag to set */
	if (!str_prefix(arg3,"act"))
	{
	    if (!IS_NPC(victim))
	    {
		send_to_char("Use plr for PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->act;
	    flag_table = act_flags;
	}

	else if (!str_prefix(arg3,"plr"))
	{
	    if (IS_NPC(victim))
	    {
		send_to_char("Use act for NPCs.\n\r",ch);
		return;
	    }

	    flag = &victim->act;
	    flag_table = plr_flags;
	}
  	else if (!str_prefix(arg3,"immunity"))
	{
	    flag = &victim->imm_flags;
	    flag_table = imm_flags;
	}

	else if (!str_prefix(arg3,"resist"))
	{
	    flag = &victim->res_flags;
	    flag_table = imm_flags;
	}

	else if (!str_prefix(arg3,"vuln"))
	{
	    flag = &victim->vuln_flags;
	    flag_table = imm_flags;
	}

	else if (!str_prefix(arg3,"off"))
	{
	    if (!IS_NPC(victim))
	    {
	 	send_to_char("OFF can't be set on PCs.\n\r",ch);
		return;
	    }
	    flag = &victim->off_flags;
	    flag_table = off_flags;
	}

	else if (!str_prefix(arg3,"form"))
	{
	    if (!IS_NPC(victim))
	    {
	 	send_to_char("Form can't be set on PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->form;
	    flag_table = form_flags;
	}

	else if (!str_prefix(arg3,"parts"))
	{
	    if (!IS_NPC(victim))
	    {
		send_to_char("Parts can't be set on PCs.\n\r",ch);
		return;
	    }

	    flag = &victim->parts;
	    flag_table = part_flags;
	}

	else if (!str_prefix(arg3,"comm"))
	{
	    if (IS_NPC(victim))
	    {
		send_to_char("Comm can't be set on NPCs.\n\r",ch);
		return;
	    }

	    flag = &victim->comm;
	    flag_table = comm_flags;
	}

	else 
	{
	    send_to_char("That's not an acceptable flag.\n\r",ch);
	    return;
	}
    } 

    else if (!str_prefix(arg1,"obj"))
    {
	obj = get_obj_world(ch,arg2);
	if (obj == NULL)
		return send_to_char("That item does not exist.\n\r",ch);

	if (!str_prefix(arg3,"wear"))
	{
	    flag = &obj->wear_flags;
	    flag_table = wear_flags;
	}

	else if (!str_prefix(arg3,"weaponflags"))
	{
	   if (obj->item_type != ITEM_WEAPON)
		return send_to_char("That is not a weapon.\n\r",ch);
	   flag = &obj->value[4];
	   flag_table = weapon_type2;
	}

	else if (!str_prefix(arg3,"restricts"))
	{
	   flag = &obj->restrict_flags;
	   flag_table = restrict_flags;
	}

	else 
	{
	    send_to_char("That's not an acceptable flag.\n\r",ch);
	    return;
	}
    }

    else {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  flag mob  <name> <field> <+/-/=> <flags>\n\r",ch);
	send_to_char("  flag char <name> <field> <+/-/=> <flags>\n\r",ch);
	send_to_char("  mob  flags: act,aff,off,imm,res,vuln,form,part\n\r",ch);
	send_to_char("  char flags: plr,comm,aff,imm,res,vuln\n\r",ch);
	send_to_char("  obj flags:  wear,extra,weaponflags,restricts\n\r",ch);
	send_to_char("  +: add flag, -: remove flag, = set equal to\n\r",ch);
	send_to_char("  otherwise flag toggles the flags listed.\n\r",ch);
	return;
    }

	old = *flag;

	if (type != '=')
	    new = old;
	
        /* mark the words */
        for (; ;)
        {
	    argument = one_argument(argument,word);

	    if (word[0] == '\0')
		break;

	    pos = flag_lookup(word,flag_table);
	    if (pos == 0)
	    {
		send_to_char("That flag doesn't exist!\n\r",ch);
		return;
	    }
	    else
	    {
		SET_BIT(marked,pos);
	    }
	}

	for (pos = 0; flag_table[pos].name != NULL; pos++)
	{

	    if ((get_trust(ch) == MAX_LEVEL || flag_table[pos].settable) && IS_SET(marked,flag_table[pos].bit))
	    {
		switch(type)
		{
		    case '=':
		    case '+':
			SET_BIT(new,flag_table[pos].bit);
			send_to_char("You set that flag.\n\r",ch);
			break;
		    case '-':
			REMOVE_BIT(new,flag_table[pos].bit);
			send_to_char("You removed that flag.\n\r",ch);
			break;
		    default:
			if (IS_SET(new,flag_table[pos].bit))
			{
			    REMOVE_BIT(new,flag_table[pos].bit);
			    send_to_char("You removed that flag.\n\r",ch);
			}
			else
			{
			    SET_BIT(new,flag_table[pos].bit);
			    send_to_char("You set that flag.\n\r",ch);
			}
		}
	    } else {
		continue;
	    }
	}
	*flag = new;
	return;
}

void do_brands( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    OBJ_DATA *objj;

    if (ch->level < 53 || IS_NPC(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        send_to_char("Syntax: brands <char>\n\r", ch);
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't playing.\n\r", ch );
        return;
    }

    if (ch == victim)
    {
        send_to_char( "Trying to brand yourself?\n\r", ch );
        return;
    }
    if ( (obj = get_eq_char(victim, WEAR_BRAND) ) != NULL )
    {
        send_to_char("That person already has a brand.\n\r", ch);
        return;
    }

    if ( (obj = get_eq_char(ch, WEAR_BRAND) ) != NULL )
    {
	objj = create_object(obj->pIndexData, 0);
        clone_object(obj, objj);
        obj_to_char( objj, victim );
        equip_char( victim, objj, WEAR_BRAND );
        send_to_char("That person now has your brand.\n\r", ch);
        send_to_char("You wince in agony as the hot metal brands you!\n\r", victim);
        sprintf(buf,"AUTO: Branded by %s.\n\r", ch->name);
        if (!IS_IMMORTAL(victim) && !IS_NPC(victim))
            add_history(NULL,victim,buf);
        return;
    }
    send_to_char("You don't have a brand yourself.\n\r", ch);
    return;
}

void do_unbrands( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if (ch->level < 53 || IS_NPC(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        send_to_char("Syntax: unbrand <char>\n\r", ch);
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't playing.\n\r", ch );
        return;
    }
    if (IS_NPC(victim))
    {
	send_to_char( "Trying to unbrand a mob?\n\r", ch );
	return;
    }
    if (ch == victim)
    {
        send_to_char( "Trying to unbrand yourself?\n\r", ch );
        return;
    }
    if ((obj = get_eq_char(victim, WEAR_BRAND)) == NULL)
    {
	send_to_char( "That person doesn't have a brand.\n\r", ch );
	return;
    }
    unequip_char(victim, obj);
    obj_from_char(obj);
    extract_obj(obj);
    send_to_char("You have unbranded that person.\n\r", ch);
    send_to_char("Your brand is painfully removed.\n\r", victim);
    sprintf(buf,"AUTO: Unbranded by %s.\n\r",ch->name);
    if (!IS_IMMORTAL(victim))
        add_history(NULL,victim,buf);
    return;
}

void do_damage(CHAR_DATA *ch,char *argument)
{
        int dam = 0;
        char arg1[MAX_STRING_LENGTH];
        char arg2[MAX_STRING_LENGTH];
        CHAR_DATA *victim;

        argument = one_argument(argument,arg1);
        argument = one_argument(argument,arg2);

        if (arg1[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0')
        {
                send_to_char("Syntax: damage <char> <damage> <noun>\n\r",ch);
                return;
        }

        if ((victim = get_char_room(ch, arg1)) == NULL)
        {
                send_to_char("They aren't here.\n\r", ch);
                return;
        }

        if (!is_number(arg2))
        {
                send_to_char("Damage must be numerical.\n\r",ch);
                return;
        }
        dam = atoi(arg2);

        damage_new(ch,victim,dam,TYPE_UNDEFINED,DAM_OTHER,TRUE,HIT_UNBLOCKABLE,HIT_NOADD,HIT_NOMULT,argument);
        if (ch->fighting == victim)
                stop_fighting(ch,FALSE);
        if (victim->fighting == ch)
                stop_fighting(victim,FALSE);
        return;
}


void do_auto_shutdown()
{
    	FILE *fp;
    	extern bool merc_down;
    	DESCRIPTOR_DATA *d,*d_next;
    	merc_down = TRUE;

    	fclose(fpReserve);
    
	if((fp = fopen(LAST_COMMAND_FILE,"a")) != NULL)
      		bug("Error in do_auto_save opening last_command.txt",0);
   
      	fprintf(fp,"Last Command: %s\n", last_command);

    	fclose( fp );
    	fpReserve = fopen( NULL_FILE, "r" );

    	for ( d = descriptor_list; d != NULL; d = d_next)
    	{

		if(d->character)
		{
			send_to_char("{GMUD crash imminent :: Saving character.{x\n\r", d->character);
			do_save (d->character, "");

			n_logf("Auto_shutdown: Saving pfile for %s.", d->character->original_name);

            		if (is_affected(d->character,skill_lookup("channel"))){
                		affect_strip(d->character,skill_lookup("channel"));
            		}

            		if (is_affected(d->character,skill_lookup("defiance"))){
                		affect_strip(d->character,skill_lookup("defiance"));
            		}
		}

      		d_next = d->next;
      		close_socket(d);
    	}
    	return;
}

void do_arealinks(CHAR_DATA *ch, char *argument)
{
    FILE *fp;
    BUFFER *buffer = new_buf();
    AREA_DATA *parea;
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *to_room;
    ROOM_INDEX_DATA *from_room;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    long vnum = 0;
    int iHash, door;
    bool found = FALSE;

    static char * const dir_name[] = {"north","east","south","west","up","down"};

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    /* First, the 'all' option */
    if (!str_cmp(arg1,"all"))
    {
	if (arg2[0] != '\0')
	{
	    fclose(fpReserve);
	    if( (fp = fopen(arg2, "w")) == NULL)
	    {
		send_to_char("Error opening file, printing to screen.\n\r",ch);
		fclose(fp);
		fpReserve = fopen(NULL_FILE, "r");
		fp = NULL;
	    }
	}
	else
	    fp = NULL;

	/* Open a buffer if it's to be output to the screen */
	if (!fp)
	    buffer = new_buf();

	/* Loop through all the areas */
	for (parea = area_first; parea != NULL; parea = parea->next)
	{
	    /* First things, add area name  and vnums to the buffer */
	    sprintf(buf, "*** %s (%ld to %ld) ***\n\r",
			 parea->name, parea->min_vnum, parea->max_vnum);
	    fp ? fprintf(fp, buf) : add_buf(buffer, buf);

	    /* Now let's start looping through all the rooms. */
	    found = FALSE;
	    for(iHash = 0; iHash < MAX_KEY_HASH; iHash++)
	    {
		for( from_room = room_index_hash[iHash];
		     from_room != NULL;
		     from_room = from_room->next )
		{
		    /*
		     * If the room isn't in the current area,
		     * then skip it, not interested.
		     */
		    if ( from_room->vnum < parea->min_vnum
		    ||   from_room->vnum > parea->max_vnum )
			continue;

		    /* Aha, room is in the area, lets check all directions */
		    for (door = 0; door < 5; door++)
		    {
			/* Does an exit exist in this direction? */
			if( (pexit = from_room->exit[door]) != NULL )
			{
			    to_room = pexit->u1.to_room;

			    /*
			     * If the exit links to a different area
			     * then add it to the buffer/file
			     */
			    if( to_room != NULL
			    &&  (to_room->vnum < parea->min_vnum
			    ||   to_room->vnum > parea->max_vnum) )
			    {
				found = TRUE;
				sprintf(buf, "    (%ld) links %s to %s (%ld)\n\r",
				    from_room->vnum, dir_name[door],
				    to_room->area->name, to_room->vnum);

				/* Add to either buffer or file */
				if(fp == NULL)
				    add_buf(buffer, buf);
				else
				    fprintf(fp, buf);
			    }
			}
		    }
		}
	    }

	    /* Informative message for areas with no external links */
	    if (!found)
		add_buf(buffer, "    No links to other areas found.\n\r");
	}

	/* Send the buffer to the player */
	if (!fp)
	{
	    page_to_char(buf_string(buffer), ch);
	    free_buf(buffer);
	}
	/* Or just clean up file stuff */
	else
	{
	    fclose(fp);
	    fpReserve = fopen(NULL_FILE, "r");
	}

	return;
    }

    /* No argument, let's grab the char's current area */
    if(arg1[0] == '\0')
    {
	parea = ch->in_room ? ch->in_room->area : NULL;

	/* In case something wierd is going on, bail */
	if (parea == NULL)
	{
	    send_to_char("You aren't in an area right now, funky.\n\r",ch);
	    return;
	}
    }
    /* Room vnum provided, so lets go find the area it belongs to */
    else if(is_number(arg1))
    {
	vnum = atoi(arg1);

	/* Hah! No funny vnums! I saw you trying to break it... */
	if (vnum <= 0 || vnum > 65536)
	{
	    send_to_char("The vnum must be between 1 and 65536.\n\r",ch);
	    return;
	}

	/* Search the areas for the appropriate vnum range */
	for (parea = area_first; parea != NULL; parea = parea->next)
	{
	    if(vnum >= parea->min_vnum && vnum <= parea->max_vnum)
		break;
	}

	/* Whoops, vnum not contained in any area */
	if (parea == NULL)
	{
	    send_to_char("There is no area containing that vnum.\n\r",ch);
	    return;
	}
    }
    /* Non-number argument, must be trying for an area name */
    else
    {
	/* Loop the areas, compare the name to argument */
	for(parea = area_first; parea != NULL; parea = parea->next)
	{
	    if(!str_prefix(arg1, parea->name))
		break;
	}

	/* Sorry chum, you picked a goofy name */
	if (parea == NULL)
	{
	    send_to_char("There is no such area.\n\r",ch);
	    return;
	}
    }

    /* Just like in all, trying to fix up the file if provided */
    if (arg2[0] != '\0')
    {
	fclose(fpReserve);
	if( (fp = fopen(arg2, "w")) == NULL)
	{
	    send_to_char("Error opening file, printing to screen.\n\r",ch);
	    fclose(fp);
	    fpReserve = fopen(NULL_FILE, "r");
	    fp = NULL;
	}
    }
    else
	fp = NULL;

    /* And we loop the rooms */
    for(iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for( from_room = room_index_hash[iHash];
	     from_room != NULL;
	     from_room = from_room->next )
	{
	    /* Gotta make sure the room belongs to the desired area */
	    if ( from_room->vnum < parea->min_vnum
	    ||   from_room->vnum > parea->max_vnum )
		continue;

	    /* Room's good, let's check all the directions for exits */
	    for (door = 0; door < 5; door++)
	    {
		if( (pexit = from_room->exit[door]) != NULL )
		{
		    to_room = pexit->u1.to_room;

		    /* Found an exit, does it lead to a different area? */
		    if( to_room != NULL
		    &&  (to_room->vnum < parea->min_vnum
		    ||   to_room->vnum > parea->max_vnum) )
		    {
			found = TRUE;
			sprintf(buf, "%s (%ld) links %s to %s (%ld)\n\r",
				    parea->name, from_room->vnum, dir_name[door],
				    to_room->area->name, to_room->vnum);

			/* File or buffer output? */
			if(fp == NULL)
			    send_to_char(buf, ch);
			else
			    fprintf(fp, buf);
		    }
		}
	    }
	}
    }

    /* Informative message telling you it's not externally linked */
    if(!found)
    {
	send_to_char("No links to other areas found.\n\r",ch);
	/* Let's just delete the file if no links found */
	if (fp)
	    unlink(arg2);
	return;
    }

    /* Close up and clean up file stuff */
    if(fp)
    {
	fclose(fp);
	fpReserve = fopen(NULL_FILE, "r");
    }

}

void do_info(CHAR_DATA *ch, char *argument)
{
	char arg[MIL], arg2[MIL], buf[MSL];
	BUFFER *buffer;
	int low, high, vnum, tfound = 0;
	OBJ_INDEX_DATA *pObjIndex;
	MOB_INDEX_DATA *pMobIndex;
	ROOM_INDEX_DATA *pRoomIndex;

	buffer = new_buf();
	argument = one_argument( argument, arg );
	argument = one_argument( argument, arg2 );
	low = ch->in_room->area->min_vnum;
	high = ch->in_room->area->max_vnum;

	if (arg[0] == '\0')
	{
	  send_to_char("Syntax:                 Short Description:\n\r",ch);
	  send_to_char("info room             - Print room information.\n\r",ch);
	  send_to_char("info rflag            - Print room flags.\n\r",ch);
	  send_to_char("info obj              - Print obj information.\n\r",ch);
	  send_to_char("info type <argument>  - Print obj type list, argument is obj type.\n\r",ch);
	  send_to_char("info mob              - Print mob information.\n\r",ch);
	  return;
	}
	if (!str_cmp(arg,"obj"))
	{
		sprintf(buf,"{W{uVnum    Lvl  Lim  Material   Short Desc{x\n\r");
		add_buf( buffer, buf);
		for ( vnum = low; vnum <= high; vnum++ )
		if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
		{
		  sprintf( buf, "[%5ld] [%2d] [%2d] %-20s %s\n\r", 
			pObjIndex->vnum, pObjIndex->level, pObjIndex->limtotal, 
			pObjIndex->material->name, pObjIndex->short_descr);
		  add_buf( buffer, buf );
		}
	}
	else if (!str_cmp(arg,"type"))
	{
		if (arg2[0] == '\0')
		{
		  print_obj_types(ch);
		  return;
		}
		for ( vnum = low; vnum <= high; vnum++ )
		if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL && !str_cmp(arg2, item_name(pObjIndex->item_type)))
		{
		  tfound = 1;
		  sprintf( buf, "[%5ld] [%2d] [%2d] %-10s %s\n\r", 
			pObjIndex->vnum, pObjIndex->level, pObjIndex->limtotal, 
			pObjIndex->material->name, pObjIndex->short_descr);
		  add_buf( buffer, buf );
		}
	}
	else if (!str_cmp(arg,"mob"))
	{
		sprintf(buf,"Vnum    Lvl  #Mob Wealth     Short Desc\n\r");
		add_buf( buffer, buf);
		for ( vnum = low; vnum <= high; vnum++ )
		if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
		{
		  sprintf( buf, "[%5ld] [%2d] [%2d] [%8ld] %s\n\r", 
			pMobIndex->vnum, pMobIndex->level, pMobIndex->count, 
			pMobIndex->wealth, pMobIndex->short_descr );
		  add_buf( buffer, buf );
		}
	}
	else if (!str_cmp(arg,"room"))
	{
		sprintf(buf,"Vnum    Heal  Mana  Room Name\n\r");
		add_buf( buffer, buf);
		for ( vnum = low; vnum <= high; vnum++ )
		if ( ( pRoomIndex = get_room_index( vnum ) ) != NULL)
		{
		  sprintf( buf, "[%5ld] [%3d] [%3d] %s\n\r", 
			pRoomIndex->vnum, pRoomIndex->heal_rate, 
			pRoomIndex->mana_rate, pRoomIndex->name );
		  add_buf( buffer, buf );
		}
	}
	else if (!str_cmp(arg,"rflag"))
	{
		sprintf(buf,"Vnum    RoomFlags\n\r");
		add_buf( buffer, buf);
		for ( vnum = low; vnum <= high; vnum++ )
		if ( ( pRoomIndex = get_room_index( vnum ) ) != NULL)
		{
		  sprintf( buf, "[%5ld] %s\n\r", 
			pRoomIndex->vnum, flag_string( room_flags, 
			pRoomIndex->room_flags ) );
		  add_buf( buffer, buf );
		}
	}
	else
	{
	  sprintf(buf,"Syntax: info room|rflag|obj|type|mob\n\r");
	  add_buf( buffer, buf);
	}

	if (!str_cmp(arg,"type"))
	{
		if (tfound == 1)
		  send_to_char("Vnum    Lev  Lim #Obj Material   Short Desc\n\r",ch);
		else
		{ // No obj type found in this area, better print it.
		  sprintf(buf,"No object of this type found in this area.\n\r");
		  add_buf( buffer, buf);
		}
	}
	page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
}

void print_obj_types( CHAR_DATA *ch )
{
	char buf[MSL], buf1[MSL];
	int type, col = 0;

	buf1[0] = '\0';

	send_to_char( "{WPossible object types are:{x\n\r\n\r", ch );
	for (type = 0; item_table[type].name != NULL; type++)
	{
	  sprintf( buf, "%-14s", item_table[type].name );
	  strcat( buf1, buf );
	  if ( ++col % 4 == 0 )
		strcat( buf1, "\n\r" );
	}
	if ( col % 4 != 0 )
	strcat( buf1, "\n\r" );

	send_to_char( buf1, ch );
	return;
}

void do_clearreply(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    char buf[MSL];
    char arg[MIL];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	for (d = descriptor_list; d != NULL; d = d->next)
    	{
	    if (d->connected == CON_PLAYING && d->character->reply == ch)
	    {
		printf_to_char(ch, "Replies from %s cleared.\n\r", d->character->name);
		d->character->reply = NULL;
	    } 
    	}
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL || ( IS_NPC(victim) ) )
    {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }

    victim->reply = NULL;
    printf_to_char(ch, "Replies from %s cleared.\n\r", victim->name);
    send_to_char(buf, ch);
    return;
}

char *timestamp(void)
{
    char result[200];
    strftime(result, 200, "%l:%M%P", localtime(&current_time));
    return str_dup(result);
}

void show_alist_free( CHAR_DATA *ch, char *argument )
{
	BUFFER *output;
	AREA_DATA **areas;
	AREA_DATA *pArea;
	char buf[MSL];
	int i = 0;

	output = new_buf();
	areas  = alloc_mem( top_area * sizeof(AREA_DATA *) );

	sprintf(buf, "[%-5s-%5s] %-5s\n\r"
	"------------- -----\n\r", "lvnum", "uvnum", "Total");
	add_buf(output, buf);

	for (pArea = area_first; pArea != NULL; pArea = pArea->next)
	  areas[i++] = pArea;

	qsort(areas, i, sizeof (pArea), compare_area);

	while (--i >= 0)
	{
		if ( i > 0 && areas[i]->max_vnum+1 != areas[i-1]->min_vnum)
		{
		  sprintf(buf, "[%-5ld-%5ld] %5ld\n\r", areas[i]->max_vnum+1, areas[i-1]->min_vnum-1, (((areas[i-1]->min_vnum-1) - 
			(areas[i]->max_vnum+1)) + 1));
		  add_buf(output, buf);
		}
    }

	sprintf(buf, "[%-5ld-%5d] %5ld\n\r", areas[0]->max_vnum+1, MAX_VNUM, ((MAX_VNUM - areas[0]->max_vnum+1) + 1));
	add_buf(output, buf);

	page_to_char(buf_string(output), ch);
	free_buf(output);
	free_mem(areas, top_area * sizeof(AREA_DATA *)  );
	return;
}

int compare_area(const void *v1, const void *v2)
{
	return (*(AREA_DATA * *)v2)->min_vnum - (*(AREA_DATA * *)v1)->min_vnum;
}

void room_echo( ROOM_INDEX_DATA *room, char *argument )
{
    	DESCRIPTOR_DATA *d;
    	char buffer[MAX_STRING_LENGTH*2];

    	if ( argument[0] == '\0' )
    	{
        	return;
    	}

    	for ( d = descriptor_list; d; d = d->next )
    	{
        	if ( d->connected == CON_PLAYING && d->character->in_room == room )
        	{
            		colorconv(buffer, argument, d->character);
            
			if (get_trust(d->character) >= 54)
                		send_to_char( "Room> ",d->character);
            
			send_to_char( buffer, d->character );
            		send_to_char( "\n\r",   d->character );
        	}
    	}
    	return;
}

void do_global_get(CHAR_DATA *ch, char* argument)
{
	if (argument[0] == '\0')
		return send_to_char("Get what globally?\n\r",ch);

	OBJ_DATA *obj;
	OBJ_DATA *objReplace;
	int objID = atoi(argument);
	bool isFound = FALSE;

	for ( obj = object_list; obj != NULL; obj = obj->next )
	{
		if (obj->pIndexData->vnum == objID && !isFound)
		{
			objReplace = obj;
			extract_obj(obj);
			obj_to_char(objReplace,ch);
			isFound = TRUE;
		}
	}
	
	if (!isFound)
	{
		send_to_char("Object not found.\n\r",ch);
	}
	return;
}

