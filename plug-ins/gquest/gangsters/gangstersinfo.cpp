/* $Id: gangstersinfo.cpp,v 1.1.2.2.6.1 2008/11/13 03:33:28 rufina Exp $
 * 
 * ruffina, 2003
 */

#include "gangstersinfo.h"
#include "gangsters.h"

#include "class.h"

#include "pcharacter.h"

#include "mercdb.h"
#include "def.h"

GangstersInfo* GangstersInfo::thisClass = NULL;
const DLString GangstersInfo::QUEST_ID= "gangsters";


GangstersInfo::GangstersInfo( ) 
{
    thisClass = this;
    Class::regMoc<GangstersInfo>( );
    Class::regMoc<Gangsters>( );
}

GangstersInfo::~GangstersInfo( )
{
    Class::unregMoc<GangstersInfo>( );
    Class::unregMoc<Gangsters>( );
    thisClass = NULL;
}
   
bool GangstersInfo::canAutoStart( const PlayerList &players, Config &config ) const
{
    int level, i;
    int psize = players.size( );
    
    if (psize == 0)
        return false;

    i = number_range( 0, psize - 1 );
    level = players[i]->getModifyLevel( );
    
    config.minLevel = max( 1, level - lowGap.getValue( ) );
    config.maxLevel = level + highGap.getValue( );

    config.time = getDefaultTime( );
    return true;
}

bool GangstersInfo::canParticipate( PCharacter *ch ) const
{
    if (!GlobalQuestInfo::canParticipate( ch ))
        return false;

    if (ch->getAttributes( ).isAvailable( "nogangsters" ))
        return false;

    return true;
}

GlobalQuest::Pointer GangstersInfo::getQuestInstance( ) const
{
    return Gangsters::Pointer( NEW, QUEST_ID );
}

int GangstersInfo::getDefaultTime( ) const
{
    return number_range( 15, 35 );
}

const DLString & GangstersInfo::getQuestID( ) const 
{
    return QUEST_ID;
}
