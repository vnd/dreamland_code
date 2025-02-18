/* $Id: cquest.cpp,v 1.1.4.8.6.7 2009/02/15 01:44:56 rufina Exp $
 *
 * ruffina, 2003
 */

#include "class.h"

#include "behavior_utils.h"
#include "pcharacter.h"
#include "npcharacter.h"
#include "pcharactermanager.h"
#include "room.h"
#include "hometown.h"

#include "feniamanager.h"
#include "wrappermanagerbase.h"
#include "wrapperbase.h"
#include "register-impl.h"
#include "lex.h"
#include "subr.h"
#include "native.h"

#include "attract.h"
#include "occupations.h"
#include "act.h"
#include "merc.h"
#include "handler.h"
#include "mercdb.h"

#include "cquest.h"
#include "quest.h"
#include "questregistrator.h"
#include "questmanager.h"
#include "questor.h"
#include "questtrader.h"
#include "xmlattributequestdata.h"
#include "def.h"

HOMETOWN(frigate);
enum {
    QCMD_NONE = 0,
    QCMD_BUY,
    QCMD_LIST,
    QCMD_TROUBLE,
    QCMD_REQUEST,
    QCMD_CANCEL,
    QCMD_COMPLETE,
    QCMD_FIND
};        

static const char * qcmd_names [] = {
    "", "buy", "list", "trouble", "request", "cancel", "complete", "find"
};

static void mprog_quest( Character *questor, Character *client, const DLString &cmd, const DLString &args )
{
    FENIA_VOID_CALL(questor, "Quest", "Css", client, cmd.c_str(), args.c_str());
    FENIA_NDX_VOID_CALL(questor->getNPC(), "Quest", "CCss", questor, client, cmd.c_str(), args.c_str());
}

static void see_also( PCharacter *pch )
{
    pch->println( "Смотри также {y{lRквест ?{lEquest ?{x для списка всех возможных действий." );
}

COMMAND(CQuest, "quest")
{
    Questor::Pointer questman;
    DLString arguments = constArguments;
    DLString cmd = arguments.getOneArgument( );
    PCharacter *pch = ch->getPC( );
        
    if (!pch)
        return;

    if (IS_GHOST( pch )) {
        pch->send_to("Наслажденье жизнью недоступно призракам.\r\n");
        return;
    }

    // Parse commands that can be done anywhere.
    if (cmd.empty( )) {
        doInfo( pch );
        see_also( pch );
        return;
    }
    else if (arg_is_info( cmd )) {
        doInfo( pch );
        return;
    }
    else if (arg_oneof(cmd, "newinfo")) {
        doNewInfo(pch, arguments);
        return;
    }
    else if (arg_oneof( cmd, "points", "очки" )) {
        doPoints( pch );
        return;
    }
    else if (arg_is_time( cmd )) {
        doTime( pch );
        return;
    }
    else if (arg_oneof( cmd, "stat", "статистика" )) {
        doStat( pch );
        return;
    }
    else if (arg_oneof( cmd, "set", "установить" ) && pch->isCoder( )) {
        doSet( pch, arguments );
        return;
    } else if (arg_is_help( cmd )) {
        usage( pch );
        return;
    }
   
    int qcmd = QCMD_NONE;
    
    // Parse quest trading commands.
    if (arg_is_list( cmd ))
        qcmd = QCMD_LIST;
    else if (arg_oneof( cmd, "buy", "купить" ))
        qcmd = QCMD_BUY;
    else if (arg_oneof( cmd, "trouble", "вернуть" ))
        qcmd = QCMD_TROUBLE;

    // Execute quest trading commands.
    if (qcmd != QCMD_NONE) { 
        QuestTrader::Pointer trader;

        trader = find_attracted_mob_behavior<QuestTrader>( pch, OCC_QUEST_TRADER );

        if (!trader) {
            pch->send_to( "Здесь нет торговца квестовыми благами.\r\n" );
            see_also( pch );
            return;
        }

        switch(qcmd) {
            case QCMD_LIST:
                trader->doList( pch );
                break;
            case QCMD_BUY:
                trader->doBuy( pch, arguments );
                break;
            case QCMD_TROUBLE:
                trader->doTrouble( pch, arguments );
                break;
            default:
                return;
        }
    
        mprog_quest(trader->getChar(), pch, qcmd_names[qcmd], arguments);
        return;
    }
   
    // Parse questor commands.
    if (arg_oneof( cmd, "request", "попросить", "получить", "просить" )) 
        qcmd = QCMD_REQUEST;
    else if (arg_oneof( cmd, "complete", "сдать", "завершить" )) 
        qcmd = QCMD_COMPLETE;
    else if (arg_oneof( cmd, "cancel", "отменить" )) 
        qcmd = QCMD_CANCEL;
    else if (arg_oneof( cmd, "find", "найти" )) 
        qcmd = QCMD_FIND;

    if (qcmd == QCMD_NONE) {
        usage( pch ); 
        return;
    }

    questman = find_attracted_mob_behavior<Questor>( pch, OCC_QUEST_MASTER );
    if (!questman) {
        if (pch->getHometown( ) != home_frigate) {
            pch->println("Здесь нет квестора.");
            see_also( pch );
            return;
        }

        // Special handling for newbie quests. TODO: add Fenia triggers for each command.
        switch(qcmd) {
            case QCMD_CANCEL:
                pch->println("Твое задание не нужно отменять, оно исчезнет как только ты сойдешь с корабля.");
                break;
            case QCMD_REQUEST:
            case QCMD_FIND:
            case QCMD_COMPLETE:
                pch->println("Эта команда недоступна, пока ты на корабле.");
                pch->println("Список текущих квестов показывает команда {y{lRквест инфо{lEquest info{x.");
                break;
        }
        return;
    }

    if (!questman->canGiveQuest( pch ))
        return;
   
    // Execute questor commands.
    switch(qcmd) {
        case QCMD_REQUEST:
            questman->doRequest( pch, arguments );
            break;
        case QCMD_COMPLETE:
            questman->doComplete( pch, arguments );
            break;
        case QCMD_CANCEL:
            questman->doCancel( pch );
            break;
        case QCMD_FIND:
            questman->doFind( pch );
            break;
        default:
            return;
    }

    mprog_quest(questman->getChar(), pch, qcmd_names[qcmd], arguments);
}

