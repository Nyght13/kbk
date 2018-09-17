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
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
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

void init_mysql(void)
{
        if (!mysql_init(&conn))
	{
		n_logf("Init_mysql: mysql_init() failed. Reason: %s", mysql_error(&conn));
                return;
	}

        if ((mysql_real_connect(&conn, SQL_SERVER, SQL_USER, SQL_PWD, SQL_DB, 0, NULL, 0)) == NULL)
        {
                mysql_close(&conn);
		n_logf("Init_mysql: mysql_real_connect() failed. Reason: %s", mysql_error(&conn));
                return;
        }

	log_string("Mysql_init: Established connection to MySQL database.");
	return;
}

void close_db( void )
{
	log_string("Close_db: mysql_close() invoked.");
        mysql_close(&conn);
        return;
}

int mysql_safe_query (char *fmt, ...)
{
        va_list         argp;
        int             i = 0;
        double          j = 0;
	long int 	l = 0;
        char            *s = 0, *out = 0, *p = 0;
        char            safe [MAX_STRING_LENGTH];
        char            query [MAX_STRING_LENGTH];
	int		result = -1;
        *query = '\0';
        *safe = '\0';

        va_start (argp, fmt);
                        
        for ( p = fmt, out = query; *p != '\0'; p++ ) {
                if ( *p != '%' ) {
			*out++ = *p;
                        continue;
                }
        
                switch ( *++p ) {
                        case 'c':
                                i = va_arg (argp, int);
				out += sprintf (out, "%c", i);
                                break;
                        case 's':
                                s = va_arg (argp, char *);
				if ( !s ) {
					out += sprintf (out, " ");
					break;
				}
                                mysql_real_escape_string (&conn, safe, s, strlen(s));
                                out += sprintf (out, "%s", safe);
                                *safe = '\0';
                                break;
                        case 'd':
                                i = va_arg (argp, int);
                                out += sprintf (out, "%d", i);
                                break;
                        case 'f':
                                j = va_arg (argp, double);
                                out += sprintf (out, "%f", j);
                                break;
			case 'l':
				l = va_arg(argp, long int);
				out += sprintf(out, "%ld", l);
				break;
                        case '%':
                                out += sprintf (out, "%%");
                                break;
                }
        }
	*out = '\0';

        va_end (argp);

        result = (mysql_real_query (&conn, query, strlen(query)));

        if(mysql_error(&conn)[0] != '\0') {
                n_logf("MySQL Error %d: %s\n\r--- Offending Query ---\n\r%s\n\r", mysql_errno(&conn), mysql_error(&conn), query);
        }

	return result;
}

void do_pktrack(CHAR_DATA *ch, char *argument)
{
        MYSQL_ROW       row;
        MYSQL_RES       *res_set;
	BUFFER *buffer;
	char arg1[MSL], buf[MSL];
	int i = 0;
	bool found = FALSE;

	if(!str_cmp(argument,""))
		return send_to_char("Syntax: pktrack <wins/losses/all/date/location> <value>\n\r",ch);

	buffer = new_buf();
	argument = one_argument(argument, arg1);

        if(!str_cmp(arg1,"wins"))
                mysql_safe_query("SELECT * FROM pklogs WHERE killer RLIKE '%s'", argument);
        else if(!str_cmp(arg1,"losses"))
                mysql_safe_query("SELECT * FROM pklogs WHERE dead RLIKE '%s'", argument);
        else if(!str_cmp(arg1,"all"))
		mysql_safe_query("SELECT * FROM pklogs WHERE killer RLIKE '%s' OR dead RLIKE '%s'", argument, argument);
        else if(!str_cmp(arg1,"date"))
		mysql_safe_query("SELECT * FROM pklogs WHERE time RLIKE '%s'", argument);
        else if(!str_cmp(arg1,"location"))
		mysql_safe_query("SELECT * FROM pklogs WHERE room RLIKE '%s'", argument);
        else return send_to_char("Invalid option.\n\r",ch);

        res_set = mysql_store_result(&conn);

	if (res_set == NULL)
	{
		send_to_char("Error accessing results.\n\r", ch);
		return;
	}
	else if (!mysql_affected_rows(&conn))
	{
		send_to_char("No results found.\n\r", ch);
		return;
	}
        else if (res_set)
        {
                while((row = mysql_fetch_row (res_set)) != NULL)
                {
			sprintf(buf, "%3d) %s killed %s at %s on %s", ++i, row[1], row[0], row[2], row[3]);
			add_buf(buffer, buf);
			found = TRUE;
                }
		if(!found)
			return send_to_char("No matching results found.\n\r",ch);
                mysql_free_result (res_set);
		page_to_char(buf_string(buffer),ch);
		free_buf(buffer);
        }
	else
		send_to_char("Unknown error!\n\r", ch);
	return;
}

