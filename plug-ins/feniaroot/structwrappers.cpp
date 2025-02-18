/* $Id$
 *
 * ruffina, 2004
 */
#include "structwrappers.h"

#include "hometown.h"
#include "skill.h"
#include "skillcommand.h"
#include "profession.h"
#include "subprofession.h"
#include "room.h"
#include "pcharacter.h"
#include "pcrace.h"
#include "desire.h"
#include "clan.h"
#include "clantypes.h"

#include "nativeext.h"
#include "regcontainer.h"
#include "reglist.h"
#include "wrappermanager.h"
#include "subr.h"
#include "schedulerwrapper.h"
#include "characterwrapper.h"
#include "wrap_utils.h"

#include "calendar_utils.h"
#include "skill_utils.h"
#include "handler.h"
#include "gsn_plugin.h"
#include "profflags.h"
#include "merc.h"
#include "mercdb.h"
#include "def.h"

using namespace std;

DESIRE(thirst);
DESIRE(hunger);
DESIRE(full);

/*----------------------------------------------------------------------
 * Area
 *----------------------------------------------------------------------*/
NMI_INIT(AreaWrapper, "area, зона");

static AREA_DATA *find_area( const DLString &filename )
{
    AREA_DATA *area;

    for (area = area_first; area; area = area->next)
        if (filename == area->area_file->file_name)
            return area;

    throw Scripting::Exception( "Area " + filename + " not found." );
}

AreaWrapper::AreaWrapper( const DLString &n )
                  : filename( n )
{
}

Scripting::Register AreaWrapper::wrap( const DLString &filename )
{
    AreaWrapper::Pointer aw( NEW, filename );

    Scripting::Object *sobj = &Scripting::Object::manager->allocate( );
    sobj->setHandler( aw );

    return Scripting::Register( sobj );
}

NMI_INVOKE( AreaWrapper, api, "(): печатает этот api" )
{
    ostringstream buf;
    
    Scripting::traitsAPI<AreaWrapper>( buf );
    return Scripting::Register( buf.str( ) );
}

NMI_GET( AreaWrapper, filename, "название файла зоны" ) 
{
    return Scripting::Register( filename );
}

NMI_GET( AreaWrapper, name, "имя зоны (как видно по 'where')" ) 
{
    return Scripting::Register( find_area( filename )->name );
}

NMI_GET( AreaWrapper, area_flag, "флаги зоны (таблица .tables.area_flags)" ) 
{
    return Scripting::Register((int)(find_area( filename )->area_flag));
}

/*----------------------------------------------------------------------
 * Hometown
 *----------------------------------------------------------------------*/
NMI_INIT(HometownWrapper, "hometown, город");

HometownWrapper::HometownWrapper( const DLString &n )
                  : name( n )
{
}

Scripting::Register HometownWrapper::wrap( const DLString &name )
{
    HometownWrapper::Pointer hw( NEW, name );

    Scripting::Object *sobj = &Scripting::Object::manager->allocate( );
    sobj->setHandler( hw );

    return Scripting::Register( sobj );
}

NMI_INVOKE( HometownWrapper, isAllowed, "(ch): доступен ли город персонажу ch" )
{
    Hometown *ht = hometownManager->find( name );
    CharacterWrapper *charWrap;

    if (!ht)
        return Scripting::Register( false );

    if (args.empty( ))
        throw Scripting::NotEnoughArgumentsException( );
    
    charWrap = wrapper_cast<CharacterWrapper>( args.front( ) );
    
    if (charWrap->getTarget( )->is_npc( ))
        throw Scripting::Exception( "PC field requested on NPC" ); 

    return Scripting::Register( ht->isAllowed( charWrap->getTarget( )->getPC( ) ) );
}

NMI_INVOKE( HometownWrapper, api, "(): печатает этот api" )
{
    ostringstream buf;
    
    Scripting::traitsAPI<HometownWrapper>( buf );
    return Scripting::Register( buf.str( ) );
}

