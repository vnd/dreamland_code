/* $Id$
 *
 * ruffina, 2004
 */
/***************************************************************************
 * Все права на этот код 'Dream Land' пренадлежат Igor {Leo} и Olga {Varda}*
 * Некоторую помощь в написании этого кода, а также своими идеями помогали:*
 *    Igor S. Petrenko     {NoFate, Demogorgon}                            *
 *    Koval Nazar          {Nazar, Redrum}                                 *
 *    Doropey Vladimir     {Reorx}                                         *
 *    Kulgeyko Denis       {Burzum}                                        *
 *    Andreyanov Aleksandr {Manwe}                                         *
 *    и все остальные, кто советовал и играл в этот MUD                    *
 ***************************************************************************/
/***************************************************************************
 *     ANATOLIA 2.1 is copyright 1996-1997 Serdar BULUT, Ibrahim CANPUNAR  *        
 *     ANATOLIA has been brought to you by ANATOLIA consortium                   *
 *         Serdar BULUT {Chronos}                bulut@rorqual.cc.metu.edu.tr       *        
 *         Ibrahim Canpunar  {Asena}        canpunar@rorqual.cc.metu.edu.tr    *        
 *         Murat BICER  {KIO}                mbicer@rorqual.cc.metu.edu.tr           *        
 *         D.Baris ACAR {Powerman}        dbacar@rorqual.cc.metu.edu.tr           *        
 *     By using this code, you have agreed to follow the terms of the      *
 *     ANATOLIA license, in the file Anatolia/anatolia.licence             *        
 ***************************************************************************/

/***************************************************************************
 *  This file is a combination of:                                         *
 *                (H)unt.c, (E)nter.c, (R)epair.c and (A)uction.c            *
 *  Thus it is called ACT_HERA.C                                           *
 **************************************************************************/
 
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
*        ROM 2.4 is copyright 1993-1995 Russ Taylor                           *
*        ROM has been brought to you by the ROM consortium                   *
*            Russ Taylor (rtaylor@pacinfo.com)                                   *
*            Gabrielle Taylor (gtaylor@pacinfo.com)                           *
*            Brian Moore (rom@rom.efn.org)                                   *
*        By using this code, you have agreed to follow the terms of the           *
*        ROM license, in the file Rom24/doc/rom.license                           *
***************************************************************************/


#include "skill.h"
#include "spell.h"

#include "char.h"
#include "commandtemplate.h"
#include "affect.h"
#include "room.h"

#include "dreamland.h"
#include "merc.h"
#include "descriptor.h"
#include "ban.h"
#include "act.h"
#include "gsn_plugin.h"
#include "mercdb.h"
#include "npcharacter.h"
#include "pcharacter.h"
#include "race.h"
#include "object.h"

#include "wiznet.h"
#include "def.h"
#include "handler.h"
#include "act_move.h"
#include "vnum.h"

/***************************************************************************
 ************************      auction.c      ******************************
 ***************************************************************************/

#define PULSE_AUCTION             (45 * dreamland->getPulsePerSecond( )) /* 60 seconds */

void talk_auction(const char *argument)
{
    Descriptor *d;
    char buf[MAX_STRING_LENGTH];

    sprintf (buf,"{YAUCTION: %s{x", argument);

    for (d = descriptor_list; d != 0; d = d->next) {
        if (d->connected != CON_PLAYING)
            continue;

        Character *ch = d->character;

        if (!ch)
            continue;

        if (IS_SET(ch->getPC( )->comm, COMM_NOAUCTION))
            continue;
        
        act_p(buf, ch, 0, 0, TO_CHAR,POS_RESTING);
    }
}


/*
  This function allows the following kinds of bets to be made:

  Absolute bet
  ============

  bet 14k, bet 50m66, bet 100k

  Relative bet
  ============

  These bets are calculated relative to the current bet. The '+' symbol adds
  a certain number of percent to the current bet. The default is 25, so
  with a current bet of 1000, bet + gives 1250, bet +50 gives 1500 etc.
  Please note that the number must follow exactly after the +, without any
  spaces!

  The '*' or 'x' bet multiplies the current bet by the number specified,
  defaulting to 2. If the current bet is 1000, bet x  gives 2000, bet x10
  gives 10,000 etc.

*/

