/* $Id$
 *
 * ruffina, 2004
 */
/***************************************************************************
                          pcharacter.cpp  -  description
                             -------------------
    begin                : Thu May 3 2001
    copyright            : (C) 2001 by Igor S. Petrenko
    email                : nofate@europe.com
 ***************************************************************************/

#include <fstream>

#include "class.h"
#include "logstream.h"
#include "grammar_entities_impl.h"
#include "ru_pronouns.h"

#include "skill.h"
#include "skillmanager.h"
#include "skillreference.h"
#include "skillgroup.h"
#include "clanreference.h"

#include "pcharacter.h"
#include "npcharacter.h"
#include "pcharactermemory.h"
#include "pcrace.h"
#include "affect.h"
#include "object.h"
#include "room.h"
#include "desire.h"

#include "dreamland.h"
#include "merc.h"
#include "mercdb.h"
#include "def.h"

CLAN(none);
PROF(none);
HOMETOWN(none);


/**************************************************************************
 * PlayerAge 
 **************************************************************************/
const int PlayerAge::BASE_AGE = 17;
const int PlayerAge::HOURS_PER_YEAR = 20;

PlayerAge::PlayerAge( )
{
    clear( );
}

void PlayerAge::clear( )
{
    logon = dreamland->getCurrentTime( );
    true_played = played = 0;
}

Date PlayerAge::getLogon( ) const
{
    return Date( logon );
}

void PlayerAge::setLogon( time_t l )
{
    logon = l;
}

void PlayerAge::setTruePlayed( int t )
{
    played = true_played = t;
}

void PlayerAge::modifyYears( int mod )
{
    played += mod * HOURS_PER_YEAR * Date::SECOND_IN_HOUR;
}

int PlayerAge::getTime( ) const
{
    return played + (int)(dreamland->getCurrentTime( ) - logon);
}

int PlayerAge::getTrueTime( ) const
{
    return true_played + (int)(dreamland->getCurrentTime( ) - logon);
}

int PlayerAge::getYears( ) const
{
    return BASE_AGE + getHours( ) / HOURS_PER_YEAR;
}

int PlayerAge::getTrueYears( ) const
{
    return BASE_AGE + getTrueHours( ) / HOURS_PER_YEAR;
}

int PlayerAge::getHours( ) const
{
    return getTime( ) / Date::SECOND_IN_HOUR;
}

int PlayerAge::getTrueHours( ) const
{
    return getTrueTime( ) / Date::SECOND_IN_HOUR;
}

/**************************************************************************
 * XMLPlayerAge 
 **************************************************************************/
bool XMLPlayerAge::toXML( XMLNode::Pointer &parent ) const
{
    return XMLInteger( getTrueTime( ) ).toXML( parent );
}

void XMLPlayerAge::fromXML( const XMLNode::Pointer &parent ) throw( ExceptionBadType )
{
    XMLInteger t;

    t.fromXML( parent );
    setTruePlayed( t );
}

/**************************************************************************
 * CachedNoun 
 **************************************************************************/
void CachedNoun::clear( )
{
    name.clear( );
    russian.clear( );
    vampire.clear( );
    vampire2.clear( );
    immortal.clear( );
    pretitle.clear( );
    pretitleRussian.clear( );
}