NMI_GET( HometownWrapper, name, "английское название" ) 
{
    return Scripting::Register( name );
}

NMI_GET( HometownWrapper, recall, "vnum комнаты возврата (recall)" ) 
{
    return Scripting::Register( hometownManager->find( name )->getRecall( ) );
}

NMI_GET( HometownWrapper, areaname, "полное название арии" ) 
{
    Room *room = get_room_index( hometownManager->find( name )->getAltar( ) );

    if (room)
        return Scripting::Register( room->area->name );
    else
        return Scripting::Register( DLString::emptyString );
}

NMI_GET( HometownWrapper, altname, "альтернативное название арии" ) 
{
    Room *room = get_room_index( hometownManager->find( name )->getAltar( ) );

    if (room)
        return Scripting::Register( room->area->altname );
    else
        return Scripting::Register( DLString::emptyString );
}

NMI_GET( HometownWrapper, credits, "оригинальное англ название арии" ) 
{
    Room *room = get_room_index( hometownManager->find( name )->getAltar( ) );

    if (room)
        return Scripting::Register( room->area->credits );
    else
        return Scripting::Register( DLString::emptyString );
}

/*----------------------------------------------------------------------
 * Profession
 *----------------------------------------------------------------------*/
NMI_INIT(ProfessionWrapper, "profession, класс персонажа");

ProfessionWrapper::ProfessionWrapper( const DLString &n )
                  : name( n )
{
}

Scripting::Register ProfessionWrapper::wrap( const DLString &name )
{
    ProfessionWrapper::Pointer hw( NEW, name );

    Scripting::Object *sobj = &Scripting::Object::manager->allocate( );
    sobj->setHandler( hw );

    return Scripting::Register( sobj );
}

NMI_INVOKE( ProfessionWrapper, api, "(): печатает этот api" )
{
    ostringstream buf;
    
    Scripting::traitsAPI<ProfessionWrapper>( buf );
    return Scripting::Register( buf.str( ) );
}

NMI_GET( ProfessionWrapper, points, "дополнительные очки опыта" ) 
{
    return professionManager->find( name )->getPoints( );
}

static int weapon_vnum( int wclass )
{
    switch (wclass) {
        case WEAPON_SWORD:
            return 40102;
        case WEAPON_DAGGER:
            return 40101;
        case WEAPON_AXE:
            return 40119;
        case WEAPON_MACE:
            return 40117;
    }
    return -1;
}

NMI_INVOKE( ProfessionWrapper, bestWeapon, "(ch): vnum лучшего новичкового оружия для расы и класса персонажа ch" )
{
    CharacterWrapper *ch;
    
    if (args.empty( ))
        throw Scripting::NotEnoughArgumentsException( );
    
    ch = wrapper_cast<CharacterWrapper>(args.front( ));
    if (ch->getTarget( )->getSkill( gsn_axe ) == 100)
        return weapon_vnum( WEAPON_AXE );
    if (ch->getTarget( )->getSkill( gsn_sword ) == 100)
        return weapon_vnum( WEAPON_SWORD );
    if (ch->getTarget( )->getSkill( gsn_dagger ) == 100)
        return weapon_vnum( WEAPON_DAGGER );
    if (ch->getTarget( )->getSkill( gsn_spear ) == 100)
        return weapon_vnum( WEAPON_SPEAR );

    return professionManager->find( name )->getWeapon( );
}

NMI_GET( ProfessionWrapper, name, "английское название" ) 
{
    return professionManager->find( name )->getName( );
}

NMI_GET( ProfessionWrapper, nameRus, "русское название с падежами" ) 
{
    return professionManager->find( name )->getRusName( );
}

NMI_GET( ProfessionWrapper, nameMlt, "русское название во множ.числе с падежами" ) 
{
    return professionManager->find( name )->getMltName( );
}

NMI_GET( ProfessionWrapper, ethos, "список подходящих мировоззрений" ) 
{
    return professionManager->find( name )->getEthos( ).names( );
}

