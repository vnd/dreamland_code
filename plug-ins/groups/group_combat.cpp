
/* $Id: group_combat.cpp,v 1.1.2.17.6.11 2009/09/11 11:24:54 rufina Exp $
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

#include "spelltemplate.h"

#include "so.h"
#include "pcharacter.h"
#include "room.h"
#include "object.h"
#include "affect.h"
#include "magic.h"
#include "fight.h"
#include "damage.h"
#include "handler.h"
#include "act_move.h"
#include "gsn_plugin.h"

#include "merc.h"
#include "mercdb.h"
#include "act.h"
#include "effects.h"
#include "def.h"



SPELL_DECL(AcetumPrimus);
VOID_SPELL(AcetumPrimus)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 8 );
    if (saves_spell( level, victim, DAM_ACID,ch, DAMF_SPELL ))
        dam /= 2;
    
    act("Плащ первобытной кислоты обволакивает $C4 и, искрясь, поглощает все, к чему прикасается.", ch, 0, victim, TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_ACID,true, DAMF_SPELL);
}

SPELL_DECL(AcidArrow);
VOID_SPELL(AcidArrow)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    dam = dice( level, 12 );
    if ( saves_spell( level, victim, DAM_ACID,ch, DAMF_SPELL ) )
        dam /= 2;
    damage( ch, victim, dam, sn,DAM_ACID,true, DAMF_SPELL);
    return;

}


SPELL_DECL(AcidBlast);
VOID_SPELL(AcidBlast)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    dam = dice( level, 18 );
    if ( saves_spell( level, victim, DAM_ACID,ch, DAMF_SPELL ) )
        dam /= 2;
    damage( ch, victim, dam, sn,DAM_ACID,true, DAMF_SPELL);
    return;

}



SPELL_DECL(BurningHands);
VOID_SPELL(BurningHands)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    dam = dice(level , 2) + 7;
    if ( saves_spell( level, victim,DAM_FIRE,ch, DAMF_SPELL) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_FIRE,true, DAMF_SPELL);
    return;

}


SPELL_DECL(CausticFont);
VOID_SPELL(CausticFont)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    dam = dice( level, 9 );
    if ( saves_spell( level, victim, DAM_ACID,ch, DAMF_SPELL ) )
        dam /= 2;
        
    act("Фонтан едкой жидкости образуется у ног $C2.\n\r"
        "Запах $S разлагающихся тканей просто отвратителен!",
        ch, 0, victim, TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_ACID,true, DAMF_SPELL);
}


SPELL_DECL(ChainLightning);
VOID_SPELL(ChainLightning)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    Character *tmp_vict,*last_vict,*next_vict;
    bool found;
    int dam;

    /* first strike */

    act_p("Разряд молнии, созданный $c5, поражает $C4.",
           ch,0,victim,TO_NOTVICT,POS_RESTING);
    act_p("Созданный тобой разряд молнии поражает $C4.",
           ch,0,victim,TO_CHAR,POS_RESTING);
    act_p("Разряд молнии, созданный $c5, поражает тебя!",
           ch,0,victim,TO_VICT,POS_RESTING);

    dam = dice(level,6);
    if (saves_spell(level,victim,DAM_LIGHTNING,ch, DAMF_SPELL))
        dam /= 3;
    damage(ch,victim,dam,sn,DAM_LIGHTNING,true, DAMF_SPELL);
    
    last_vict = victim;
    level -= 4;   /* decrement damage */

    /* new targets */
    while (level > 0)
    {
        found = false;
        for (tmp_vict = ch->in_room->people;
             tmp_vict != 0;
             tmp_vict = next_vict)
        {
          next_vict = tmp_vict->next_in_room;
          if (!is_safe_spell(ch,tmp_vict,true) && tmp_vict != last_vict)
          {

            if ( tmp_vict->is_mirror()
            && ( number_percent() < 50 ) ) continue;

            found = true;
            last_vict = tmp_vict;
            if (is_safe(ch, tmp_vict) )  {
              act_p( "Разряд молнии затухает, не достигнув $c2.",
                      ch, 0, 0, TO_ROOM,POS_RESTING);
              act_p( "Разряд молнии затухает, не достигнув тебя.",
                      ch, 0, 0, TO_CHAR,POS_RESTING);
            }
            else  {
              act_p("Разряд молнии поражает $c4!",
                     tmp_vict,0,0,TO_ROOM,POS_RESTING);
              act_p("Разряд молнии поражает тебя!",
                     tmp_vict,0,0,TO_CHAR,POS_RESTING);
              dam = dice(level,6);
                
                if (ch->fighting != tmp_vict && tmp_vict->fighting != ch)
                    yell_panic( ch, tmp_vict );

              if (saves_spell(level,tmp_vict,DAM_LIGHTNING,ch, DAMF_SPELL))
                dam /= 3;
              damage(ch,tmp_vict,dam,sn,DAM_LIGHTNING,true, DAMF_SPELL);
              level -= 4;  /* decrement damage */
            }
          }
        }   /* end target searching loop */

        if (!found) /* no target found, hit the caster */
        {
          if (ch == 0 )
            return;

          if (last_vict == ch) /* no double hits */
          {
            act_p("Разряд молнии исчезает.",ch,0,0,TO_ROOM,POS_RESTING);
            act_p("Разряд молнии исчезает, не достигнув тебя.",
                   ch,0,0,TO_CHAR,POS_RESTING);
            return;
          }

          last_vict = ch;
          act_p("Разряд молнии поражает $c4..!",ch,0,0,TO_ROOM,POS_RESTING);
          ch->send_to("Созданная тобой молния поражает тебя же!\n\r");
          dam = dice(level,6);
          if (saves_spell(level,ch,DAM_LIGHTNING,ch, DAMF_SPELL))
           dam /= 3;
          damage(ch,ch,dam,sn,DAM_LIGHTNING,true, DAMF_SPELL);
          level -= 4;  /* decrement damage */
          if (ch == 0)
            return;
        }
    /* now go back and find more targets */
    }

}