void CachedNoun::update( PCharacter *ch )
{
    static const DLString vampireName = "{DСоздани|е|я|ю|е|ем|и ночи{x";
    static const DLString immortalName = "Immortal";
    MultiGender mg( ch->getSex( ), Number::SINGULAR );
    
    /* plain english name */
    if (!name) {
        name = RussianString::Pointer( NEW, ch->getName( ), mg );
    }
    else {
        name->setFullForm( ch->getName( ) );
        name->setGender( mg );
    }
    
    /* russian name if set. defaults to english */
    DLString rname = ch->getRussianName( ).getFullForm( );
    if (rname.empty( ))
        rname = ch->getName( );
    
    if (!russian) {
        russian = RussianString::Pointer( NEW, rname, mg );
    }
    else {
        russian->setFullForm( rname );
        russian->setGender( mg );
    }
    
    /* vampire as visible to non-vampires */
    if (!vampire) {
        vampire = RussianString::Pointer( NEW, vampireName, mg );
    }
    else {
        vampire->setGender( mg );
    }
    
    /* vampire as visible to immortals */
    DLString v2name = vampireName + " [" + ch->getName( ) + "]";

    if (!vampire2) {
        vampire2 = RussianString::Pointer( NEW, v2name, mg );
    }
    else {
        vampire2->setFullForm( v2name );
        vampire2->setGender( mg );
    }
    
    /* immortals under wizinvis */
    if (!immortal) {
        immortal = RussianString::Pointer( NEW, immortalName, mg );
    }
    else {
        immortal->setGender( mg );
    }
    
    /* english name with pretitle, russian name with russian or english pretitle */
    DLString prt, rprt;
    bool colored = ch->getRemorts( ).pretitle;
    
    if (ch->getPretitle( ).colorLength( ) > 0) {
        prt << (colored ? ch->getPretitle( ) : ch->getPretitle( ).colourStrip( )) << " ";
    }

    if (ch->getRussianPretitle( ).colorLength( ) > 0) {
        rprt << (colored ? ch->getRussianPretitle( ) : ch->getRussianPretitle( ).colourStrip( )) << " "; 
    } else {
        rprt << prt;
    }

    prt << ch->getName( );
    rprt << rname;

    if (!pretitle) {
        pretitle = RussianString::Pointer( NEW, prt, mg );
    }
    else {
        pretitle->setFullForm( prt );
        pretitle->setGender( mg );
    }

    if (!pretitleRussian) {
        pretitleRussian = RussianString::Pointer( NEW, rprt, mg );
    }
    else {
        pretitleRussian->setFullForm( rprt );
        pretitleRussian->setGender( mg );
    }
}

/**************************************************************************
 * PCharacter 
 **************************************************************************/

PCharacter::PCharacter( )
        :        wearloc( wearlocationManager ),
                desires( desireManager ),
                config( 0, &config_flags ),
                mod_skills(skillManager),
                mod_skill_groups(skillGroupManager)
{
    init( );
}

PCharacter::~PCharacter( )
{
}

/**************************************************************************
 * recycle
 **************************************************************************/
void PCharacter::init( )
{
    Character::init( );

    password = "";
    lastAccessTime.setTime( 0 );
    lastAccessHost = "";
    petition.assign( clan_none );
    clanLevel = 0;
    hometown.assign( home_none );
    profession.assign( prof_none );
    subprofession.assign( prof_none );

    attributes.clear( );
    remorts.clear( );
    trust = 0;
    russianName.setFullForm( DLString::emptyString );
    cachedNoun.clear( );

    title.setValue( "" );
    pretitle.setValue( "" );
    russianPretitle.setValue( "" );
    description.setValue( "" );
    skills.clear( );
    bonuses.clear();
    security = 0;
    newbie_hit_counter = 0;

    wearloc.clear( );
    desires.clear( );        
    for (int i = 0; i < desireManager->size( ); i++)
        desireManager->find( i )->reset( this );
    
    bamfin.setValue( "" );
    bamfout.setValue( "" );
    wiznet = 0;
    
    age.clear( );

    last_death_time = 0;
    ghost_time = 0;
    PK_time_v = 0;
    PK_time_sk = 0;
    PK_time_t = 0;
    PK_flag = 0;
    death = 0;
    anti_killed = 0;
    has_killed = 0;

    perm_hit = max_hit;
    perm_mana = max_mana;
    perm_move = max_move;

    max_skill_points = 1000;
    practice = 0;
    train = 0;
    loyalty = 0;
    curse = 100;
    bless = 0;
    mod_skills.clear();
    mod_skill_groups.clear();

    bank_s = 0;
    bank_g = 0;

    questpoints = 0;

    pet = 0;
    guarding = 0;
    guarded_by = 0;
    shadow = -1;

    config = 0;
    confirm_delete = false;

    switchedTo = 0;

    start_room = 0;
}