NMI_GET( ProfessionWrapper, alignName, "русское имя подходящего характера или 'любой'" ) 
{
    const Flags &a = professionManager->find( name )->getAlign( );
    
    if (a.equalsToBitNumber( N_ALIGN_EVIL ))
        return "злой";
    if (a.equalsToBitNumber( N_ALIGN_GOOD ))
        return "добрый";
    if (a.equalsToBitNumber( N_ALIGN_NEUTRAL ))
        return "нейтр.";

    return "любой";
}

NMI_GET( ProfessionWrapper, statPlus, "какие параметры увеличиваются у представителей этой профессии" ) 
{
    Profession *prof = professionManager->find( name );
    int stat;
    ostringstream buf;
    
    for (int s = 0; s < stat_table.size; s++) {
        if (s == STAT_CHA)
            continue;

        stat = prof->getStat( s );
        
        if (stat <= 0)
            continue;
        
        if (!buf.str( ).empty( ))
            buf << ",";

        buf <<  stat_table.name( s );
    }

    return buf.str( );
}

NMI_INVOKE( ProfessionWrapper, goodSex, "(ch): проверить ограничения по полу на профессию для персонажа ch" )
{
    CharacterWrapper *ch;
    
    if (args.empty( ))
        throw Scripting::NotEnoughArgumentsException( );
   
    Profession *prof =  professionManager->find( name );
    ch = wrapper_cast<CharacterWrapper>(args.front( ));
    return prof->getSex( ).isSetBitNumber( ch->getTarget( )->getSex( ) );
}

NMI_INVOKE( ProfessionWrapper, goodRace, "(ch): проверить ограничения по расе на профессию для персонажа ch" )
{
    CharacterWrapper *ch;
    
    if (args.empty( ))
        throw Scripting::NotEnoughArgumentsException( );
    
    Profession *prof =  professionManager->find( name );
    if (prof->getFlags( ).isSet( PROF_NEWLOCK ))
        return false;

    ch = wrapper_cast<CharacterWrapper>(args.front( ));
    return ch->getTarget( )->getRace( )->getPC( )->getClasses( )[prof->getIndex( )] > 0;
}

NMI_INVOKE( ProfessionWrapper, goodPersonality, "(ch): проверить ограничение на характер и этос на профессию для персонажа ch" )
{
    CharacterWrapper *ch;
    
    if (args.empty( ))
        throw Scripting::NotEnoughArgumentsException( );
    
    ch = wrapper_cast<CharacterWrapper>(args.front( ));
    Profession *prof = professionManager->find( name );
    if (!prof->getEthos( ).isSetBitNumber( ch->getTarget( )->ethos ))
        return false;
    if (!prof->getAlign( ).isSetBitNumber( ALIGNMENT(ch->getTarget( )) ))
        return false;
    return true;
}

/*----------------------------------------------------------------------
 * Race
 *----------------------------------------------------------------------*/
NMI_INIT(RaceWrapper, "race, раса персонажа и моба");

RaceWrapper::RaceWrapper( const DLString &n )
                  : name( n )
{
}

Scripting::Register RaceWrapper::wrap( const DLString &name )
{
    RaceWrapper::Pointer hw( NEW, name );

    Scripting::Object *sobj = &Scripting::Object::manager->allocate( );
    sobj->setHandler( hw );

    return Scripting::Register( sobj );
}

NMI_INVOKE( RaceWrapper, api, "(): печатает этот api" )
{
    ostringstream buf;
    
    Scripting::traitsAPI<RaceWrapper>( buf );
    return Scripting::Register( buf.str( ) );
}

NMI_GET( RaceWrapper, name, "английское название" ) 
{
    return raceManager->find( name )->getName( );
}

NMI_GET( RaceWrapper, nameMlt, "русское название во множ.числе с падежами" ) 
{
    return raceManager->find( name )->getPC( )->getMltName( );
}

NMI_GET( RaceWrapper, nameMale, "русское название в мужском роде с падежами" ) 
{
    return raceManager->find( name )->getPC( )->getMaleName( );
}

