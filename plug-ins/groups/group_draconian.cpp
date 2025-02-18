
/* $Id: group_draconian.cpp,v 1.1.2.11.6.8 2010/01/01 15:48:15 rufina Exp $
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
#include "act_move.h"
#include "gsn_plugin.h"
#include "effects.h"

#include "merc.h"
#include "mercdb.h"
#include "act.h"
#include "def.h"




SPELL_DECL(AcidBreath);
VOID_SPELL(AcidBreath)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    int dam,hp_dam,dice_dam,hpch;

    act_p("$c1 брызгает кислотой на $C4.",ch,0,victim,TO_NOTVICT,POS_RESTING);
    act_p("$c1 брызгает струей едкой кислоты на тебя.",
           ch,0,victim,TO_VICT,POS_RESTING);
    act_p("Ты брызгаешь кислотой на $C4.",ch,0,victim,TO_CHAR,POS_RESTING);

    hpch = max(12,(int)ch->hit);
    hp_dam = number_range(hpch/11 + 1, hpch/6);
    dice_dam = dice(level,16);

    dam = max(hp_dam + dice_dam/10,dice_dam + hp_dam/10);

    if (saves_spell(level,victim,DAM_ACID, ch, DAMF_SPELL))
    {
        acid_effect(victim,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
        damage(ch,victim,dam/2,sn,DAM_ACID,true, DAMF_SPELL);
    }
    else
    {
        acid_effect(victim,level,dam,TARGET_CHAR, DAMF_SPELL);
        damage(ch,victim,dam,sn,DAM_ACID,true, DAMF_SPELL);
    }

}


SPELL_DECL(DragonBreath);
VOID_SPELL(DragonBreath)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
  
  int dam;

  dam = dice(level , 6);
  if (!is_safe_spell(ch, victim, true))
    {
      if (saves_spell(level, victim, DAM_FIRE, ch, DAMF_SPELL))
        dam /= 2;
      damage(ch, victim, dam, sn, DAM_FIRE, true, DAMF_SPELL);
    }

}

SPELL_DECL(DragonsBreath);
VOID_SPELL(DragonsBreath)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
        
        Character *vch, *vch_next;
        int dam,hp_dam,dice_dam;
        int hpch;

        act("Ты призываешь на помощь могущественную силу дракона.", ch,0,0,TO_CHAR);
        act("Дыхание $c2 приобретает силу дракона.", ch,0,victim,TO_ROOM);
        act("Ты дышишь дыханием Повелителя Драконов.", ch,0,0,TO_CHAR);

        hpch = max( 10, (int)ch->hit );
        hp_dam  = number_range( hpch/9+1, hpch/5 );

        if ( ch->is_npc( ) )
                hp_dam /= 6;

        dice_dam = dice(level,20);

        dam = max(hp_dam + dice_dam / 5, dice_dam + hp_dam / 5);

        switch( dice(1,5) )
        {
        case 1:
                fire_effect(victim->in_room,level,dam/2,TARGET_ROOM, DAMF_SPELL);

                for (vch = victim->in_room->people; vch != 0; vch = vch_next)
                {
                        vch_next = vch->next_in_room;

                        if ( is_safe_spell(ch,vch,true)
                                || ( vch->is_npc() && ch->is_npc()
                                        && (ch->fighting != vch && vch->fighting != ch)))
                                continue;

                        if ( is_safe(ch, vch) )
                                continue;

                        if (vch == victim) /* full damage */
                        {
                                if (saves_spell(level,vch,DAM_FIRE,ch, DAMF_SPELL))
                                {
                                        fire_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                                        damage(ch,vch,dam/2,sn,DAM_FIRE,true, DAMF_SPELL);
                                }
                                else
                                {
                                        fire_effect(vch,level,dam,TARGET_CHAR, DAMF_SPELL);
                                        damage(ch,vch,dam,sn,DAM_FIRE,true, DAMF_SPELL);
                                }
                        }
                        else /* partial damage */
                        {
                                if (saves_spell(level - 2,vch,DAM_FIRE,ch, DAMF_SPELL))
                                {
                                        fire_effect(vch,level/4,dam/8,TARGET_CHAR, DAMF_SPELL);
                                        damage(ch,vch,dam/4,sn,DAM_FIRE,true, DAMF_SPELL);
                                }
                                else
                                {
                                        fire_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                                        damage(ch,vch,dam/2,sn,DAM_FIRE,true, DAMF_SPELL);
                                }
                        }
                }
    break;

        case 2:
                if (saves_spell(level,victim,DAM_ACID,ch, DAMF_SPELL))
                {
                        acid_effect(victim,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                        damage(ch,victim,dam/2,sn,DAM_ACID,true, DAMF_SPELL);
                }
                else
                {
                        acid_effect(victim,level,dam,TARGET_CHAR, DAMF_SPELL);
                        damage(ch,victim,dam,sn,DAM_ACID,true, DAMF_SPELL);
                }
                break;

        case 3:
                cold_effect(victim->in_room,level,dam/2,TARGET_ROOM, DAMF_SPELL);

                for (vch = victim->in_room->people; vch != 0; vch = vch_next)
                {
                        vch_next = vch->next_in_room;

                        if ( is_safe_spell(ch,vch,true)
                                || ( vch->is_npc() && ch->is_npc()
                                        && (ch->fighting != vch && vch->fighting != ch)))
                                continue;

                        if ( is_safe(ch, vch) )
                                continue;

                        if (vch == victim) /* full damage */
                        {
                                if (saves_spell(level,vch,DAM_COLD,ch, DAMF_SPELL))
                                {
                                        cold_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                                        damage(ch,vch,dam/2,sn,DAM_COLD,true, DAMF_SPELL);
                                }
                                else
                                {
                                        cold_effect(vch,level,dam,TARGET_CHAR, DAMF_SPELL);
                                        damage(ch,vch,dam,sn,DAM_COLD,true, DAMF_SPELL);
                                }
                        }
                        else
                        {
                                if (saves_spell(level - 2,vch,DAM_COLD,ch, DAMF_SPELL))
                                {
                                        cold_effect(vch,level/4,dam/8,TARGET_CHAR, DAMF_SPELL);
                                        damage(ch,vch,dam/4,sn,DAM_COLD,true, DAMF_SPELL);
                                }
                                else
                                {
                                        cold_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                                        damage(ch,vch,dam/2,sn,DAM_COLD,true, DAMF_SPELL);
                                }
                        }
                }
                break;

        case 4:
                poison_effect(ch->in_room,level,dam,TARGET_ROOM, DAMF_SPELL);

                for (vch = ch->in_room->people; vch != 0; vch = vch_next)
                {
                        vch_next = vch->next_in_room;

                        if ( is_safe_spell(ch,vch,true)
                                || ( ch->is_npc() && vch->is_npc()
                                        && (ch->fighting != vch && vch->fighting != ch)))
                                continue;

                        if ( is_safe(ch, vch) )
                                continue;
                        
                        if (ch->fighting != vch && vch->fighting != ch)
                            yell_panic( ch, vch );

                        if (saves_spell(level,vch,DAM_POISON,ch, DAMF_SPELL))
                        {
                                poison_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                                damage(ch,vch,dam/2,sn,DAM_POISON,true, DAMF_SPELL);
                        }
                        else
                        {
                                poison_effect(vch,level,dam,TARGET_CHAR, DAMF_SPELL);
                                damage(ch,vch,dam,sn,DAM_POISON,true, DAMF_SPELL);
                        }
                }
                break;

        case 5:
                if (saves_spell(level,victim,DAM_LIGHTNING,ch, DAMF_SPELL))
                {
                        shock_effect(victim,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                        damage(ch,victim,dam/2,sn,DAM_LIGHTNING,true, DAMF_SPELL);
                }
                else
                {
                        shock_effect(victim,level,dam,TARGET_CHAR, DAMF_SPELL);
                        damage(ch,victim,dam,sn,DAM_LIGHTNING,true, DAMF_SPELL);
                }
                break;
        }

}