bool CQuest::gprog_questinfo( PCharacter *ch )
{
    static Scripting::IdRef ID_TMP( "tmp" ), ID_QUEST( "quest" ), ID_INFO( "info" );
    Scripting::Register tmpQuest;
    Scripting::RegisterList regList;

    if (!FeniaManager::wrapperManager)
        return false;

    try {
        tmpQuest = *(*Scripting::Context::root[ID_TMP])[ID_QUEST];
        regList.push_front( FeniaManager::wrapperManager->getWrapper( ch ) );
        return tmpQuest[ID_INFO]( regList ).toBoolean( );
    }
    catch (const Scripting::Exception &e) {
        LogStream::sendWarning( ) << "quest: " << e.what( ) << endl;
        return false;
    }
}

static bool gprog_quest( PCharacter *ch, const DLString &cmd, const DLString &arg )
{
    static Scripting::IdRef ID_TMP( "tmp" ), ID_QUEST( "quest" );
    Scripting::IdRef ID_CMD( cmd );
    Scripting::Register tmpQuest;
    Scripting::RegisterList regList;

    if (!FeniaManager::wrapperManager)
        return false;

    try {
        tmpQuest = *(*Scripting::Context::root[ID_TMP])[ID_QUEST];
        regList.push_back( FeniaManager::wrapperManager->getWrapper( ch ) );
        regList.push_back(Scripting::Register(arg));
        return tmpQuest[ID_CMD]( regList ).toBoolean( );
    }
    catch (const Scripting::Exception &e) {
        LogStream::sendWarning( ) << "quest: " << e.what( ) << endl;
        return false;
    }
}

void CQuest::doNewInfo( PCharacter *ch, const DLString &arguments ) 
{
    gprog_quest(ch, "newinfo", arguments);
}

void CQuest::doInfo( PCharacter *ch ) 
{
    std::basic_ostringstream<char> buf;
    int time;
    Quest::Pointer quest;
    
    quest = ch->getAttributes( ).findAttr<Quest>( "quest" );
    time = ch->getAttributes( ).getAttr<XMLAttributeQuestData>( "questdata" )->getTime( );

    if (!quest) {
        if (ch->getAttributes( ).isAvailable( "quest" )) 
            buf << "Твое задание невозможно ни выполнить, ни отменить." << endl;
    } else {
        quest->info( buf, ch );
        buf << "У тебя {Y" << time << "{x минут" << GET_COUNT(time, "а", "ы", "")
            << " на выполнение задания." << endl;
    }
    
    if (!gprog_questinfo( ch ) && buf.str( ).empty( ))
        buf << "У тебя сейчас нет задания." << endl;

    ch->send_to( buf );
}

void CQuest::doPoints( PCharacter *ch )  
{
    char buf [MAX_STRING_LENGTH];
    int points = ch->getQuestPoints();

    sprintf( buf, "У тебя {Y%d{x квестов%s единиц%s.\n\r", 
             points, GET_COUNT(points, "ая", "ых", "ых"), GET_COUNT(points, "а", "ы", ""));
    ch->send_to(buf);
}