NMI_GET( RaceWrapper, nameFemale, "русское название в женском роде с падежами" ) 
{
    return raceManager->find( name )->getPC( )->getFemaleName( );
}

NMI_INVOKE( RaceWrapper, nameRus, "(ch): русское название в зависимости от пола персонажа ch" ) 
{
    CharacterWrapper *ch;
    
    if (args.empty( ))
        throw Scripting::NotEnoughArgumentsException( );
    
    ch = wrapper_cast<CharacterWrapper>(args.front( ));

    if (ch->getTarget( )->getSex( ) == SEX_FEMALE)
        return raceManager->find( name )->getPC( )->getFemaleName( );
    else
        return raceManager->find( name )->getPC( )->getMaleName( );
}

NMI_GET( RaceWrapper, hpBonus, "бонус на здоровья при создании персонажа этой расы" ) 
{
    return raceManager->find( name )->getPC( )->getHpBonus( );
}

NMI_GET( RaceWrapper, manaBonus, "бонус на ману при создании персонажа этой расы" ) 
{
    return raceManager->find( name )->getPC( )->getManaBonus( );
}

NMI_GET( RaceWrapper, pracBonus, "бонус на кол-во практик при создании персонажа этой расы" ) 
{
    return raceManager->find( name )->getPC( )->getPracBonus( );
}

NMI_GET( RaceWrapper, det, "врожденные детекты (таблица .tables.detect_flags)" ) 
{
    return Scripting::Register( (int) raceManager->find( name )->getDet( ).getValue( ) );
}

NMI_GET( RaceWrapper, aff, "врожденные аффекты (таблица .tables.affect_flags)" ) 
{
    return Scripting::Register( (int) raceManager->find( name )->getAff( ).getValue( ) );
}

NMI_GET( RaceWrapper, vuln, "врожденные уязвимости (таблица .tables.vuln_flags)" ) 
{
    return Scripting::Register( (int) raceManager->find( name )->getVuln( ).getValue( ) );
}

NMI_GET( RaceWrapper, res, "врожденная сопротивляемость (таблица .tables.res_flags)" ) 
{
    return Scripting::Register( (int) raceManager->find( name )->getRes( ).getValue( ) );
}

NMI_GET( RaceWrapper, imm, "врожденный иммунитет (таблица .tables.imm_flags)" ) 
{
    return Scripting::Register( (int) raceManager->find( name )->getImm( ).getValue( ) );
}

NMI_GET( RaceWrapper, form, "формы тела (таблица .tables.form_flags)" ) 
{
    return Scripting::Register( (int) raceManager->find( name )->getForm( ).getValue( ) );
}

NMI_GET( RaceWrapper, parts, "части тела (таблица .tables.part_flags)" ) 
{
    return Scripting::Register( (int) raceManager->find( name )->getParts( ).getValue( ) );
}

NMI_GET( RaceWrapper, size, "размер (таблица .tables.size_table)" ) 
{
    return raceManager->find( name )->getSize( ).getValue( );
}

NMI_GET( RaceWrapper, wearloc, "список доступных wear locations" ) 
{
    return raceManager->find( name )->getWearloc( ).toString( );
}

/*----------------------------------------------------------------------
 * Liquid
 *----------------------------------------------------------------------*/
NMI_INIT(LiquidWrapper, "liquid, жидкость");

LiquidWrapper::LiquidWrapper( const DLString &n )
                  : name( n )
{
}

Scripting::Register LiquidWrapper::wrap( const DLString &name )
{
    LiquidWrapper::Pointer hw( NEW, name );

    Scripting::Object *sobj = &Scripting::Object::manager->allocate( );
    sobj->setHandler( hw );

    return Scripting::Register( sobj );
}

NMI_INVOKE( LiquidWrapper, api, "(): печатает этот api" )
{
    ostringstream buf;
    
    Scripting::traitsAPI<LiquidWrapper>( buf );
    return Scripting::Register( buf.str( ) );
}