void login_log(CHAR_DATA *ch, int type)
{
	char vbuf[MSL];

	if (ch->desc == NULL) return;

	if (ch->in_room != NULL) {
		sprintf(vbuf, "%ld", ch->in_room->vnum);
	}

	mysql_safe_query("INSERT INTO traffic VALUES('%s', INET_ATON(\'%s\'), '%s', CURRENT_TIMESTAMP, %d, '%s')",
		ch->original_name,
		type == LTYPE_AUTO ? ch->desc->ip : ch->desc->ip,
		type == LTYPE_AUTO ? ch->desc->host : ch->desc->host,
		type,
		ch->in_room == NULL ? vbuf : vbuf
	);
}

void saveCharmed(CHAR_DATA *ch)
{
	return;
	CHAR_DATA *charm;

	if (IS_NPC(ch))
		return;

	if (mysql_safe_query("DELETE FROM charmed WHERE owner='%s'", ch->original_name) != 0)
		return;

	for (charm = char_list; charm != NULL; charm = charm->next)
	{
		if (charm->in_room == ch->in_room && charm->leader == ch && IS_NPC(charm))
			charmedToDatabase(charm);
	}
	return;		
}

void charmedToDatabase(CHAR_DATA *charm)
{
	return;
	mysql_safe_query("INSERT INTO charmed VALUES('%s','%d','%s','%s','%s',%d,%d,%d,%d,%d,%d,%d)",
		charm->leader->original_name,
		charm->pIndexData->vnum,
		charm->name,
		charm->short_descr,
		charm->long_descr,
		charm->level,
		charm->max_hit,
		charm->hit,
		charm->alignment, 
		charm->damage[DICE_NUMBER],
		charm->damage[DICE_TYPE],
		charm->damroll
	);
	return;
}

void loadCharmed(CHAR_DATA *ch)
{
	return;
	MYSQL_ROW row;
	MYSQL_RES *result;
	CHAR_DATA *mob;
	long vnum=0;
	AFFECT_DATA af;
	
	mysql_safe_query("SELECT * FROM charmed WHERE owner='%s'", ch->original_name);
	
	result = mysql_store_result(&conn);

	if ((!result) || !mysql_affected_rows(&conn))
		return;

	while((row = mysql_fetch_row(result)) != NULL)
	{
		vnum = atoi(row[1]);
		if (get_mob_index(vnum) == NULL)
		{
			bug("LoadCharmed: invalid vnum %ld.", vnum);
			continue;
		}
		mob = create_mobile(get_mob_index(vnum));
		free_string(mob->name);
		free_string(mob->short_descr);
		free_string(mob->long_descr);
		
		mob->name 		= str_dup(row[2]);
		mob->short_descr 	= str_dup(row[3]);
		mob->long_descr 	= str_dup(row[4]);
		mob->level 		= atoi(row[5]);
		mob->max_hit 		= atoi(row[6]);
		mob->hit		= atoi(row[7]);
		mob->alignment 		= atoi(row[8]);
		mob->damage[DICE_NUMBER]= atoi(row[9]);
		mob->damage[DICE_TYPE]	= atoi(row[10]);
		mob->damroll 		= atoi(row[11]);
		
		char_to_room(mob, ch->in_room);
		add_follower(mob, ch);
		mob->leader = ch;

		init_affect(&af);
		af.where	= TO_AFFECTS;
		af.level	= ch->level;
		af.aftype	= AFT_SPELL;
		af.location	= 0;
		af.modifier	= 0;
		af.duration	= -1;
		af.bitvector	= AFF_CHARM;
		af.type		= skill_lookup("charm person");
		if(mob->pIndexData->vnum==MOB_VNUM_ZOMBIE)
			af.type	= skill_lookup("animate dead");
		affect_to_char(mob,&af);

		if(mob->pIndexData->vnum==MOB_VNUM_ZOMBIE)
		{
			act("$N rises from the earth and bows before $n, awaiting command.",ch,0,mob,TO_ROOM);
			act("$N rises from the earth and bows before you, awaiting command.",ch,0,mob,TO_CHAR);
		}
		else if(mob->pIndexData->vnum==MOB_VNUM_FIRE_ELEMENTAL)
		{
			act("$N blasts into the room in a burst of flames, ready to serve $n.",ch,0,mob,TO_ROOM);
			act("$N blasts into the room in a burst of flames, ready to serve you.",ch,0,mob,TO_CHAR);
		}
		else if(mob->pIndexData->vnum==MOB_VNUM_WATER_ELEMENTAL)
		{
			act("$N rises from a nearby puddle, ready to serve $n.",ch,0,mob,TO_ROOM);
			act("$N rises from a nearby puddle, ready to serve you.",ch,0,mob,TO_CHAR);
		}
		else if(mob->pIndexData->vnum==MOB_VNUM_AIR_ELEMENTAL)
		{
			act("$N emerges from a swirling cyclone, ready to serve $n.",ch,0,mob,TO_ROOM);
			act("$N emerges from a swirling cyclone, ready to serve you.",ch,0,mob,TO_CHAR);
		}
		else if(mob->pIndexData->vnum==MOB_VNUM_EARTH_ELEMENTAL)
		{
			act("The earth rumbles and $N emerges from the ground, ready to serve $n.",ch,0,mob,TO_ROOM);
			act("The earth rumbles and $N emerges from the ground, ready to serve you.",ch,0,mob,TO_CHAR);
		}
		else
		{
			act("$N materializes and nods loyally towards $n.",ch,0,mob,TO_ROOM);
			act("$N materializes and nods loyally towards you.",ch,0,mob,TO_CHAR);
		}
	}
	mysql_free_result(result);
	return;
}