int advatoi (const char *s)
/*
  util function, converts an 'advanced' ASCII-number-string into a number.
  Used by parsebet() but could also be used by do_give or do_wimpy.

  Advanced strings can contain 'k' (or 'K') and 'm' ('M') in them, not just
  numbers. The letters multiply whatever is left of them by 1,000 and
  1,000,000 respectively. Example:

  14k = 14 * 1,000 = 14,000
  23m = 23 * 1,000,0000 = 23,000,000

  If any digits follow the 'k' or 'm', the are also added, but the number
  which they are multiplied is divided by ten, each time we get one left. This
  is best illustrated in an example :)

  14k42 = 14 * 1000 + 14 * 100 + 2 * 10 = 14420

  Of course, it only pays off to use that notation when you can skip many 0's.
  There is not much point in writing 66k666 instead of 66666, except maybe
  when you want to make sure that you get 66,666.

  More than 3 (in case of 'k') or 6 ('m') digits after 'k'/'m' are automatically
  disregarded. Example:

  14k1234 = 14,123

  If the number contains any other characters than digits, 'k' or 'm', the
  function returns 0. It also returns 0 if 'k' or 'm' appear more than
  once.

*/

{

/* the pointer to buffer stuff is not really necessary, but originally I
   modified the buffer, so I had to make a copy of it. What the hell, it
   works:) (read: it seems to work:)
*/

  char string[MAX_INPUT_LENGTH]; /* a buffer to hold a copy of the argument */
  char *stringptr = string; /* a pointer to the buffer so we can move around */
  char tempstring[2];       /* a small temp buffer to pass to atoi*/
  int number = 0;           /* number to be returned */
  int multiplier = 0;       /* multiplier used to get the extra digits right */


  strcpy (string,s);        /* working copy */

  while ( isdigit (*stringptr)) /* as long as the current character is a digit */
  {
      strncpy (tempstring,stringptr,1);           /* copy first digit */
      number = (number * 10) + atoi (tempstring); /* add to current number */
      stringptr++;                                /* advance */
  }

  switch (Char::upper(*stringptr)) {
      case 'K'  : multiplier = 1000;    number *= multiplier; stringptr++; break;
      case 'M'  : multiplier = 1000000; number *= multiplier; stringptr++; break;
      case '\0' : break;
      default   : return 0; /* not k nor m nor NUL - return 0! */
  }

  while ( isdigit (*stringptr) && (multiplier > 1)) /* if any digits follow k/m, add those too */
  {
      strncpy (tempstring,stringptr,1);           /* copy first digit */
      multiplier = multiplier / 10;  /* the further we get to right, the less are the digit 'worth' */
      number = number + (atoi (tempstring) * multiplier);
      stringptr++;
  }

  if (*stringptr != '\0' && !isdigit(*stringptr)) /* a non-digit character was found, other than NUL */
    return 0; /* If a digit is found, it means the multiplier is 1 - i.e. extra
                 digits that just have to be ignore, liked 14k4443 -> 3 is ignored */


  return (number);
}


int parsebet (const int currentbet, const char *argument)
{
        int newbet = 0;                /* a variable to temporarily hold the new bet */
        char string[MAX_INPUT_LENGTH]; /* a buffer to modify the bet string */
        char *stringptr = string;      /* a pointer we can move around */
        char buf2[MAX_STRING_LENGTH];

        strcpy (string,argument);      /* make a work copy of argument */

        if (*stringptr)               /* check for an empty string */
        {
                if (isdigit (*stringptr)) /* first char is a digit assume e.g. 433k */
                        newbet = advatoi (stringptr); /* parse and set newbet to that value */
    else if (*stringptr == '+') /* add ?? percent */
                {
                        if (strlen (stringptr) == 1) /* only + specified, assume default */
                                newbet = (currentbet * 125) / 100; /* default: add 25% */
                        else
                                newbet = (currentbet * (100 + atoi (++stringptr))) / 100; /* cut off the first char */
                }
                else
                {
                        sprintf (buf2,"considering: * x \n\r");
                        if ((*stringptr == '*') || (*stringptr == 'x')) /* multiply */
                        {
                                if (strlen (stringptr) == 1) /* only x specified, assume default */
                                        newbet = currentbet * 2 ; /* default: twice */
                                else /* user specified a number */
                                        newbet = currentbet * atoi (++stringptr); /* cut off the first char */
                        }
                }
        }

        return newbet;        /* return the calculated bet */
}

