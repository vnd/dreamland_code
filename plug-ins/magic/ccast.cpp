/* $Id: ccast.cpp,v 1.1.2.35.6.17 2010-09-01 21:20:45 rufina Exp $
 *
 * ruffina, 2004
 * logic based on 'do_cast' from DreamLand 2.0
 */
#include "wrapperbase.h"
#include "register-impl.h"
#include "lex.h"

#include "profflags.h"
#include "behavior_utils.h"
#include "commandtemplate.h"
#include "skillreference.h"
#include "spellmanager.h"
#include "skill.h"
#include "spell.h"
#include "spelltarget.h"
#include "pcharacter.h"
#include "npcharacter.h"
#include "room.h"

#include "dreamland.h"
#include "clanreference.h"
#include "fight.h"
#include "magic.h"
#include "act.h"
#include "interp.h"
#include "merc.h"
#include "mercdb.h"
#include "handler.h"
#include "def.h"

#include "logstream.h"
#define log(x) LogStream::sendNotice() << x << endl


CLAN(battlerager);
GSN(shielding);
GSN(garble);
GSN(deafen);
BONUS(mana);


static bool mprog_spell( Character *victim, Character *caster, Skill::Pointer skill, bool before )
{
    const char *sname = skill->getName().c_str();
    FENIA_CALL( victim, "Spell", "Csi", caster, sname, before );
    FENIA_NDX_CALL( victim->getNPC( ), "Spell", "CCsi", victim, caster, sname, before );
    BEHAVIOR_CALL( victim->getNPC( ), spell, caster, skill->getIndex(), before );
    return false;
}

static void mprog_cast( Character *caster, SpellTarget::Pointer target, Skill::Pointer skill, bool before )
{
    // Call Fenia progs only once.
    if (before == false) {
        const char *sname = skill->getName().c_str();
        for (Character *rch = caster->in_room->people; rch; rch = rch->next_in_room) {
            FENIA_VOID_CALL( rch, "Cast", "Cs", caster, sname );
            FENIA_NDX_VOID_CALL( rch->getNPC( ), "Cast", "CCs", rch, caster, sname );
        }
    }

    BEHAVIOR_VOID_CALL( caster->getNPC( ), cast, target, skill->getIndex(), before );

}