void CQuest::doTime( PCharacter *ch ) 
{
    std::basic_ostringstream<char> buf;
    int time;
    Quest::Pointer quest;
    
    quest = ch->getAttributes( ).findAttr<Quest>( "quest" );
    time = ch->getAttributes( ).getAttr<XMLAttributeQuestData>( "questdata" )->getTime( );
    
    if (!quest) {
        if (ch->getAttributes( ).isAvailable( "quest" )) 
            buf << "Твое задание невозможно ни выполнить, ни отменить." << endl;
        else {
            buf << "У тебя сейчас нет задания." << endl;
            
            if (time > 1) 
                buf << "До того, как ты снова сможешь получить задание, {Y"
                    << time <<  "{x минут"
                    << GET_COUNT(time, "а", "ы", "") << endl;
            else if (time == 1) 
                buf <<"Осталось меньше минуты до того, как ты снова сможешь получить задание." << endl;
        }
        
    }
    else {
         buf << "У тебя {Y" << time << "{x минут" << GET_COUNT(time, "а", "ы", "")
             << " на выполнение задания." << endl;
    }

    ch->send_to( buf );
}

void CQuest::doSet( PCharacter *ch, DLString& arguments )
{
    int count;
    PCMemoryInterface *pci;
    DLString name, questID, number;
    bool plus;
    XMLAttributeQuestData::Pointer attr;
    QuestRegistratorBase::Pointer qbase;
        
    name = arguments.getOneArgument( );
    questID = arguments.getOneArgument( ); 
    number = arguments.getOneArgument( ); 
    plus = false;
    
    if (name == "clear") {
        PCharacterMemoryList::const_iterator i;
        const PCharacterMemoryList &pcm = PCharacterManager::getPCM( );
        
        for (i = pcm.begin( ); i != pcm.end( ); i++) {
            XMLAttributeQuestData::Pointer attr;
            PCMemoryInterface *pc;

            pc = i->second;
            attr = pc->getAttributes( ).findAttr<XMLAttributeQuestData>( "questdata" );

            if (attr) {
                XMLAttributeStatistic::Victories::const_iterator j;
                const XMLAttributeStatistic::Victories &v = attr->getVictories( );

                for (j = v.begin( ); j != v.end( ); j++)
                    attr->setVictories( j->first, 0 );

                PCharacterManager::saveMemory( pc );
            }
        }
        
        return;
    }
    
    if (name.empty( ) || questID.empty( ) || number.empty( )) {
        ch->send_to("Использование: quest set <player> <quest id> [+]<num. of victories>\r\n");
        return;
    }

    pci = PCharacterManager::find( name );

    if (!pci) {
        ch->printf("%s: имя не найдено.\r\n", name.c_str( ));
        return;
    }
    
    qbase = QuestManager::getThis( )->findQuestRegistrator( questID );

    if (!qbase) {
        ch->send_to("Неправильный ID.\r\n");
        return;
    }
    
    try {
        if (number.at( 0 ) == '+') {
            plus = true;
            number.erase( 0, 1 );
        }
        
        count = number.toInt( );
    } catch (const ExceptionBadType&) {
        ch->send_to("Неверное количество побед.\r\n");
        return;
    }
   
    attr = pci->getAttributes( ).getAttr<XMLAttributeQuestData>( "questdata" );
    
    if (plus)
        count += attr->getVictories( qbase->getName( ) );

    attr->setVictories( qbase->getName( ), count );
    PCharacterManager::saveMemory( pci );
    ch->send_to("Done.\r\n");
}

void CQuest::doStat( PCharacter *ch )
{
    ostringstream buf;
    XMLAttributeStatistic::Statistic stat;
    XMLAttributeStatistic::Statistic::iterator s;
    
    stat = XMLAttributeStatistic::gatherAll( "questdata" );

    buf << "{wЛучшие квестодеятели Мира Грез: {x" << endl;

    for (s = stat.begin( ); s != stat.end( ); s++) {
        XMLAttributeStatistic::StatRecordList::iterator r;
        QuestRegistratorBase::Pointer qb;
        int last = 0, cnt = 0;
        
        qb = QuestManager::getThis( )->findQuestRegistrator( s->first );

        if (!qb)
            continue;

        XMLAttributeStatistic::StatRecordList &records = s->second;
        
        buf << "{W\"" << qb->getShortDescr( ) << "\"{x" << endl;

        for (r = records.begin( ); r != records.end( ) && cnt < 5; cnt++) {
            last = r->second;
            buf << dlprintf( "          {W%4d{w ", last ); 

            for ( ; r != records.end( ) && r->second == last; r++)
                buf << r->first << " ";

            buf << "{x" << endl;
        }
        
        buf << endl;
    }

    ch->send_to( buf );
}

void CQuest::usage( PCharacter *ch ) 
{
    ostringstream buf;

    buf << "Укажи одну из квестовых команд:" << endl << "    "
        << "{lRочки инфо время попросить сдать список купить вернуть найти отменить стат{lEpoints info time request complete list buy trouble find cancel stat{lx." 
        << endl
        << "Также смотри {W{lR? квесты{lE? quests{x." << endl;
    ch->send_to( buf );
}

    