void auction_update (void)
{
    char buf[MAX_STRING_LENGTH];
    if (auction->item != 0)
        if (--auction->pulse <= 0) /* decrease pulse */
        {
            auction->pulse = PULSE_AUCTION;
            switch (++auction->going) /* increase the going state */
            {
            case 1 : /* going once */
            case 2 : /* going twice */
            if (auction->bet > 0)
            {
                talk_auction(fmt(0, "%1$O1{Y: буд%1$nет|ут прода%1$Gно|н|на|ны за %2$d золот%3$s - %4$s{x.", 
                        auction->item,
                        auction->bet,
                        GET_COUNT(auction->bet,"ую монету","ые монеты","ых монет"),
                        ((auction->going == 1) ? "раз" : "два")).c_str( ));
                break;
            }
            else
            {
                sprintf (buf, "%s{Y: ставок не получено - %s{x.", auction->item->getShortDescr( '1' ).c_str( ),
                     ((auction->going == 1) ? "раз" : "два"));
                talk_auction (buf);
                if (auction->startbet != 0)
                {
                  sprintf(buf, "Начальная цена: %d золот%s{x.", auction->startbet,
                                GET_COUNT(auction->startbet,"ая монета","ые монеты","ых монет"));
                  talk_auction(buf);
                }
                break;
            }
            case 3 : /* SOLD! */

            if (auction->bet > 0)
            {
                sprintf (buf, "%s получает %s{Y за %d золот%s{x.",
                    auction->buyer->getNameP( '1' ).c_str( ),
                    auction->item->getShortDescr( '4' ).c_str( ), auction->bet,
                    GET_COUNT(auction->bet,"ую монету","ые монеты","ых монет"));
                talk_auction(buf);
                obj_to_char (auction->item,auction->buyer);
                act_p("Из дымки появляется аукционер и передает тебе $o4.",
                     auction->buyer,auction->item,0,TO_CHAR,POS_DEAD);
                act_p("$c1 получает от прибывшего аукционера $o4.",
                     auction->buyer,auction->item,0,TO_ROOM,POS_RESTING);

                auction->seller->gold += auction->bet; /* give him the money */
                act_p("Из дымки появляется аукционер и передает тебе вырученные деньги.",
                     auction->seller,auction->item,0,TO_CHAR,POS_DEAD);
                act_p("$c1 получает вырученные деньги от прибывшего аукционера.",
                     auction->seller,auction->item,0,TO_ROOM,POS_RESTING);

                auction->item = 0; /* reset item */
                auction->seller = 0;

            }
            else /* not sold */
            {
                sprintf (buf, "Ставок не получено - %s{Y снят с аукциона{x.",auction->item->getShortDescr( '1' ).c_str( ));
                talk_auction(buf);

                act_p("Из дымки перед тобой появляется аукционер и возвращает тебе {W$o4{w.",
                      auction->seller,auction->item,0,TO_CHAR,POS_DEAD);
                act_p("Аукционер появляется перед $c5 и возвращает $m {W$o4{w.",
                      auction->seller,auction->item,0,TO_ROOM,POS_RESTING);
                obj_to_char (auction->item,auction->seller);
                auction->item = 0; /* clear auction */
                auction->seller = 0;
            } /* else */

            } /* switch */
        } /* if */
} /* func */