NMI_GET( LiquidWrapper, name, "английское название" ) 
{
    return liquidManager->find( name )->getName( );
}
NMI_GET( LiquidWrapper, short_descr, "русское название с цветами и падежами" ) 
{
    return liquidManager->find( name )->getShortDescr( );
}
NMI_GET( LiquidWrapper, color, "прилагательное цвета с падежами" ) 
{
    return liquidManager->find( name )->getColor( );
}
NMI_GET( LiquidWrapper, sip_size, "размер глотка" ) 
{
    return liquidManager->find( name )->getSipSize( );
}
NMI_GET( LiquidWrapper, flags, "флаги жидкости (таблица .tables.liquid_flags)" ) 
{
    return Scripting::Register( (int)liquidManager->find( name )->getFlags( ).getValue( ) );;
}
NMI_GET( LiquidWrapper, index, "внутренний порядковый номер" ) 
{
    return liquidManager->find( name )->getIndex( );
}
NMI_GET( LiquidWrapper, hunger, "как хорошо утоляет голод" ) 
{
    return liquidManager->find( name )->getDesires( )[desire_hunger];
}
NMI_GET( LiquidWrapper, thirst, "как хорошо утоляет жажду" ) 
{
    return liquidManager->find( name )->getDesires( )[desire_thirst];
}
NMI_GET( LiquidWrapper, full, "как хорошо насыщает" ) 
{
    return liquidManager->find( name )->getDesires( )[desire_full];
}

/*----------------------------------------------------------------------
 * Clan
 *----------------------------------------------------------------------*/
NMI_INIT(ClanWrapper, "clan, клан");

ClanWrapper::ClanWrapper( const DLString &n )
                  : name( n )
{
}

Scripting::Register ClanWrapper::wrap( const DLString &name )
{
    ClanWrapper::Pointer hw( NEW, name );

    Scripting::Object *sobj = &Scripting::Object::manager->allocate( );
    sobj->setHandler( hw );

    return Scripting::Register( sobj );
}

NMI_INVOKE( ClanWrapper, api, "(): печатает этот api" )
{
    ostringstream buf;
    
    Scripting::traitsAPI<ClanWrapper>( buf );
    return Scripting::Register( buf.str( ) );
}


NMI_GET( ClanWrapper, name, "английское название" ) 
{
    return clanManager->find( name )->getName( );
}
NMI_GET( ClanWrapper, index, "внутренний порядковый номер" ) 
{
    return clanManager->find( name )->getIndex( );
}
NMI_GET( ClanWrapper, color, "буква цвета" ) 
{
    return clanManager->find( name )->getColor( );
}

static const char *diplomacy_names [] = {
    "alliance",
    "peace",
    "truce",
    "distrust",
    "aggression",
    "war",
    "subordination",
    "oppress",
    "none"
};

static const int diplomacy_count = sizeof(diplomacy_names) / sizeof(char *);

static int diplomacy_number( Clan *clan, Clan *otherClan )
{
    if (!otherClan || !clan)
        throw Scripting::CustomException( "No such clan" );
    
    if (!clan->hasDiplomacy( ) || !otherClan->hasDiplomacy( ))
        return diplomacy_count - 1;

    int dnum = clan->getData( )->getDiplomacy( otherClan );
    return URANGE( 0, dnum, diplomacy_count - 1 );
}

NMI_INVOKE( ClanWrapper, diplomacy, "(clan): англ название дипломатии с кланом clan (clan dip list)" ) 
{
    DLString otherName;
    const Register &arg = get_unique_arg( args );

    if (arg.type == Register::STRING)
        otherName = arg.toString( );
    else 
        otherName = wrapper_cast<ClanWrapper>( arg )->name;

    return diplomacy_names
              [ diplomacy_number( clanManager->find( name ),
                                  clanManager->findExisting( otherName ) )
              ];
}

/*----------------------------------------------------------------------
 * CraftProfession
 *----------------------------------------------------------------------*/
NMI_INIT(CraftProfessionWrapper, "craftprofession, дополнительная профессия");