/**************************************************************************
 * gate to pc/npc info 
 **************************************************************************/
PCharacter *PCharacter::getPC()
{
    return this;
}

NPCharacter *PCharacter::getNPC()
{
    return 0;
}

const PCharacter *PCharacter::getPC() const
{
    return this;
}

const NPCharacter *PCharacter::getNPC() const
{
    return 0;
}

bool PCharacter::is_npc( ) const
{
    return false;
}


/**************************************************************************
 * pc memory update 
 **************************************************************************/
PCharacterMemory* PCharacter::getMemory( )
{
        PCharacterMemory* mem = dallocate( PCharacterMemory );
        mem->setName( getName( ) );
        mem->setPassword( getPassword( ) );
        mem->setLastAccessTime( getLastAccessTime( ) );
        mem->setLastAccessHost( getLastAccessHost( ) );
        mem->setLevel( getLevel( ) );
        mem->setTrust( getTrust( ) );
        mem->setSecurity( getSecurity( ) );
        mem->setQuestPoints(getQuestPoints());
        mem->setClan( getClan( ) );
        mem->setPetition( getPetition( ) );
        mem->setProfession( getProfession( ) );
        mem->setRace( getRace( ) );
        mem->setClanLevel( getClanLevel( ) );
        mem->setSex( getSex( ) );
        mem->setHometown( getHometown( ) );
        mem->setAttributes( getAttributes( ) );
        mem->setRemorts( getRemorts( ) );
        mem->setRussianName( getRussianName( ).getFullForm( ) );
        mem->setReligion( getReligion( ) );

        return mem;
}

void PCharacter::setMemory( PCharacterMemory* pcm )
{
        setName( pcm->getName( ) );
        setPassword( pcm->getPassword( ) );
        setLastAccessTime( pcm->getLastAccessTime( ) );
        setLastAccessHost( pcm->getLastAccessHost( ) );
        setLevel( pcm->getLevel( ) );
        setTrust( pcm->getTrust( ) );
        setSecurity( pcm->getSecurity( ) );
        setQuestPoints(pcm->getQuestPoints());
        setClan( pcm->getClan( ) );
        setPetition( pcm->getPetition( ) );
        setProfession( pcm->getProfession( ) );
        setRace( pcm->getRace( ) );
        setClanLevel( pcm->getClanLevel( ) );
        setSex( pcm->getSex( ) );
        setHometown( pcm->getHometown( ) );
        setAttributes( pcm->getAttributes( ) );
        setRemorts( pcm->getRemorts( ) );
        setRussianName( pcm->getRussianName( ).getFullForm( ) );
        setReligion( pcm->getReligion( ) );
}

/**************************************************************************
 * xml container 
 **************************************************************************/
bool PCharacter::nodeFromXML( const XMLNode::Pointer& child )
{
    if (!XMLVariableContainer::nodeFromXML( child )) 
        LogStream::sendWarning( ) 
            << getName( ) << "::ignoring unhandled node " << child->getName( ) << endl;
    
    return true;
}

/**************************************************************************
 * set-get methods inherited from PCMemoryInterface
 **************************************************************************/
bool PCharacter::isOnline( ) const
{
    return true;
}

PCharacter * PCharacter::getPlayer( ) 
{
    return this;
}