void printCharmed(CHAR_DATA *ch, char *name)
{
	MYSQL_ROW row;
	MYSQL_RES *result;
	BUFFER *buffer;
	int c=0;
	char buf[MSL];
	
	mysql_safe_query("SELECT * FROM charmed WHERE owner='%s'", name);
	
	result = mysql_store_result(&conn);

	buffer = new_buf();

	if ( result && mysql_num_rows(result) )
	{
		send_to_char("Num Vnum     Charmed Name\n\r", ch);
		send_to_char("--------------------------------------------------\n\r", ch);
		while ( (row = mysql_fetch_row(result)) ) 
		{
			c++;
			sprintf(buf, "%-2d) %-6ld - %s\n\r", c, (long)atoi(row[1]), row[3] );
			add_buf(buffer, buf);
		}
		mysql_free_result(result);
		page_to_char(buf_string(buffer),ch);
		free_buf(buffer);
	} else {
		printf_to_char(ch, "No charmed mobs have been recorded for %s.\n\r", name);
	}
	return;
}
			
void do_charmed(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if ( argument[0] == '\0' )
		return send_to_char("Syntax: charmed <player name>\n\r", ch);

	printCharmed(ch, argument);
	return;
}

void pruneDatabase(void)
{
	// Notes
	if (mysql_safe_query("DELETE FROM notes WHERE timestamp + 2592000 < %d", current_time) != 0)
		n_logf("MySQL: Failed to clean notes table.");
	else
		n_logf("MySQL: Notes tables has been cleaned.");

	// Logins
	if (mysql_safe_query("DELETE FROM logins WHERE ctime + 5184000 < %d", current_time) != 0)
		n_logf("MySQL: Failed to clean logins table.");
	else
		n_logf("MySQL: Login records have been cleaned.");
	
	return;
}

void updatePlayerAuth(CHAR_DATA *ch)
{
	//mysql_safe_query("DELETE FROM player_auth WHERE name='%s'", ch->original_name);
	//mysql_safe_query("INSERT INTO player_auth VALUES('%s','%s',%d)", ch->original_name, ch->pcdata->pwd, ch->level);
	return;
}

void delete_char(char *name, bool save_pfile)
{
        char buf2[MSL];
        name    = capitalize(name);

        mysql_safe_query("DELETE FROM bounties WHERE victim='%s'",name);
        n_logf("Delete_char: wiping bounties for %s.", name);

        mysql_safe_query("DELETE FROM charmed WHERE owner='%s'",name);
        n_logf("Delete_char: wiping saved charmed mobs for %s.",name);

	mysql_safe_query("DELETE FROM pklogs WHERE dead='%s' OR killer='%s'",name,name);
	n_logf("Delete_char: wiping pklogs for %s.", name);

        if(save_pfile)
                sprintf(buf2,"mv %s%s.plr %sdead_char/%s.plr",PLAYER_DIR,name,PLAYER_DIR,name);
        else
                sprintf(buf2,"rm %s%s.plr",PLAYER_DIR,name);
        system(buf2);
}