CMDRUNP( auction )
{
        Object *obj;
        char arg1[MAX_INPUT_LENGTH];
        char buf[MAX_STRING_LENGTH];
        char betbuf[MAX_STRING_LENGTH];
        argument = one_argument (argument, arg1);

        if (ch->is_npc())    /* NPC extracted can't auction! */
                return;

        if (IS_SET(ch->comm,COMM_NOAUCTION))
        {
                if (arg_is_switch_on( arg1 ))
                {
                        ch->send_to("Канал Аукциона (Auction) теперь {Rвключен{x.\n\r");
                        REMOVE_BIT(ch->comm,COMM_NOAUCTION);
                        return;
                }
                else
                {
                        ch->send_to("Канал Аукциона (Auction) теперь {Rвыключен{x.\n\r");
                        ch->send_to("Для получения информации по этому каналу включите его.\n\r");
                        return;
                }
        }

        if (arg1[0] == '\0')
        {
                if (auction->item != 0)
                {
                        if ( ch->is_immortal() )
                        {
                                sprintf(buf,"Продавец: %s Текущая ставка: %s\n\r",
                                        auction->seller->getNameP(),
                                        auction->buyer ? auction->buyer->getNameP() : "Нет");
                                ch->send_to(buf);
                        }
                        /* show item data here */
                        if (auction->bet > 0)
                        {
                                sprintf (buf, "Текущая ставка на выставленный лот - %d золот%s{x.\n\r",
                                        auction->bet,
                                        GET_COUNT(auction->bet,"ая монета","ые монеты","ых монет"));
                                ch->send_to( buf);
                        }
                        else
                        {
                                sprintf (buf, "Ставок на выставленный лот не получено{x.\n\r");
                                ch->send_to( buf);
                                if (auction->startbet != 0)
                                {
                                        sprintf(buf, "Начальная цена: %d золот%s{x.\n\r", auction->startbet,
                                                GET_COUNT(auction->startbet,"ая монета","ые монеты","ых монет"));
                                        ch->send_to( buf);
                                }
                        }

                        obj = auction->item;
                        if ( ch->is_immortal() )
                        {
                            if (gsn_identify->getSpell( ))
                                gsn_identify->getSpell( )->run( ch, auction->item, gsn_identify, 0 );
                            return;
                        }
                        sprintf( buf,
                                "Лот: '%s{x'. Тип: %s. Экстра флаги: %s.\n\rВес: %d. Стоимость: %d. Уровень: %d.\n\r",
                                obj->getShortDescr( '1' ).c_str( ),
                                item_table.message(obj->item_type).c_str( ), 
                                extra_flags.messages( obj->extra_flags).c_str( ),
                                obj->weight / 10, obj->cost, obj->level );
                        ch->send_to( buf);

                        {        
                            map<DLString, bool> purposes;
                            map<DLString, bool>::iterator p;

                            for (int i = 0; i < wearlocationManager->size( ); i++) {
                                Wearlocation *loc = wearlocationManager->find( i );
                                if (loc->matches( obj ))
                                    purposes[loc->getPurpose( )] = true;
                            }
                            for (p = purposes.begin( ); p != purposes.end( ); p++)
                                ch->println( p->first.c_str( ) );
                        }

                        if  (obj->item_type == ITEM_WEAPON) {
                                ch->printf("Тип оружия: %s (%s)\r\n",
                                           weapon_class.message(obj->value[0] ).c_str( ),
                                           weapon_class.name( obj->value[0] ).c_str( )
                                          );
                        }

                        return;
                }
                else
                {
                        ch->send_to( "{RВыставить на Аукцион ЧТО{x?\n\r");
                        return;
                }
        }

        if (arg_is_switch_off( arg1 ))
        {
                ch->send_to("Канал Аукциона (Auction) теперь {Rвыключен{x.\n\r");
                SET_BIT(ch->comm,COMM_NOAUCTION);
                return;
        }
        
        if (arg_oneof_strict( arg1, "talk", "реклама", "говорить" ))
        {
            if ( ch != auction->seller ) {
                ch->send_to("Ты ничего не выставлял на аукцион - рекламировать тебе нечего.\r\n");
                return;
            }
            if (argument[0] == '\0') {
                ch->send_to("Как ты хочешь разрекламировать товар?\r\n");
                return;
            }
            
            REMOVE_BIT(ch->comm,COMM_NOAUCTION);
            talk_auction( argument );
            return;
        }
        
        if (ch->is_immortal() && arg_oneof_strict( arg1, "stop", "стоп" ))
        {
                if (auction->item == 0)
                {
                        ch->send_to("На аукцион ничего не выставлено. Будь внимательней!\n\r");
                        return;
                }
                else /* stop the auction */
                {
                        sprintf(buf,"Продажа остановлена Богами. Лот '%s{Y' конфискован{x.",
                                auction->item->getShortDescr( '1' ).c_str( ));
                        talk_auction(buf);
                        obj_to_char(auction->item, auction->seller);
                        auction->item = 0;
                        auction->seller = 0;
                        if (auction->buyer != 0) /* return money to the buyer */
                        {
                                auction->buyer->gold += auction->bet;
                                auction->buyer->send_to("Твои деньги возвращены.\n\r");
                        }
                        return;
                }
        }

        if (arg_oneof_strict( arg1, "bet", "ставка" ))
        {
                if (auction->item != 0)
                {
                        int newbet;

                        if ( ch == auction->seller )
                        {
                                ch->send_to("Ты не можешь купить свой же лот..:)\n\r");
                                return;
                        }
                        /* make - perhaps - a bet now */
                        if (argument[0] == '\0')
                        {
                                ch->send_to("Ставка (Bet) сколько?\n\r");
                                return;
                        }

                        newbet = parsebet (auction->bet, argument);
                        sprintf (betbuf,"Ставка: %d\n\r",newbet);

                        if ((auction->startbet != 0) && (newbet < (auction->startbet + 1)))
                        {
                                ch->send_to("Тебе необходимо повысить ставку хотя бы на 1 золотой выше начальной цены.\n\r");
                                return;
                        }

                        if (newbet < (auction->bet + 1))
                        {
                                ch->send_to("Тебе необходимо повысить ставку хотя бы на 1 золотой выше текущей ставки.\n\r");
                                return;
                        }

                        if (newbet > ch->gold)
                        {
                                ch->send_to("У тебя нет необходимой суммы!\n\r");
                                return;
                        }

                        /* the actual bet is OK! */

                        /* return the gold to the last buyer, if one exists */
                        if (auction->buyer != 0)
                                auction->buyer->gold += auction->bet;

                        ch->gold -= newbet; /* substract the gold - important :) */
                        auction->buyer = ch;
                        auction->bet   = newbet;
                        auction->going = 0;
                        auction->pulse = PULSE_AUCTION; /* start the auction over again */

                        sprintf( buf, "На %s{Y получена новая ставка: %d золот%s{x.\n\r",
                                auction->item->getShortDescr( '4' ).c_str( ),newbet,
                                GET_COUNT(newbet,"ая монета","ые монеты","ых монет"));
                        talk_auction( buf );
                        return;

                }
                else
                {
                        ch->send_to("В данный момент на аукцион ничего не выставлено.\n\r");
                        return;
                }
        }

        /* finally... */

        obj = get_obj_carry (ch, arg1 ); /* does char have the item ? */

        if (obj == 0)
        {
                ch->send_to("У тебя нет этого.\n\r");
                return;
        }

        if (obj->timer != 0)
        {
                sprintf( buf, "Этот предмет не может быть выставлен на аукцион, т.к. исчезнет через %d часов.\n\r", obj->timer );
                ch->send_to( buf);
                return;
        }

        if (IS_OBJ_STAT(obj, ITEM_NOSELL)) {
            ch->send_to("Этот предмет не подлежит продаже.\r\n");
            return;
        }

        if (auction->item == 0) {
                if (ch->desc && banManager->check( ch->desc, BAN_COMMUNICATE )) {
                    ch->println( "Ты не можешь ничего выставлять на аукцион." );
                    return;
                }

                switch (obj->item_type)
                {
                case ITEM_MONEY:
                case ITEM_CORPSE_PC:
                case ITEM_CORPSE_NPC:
                case ITEM_TATTOO:
                        act_p("Ты не можешь выставить на аукцион $T.",
                                ch, 0, item_table.message(obj->item_type).c_str( ),TO_CHAR,POS_SLEEPING);
                        return;
                default:
                        obj_from_char (obj);
                        auction->item = obj;
                        auction->bet = 0;         /* obj->cost / 100 */
                        auction->startbet = parsebet (auction->startbet, argument);
                        auction->buyer = 0;
                        auction->seller = ch;
                        auction->pulse = PULSE_AUCTION;
                        auction->going = 0;

                        sprintf(buf, "На аукцион выставлен новый лот: %s{x.",
                                obj->getShortDescr( '1' ).c_str( ));
                        talk_auction( buf );
                        if (auction->startbet == 0)
                        {
                                sprintf(buf, "Начальная цена владельцем не установлена{x.");
                                talk_auction( buf );
                        }
                        else
                        {
                                sprintf(buf, "Начальная цена: %d золот%s{x.", auction->startbet,
                                        GET_COUNT(auction->startbet,"ая монета","ые монеты","ых монет"));
                                talk_auction( buf );
                        }
                        wiznet( 0, 0, 0, "Продавец - %C1", ch );
                        return;

                } /* switch */
        }
        else
        {
                act_p("Попробуй позже! Кто-то другой уже выставил на аукцион $o4!",
                        ch,auction->item,0,TO_CHAR,POS_RESTING);
                return;
        }
}