TYPE_SPELL(int, ChainLightning)::getMaxRange( Character * ) const
{
    return 0;
}


SPELL_DECL(ChillTouch);
VOID_SPELL(ChillTouch)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    Affect af;
    int dam;

    dam = number_range(1,level);
    if ( !saves_spell( level, victim,DAM_COLD,ch, DAMF_SPELL ) )
    {
        act_p("$c1 замерзает от ледяного прикосновения.",
               victim,0,0,TO_ROOM,POS_RESTING);
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 6;
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = 0;
        affect_join( victim, &af );
    }
    else
    {
        dam /= 2;
    }

    damage( ch, victim, dam, sn, DAM_COLD,true, DAMF_SPELL );
    return;

}


SPELL_DECL(ColourSpray);
VOID_SPELL(ColourSpray)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    dam = dice(level,5) + 13;
    if ( saves_spell( level, victim,DAM_LIGHT,ch, DAMF_SPELL) )
        dam /= 2;
    else
        spell(gsn_blindness, level / 3 * 2, ch,  victim );

    damage( ch, victim, dam, sn, DAM_LIGHT,true, DAMF_SPELL );
    return;

}


SPELL_DECL(DesertFist);
VOID_SPELL(DesertFist)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
        
        int dam;

        if ( (ch->in_room->sector_type != SECT_HILLS)
                && (ch->in_room->sector_type != SECT_MOUNTAIN)
                && (ch->in_room->sector_type != SECT_DESERT) )
        {
                ch->println("Здесь недостаточно песка, чтобы сформировать кулак.");
                ch->wait = 0;
                return;
        }
        
        act("Вихрь песка поднимается с земли, образуя огромный кулак, и ударяет $c4.", victim, 0, 0, TO_ROOM);
        act("Вихрь песка поднимается с земли, образуя огромный кулак, и ударяет тебя.", victim, 0, 0, TO_CHAR);
        dam = dice( level , 14 );

        try {
            damage_nocatch(ch,victim,dam,sn,DAM_OTHER,true, DAMF_SPELL);
            sand_effect(victim,level,dam,TARGET_CHAR, DAMF_SPELL);
        } catch (const VictimDeathException &) {
        }

}