SPELL_DECL(FireBreath);
VOID_SPELL(FireBreath)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    Character *vch, *vch_next;
    int dam,hp_dam,dice_dam;
    int hpch;
    
    act("$c1 выдыхает воронку бушующего огня.", ch,0,victim,TO_NOTVICT);
    act("$c1 выдыхает на тебя воронку бушующего огня!", ch,0,victim,TO_VICT);
    act("Ты выдыхаешь воронку бушующего огня.", ch,0,0,TO_CHAR);

    hpch = max( 10, (int)ch->hit );
    hp_dam  = number_range( hpch/9+1, hpch/5 );
    dice_dam = dice(level,20);

    dam = max(hp_dam + dice_dam /10, dice_dam + hp_dam / 10);
    fire_effect(victim->in_room,level,dam/2,TARGET_ROOM, DAMF_SPELL);

    for (vch = victim->in_room->people; vch != 0; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if ( vch->is_mirror()
            && ( number_percent() < 50 ) ) continue;


        if (is_safe_spell(ch,vch,true)
        ||  ( vch->is_npc() && ch->is_npc()
        &&  (ch->fighting != vch /*|| vch->fighting != ch */)))
            continue;
        if ( is_safe(ch, vch) )
          continue;

        if (vch == victim) /* full damage */
        {
            if (saves_spell(level,vch,DAM_FIRE, ch, DAMF_SPELL))
            {
                fire_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                damage(ch,vch,dam/2,sn,DAM_FIRE,true, DAMF_SPELL);
            }
            else
            {
                fire_effect(vch,level,dam,TARGET_CHAR, DAMF_SPELL);
                damage(ch,vch,dam,sn,DAM_FIRE,true, DAMF_SPELL);
            }
        }
        else /* partial damage */
        {
            if (saves_spell(level - 2,vch,DAM_FIRE, ch, DAMF_SPELL))
            {
                fire_effect(vch,level/4,dam/8,TARGET_CHAR, DAMF_SPELL);
                damage(ch,vch,dam/4,sn,DAM_FIRE,true, DAMF_SPELL);
            }
            else
            {
                fire_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                damage(ch,vch,dam/2,sn,DAM_FIRE,true, DAMF_SPELL);
            }
        }
    }

}

