/* $Id$
 *
 * ruffina, 2004
 */
#include "logstream.h"
#include "profiler.h"
#include "update_areas.h"

#include "pcharacter.h"
#include "npcharacter.h"
#include "room.h"
#include "object.h"
#include "wearlocation.h"

#include "dreamland.h"
#include "loadsave.h"
#include "wiznet.h"
#include "gsn_plugin.h"
#include "descriptor.h"
#include "occupations.h"
#include "save.h"
#include "merc.h"
#include "mercdb.h"
#include "def.h"

/*
 * Repopulate areas periodically.
 */
void area_update( )
{
//    ProfilerBlock be("area_update");

    for (AREA_DATA *pArea = area_first; pArea != 0; pArea = pArea->next )
    {
        if ( ++pArea->age < 3 )
            continue;

        /*
         * Check age and reset.
         * Note: Mud School resets every 3 minutes (not 15).
         */
        if ( (!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15))
                || pArea->age >= 31)
        {
            reset_area( pArea );
            wiznet( WIZ_RESETS, 0, 0, "%s has just been reset.", pArea->name );

            pArea->age = number_range( 0, 3 );
            if (IS_SET(pArea->area_flag, AREA_POPULAR))
                pArea->age = 15 - 2;
            else if (pArea->nplayer == 0)
                pArea->empty = true;
        }
    }
}

static Object * get_obj_list_vnum( Object *list, int vnum )
{
    Object *obj;

    for (obj = list; obj; obj = obj->next_content) 
        if (obj->pIndexData->vnum == vnum)
            return obj;
    
    return NULL;
}

static Object * get_obj_here_vnum( Room *room, int vnum )
{
    Object *obj, *result;
    
    for (obj = room->contents; obj; obj = obj->next_content) {
        if (obj->pIndexData->vnum == vnum)
            return obj;
            
        if (( result = get_obj_list_vnum( obj->contains, vnum ) ))
            return result;
    }

    return NULL;
}

static RESET_DATA * find_mob_reset(Room *pRoom, NPCharacter *mob)
{
    RESET_DATA *pReset;
    for (pReset = pRoom->reset_first; pReset != 0; pReset = pReset->next)
        if (pReset->command == 'M' && pReset->arg1 == mob->pIndexData->vnum)
            return pReset;

    return 0;
}

/** Create an item for inventory or equipment for a given mob, based on reset data. */
static Object * create_item_for_mob(RESET_DATA *pReset, OBJ_INDEX_DATA *pObjIndex, NPCharacter *mob)
{
    Object *obj = NULL;

    // Obj limit reached, do nothing.
    if (pObjIndex->limit != -1 && pObjIndex->count >= pObjIndex->limit) 
        return obj;
    
    obj = create_object(pObjIndex, 0);
    obj->reset_mob = mob->getID();

    // Mark shop items with 'inventory' flag.
    if (mob->behavior && IS_SET(mob->behavior->getOccupation( ), (1 << OCC_SHOPPER)))
    {
        if (pReset->command == 'G')
            SET_BIT( obj->extra_flags, ITEM_INVENTORY );
    }

    // Give and equip, if the slot is empty.
    obj_to_char( obj, mob );

    if (pReset->command == 'E') {
        Wearlocation *wloc = wearlocationManager->find( pReset->arg3 );
        if (wloc && wloc->find(mob) == 0)
            wloc->equip( obj );
    }

    return obj;
}

/** Recreate an item on a mob based on reset data, if it doesn't exist yet. */
static bool reset_mob_item(RESET_DATA *myReset, NPCharacter *mob)
{
    OBJ_INDEX_DATA *pObjIndex = get_obj_index(myReset->arg1);
    if (!pObjIndex)
        return false;

    Object *self = get_obj_list_vnum(mob->carrying, pObjIndex->vnum);
    if (self) // TODO handle 'P' resets inside 'E'/'G' resets
        return false;

    Object *newItem = create_item_for_mob(myReset, pObjIndex, mob);
    if (newItem) {
        wiznet(WIZ_RESETS, 0, 0, "Created [%d] %s for mob %s in [%d].", 
               pObjIndex->vnum, newItem->getShortDescr('1').c_str(), 
               mob->getNameP('1').c_str(), mob->in_room->vnum);
        return true;
    }

    return false;
}

/** Recreate inventory and equipment for a mob, if an item has been
  * requested or stolen from it.
  */
static bool reset_one_mob(NPCharacter *mob)
{
    // Find room where this mob was created and corresponding reset for it.
    // Reset may not be exact, p.ex. when several guards are reset in the same room with different equipment.
    // Such situations should be avoided in general, by creating different vnums for guards.
    Room *home = get_room_index(mob->reset_room);
    if (!home)
        return false;

    RESET_DATA *myReset = find_mob_reset(home, mob);
    if (!myReset)
        return false;
   
    // Update all inventory and equipment on the existing mob. 
    RESET_DATA *pReset;
    bool changed = false;
    for (pReset = myReset->next; pReset; pReset = pReset->next)
        switch (pReset->command) {
        case 'E':
        case 'G':
            if (reset_mob_item(pReset, mob)) 
                changed = true;
            break;

        default:
            return changed;
        }
        
        
    return changed; 
}