const DLString& PCharacter::getName( ) const throw( )
{
    return Character::getName( );
}
const DLString& PCharacter::getPassword( ) const throw( )
{
    return password.getValue( );
}
void PCharacter::setPassword( const DLString &passwd) throw( )
{
    this->password.setValue( passwd );
}
const Date& PCharacter::getLastAccessTime( ) const throw( )
{
    return lastAccessTime;
}
void PCharacter::setLastAccessTime( const Date& lastAccessTime ) throw( )
{
    this->lastAccessTime = lastAccessTime;
}
const DLString& PCharacter::getLastAccessHost( ) const throw( )
{
    return lastAccessHost.getValue( );
}
void PCharacter::setLastAccessHost( const DLString& lastAccessHost ) throw( )
{
    this->lastAccessHost.setValue( lastAccessHost );
}
void PCharacter::setLastAccessTime( )
{
    lastAccessTime = Date::newInstance( );
}
short PCharacter::getLevel( ) const throw( )
{
    return getRealLevel( );
}
int PCharacter::getTrust( ) const throw( )
{
    return trust.getValue( );
}
void PCharacter::setTrust( int trust ) throw( )
{
    this->trust = trust;
}
int PCharacter::getSecurity( ) const throw( )
{
    return security.getValue( );
}
void PCharacter::setSecurity( int security ) throw( )
{
    this->security = security;
}
int PCharacter::getQuestPoints( ) const throw( )
{
        return questpoints;
}
void PCharacter::setQuestPoints( int questpoints ) throw( )
{
        this->questpoints = questpoints;
}
int PCharacter::addQuestPoints(int delta)
{
    this->questpoints += delta;
    return this->questpoints;
}
ClanReference &PCharacter::getPetition( ) throw( )
{
    return petition;
}
void PCharacter::setPetition( const ClanReference & petition ) throw( )
{
    this->petition.assign( petition );
}
ClanReference & PCharacter::getClan( ) throw( )
{
    return Character::getClan( );
}
void PCharacter::setClan( const ClanReference & clan ) throw( )
{
    this->clan.assign( clan );
}
HometownReference &PCharacter::getHometown( ) throw( )
{
    return hometown;
}
void PCharacter::setHometown( const HometownReference & hometown ) throw( )
{
    this->hometown.assign( hometown );
}
ProfessionReference & PCharacter::getSubProfession( )
{
    return subprofession;
}
void PCharacter::setSubProfession( const ProfessionReference &sub )
{
    subprofession.assign( sub );
}
ProfessionReference &PCharacter::getTrueProfession( )
{
    if (subprofession != prof_none)
        return subprofession;

    return getProfession( );
}

short PCharacter::getClanLevel( ) const throw( )
{
    return clanLevel.getValue( );
}
void PCharacter::setClanLevel( short clanLevel ) throw( )
{
    this->clanLevel.setValue( clanLevel );
}
short PCharacter::getSex( ) const throw( )
{
    return Character::getSex( );
}
XMLAttributes& PCharacter::getAttributes( ) throw( )
{
    return attributes;
}
const XMLAttributes& PCharacter::getAttributes( ) const throw( )
{
    return attributes;
}
void PCharacter::setAttributes( const XMLAttributes& attributes ) throw( )
{
    this->attributes = attributes;
}
const DLString& PCharacter::getPretitle( ) const throw( )
{
    return pretitle.getValue( );
}
void PCharacter::setPretitle( const DLString& pretitle ) throw( )
{
    this->pretitle.setValue( pretitle );
    updateCachedNoun( );
}
const DLString& PCharacter::getRussianPretitle( ) const throw( )
{
    return russianPretitle.getValue( );
}
void PCharacter::setRussianPretitle( const DLString& pretitle ) throw( )
{
    this->russianPretitle.setValue( pretitle );
    updateCachedNoun( );
}
Remorts& PCharacter::getRemorts( ) throw( )
{
    return remorts;
}
const Remorts& PCharacter::getRemorts( ) const throw( )
{
    return remorts;
}
void PCharacter::setRemorts( const Remorts& remorts ) throw( ) 
{
    this->remorts = remorts;
}
const RussianString& PCharacter::getRussianName( ) const throw( )
{
    return russianName;
}
void PCharacter::setRussianName( const DLString& name ) throw( )
{
    russianName.setFullForm( name );
    updateCachedNoun( );
}

/**************************************************************************
 * set-get methods inherited from Character
 **************************************************************************/
void PCharacter::setDescription( const DLString& d )
{
    description = d;
}
const char * PCharacter::getDescription( ) const
{
    return description.c_str( );
}
const GlobalBitvector & PCharacter::getWearloc( )
{
    return wearloc;
}

/**************************************************************************
 * title 
 **************************************************************************/