SPELL_DECL(Disruption);
VOID_SPELL(Disruption)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 9 );
    if ( saves_spell( level, victim, DAM_ENERGY,ch, DAMF_SPELL ) )
        dam /= 2;

    act("Загадочная сила охватывает $C4, заставляя усомниться в $S дальнейшем существовании.", ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_ENERGY,true, DAMF_SPELL);
}



SPELL_DECL(EtheralFist);
VOID_SPELL(EtheralFist)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;
    
    if (level <= 50)        dam = dice( level, 12 );
    else if (level <= 75)   dam = dice( level, 15 );
    else                    dam = dice( level, 18 );
        
    if ( saves_spell( level, victim, DAM_ENERGY,ch, DAMF_SPELL ) )
        dam /= 2;
    act("Кулак из темного эфира иных миров сокрушает $C4, повергая $S в ошеломление!",
           ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_ENERGY,true, DAMF_SPELL);
}



SPELL_DECL(Fireball);
VOID_SPELL(Fireball)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 15);
    if ( saves_spell( level, victim, DAM_FIRE,ch, DAMF_SPELL) )
        dam /= 2;

    damage( ch, victim, dam, sn, DAM_FIRE ,true, DAMF_SPELL);
}



SPELL_DECL(Flamestrike);
VOID_SPELL(Flamestrike)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice(level, 10);
    if ( saves_spell( level, victim,DAM_FIRE,ch, DAMF_SPELL) )
        dam /= 2;

    damage( ch, victim, dam, sn, DAM_FIRE ,true, DAMF_SPELL);
}



SPELL_DECL(GalvanicWhip);
VOID_SPELL(GalvanicWhip)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 7 );
    if ( saves_spell( level, victim, DAM_LIGHTNING,ch, DAMF_SPELL ) )
        dam /= 2;
    
    act("$c1 заклинает хлыст ионизированных частиц, который яростно хлещет $C4.",
           ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_LIGHTNING,true, DAMF_SPELL);
}



SPELL_DECL(HandOfUndead);
VOID_SPELL(HandOfUndead)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    if ( saves_spell( level, victim,DAM_NEGATIVE,ch, DAMF_SPELL) )
    {
        if (ch != victim)
            act("Рука умертвия пытается схватить $C4, но безуспешно.", ch, 0, victim, TO_CHAR);
        act("Ты на мгновение почувствова$gло|л|ла озноб.", victim, 0, 0, TO_CHAR);
        return;
    }

    if ( (victim->is_npc() && IS_SET(victim->act,ACT_UNDEAD))
        || IS_VAMPIRE(victim) )
        {
            ch->println("Рука умертвия не властна над твоей жертвой.");
             return;
        }
    if( victim->getModifyLevel() <= 2 )
    {
        dam                 = ch->hit + 1;
    }
    else
    {
     dam = dice( level, 10 );
     victim->mana        /= 2;
     victim->move        /= 2;
     ch->hit                += dam / 2;
    }
    
    victim->println("Ты чувствуешь, как жизнь ускользает от тебя!");
    act("Непостижимая Рука Умертвия хватает $C4!", ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_NEGATIVE,true, DAMF_SPELL);
}



SPELL_DECL(Iceball);
VOID_SPELL(Iceball)::run( Character *ch, Room *room, int sn, int level ) 
{ 
        Character *tmp_vict;
        Character *tmp_next;
        int dam;
        int movedam;

        if (level <= 60)        dam = dice( level, 12 );
        else if (level <= 80)   dam = dice( level, 15 );
        else                    dam = dice( level, 18 );

        movedam     = number_range( ch->getModifyLevel(), 2 * ch->getModifyLevel() );

        for (tmp_vict = room->people; tmp_vict != 0; tmp_vict = tmp_next)
        {
                tmp_next = tmp_vict->next_in_room;

                if ( tmp_vict->is_mirror()
                    && ( number_percent() < 50 ) ) continue;

                if ( !is_safe_spell(ch,tmp_vict,true))
                {
                        if (ch->fighting != tmp_vict && tmp_vict->fighting != ch)
                            yell_panic( ch, tmp_vict );

                        if (saves_spell(level,tmp_vict, DAM_COLD,ch, DAMF_SPELL))
                                dam /= 2;

                        damage( ch, tmp_vict, dam, sn, DAM_COLD, true, DAMF_SPELL );
                        tmp_vict->move -= min((int)tmp_vict->move,movedam);
                }
        }

}