CraftProfessionWrapper::CraftProfessionWrapper( const DLString &n )
                  : name( n )
{
}

Scripting::Register CraftProfessionWrapper::wrap( const DLString &name )
{
    CraftProfessionWrapper::Pointer hw( NEW, name );

    Scripting::Object *sobj = &Scripting::Object::manager->allocate( );
    sobj->setHandler( hw );

    return Scripting::Register( sobj );
}

CraftProfession * CraftProfessionWrapper::getTarget() const
{
    CraftProfession::Pointer prof = craftProfessionManager->get(name);
    if (!prof)
        throw Scripting::Exception("Profession not found");
    return *prof;
}

NMI_INVOKE( CraftProfessionWrapper, api, "(): печатает этот api" )
{
    ostringstream buf;
    
    Scripting::traitsAPI<CraftProfessionWrapper>( buf );
    return Scripting::Register( buf.str( ) );
}

NMI_GET( CraftProfessionWrapper, name, "английское название" ) 
{
    return getTarget()->getName( );
}

NMI_GET( CraftProfessionWrapper, nameRus, "русское название с падежами" ) 
{
    return getTarget()->getRusName( );
}

NMI_GET( CraftProfessionWrapper, nameMlt, "название во множественном числе с падежами" ) 
{
    return getTarget()->getMltName( );
}

NMI_INVOKE( CraftProfessionWrapper, setLevel, "(ch, level): установить персонажу уровень мастерства в этой профессии" )
{
    if (args.size( ) != 2)
        throw Scripting::NotEnoughArgumentsException( );
    
    PCharacter *ch = arg2player(args.front());
    int level = args.back().toNumber();
    getTarget()->setLevel(ch, level);
    return Scripting::Register();
}

NMI_INVOKE( CraftProfessionWrapper, getLevel, "(ch): получить уровень мастерства персонажа в этой профессии" )
{
    PCharacter *ch = args2player(args);
    return getTarget()->getLevel(ch);
}

NMI_INVOKE( CraftProfessionWrapper, getTotalExp, "(ch): суммарный опыт персонажа в этой профессии" )
{
    PCharacter *ch = args2player(args);
    return getTarget()->getCalculator(ch)->totalExp();
}

NMI_INVOKE( CraftProfessionWrapper, getExpToLevel, "(ch): кол-во опыта до следующего уровня мастерства в этой профессии" )
{
    PCharacter *ch = args2player(args);
    return getTarget()->getCalculator(ch)->expToLevel();
}

NMI_INVOKE( CraftProfessionWrapper, gainExp, "(ch, exp): заработать очков опыта в этой профессии" )
{
    if (args.size( ) != 2)
        throw Scripting::NotEnoughArgumentsException( );
    
    PCharacter *ch = arg2player(args.front());
    int exp = args.back().toNumber();
    getTarget()->gainExp(ch, exp);
    return Scripting::Register();
}

/*----------------------------------------------------------------------
 * Skill
 *----------------------------------------------------------------------*/
NMI_INIT(SkillWrapper, "skill, умение или заклинание");

SkillWrapper::SkillWrapper( const DLString &n )
                  : name( n )
{
}

Scripting::Register SkillWrapper::wrap( const DLString &name )
{
    SkillWrapper::Pointer hw( NEW, name );

    Scripting::Object *sobj = &Scripting::Object::manager->allocate( );
    sobj->setHandler( hw );

    return Scripting::Register( sobj );
}

NMI_INVOKE( SkillWrapper, api, "(): печатает этот api" )
{
    ostringstream buf;
    
    Scripting::traitsAPI<SkillWrapper>( buf );
    return Scripting::Register( buf.str( ) );
}


NMI_GET( SkillWrapper, name, "английское название" ) 
{
    return skillManager->find( name )->getName( );
}

NMI_GET( SkillWrapper, nameRus, "русское название" ) 
{
    return skillManager->find( name )->getRussianName( );
}