/** Recreate inventory and equipment for all NPC in the room. */
static bool reset_room_mobs(Room *pRoom)
{
    bool changed = false;

    for (Character *rch = pRoom->people; rch; rch = rch->next_in_room) {
        if (!rch->is_npc())
            continue;
        if (IS_CHARMED(rch))
            continue;

        NPCharacter *mob = rch->getNPC();
        if (mob->reset_room == 0)
            continue;
        if (mob->zone != 0 && mob->in_room->area != mob->zone)
            continue;

        if (reset_one_mob(mob))
            changed = true;
    }

    return changed;
}

void reset_room(Room *pRoom)
{
    RESET_DATA *pReset;
    EXTRA_EXIT_DATA *eexit;
    NPCharacter *mob;
    bool last;
    short level;
    int iExit;
    bool changedMob, changedObj;
    
    if (weather_info.sky == SKY_RAINING 
        && !IS_SET(pRoom->room_flags, ROOM_INDOORS) ) 
    {
        pRoom->history.erase( );

        if (number_percent( ) < 50)
            pRoom->history.erase( );
    }

    for(iExit = 0; iExit < DIR_SOMEWHERE; iExit++) {
        EXIT_DATA *pExit = pRoom->exit[iExit];
        if(pExit) {
            if(IS_SET(pExit->exit_info_default, EX_LOCKED)) {
                if(!IS_SET(pExit->exit_info, EX_LOCKED))
                    SET_BIT(pExit->exit_info, EX_LOCKED);
            } else {
                if(IS_SET(pExit->exit_info, EX_LOCKED))
                    REMOVE_BIT(pExit->exit_info, EX_LOCKED);
            }
            if(IS_SET(pExit->exit_info_default, EX_CLOSED)) {
                if(!IS_SET(pExit->exit_info, EX_CLOSED))
                    SET_BIT(pExit->exit_info, EX_CLOSED);
            } else {
                if(IS_SET(pExit->exit_info, EX_CLOSED))
                    REMOVE_BIT(pExit->exit_info, EX_CLOSED);
            }
        }
    }

    for(eexit = pRoom->extra_exit; eexit; eexit = eexit->next) {
        eexit->exit_info = eexit->exit_info_default;
    }

    mob     = 0;
    last    = true;
    level    = 0;
    changedMob = changedObj = false;

    dreamland->removeOption( DL_SAVE_OBJS );
    dreamland->removeOption( DL_SAVE_MOBS );

    // Update existing mobs before creating new ones.
    if (reset_room_mobs(pRoom))
        changedMob = true;

    for ( pReset = pRoom->reset_first; pReset != 0; pReset = pReset->next )
    {
        MOB_INDEX_DATA *pMobIndex;
        OBJ_INDEX_DATA *pObjIndex;
        OBJ_INDEX_DATA *pObjToIndex;
        EXIT_DATA *pexit;
        Object *obj = 0;
        Object *obj_to = 0;
        int count = 0, limit;
        
        switch ( pReset->command )
        {
        default:
            LogStream::sendError( ) << "Reset_area: bad command " << pReset->command << '.' << endl;
            break;

        case 'M':

            if ( ( pMobIndex = get_mob_index( pReset->arg1 ) ) == 0 )
            {
                bug( "Reset_area: 'M': bad vnum %d.", pReset->arg1 );
                continue;
            }

            if (pReset->arg2 != -1 && pMobIndex->count >= pReset->arg2 )
            {
                last = false;
                break;
            }

            count = 0;

            for (Character *vch = pRoom->people; vch; vch = vch->next_in_room )
                if (vch->is_npc() && vch->getNPC()->pIndexData == pMobIndex )
                {
                    count++;

                    if ( count >= pReset->arg4 )
                    {
                        last = false;
                        break;
                    }
                }

            if ( count >= pReset->arg4 ) 
                break;

            mob = create_mobile( pMobIndex );

            /* set area */
            mob->zone = pRoom->area;
            mob->reset_room = pRoom->vnum;

            char_to_room( mob, pRoom );
            changedMob = true;
            level = URANGE( 0, mob->getRealLevel( ) - 2, LEVEL_HERO - 1 );
            last  = true;
            break;

        case 'O':
            if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == 0 )
            {
                bug( "Reset_area: 'O': bad vnum %d.", pReset->arg1 );
                continue;
            }

            if ((pRoom->area->nplayer > 0 && !IS_SET(pRoom->area->area_flag, AREA_POPULAR))
                || count_obj_list( pObjIndex, pRoom->contents ) > 0 )
            {
                last = false;
                break;
            }

            if ( ( pObjIndex->limit != -1 )
                && ( pObjIndex->count >= pObjIndex->limit ) )
            {
                last = false;
                break;
            }

            obj = create_object( pObjIndex, min(number_fuzzy(level), LEVEL_HERO - 1) );
            obj->cost = 0;
            obj->reset_room = pRoom->vnum;
            obj_to_room( obj, pRoom );
            changedObj = true;
            last = true;
            break;

        case 'P':
            if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == 0 )
            {
                bug( "Reset_area: 'P': bad vnum %d.", pReset->arg1 );
                continue;
            }

            if ( ( pObjToIndex = get_obj_index( pReset->arg3 ) ) == 0 )
            {
                bug( "Reset_area: 'P': bad vnum %d.", pReset->arg3 );
                continue;
            }

            if ( pReset->arg2 > 50 )         /* old format */
                limit = 6;
            else if ( pReset->arg2 == -1 )     /* no limit */
                limit = 999;
            else
                limit = pReset->arg2;

            if ((pRoom->area->nplayer > 0 && !IS_SET(pRoom->area->area_flag, AREA_POPULAR))
                || ( obj_to = get_obj_here_vnum( pRoom, pObjToIndex->vnum ) ) == 0
                || ( obj_to->in_room == 0 && !last )
                || ( pObjIndex->count >= limit && number_range(0,4) != 0 )
                || ( count = count_obj_list(pObjIndex,obj_to->contains) )
                            > pReset->arg4 )
            {
                last = false;
                break;
            }

            if ( ( pObjIndex->limit != -1 )
                && ( pObjIndex->count >= pObjIndex->limit ) )
            {
                last = false;
                LogStream::sendNotice( ) << "Reseting area: [P] OBJ limit reached: " << pObjIndex->area->name << endl;
                break;
            }

            while (count < pReset->arg4)
            {
                obj = create_object( pObjIndex, number_fuzzy(obj_to->level) );
                obj->reset_obj = obj_to->getID();
                obj_to_obj( obj, obj_to );
                changedObj = true;
                count++;

                if ( pObjIndex->count >= limit )
                    break;
            }

            /* fix object lock state! */
            obj_to->value[1] = obj_to->pIndexData->value[1];
            last = true;
            
            break;

        case 'G':
        case 'E':
            if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == 0 )
            {
                bug( "Reset_area: 'E' or 'G': bad vnum %d.", pReset->arg1 );
                continue;
            }

            if ( !last )
                break;

            if ( mob == 0 )
            {
                bug( "Reset_area: 'E' or 'G': null mob for vnum %d.",    pReset->arg1 );
                last = false;
                break;
            }

            obj = create_item_for_mob(pReset, pObjIndex, mob);
            if (!obj)
                break;

            changedMob = true;
            last = true;
            break;

        case 'D':
            break;

        case 'R':
            {
                int d0, d1;
                int min, max;

                min = pReset->arg3;
                max = pReset->arg2 - 1;
            
                for (d0 = min; d0 < max; d0++)
                {
                    d1 = number_range( d0, max );
                    pexit = pRoom->exit[d0];
                    pRoom->exit[d0] = pRoom->exit[d1];
                    pRoom->exit[d1] = pexit;
                }
            }
            break;
        }
    }

    dreamland->resetOption( DL_SAVE_OBJS );
    dreamland->resetOption( DL_SAVE_MOBS );

    if (changedMob)
        save_mobs( pRoom );

    if (changedObj)
        save_items( pRoom );
}

/*
 * Reset one area.
 */
void reset_area( AREA_DATA *pArea )
{
    const char *resetmsg;
    static const char *default_resetmsg = "Ты слышишь мелодичный перезвон колокольчиков.";        

    for (map<int, Room *>::iterator i = pArea->rooms.begin( ); i != pArea->rooms.end( ); i++)
        reset_room( i->second );
    
    if (pArea->behavior) 
        pArea->behavior->update( );

    if (pArea->resetmsg)
        resetmsg = pArea->resetmsg;
    else
        resetmsg = default_resetmsg;

    for (Descriptor *d = descriptor_list; d != 0; d = d->next) {
        Character *ch;
        
        if (d->connected == CON_PLAYING
                && ( ch = d->character )
                && IS_AWAKE(ch)
                && ch->in_room 
                && !IS_SET(ch->in_room->room_flags, ROOM_NOWHERE)
                && ch->in_room->area == pArea) 
        {
            if (weather_info.sky == SKY_RAINING 
                && !IS_SET(ch->in_room->room_flags, ROOM_INDOORS)
                && gsn_track->getEffective( ch ) > 50)
            {
                ch->println( "Внезапно налетевший дождь смывает все следы." );
            }

            ch->println( resetmsg );
        }
    }
}