SPELL_DECL(LightningBolt);
VOID_SPELL(LightningBolt)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    dam = dice(level,4) + 12;
    if ( saves_spell( level, victim,DAM_LIGHTNING,ch, DAMF_SPELL) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_LIGHTNING ,true, DAMF_SPELL);
    return;

}


SPELL_DECL(MagicMissile);
VOID_SPELL(MagicMissile)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int mlevel;    

    static const int dam_each[] =
    {
         0,
         3,  3,  4,  4,  5,         6,  6,  6,  6,  6,
         7,  7,  7,  7,  7,         8,  8,  8,  8,  8,
         9,  9,  9,  9,  9,        10, 10, 10, 10, 10,
        11, 11, 11, 11, 11,        12, 12, 12, 12, 12,
        13, 13, 13, 13, 13,        14, 14, 14, 14, 14
    };

    int dam;

    if (victim->isAffected(gsn_shield ))  {
        act("Твоя волшебная стрела исчезает, не достигнув цели.", ch, 0, victim, TO_CHAR);

        if (victim != ch)
            act("Твой щит блокирует волшебную стрелу $c2.", ch, 0, victim, TO_VICT);

        return;
    }

    mlevel = ch->getModifyLevel( );
    level = min(level, (int)( sizeof(dam_each)/sizeof(dam_each[0]) - 1 ) );
    level = max( 0, level);
    
    if( mlevel > 50)
        dam = level / 4;
    else
        dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    
    try {
        if ( saves_spell( level, victim,DAM_ENERGY,ch, DAMF_SPELL) )
            dam /= 2;
        damage_nocatch( ch, victim, dam, sn, DAM_ENERGY ,true, DAMF_SPELL);

        if ( mlevel > 4 )  {
          dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
          if ( saves_spell( level, victim,DAM_ENERGY,ch, DAMF_SPELL) )
              dam /= 2;
          damage_nocatch( ch, victim, dam, sn, DAM_ENERGY ,true, DAMF_SPELL);
        }
        if ( mlevel > 8 )  {
          dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
          if ( saves_spell( level, victim,DAM_ENERGY,ch, DAMF_SPELL) )
              dam /= 2;
          damage_nocatch( ch, victim, dam, sn, DAM_ENERGY ,true, DAMF_SPELL);
        }
        if( mlevel > 12 )  {
          dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
          if ( saves_spell( level, victim,DAM_ENERGY,ch, DAMF_SPELL) )
              dam /= 2;
          damage_nocatch( ch, victim, dam, sn, DAM_ENERGY ,true, DAMF_SPELL);
        }
        if( mlevel > 16 )  {
          dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
          if ( saves_spell( level, victim,DAM_ENERGY,ch, DAMF_SPELL) )
              dam /= 2;
          damage_nocatch( ch, victim, dam, sn, DAM_ENERGY ,true, DAMF_SPELL);
        }
    }
    catch (const VictimDeathException& e) {
    }
}

SPELL_DECL(MagneticTrust);
VOID_SPELL(MagneticTrust)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 8 );
    if ( saves_spell( level, victim, DAM_LIGHTNING,ch, DAMF_SPELL ) )
        dam /= 2;

    act("Твои волосы встают дыбом от ощущения невидимых потоков энергии рядом с тобой.", ch, 0, victim, TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_LIGHTNING,true, DAMF_SPELL);
}

SPELL_DECL(MindWrack);
VOID_SPELL(MindWrack)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 7 );
    if ( saves_spell( level, victim, DAM_MENTAL,ch, DAMF_SPELL ) )
        dam /= 2;
    act("$c1 пристально смотрит на $C4, повергая $S в сонливость.",
           ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_MENTAL,true, DAMF_SPELL);
}