void PCharacter::setTitle( const DLString &title )
{
    this->title = title;
}

const DLString & PCharacter::getTitle( ) const 
{
    return title.getValue( );
}

DLString PCharacter::getParsedTitle( )
{
    ostringstream out;
    const char *str = getTitle( ).c_str( );
    
    switch (str[0]) {
    case '.': case ',': case '!': case '?':
        break;
    default:
        out << " ";
        break;
    }

    for (; *str; str++) {
        if (*str == '%') {
            DLString cl;
            
            if (*++str == '\0')
                break;

            switch (*str) {
            default:
                out << *str;
                break;
            
            case 'c':
                out << clan->getTitle( this );
                break;
                
            case 'C':
                cl = clan->getTitle( this );
                if (!cl.empty( ))
                    cl.upperFirstCharacter( );
                out << cl;
                break;
                
            case 'a':
                out << getProfession( )->getTitle( this );
                break;
            }
        }
        else
            out << *str;
    }

    return out.str( );
}

/*****************************************************************************
 * name and sex formatting
 *****************************************************************************/
using namespace Grammar;

Noun::Pointer PCharacter::toNoun( const DLObject *forWhom, int flags ) const
{
    const Character *wch = dynamic_cast<const Character *>(forWhom);
    PlayerConfig::Pointer cfg = wch ? wch->getConfig( ) : PlayerConfig::Pointer( );
    
    if (IS_SET(flags, FMT_DOPPEL))
        return getDoppel( wch )->toNoun( wch, REMOVE_BIT(flags, FMT_DOPPEL) );

    if (IS_SET(flags, FMT_INVIS) && wch) {
        if (!wch->can_see( this )) {
            if (is_immortal( ))
                return cachedNoun.immortal;
            else
                return somebody;
        }
                
        if (is_vampire( ) && !wch->is_vampire( )) {
            if (cfg->holy) 
                return cachedNoun.vampire2;
            else
                return cachedNoun.vampire;
        }
    }
    
    if (IS_SET(flags, FMT_PRETITLE)) {
        if (wch && cfg->runames)
            return cachedNoun.pretitleRussian;
        else
            return cachedNoun.pretitle;
    }
    
    if (wch && cfg->runames)
        return cachedNoun.russian;
    else
        return cachedNoun.name;
}

void PCharacter::updateCachedNoun( )
{
    cachedNoun.update( this );
}

DLString PCharacter::getNameP( char gram_case ) const
{
    ostringstream buf;

    buf << cachedNoun.russian->decline( gram_case );
    
    if (gram_case == '7')
        buf << " " << getName( );

    return buf.str( );
}

/**************************************************************************
 *  pc skills 
 **************************************************************************/
/*
 * сколько всего skill points потрачено на все разученные скилы
 */
int PCharacter::skill_points( ) 
{
        int sn;
        int points = 0;
        
        for (sn = 0; sn < SkillManager::getThis( )->size( ); sn++) 
            points += skill_points( sn );
        
        return points / 10;
}

/* 
 * сколько skill points потрачено на данный скил 
 */
int PCharacter::skill_points( int sn )
{
    int learned = getSkillData( sn ).learned;
    Skill *skill = SkillManager::getThis( )->find( sn );
    
    if (learned <= 1 || !skill->available( this ))
        return 0;

    return learned * SkillManager::getThis( )->find(sn)->getWeight( this );
}

PCSkillData & PCharacter::getSkillData( int sn )
{
    return skills.get( sn );
}

void PCharacter::updateSkills( )
{
    for (int sn = 0; sn < SkillManager::getThis( )->size( ); sn++) {
        Skill *skill = SkillManager::getThis( )->find( sn );
        PCSkillData &data = getSkillData(sn);

        // Ensure skill learned percentage is always within limits.
        if (skill->visible( this )) {
            int &percent = data.learned;

            percent = std::max( 1, percent );
            percent = std::max( skill->getLearned( this ), percent );
        }

        // For historical 'temporary' skills, set up proper skill origin value.
        if (data.temporary) {
            data.temporary = false;
            data.origin.setValue(SKILL_DREAM);
        }
    }
}                        