CMDRUN( cast )
{
    std::basic_ostringstream<char> buf;
    Character *victim;
    int mana, slevel;
    bool offensive;
    Skill::Pointer skill;
    Spell::Pointer spell;
    SpellTarget::Pointer target;
    DLString arguments, spellName; 

    if (ch->is_npc( ) && !( ch->desc != 0 || ch->master != 0 ))
        return;

    if (ch->is_npc( ) && ch->master != 0) {
        if (!ch->getProfession( )->getFlags( ch ).isSet(PROF_CASTER)) {
            act_p( "$C1 говорит тебе '{GЯ не понимаю, чего ты хочешь, хозя$gин|ин|йка.{x'", ch->master, 0, ch, TO_CHAR, POS_RESTING );
            return;
        }
    }
    
    if (!ch->is_npc( ) && !ch->move) {
        ch->send_to("У тебя нет сил даже пошевелить языком.\n\r");
        return;
    }

    if (ch->isAffected(gsn_shielding ) && number_percent( ) > 50) {
        ch->send_to("Ты пытаешься сосредоточиться на заклинании, но что-то останавливает тебя.\n\r");
        return;
    }

    if ((ch->isAffected(gsn_garble ) || ch->isAffected(gsn_deafen )) && number_percent( ) > 50) {
        ch->send_to("Ты не можешь настроиться на правильную интонацию.\n\r");
        return;
    }

    if (HALF_SHADOW(ch)) {
        ch->send_to("Твоя тень поглощает всякую попытку сотворить заклинание.\n\r");
        act_p("$c1 пытается сотворить заклинание, но тень не дает $m сосредоточится.",
                ch, 0, 0, TO_ROOM,POS_RESTING);
        return;
    }

    if (ch->death_ground_delay > 0 && ch->trap.isSet( TF_NO_CAST )) {
        ch->send_to("Тебя занимает более важное занятие - спасение своей жизни.\n\r");
        return;
    }

    arguments = constArguments;
    arguments.stripWhiteSpace( );
    spellName = arguments.getOneArgument( );
    
    if (spellName.empty( )) {
        ch->send_to("Колдовать что и на кого?\n\r");
        return;
    }

    if (ch->getClan( ) == clan_battlerager && !ch->is_immortal( )) {
        ch->send_to("Ты {RBattleRager{x, а не презренный маг!\n\r");
        return;
    }

    if (ch->is_npc( ) && ch->master && ch->master->getClan( ) == clan_battlerager) {
        say_fmt("Хозя%2$Gин|ин|йка, я уважаю твои убеждения.", ch, ch->master);
        return;
    }
    
    spell = SpellManager::lookup( spellName, ch );

    if (!spell) {
        if (ch->is_npc( ) && ch->master) 
            do_say(ch, "Да не умею я");
        else
            ch->send_to("Ты не знаешь такого заклинания.\n\r");

        return;
    }
    
    skill = spell->getSkill( );
    
    if (!spell->checkPosition( ch ))
        return;
    
    if (!skill->usable( ch, true ))
        return;
        
    if (IS_SET(ch->in_room->room_flags,ROOM_NO_CAST)) {
        ch->send_to("Стены этой комнаты поглотили твое заклинание.\n\r");
        act_p("$c1 произне$gсло|с|сла заклинание, но стены комнаты поглотили его.",
                ch, 0, 0, TO_ROOM,POS_RESTING);
        return;
    }

    mana = spell->getManaCost( ch );
    if (!ch->is_npc() && bonus_mana->isActive(ch->getPC(), time_info))
        mana /= 2;

    if (ch->mana < mana) {
        if (ch->is_npc( ) && ch->master != 0) 
            say_fmt("Хозя%2$Gин|ин|йка. У меня мана кончилась!", ch, ch->master);
        else 
            ch->send_to("У тебя не хватает энергии (mana).\n\r");

        return;
    }

    if (!( target = spell->locateTargets( ch, arguments, buf ) )) {
        ch->send_to( buf );
        return;
    }

    victim = target->victim;
    offensive = spell->getSpellType( ) == SPELL_OFFENSIVE;

    if (offensive && ch->is_npc( ) && ch->master && ch->master != victim) {
        if (victim && !victim->is_npc( ))
            say_fmt("Хозя%2$Gин|ин|йка, я %3$Gего|его|её боюсь!", ch, ch->master, victim);
        else
            do_say(ch, "Я не буду делать этого.");

        return;
    }

    spell->utter( ch );
    ch->setWait(spell->getBeats( ) );
    
    if (offensive) {
        UNSET_DEATH_TIME(ch);

        if (victim && is_safe( ch, victim ))
            return;

        if (victim)
            set_violent( ch, victim, false );
        
        yell_panic(ch, victim);
    }
     
    if (spell->spellbane( ch, victim ))
        return;
        
    if (number_percent( ) > skill->getEffective( ch )) {
        ch->send_to("Ты не можешь сконцентрироваться.\n\r");
        skill->improve( ch, false, victim );
        ch->mana -= mana / 2;
        target->castFar = false;
    }
    else {
        bool fForbidCasting = false;
        bool fForbidReaction = false;

        ch->mana -= mana;
        slevel = spell->getSpellLevel( ch, target->range );
        
        if (victim)
            fForbidCasting = mprog_spell( victim, ch, skill, true );
        
        mprog_cast( ch, target, skill, true );

        if (!fForbidCasting)
            spell->run( ch, target, slevel );

        if (victim)
            fForbidReaction = mprog_spell( victim, ch, skill, false );

        mprog_cast( ch, target, skill, false );
        skill->improve( ch, true, victim );

        if (fForbidReaction)
            return;
    }
    
    if (offensive && victim) {
        if (target->castFar && target->door != -1) {
            ch->setLastFightTime( );
            victim->setLastFightTime( );

            if (victim->is_npc( ) && victim->getNPC( )->behavior)
                victim->getNPC( )->behavior->shooted( ch, target->door );
        }
        else
            attack_caster( ch, victim );
    }
}