SPELL_DECL(MindWrench);
VOID_SPELL(MindWrench)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 9 );
    if ( saves_spell( level, victim, DAM_MENTAL,ch, DAMF_SPELL ) )
        dam /= 2;
    act("$c1 пристально смотрит на $C4, вызывая в $Z ненормальное оживление.", ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_MENTAL,true, DAMF_SPELL);
}


SPELL_DECL(QuantumSpike);
VOID_SPELL(QuantumSpike)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 9 );
    if ( saves_spell( level, victim, DAM_LIGHTNING,ch, DAMF_SPELL ) )
        dam /= 2;
    act("Кажется, будто $C1 распадается на мельчайшие несвязанные частицы и вновь мучительно собирается воедино.",
           ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_LIGHTNING,true, DAMF_SPELL);
}


SPELL_DECL(SandStorm);
VOID_SPELL(SandStorm)::run( Character *ch, Room *room, int sn, int level ) 
{ 
        Character *vch, *vch_next;
        int dam,hp_dam,dice_dam;
        int hpch;

        if ( ch->in_room->sector_type == SECT_AIR
                || ch->in_room->sector_type == SECT_INSIDE
                || ch->in_room->sector_type == SECT_WATER_SWIM
                || ch->in_room->sector_type == SECT_WATER_NOSWIM )
        {
                ch->send_to("Здесь нет ни крупицы песка!\n\r");
                ch->wait = 0;
                return;
        }

        act_p("$c1 создает песчаную бурю вокруг себя.",ch,0,0,TO_ROOM,POS_RESTING);
        ch->send_to("Ты создаешь песчаную бурю вокруг себя.\n\r");

        hpch = max( 10, (int)ch->hit );
        hp_dam = number_range( hpch / 9 + 1, hpch / 5 );
        if ( ch->is_npc() )
                hp_dam /= 8;
        dice_dam = dice(level,18);

        dam = max(hp_dam + dice_dam /10, dice_dam + hp_dam / 10);
        sand_effect(room,level,dam/2,TARGET_ROOM, DAMF_SPELL);

        for ( vch = room->people; vch != 0; vch = vch_next )
        {
                vch_next = vch->next_in_room;

                if ( is_safe_spell(ch,vch,true )
                        || ( vch->is_npc()
                                && ch->is_npc()
                                && ( ch->fighting != vch && vch->fighting != ch)))
                        continue;
                if ( is_safe(ch, vch) )
                        continue;

                if ( saves_spell(level,vch,DAM_COLD,ch, DAMF_SPELL) )
                {
                        sand_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                        damage(ch,vch,dam/2,sn,DAM_COLD,true, DAMF_SPELL);
                }
                else
                {
                        sand_effect(vch,level,dam,TARGET_CHAR, DAMF_SPELL);
                        damage(ch,vch,dam,sn,DAM_COLD,true, DAMF_SPELL);
                }
        }

}

SPELL_DECL(ShockingGrasp);
VOID_SPELL(ShockingGrasp)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    

    static const int dam_each[] =
    {
         6,
         8,  10,  12,  14,  16,         18, 20, 25, 29, 33,
        36, 39, 39, 39, 40,        40, 41, 41, 42, 42,
        43, 43, 44, 44, 45,        45, 46, 46, 47, 47,
        48, 48, 49, 49, 50,        50, 51, 51, 52, 52,
        53, 53, 54, 54, 55,        55, 56, 56, 57, 57
    };

    int dam;

    level        = min(level, ( int )( sizeof(dam_each)/sizeof(dam_each[0]) - 1 ) );
    level        = max( 0 , level);
        if( ch->getModifyLevel() > 50)
    dam         = level / 2 ;
        else
    dam                = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, victim,DAM_LIGHTNING,ch, DAMF_SPELL) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_LIGHTNING ,true, DAMF_SPELL);
    return;

}