int PCharacter::applyCurse( int def )
{
    return (def * curse) / 100;
}

/**************************************************************************
 *  visibility of things 
 **************************************************************************/
bool PCharacter::canSeeProfession( PCharacter *victim ) 
{
    if (is_immortal( ))
        return true;
    
    if (this == victim)
        return true;
    
    if (getClan( ) != clan_none 
        && getClan( ) == victim->getClan( )
        && !getClan( )->isDispersed( ))
        return true;

    return false;
}

bool PCharacter::canSeeLevel( PCharacter *victim ) 
{
    if (victim->getCurrStat( STAT_CHA ) < 18)
        return true;

    return canSeeProfession( victim );
}


/*************************************************************************
 * experience calculations
 *************************************************************************/
int PCharacter::getExpToLevel( ) 
{
    return getExpPerLevel( getLevel( ) + 1 ) - exp;
}
int PCharacter::getBaseExp( ) 
{
    int base;
    
    base = 1000;
    base += getRace( )->getPC( )->getPoints( );
    base += getProfession( )->getPoints( );
    base = base * getRace( )->getPC( )->getClasses( )[getProfession( )] / 100;

    return base;
}
int PCharacter::getExpPerLevel( int lvl, int remort ) 
{
    int base = getBaseExp( );
    
    if (lvl < 0)
        lvl = getLevel( );

    if (remort < 0)
        remort = remorts.size( );

    // Summ[x=1..lvl] (base + 6 * remort * x)
    return lvl * base + 6 * remort * lvl * (lvl + 1) / 2;
}


/**************************************************************************
 * trust and immortality 
 **************************************************************************/
bool PCharacter::isCoder( ) const
{
    return attributes.isAvailable( "coder" );
}

int PCharacter::get_trust( ) const
{
    if (getAttributes( ).isAvailable( "coder" ))
        return 0xFFFF;
    
    if (getTrust( ) != 0)
        return getTrust( );

    return getLevel( );
}
bool PCharacter::is_immortal( ) const
{
    return get_trust() >= LEVEL_IMMORTAL;
}

/****************************************************************************
 * methods for character stats retrieving
 ***************************************************************************/
int PCharacter::getCurrStat( int stat ) 
{
    return URANGE( MIN_STAT, 
                   perm_stat[stat] + mod_stat[stat], 
                   getMaxStat( stat ) );
}

int PCharacter::getMaxStat( int i ) 
{
    if (getRealLevel( ) > LEVEL_IMMORTAL)
        return MAX_STAT;

    int maxStat = getMaxTrain( i ) + remorts.stats[i].getValue( );

    if (subprofession != prof_none)
        maxStat += subprofession->getStat( i );

    return URANGE( MIN_STAT, maxStat, MAX_STAT );
}

int PCharacter::getMaxTrain( int i ) 
{
    if (getRealLevel( ) > LEVEL_IMMORTAL)
        return MAX_STAT;
    else
        return min(MAX_STAT, BASE_STAT
                                + getRace( )->getPC( )->getStats( )[i]
                                + getProfession( )->getStat( i ));
}

void PCharacter::updateStats( )
{
    int i, max_stat;

    for (i = 0; i < stat_table.size; i++) {        
        max_stat = getMaxTrain( i );

        if (perm_stat[i] > max_stat) {
            train += perm_stat[i] - max_stat;
            perm_stat[i] = max_stat;
        }
    }
}

/**************************************************************************
 * misc 
 **************************************************************************/
bool PCharacter::is_vampire( ) const
{
    return IS_SET(act, PLR_VAMPIRE);
}
bool PCharacter::is_mirror( ) const
{
    return false;
}
short PCharacter::getModifyLevel( ) const
{
    return getRealLevel( ) + remorts.level.getValue( );
}

/**************************************************************************
 * configuration 
 **************************************************************************/
PlayerConfig::Pointer PCharacter::getConfig( ) const
{
    return PlayerConfig::Pointer( NEW, this );
}