void updatePlayerDb(CHAR_DATA *ch)
{
	return;
	if (IS_NPC(ch))
		return;

	mysql_safe_query("DELETE FROM `player_data` WHERE name='%s'",ch->original_name);
	
	mysql_safe_query("INSERT INTO `player_data` VALUES(NULL,'%s',%d,%d,%d,%d,%d,%d,%d,%d)",
		ch->original_name,
		ch->level,
		ch->race,
		ch->class,
		ch->cabal,
		ch->sex,
		ch->alignment,
		ch->pcdata->ethos,
		current_time);
	return;
}

void do_cabalstat(CHAR_DATA *ch, char *argument)
{
	MYSQL_ROW row;
	MYSQL_RES *res;
	int cabal=0, member_count=0;
	char buf[MSL], time[255], arg[MIL];
	BUFFER *buffer;
	long ttime;

	argument = one_argument( argument, arg );

	if (IS_IMMORTAL(ch)) {
		if ( arg[0] == '\0' ) {
			if (ch->cabal != 0)
				cabal = ch->cabal;
			else
				return send_to_char("You must either specify a cabal or be a member of a cabal.\n\r", ch);
		} else {
			cabal = cabal_lookup(arg);
			if (cabal == 0 || cabal > MAX_CABAL)
				return send_to_char("No such cabal.\n\r", ch);
		}
	} else {
		if (ch->cabal == 0)
			return send_to_char("You aren't in a cabal.\n\r", ch);
		if (ch->pcdata->induct != CABAL_LEADER) 
			return send_to_char("You aren't the leader of your cabal.\n\r", ch);
		cabal = ch->cabal;
	}	

	mysql_safe_query("SELECT * FROM `player_data` WHERE cabal=%d AND LEVEL <= %d", cabal, ch->level);
	res = mysql_store_result(&conn);	
	member_count = mysql_num_rows(res);
	
	send_to_char("Member Name     Last Login\n\r-----------     ----------\n\r", ch);

	buffer = new_buf();

	if (res) 
	{
		while((row = mysql_fetch_row(res)) != NULL)
		{
			member_count++;
			ttime = atoi(row[9]);
			strftime(time, 255, "%A %b %e", localtime(&ttime));
			sprintf(buf, "%-11s     %-10s\n\r", row[1], time);
			add_buf(buffer, buf);
		}
                mysql_free_result(res);
                page_to_char(buf_string(buffer),ch);
                free_buf(buffer);
	} else {
		send_to_char("DB Error.\n\r", ch);
	}		

	printf_to_char(ch, "\n\rTotal Members Found: %d.\n\r", mysql_affected_rows(&conn));
	return;
}	

void do_ltrack(CHAR_DATA *ch, char *argument)
{
	MYSQL_ROW	row;
	MYSQL_RES *	results;
	BUFFER *	buffer;
	char		arg1[MIL], arg2[MIL];
	char		buf[MSL];
	int		type = -1;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		return send_to_char("Syntax: ltrack <name/date/hostname/ip/vnum> [type]\n\r",ch);
	}

	if (arg2[0] != '\0')
	{
		if (!str_cmp(arg2,"new")) {
			type = LTYPE_NEW;
		} else if (!str_cmp(arg2,"in")) {
			type = LTYPE_IN;
		} else if (!str_cmp(arg2,"out")) {
			type = LTYPE_OUT;
		}
	}

	buffer = new_buf();

	sprintf(buf,"%d",type);

	mysql_safe_query("SELECT name, INET_NTOA(ip), hostname, time, type, vnum FROM traffic WHERE (name LIKE '%s' OR INET_NTOA(ip) LIKE '%s' OR hostname LIKE '%%%s%%' or `time` LIKE '%s' or vnum like '%s') %s%s ORDER BY `time` ASC LIMIT 100",
		arg1, arg1, arg1, arg1, arg1, type > -1 ? "AND type=" : "", type > -1 ? buf : ""
	);

	results = mysql_store_result(&conn);
	if (results == NULL) {
		send_to_char("Database error encountered.\n\r",ch);
		return;
	} else if (!mysql_affected_rows(&conn)) {
		send_to_char("No matching results were found.\n\r",ch);
		return;
	} else {
		while((row = mysql_fetch_row(results)) != NULL) {
			type = atoi(row[4]);
			sprintf(buf, "%s : %-8s [%-5ld] %s@%s (%s)\n\r",
				row[3],
				type == LTYPE_NEW ? "NEW CHAR" : type == LTYPE_IN ? "LOGIN" : type == LTYPE_OUT ? "LOG OUT" : "AUTOQUIT",
				atol(row[5]),
				row[0],
				row[2],
				row[1]
			);
			add_buf(buffer, buf);
		}
		mysql_free_result(results);
		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
	}
	return;
}