NMI_INVOKE( SkillWrapper, usable, "(ch): доступно ли умение для использования прямо сейчас персонажу ch" )
{
    Character *ch = args2character(args);
    return skillManager->find( name )->usable( ch, false );
}

NMI_INVOKE( SkillWrapper, adept, "(ch): вернуть максимальное значение, до которого можно практиковаться" )
{
    PCharacter *ch = args2player(args); 
    return skillManager->find(name)->getAdept(ch);
}

NMI_INVOKE( SkillWrapper, learned, "(ch[,percent]): вернуть разученность или установить ее в percent" )
{
    PCharacter *ch = args2player(args); 
    int sn = skillManager->find(name)->getIndex();

    if (args.size() > 1) {
        int value = args.back( ).toNumber( );
        
        if (value < 0)
            throw Scripting::IllegalArgumentException( );
        
        ch->getSkillData(sn).learned = value;
        return Register( );
    }

    return Register(ch->getSkillData(sn).learned);
}

NMI_INVOKE( SkillWrapper, effective, "(ch): узнать процент раскачки у персонажа" )
{
    PCharacter *ch = args2player(args); 
    return Register( skillManager->find(name)->getEffective(ch) );
}

NMI_INVOKE( SkillWrapper, improve, "(ch,success[,victim]): попытаться улучшить знание умения на успехе/неудаче (true/false), применен на жертву" )
{
    PCharacter *ch = argnum2player(args, 1);
    int success = argnum2number(args, 2);
    Character *victim = args.size() > 2 ? argnum2character(args, 3) : NULL;
     
    skillManager->find( name )->improve( ch, success, victim );
    return Register( );
}

NMI_INVOKE( SkillWrapper, giveTemporary, "(ch[,learned[,days]]): присвоить временное умение персонажу, разученное на learned % (или на 75%), работающее days дней (или вечно). Вернет true, если присвоено успешно.")
{
    PCharacter *ch = argnum2player(args, 1);
    int learned = args.size() > 1 ? argnum2number(args, 2) : ch->getProfession()->getSkillAdept();
    long today = day_of_epoch(time_info);
    long end;

    if (args.size() <= 2)
        end = PCSkillData::END_NEVER;
    else {
        end = argnum2number(args, 3);
        if (end < 0)
            throw Scripting::Exception("end day cannot be negative");
        end = today + end;
    }

    if (learned <= 0)
        throw Scripting::Exception("learned param cannot be negative");

    // Do nothing for already available permanent skills.
    Skill *skill = skillManager->find(name);
    if (skill->visible(ch))
        return Register(false);
    
    // Do nothing for already present temporary skills.
    PCSkillData &data = ch->getSkillData(skill->getIndex());
    if (temporary_skill_active(data))
        return Register(false);

    // Create and save temporary skill data.
    data.origin = SKILL_FENIA;
    data.start = today;
    data.end = end;
    data.learned = learned;
    ch->save();

    return Register(true);
}

NMI_INVOKE( SkillWrapper, removeTemporary, "(ch): очистить временное умение у персонажа. Вернет true, если было что очищать.")
{
    PCharacter *ch = argnum2player(args, 1);
    Skill *skill = skillManager->find(name);
    PCSkillData &data = ch->getSkillData(skill->getIndex());

    if (!data.isTemporary())
        return Register(false);
    if (data.origin != SKILL_FENIA)
        return Register(false);

    data.clear();
    ch->save();

    return Register(true);
}

NMI_INVOKE(SkillWrapper, run, "(ch[,victim or level]): выполнить умение без проверок и сообщений")
{
    Skill *skill = skillManager->find(name);
    Character *ch = argnum2character(args, 1);
    
    if (args.size() < 2)
        return Register(skill->getCommand()->run(ch));

    Register arg2 = argnum(args, 2);
    if (arg2.type == Register::NUMBER) {
        int slevel = arg2.toNumber();
        return Register(skill->getCommand()->run(ch, slevel));
    }

    Character *victim = arg2character(arg2);
    return Register(skill->getCommand()->run(ch, victim));
}