SPELL_DECL(FrostBreath);
VOID_SPELL(FrostBreath)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    
    Character *vch, *vch_next;
    int dam,hp_dam,dice_dam, hpch;
    
    act("$c1 выдыхает леденящую воронку инея!", ch, 0, victim, TO_NOTVICT);
    act("$c1 выдыхает на тебя леденящую воронку инея!", ch, 0, victim, TO_VICT);
    act("Ты выдыхаешь воронку инея.", ch, 0, victim, TO_CHAR);

    hpch = max(12,(int)ch->hit);
    hp_dam = number_range(hpch/11 + 1, hpch/6);
    dice_dam = dice(level,16);

    dam = max(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
    cold_effect(victim->in_room,level,dam/2,TARGET_ROOM, DAMF_SPELL);

    for (vch = victim->in_room->people; vch != 0; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if ( vch->is_mirror()
            && ( number_percent() < 50 ) ) continue;

        if (is_safe_spell(ch,vch,true)
        ||  (vch->is_npc() && ch->is_npc()
        &&   (ch->fighting != vch /*|| vch->fighting != ch*/)))
            continue;
        if ( is_safe(ch, vch) )
          continue;


        if (vch == victim) /* full damage */
        {
            if (saves_spell(level,vch,DAM_COLD, ch, DAMF_SPELL))
            {
                cold_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                damage(ch,vch,dam/2,sn,DAM_COLD,true, DAMF_SPELL);
            }
            else
            {
                cold_effect(vch,level,dam,TARGET_CHAR, DAMF_SPELL);
                damage(ch,vch,dam,sn,DAM_COLD,true, DAMF_SPELL);
            }
        }
        else
        {
            if (saves_spell(level - 2,vch,DAM_COLD, ch, DAMF_SPELL))
            {
                cold_effect(vch,level/4,dam/8,TARGET_CHAR, DAMF_SPELL);
                damage(ch,vch,dam/4,sn,DAM_COLD,true, DAMF_SPELL);
            }
            else
            {
                cold_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
                damage(ch,vch,dam/2,sn,DAM_COLD,true, DAMF_SPELL);
            }
        }
    }

}


SPELL_DECL(GasBreath);
VOID_SPELL(GasBreath)::run( Character *ch, Room *room, int sn, int level ) 
{ 
    Character *vch;
    Character *vch_next;
    int dam,hp_dam,dice_dam,hpch;

    act("$c1 выдыхает воронку ядовитого газа!", ch, 0, 0, TO_ROOM);
    act("Ты выдыхаешь воронку ядовитого газа.", ch, 0, 0, TO_CHAR);

    hpch = max(16,(int)ch->hit);
    hp_dam = number_range(hpch/15+1,8);
    dice_dam = dice(level,12);

    dam = max(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
    poison_effect(room,level,dam,TARGET_ROOM, DAMF_SPELL);

    for (vch = room->people; vch != 0; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if ( vch->is_mirror()
            && ( number_percent() < 50 ) ) continue;

        if (is_safe_spell(ch,vch,true)
        ||  (ch->is_npc() && vch->is_npc()
        &&   (ch->fighting == vch || vch->fighting == ch)))
            continue;
        if ( is_safe(ch, vch) )
          continue;

        if (ch->fighting != vch && vch->fighting != ch)
            yell_panic( ch, vch );

        if (saves_spell(level,vch,DAM_POISON, ch, DAMF_SPELL))
        {
            poison_effect(vch,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
            damage(ch,vch,dam/2,sn,DAM_POISON,true, DAMF_SPELL);
        }
        else
        {
            poison_effect(vch,level,dam,TARGET_CHAR, DAMF_SPELL);
            damage(ch,vch,dam,sn,DAM_POISON,true, DAMF_SPELL);
        }
    }

}


SPELL_DECL(LightningBreath);
VOID_SPELL(LightningBreath)::run( Character *ch, Character *victim, int sn, int level ) 
{ 
    int dam,hp_dam,dice_dam,hpch;

    act("Выдох $c2 ударяет по $C3 разрядом молнии.", ch, 0, victim, TO_NOTVICT);
    act("Выдох $c2 ударяет по тебе разрядом молнии!", ch, 0, victim, TO_VICT);
    act("Твой выдох ударяет по $C3 разрядом молнии.", ch, 0, victim, TO_CHAR);

    hpch = max(10,(int)ch->hit);
    hp_dam = number_range(hpch/9+1,hpch/5);
    dice_dam = dice(level,20);

    dam = max(hp_dam + dice_dam/10,dice_dam + hp_dam/10);

    if (saves_spell(level,victim,DAM_LIGHTNING, ch, DAMF_SPELL))
    {
        shock_effect(victim,level/2,dam/4,TARGET_CHAR, DAMF_SPELL);
        damage(ch,victim,dam/2,sn,DAM_LIGHTNING,true, DAMF_SPELL);
    }
    else
    {
        shock_effect(victim,level,dam,TARGET_CHAR, DAMF_SPELL);
        damage(ch,victim,dam,sn,DAM_LIGHTNING,true, DAMF_SPELL);
    }

}