SPELL_DECL(SonicResonance);
VOID_SPELL(SonicResonance)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    dam = dice( level, 7 );
    if ( saves_spell( level, victim, DAM_ENERGY,ch, DAMF_SPELL ) )
        dam /= 2;
    act_p("Цилиндр вибрирующей энергии окружает $C4, заставляя $S резонировать.",
                ch,0,victim,TO_NOTVICT,POS_RESTING);
    victim->setWait( skill->getBeats( )  );
    damage( ch, victim, dam, sn,DAM_ENERGY,true, DAMF_SPELL);
    return;

}


SPELL_DECL(SpectralFuror);
VOID_SPELL(SpectralFuror)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 8 );
    if ( saves_spell( level, victim, DAM_ENERGY,ch, DAMF_SPELL ) )
        dam /= 2;

    act("Космическая материя яростно искажается вокруг $C2!", ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_ENERGY,true, DAMF_SPELL);
}



SPELL_DECL(SulfurusSpray);
VOID_SPELL(SulfurusSpray)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam;

    dam = dice( level, 7 );
    if ( saves_spell( level, victim, DAM_ACID,ch, DAMF_SPELL ) )
        dam /= 2;
    act("Вонючая струя серной жидкости низвергается дождем на $C4.",
           ch,0,victim,TO_NOTVICT);
    damage( ch, victim, dam, sn,DAM_ACID,true, DAMF_SPELL);
    return;

}


SPELL_DECL(VampiricBlast);
VOID_SPELL(VampiricBlast)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam;

    dam = dice( level, 12);
    if ( saves_spell( level, victim, DAM_ACID,ch, DAMF_SPELL ) )
        dam /= 2;
    damage( ch, victim, dam, sn,DAM_ACID,true, DAMF_SPELL);
}

SPELL_DECL(MagicArrow);
VOID_SPELL(MagicArrow)::run( Character *ch, Character *victim, int sn, int level )
{
    int dam;

    dam = dice( level, 1 );
    if ( saves_spell( level, victim, DAM_ENERGY,ch, DAMF_SPELL ) )
            dam /= 2;
    damage( ch, victim, dam, sn, DAM_ENERGY, true, DAMF_SPELL);
}


SPELL_DECL(Hurricane);
VOID_SPELL(Hurricane)::run( Character *ch, Room *room, int sn, int level ) 
{ 
    Character *vch;
    Character *vch_next;
    int dam,hp_dam,dice_dam,hpch;

    act_p("$c1 призывает повелителя ураганов на помощь.",
           ch,0,0,TO_NOTVICT,POS_RESTING);
    act_p("Ты призываешь повелителя ураганов на помощь.",
           ch,0,0,TO_CHAR,POS_RESTING);

    hpch = max(16,(int)ch->hit);
    hp_dam = number_range(hpch/15+1,8);
    dice_dam = dice(level,12);

    dam = max(hp_dam + dice_dam/10,dice_dam + hp_dam/10);

    for (vch = room->people; vch != 0; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (is_safe_spell(ch,vch,true))
            continue;

        if (ch->is_npc() && vch->is_npc()
            && ch->fighting != vch && vch->fighting != ch)
            continue;

        if (is_safe(ch, vch))
          continue;
          
        if (ch->fighting != vch && vch->fighting != ch)
            yell_panic( ch, vch );

        if (!is_flying( vch )) dam /= 2;

        if (vch->size == SIZE_TINY)  dam = ( int )( dam * 1.5 );
        else if (vch->size == SIZE_SMALL)  dam = ( int )( dam * 1.3 );
        else if (vch->size == SIZE_MEDIUM)  dam *= 1;
        else if (vch->size == SIZE_LARGE)  dam = ( int )( dam * 0.9 );
        else if (vch->size == SIZE_HUGE)  dam = ( int )( dam * 0.7 );
        else dam = ( int )( dam * 0.5 );

        if (saves_spell(level,vch,DAM_OTHER, ch, DAMF_SPELL))
            damage(ch,vch,dam/2,sn,DAM_OTHER,true, DAMF_SPELL);
        else
            damage(ch,vch,dam,sn,DAM_OTHER,true, DAMF_SPELL);
    }

}


