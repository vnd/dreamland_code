/* $Id$
 *
 * ruffina, 2004
 */
#include <list>
#include "loadsave.h"
#include "save.h"

#include "affect.h"
#include "pcharacter.h"
#include "npcharacter.h"
#include "object.h"
#include "room.h"

#include "descriptor.h"
#include "merc.h"
#include "mercdb.h"
#include "def.h"

WEARLOC(none);
bool obj_has_name( Object *obj, const DLString &arg, Character *ch );
bool char_has_name(Character *target, const char *arg);

// Parse string argument as if it contains entity ID.
long long get_arg_id( const DLString &cArgument )
{
    DLString arg( cArgument );
    arg = arg.getOneArgument( );
    LongLong id;

    if (arg.size( ) < 10)
        return 0;
    if (!is_number( arg.c_str( ) ))
        return 0;

    try {
        id.fromString( arg );
    } catch (ExceptionBadType &e) {
        return 0;
    }

    return id;
}    


/*
 * Find a char in the room.
 */
Character *get_char_room( Character *ch, const DLString & constArgument, bool fSeenOnly )
{
    char argument[MAX_INPUT_LENGTH];

    strcpy( argument, constArgument.c_str( ) );
    return get_char_room( ch, argument, fSeenOnly );
}

Character *get_char_room( Character *ch, char *argument, bool fSeenOnly )
{
    char arg[MAX_INPUT_LENGTH];
    int number;

    number = number_argument( argument, arg );
    return get_char_room( ch, ch->in_room, arg, &number, fSeenOnly );
}

Character * get_char_room( Character *ch, Room *room, const DLString &constArgument, bool fSeenOnly )
{
    char arg[MAX_INPUT_LENGTH], argument[MAX_INPUT_LENGTH];
    int number;

    strcpy( argument, constArgument.c_str( ) );
    number = number_argument( argument, arg );
    return get_char_room( ch, room, arg, &number, fSeenOnly );
}

/*
 * Find a char in the room.
 * Chronos uses in act_move.c
 */
Character *get_char_room( Character *ch, Room *room, const char *argument, int *number, bool fSeenOnly )
{
    Character *rch;
    int count;
    int ugly;
    long long id = get_arg_id( argument );

    if (room == 0)
        return 0;

    count  = 0;
    ugly   = 0;

    if (arg_is_self( argument ))
        return ch;

    if (arg_is_ugly( argument ))
        ugly = 1;

    for (rch = room->people; rch != 0; rch = rch->next_in_room)
    {
        if (!ch->can_sense( rch ))
            continue;

        if (!ch->can_see( rch ) && fSeenOnly)
            continue;

        if (ugly) {
            if (!IS_VAMPIRE(rch))
                continue;

            if (++count == *number)
                return rch;
        }
        else {

            Character *tch;
            
            tch = rch->getDoppel( ch );

            if (tch->is_npc( ))
            {
                if (id && tch->getID( ) == id)
                    return rch;
                if (id || !char_has_name(tch, argument))
                    continue;
            }
            else
            {
                if (id && tch->getID( ) == id)
                    return rch;
                if (id || !char_has_name(tch, argument))
                    continue;
            }

            if (++count == *number)
                return rch;
        }
    }

    *number -= count;

    return 0;
}


/*
 * Find a char in the area.
 */
Character *get_char_area( Character *ch, char *argument )
{
        char arg[MAX_INPUT_LENGTH];
        Character *ach;
        int number;
        int count;
        long long id;

        if ( ( ach = get_char_room( ch, argument ) ) != 0 )
                return ach;

        id = get_arg_id( argument );
        number = number_argument( argument, arg ) - count_char_room ( ch, argument ) ;
        count = 0;

        for( ach = char_list; ach != 0; ach = ach->next )
        {
                if ( ach->in_room == 0
                        || ach->in_room == ch->in_room
                        || ach->in_room->area != ch->in_room->area
                        || !ch->can_see( ach )
                        || (id && ach->getID( ) != id)
                        || (!id && !char_has_name(ach, arg)) )
                {
                        continue;
                }

                if(id || ++count == number )
                        return ach;
        }
        return 0;
}


/*
 * Find a char in the world.
 */
Character *get_char_world( Character *ch, const DLString &arg )
{
    return get_char_world( ch, arg.c_str( ) );
}
Character *get_char_world( Character *ch, const char *cArgument )
{
        char arg[MAX_INPUT_LENGTH];
        char arg_buf[strlen(cArgument)+1];
        char *argument = arg_buf;
        Character *wch;
        int number;
        int count;
        
        strcpy( arg_buf, cArgument );

        if ( ( wch = get_char_room( ch, argument ) ) != 0 )
                return wch;

        number = number_argument( argument, arg ) - count_char_room( ch, argument );

        count  = 0;
        for ( wch = char_list; wch != 0 ; wch = wch->next )
        {
                if (wch->in_room == 0)
                    continue;
                if (wch->in_room == ch->in_room)
                    continue;
                if (!ch->can_see( wch ))
                    continue;
                if (!char_has_name(wch, arg))
                    continue;

                if ( ++count >= number )
                        return wch;
        }

        return 0;
}

/*
 * Find an obj in a list.
 *
 * Change get_obj_here if correct this function
 *
 */
Object *get_obj_list( Character *ch, const DLString &cArg, Object *list, DLString pocket )
{
        char arg[MAX_INPUT_LENGTH];
        char argument[MAX_INPUT_LENGTH];
        Object *obj;
        int number;
        int count;
        long long id;
       
        strcpy(argument, cArg.c_str( ));
        id = get_arg_id( argument );
        number = number_argument( argument, arg );
        count  = 0;
        for ( obj = list; obj != 0; obj = obj->next_content )
        {
            if (!ch->can_see( obj ) && !ch->can_hear( obj ))
                continue;
           
            if (id && obj->getID( ) != id)
                continue;

            if (!id && !obj_has_name( obj, arg, ch ))
                continue;

            if (!pocket.empty( ) && obj->pocket != pocket)
                continue;
            
            if (pocket.empty( ) && !obj->pocket.empty( ))
                continue;

            if (id || ++count == number)
                return obj;
        }

        return 0;
}

DLString get_obj_name_list( Object *target, Object *list, Character *ch )
{
    Object *obj;
    int count;

    for (obj = list, count = 1; 
         obj && obj != target; 
         obj = obj->next_content) 
    {
        if (obj->getName( ) == target->getName( ))
            if (ch->can_see( obj ) || ch->can_hear( obj ))
                count++;
    }

    return DLString( count ) + "." + target->getName( );
}

DLString get_char_name_list( Character *target, Character *list, Character *ch )
{
    Character *rch;
    int count;

    for (rch = list, count = 1; 
         rch && rch != target; 
         rch = rch->next_in_room) 
    {
        if (rch->getName( ) == target->getName( ))
            if (ch->can_see( rch ))
                count++;
    }

    return DLString( count ) + "." + target->getName( );
}

Object * get_obj_carry( Character *ch, const DLString & constArgument )
{
    char argument[MAX_INPUT_LENGTH];
    strcpy(argument, constArgument.c_str( ));
    return get_obj_carry( ch, argument );
}

/*
 * Find an obj in player's inventory.
 *
 * Change get_obj_here if correct this function
 *
 */
Object *get_obj_carry( Character *ch, char *argument )
{
        char arg[MAX_INPUT_LENGTH];
        Object *obj;
        int number;
        int count;
        long long id = get_arg_id( argument );

        number = number_argument( argument, arg );
        count  = 0;

        for ( obj = ch->carrying; obj != 0; obj = obj->next_content )
        {
                if ( obj->wear_loc == wear_none
                        && ( ch->can_see( obj ) )
                        && ((id && obj->getID( ) == id) 
                             || (!id && obj_has_name( obj, arg, ch ))))
                {
                        if (id || ++count == number )
                                return obj;
                }
        }

        return 0;
}

/*
 * See obj in player's equipment
 *
 * Change get_obj_here if correct this function
 *
 */
Object *see_obj_carry( Character *stealer, Character *ch, char *argument )
{
        char arg[MAX_INPUT_LENGTH];
        Object *obj;
        int number;
        int count;
        long long id = get_arg_id( argument );

        number = number_argument( argument, arg );
        count  = 0;

        for ( obj = ch->carrying; obj != 0; obj = obj->next_content )
        {
                if ( obj->wear_loc == wear_none
                        && ( stealer->can_see( obj ) )
                        && ((id && obj->getID( ) == id) 
                             || (!id && obj_has_name( obj, arg, stealer  ))))
                {
                        if (id || ++count == number )
                                return obj;
                }
        }

        return 0;
}



/*
 * Find an obj in player's equipment.
 *
 * Change get_obj_here if correct this function
 *
 */
Object *get_obj_wear( Character *ch, const DLString &arg )
{
    return get_obj_wear( ch, arg.c_str( ) );
}
Object *get_obj_wear( Character *ch, const char *cargument )
{
    char arg[MAX_INPUT_LENGTH];
    char argument[MAX_INPUT_LENGTH];
    Object *obj;
    int number;
    int count;
    long long id = get_arg_id( cargument );
    
    strcpy(argument, cargument);
    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != 0; obj = obj->next_content )
    {
        if ( obj->wear_loc != wear_none
             && ((id && obj->getID( ) == id) 
                 || (!id && obj_has_name( obj, arg, ch ))))
        {
            if (id || ++count == number)
                return obj;
        }
    }

    return 0;
}

Object * get_obj_here( Character *ch, const DLString & constArgument )
{
    char argument[MAX_INPUT_LENGTH];
    strcpy(argument, constArgument.c_str( ));
    return get_obj_here( ch, argument );
}
/*
 * Find an obj in the room or in inventory.
 */
Object *get_obj_here( Character *ch, char *argument )
{
        char arg[MAX_INPUT_LENGTH];
        Object *obj;
        int count = 0;
        int number = number_argument( argument, arg );
        long long id = get_arg_id( argument );

        // At first look in room...
        for ( obj = ch->in_room->contents; obj != 0; obj = obj->next_content )
        {
                if ( ( ch->can_see( obj ) || ch->can_hear( obj ) )
                        && ((id && obj->getID( ) == id) || (!id && obj_has_name( obj, arg, ch ))) )
                {
                        if (id || ++count == number )
                                return obj;
                }
        }

        // ... then inventory ...
        for ( obj = ch->carrying; obj != 0; obj = obj->next_content )
        {
                if ( obj->wear_loc == wear_none
                        && ( ch->can_see( obj ) || ch->can_hear( obj ) )
                        && ((id && obj->getID( ) == id) || (!id && obj_has_name( obj, arg, ch ))) )
                {
                        if (id || ++count == number )
                                return obj;
                }
        }

        // ... and equipment, at last!
        for ( obj = ch->carrying; obj != 0; obj = obj->next_content )
        {
                if ( obj->wear_loc != wear_none
                        && ((id && obj->getID( ) == id) || (!id && obj_has_name( obj, arg, ch ))) )
                {
                        if (id || ++count == number )
                                return obj;
                }
        }

        return 0;
}



/*
 * Find an obj in the world.
 */
Object *get_obj_world( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;
    int number;
    int count;

    if ( ( obj = get_obj_here( ch, argument ) ) != 0 )
        return obj;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = object_list; obj != 0; obj = obj->next )
    {
        if ( ch->can_see( obj ) && obj_has_name( obj, arg, ch ) )
        {
            if ( ++count == number )
                return obj;
        }

    }

    return 0;
}


/*
 * Count character with specified name in the room
 */
int count_char_room( Character *ch, char *argument )
{
        char arg[MAX_INPUT_LENGTH];
        Character *rch;
        int count;
        int ugly;

        int number = number_argument( argument, arg );
        count  = 0;
        ugly   = 0;

        if (arg_is_self( arg ))
                return 1;

        if (arg_is_ugly( arg ))
                ugly = 1;

        for ( rch = ch->in_room->people; rch != 0; rch = rch->next_in_room )
        {
                if ( !ch->can_see( rch ) )
                        continue;

                if ( ugly
                        && IS_VAMPIRE(rch) )
                {
                        count++;
                        continue;
                }

                Character *tch;
                
                tch = rch->getDoppel( ch );

                if (!char_has_name(tch, argument))
                        continue;

                count++;
        }

        return count;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list( OBJ_INDEX_DATA *pObjIndex, Object *list )
{
    Object *obj;
    int nMatch;

    nMatch = 0;
    for ( obj = list; obj != 0; obj = obj->next_content )
    {
        if ( obj->pIndexData == pObjIndex )
            nMatch++;
    }

    return nMatch;
}


/*
 * Find an obj in the room
 */
Object *get_obj_room( Character *ch, const DLString &arg )
{
    return get_obj_room( ch, arg.c_str( ) );
}
Object *get_obj_room( Character *ch, const char *cArgument )
{
    char arg[MAX_INPUT_LENGTH];
    char argument[MAX_INPUT_LENGTH];
    Object *obj;
    int count = 0, number;
    long long id = get_arg_id( cArgument );

    strcpy( argument, cArgument );
    number = number_argument( argument, arg );

    for ( obj = ch->in_room->contents; obj != 0; obj = obj->next_content ) {
        if (( ch->can_see( obj ) || ch->can_hear( obj ) )
              && ((id && obj->getID( ) == id) || (!id && obj_has_name( obj, arg, ch ))) )
        {
            if (id || ++count == number )
                return obj;
        }
    }

    return NULL;
}

Object *get_obj_wear_victim( Character *victim, const DLString &cArg, Character *ch )
{
    char arg[MAX_INPUT_LENGTH];
    char argument[MAX_INPUT_LENGTH];
    Object *obj;
    int number;
    int count;
    long long id = get_arg_id( cArg );
    
    strcpy(argument, cArg.c_str( ));
    number = number_argument( argument, arg );
    count  = 0;

    for (obj = victim->carrying; obj; obj = obj->next_content )
        if (obj->wear_loc != wear_none
            && ((id && obj->getID( ) == id) || (!id && obj_has_name( obj, arg, ch ))) 
            && ch->can_see( obj ))
        {
            if (id || ++count == number)
                return obj;
        }

    return NULL;
}

Object * get_obj_list_type( Character *ch, const DLString &cArg, int type, Object *list )
{
    char arg[MAX_INPUT_LENGTH];
    char argument[MAX_INPUT_LENGTH];
    Object *obj;
    int number;
    int count;
    
    strcpy(argument, cArg.c_str( ));
    number = number_argument( argument, arg );
    count  = 0;

    for (obj = list; obj; obj = obj->next_content )
        if (obj->item_type == type
            && obj_has_name( obj, arg, ch ) 
            && (ch->can_see( obj ) || ch->can_hear( obj )))
        {
            if (++count == number)
                return obj;
        }

    return NULL;
}

Object * get_obj_list_type( Character *ch, int type, Object *list )
{
    Object *obj;

    for (obj = list; obj; obj = obj->next_content)
        if (obj->item_type == type
            && (ch->can_see( obj ) || ch->can_hear( obj )))
            return obj;

    return NULL;
}

std::list<Object *> get_objs_list_type( Character *ch, int type, Object *list )
{
    Object *obj;
    std::list<Object *> result;    

    for (obj = list; obj; obj = obj->next_content)
        if (obj->item_type == type
            && (ch->can_see( obj ) || ch->can_hear( obj )))
            result.push_back(obj);

    return result;
}

Object * get_obj_room_type( Character *ch, int type )
{
    return get_obj_list_type( ch, type, ch->in_room->contents );
}

Object * get_obj_room_type( Room *room, int type )
{
    Object *obj;

    for (obj = room->contents; obj; obj = obj->next_content)
        if (obj->item_type == type)
            return obj;

    return NULL;
}

Object * get_obj_carry_type( Character *ch, int type )
{
    return get_obj_list_type( ch, type, ch->carrying );
}

Object * get_obj_room_vnum( Room *room, int vnum )
{
    Object *obj;

    for (obj = room->contents; obj; obj = obj->next_content)
        if (obj->pIndexData->vnum == vnum)
            return obj;

    return NULL;
}

Object * get_obj_list_vnum( Character *ch, int vnum, Object *list )
{
    Object *obj;

    for (obj = list; obj; obj = obj->next_content) 
        if (obj->pIndexData->vnum == vnum
            && (ch->can_see( obj ) || ch->can_hear( obj )))
            return obj;

    return NULL;
}

Object * get_obj_carry_vnum( Character *ch, int vnum )
{
    return get_obj_list_vnum( ch, vnum, ch->carrying );
}

/*
 * Find a char in the world, taking doppel in consideration
 */
Character *get_char_world_doppel( Character *ch, const char *cArgument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg_buf[strlen(cArgument)+1];
    char *argument = arg_buf;
    Character *wch;
    int number;
    int count;
    
    strcpy( arg_buf, cArgument );

    if ( ( wch = get_char_room( ch, argument ) ) != 0 )
            return wch;

    number = number_argument( argument, arg ) - count_char_room ( ch, argument ) ;

    count  = 0;
    for ( wch = char_list; wch != 0 ; wch = wch->next )
    {
        Character *dch = wch->getDoppel( ch );

        if (dch->in_room == 0)
            continue;
        if (dch->in_room == ch->in_room)
            continue;
        if (!ch->can_see( dch ))
            continue;
        if (!char_has_name( dch, arg ))
            continue;

        if ( ++count >= number )
                return wch;
    }

    return 0;
}



PCharacter * get_player_world( PCharacter *ch, const char *arg )
{
    Descriptor *d;

    for (d = descriptor_list; d != 0; d = d->next) {
        PCharacter *victim;
        
        if (d->connected != CON_PLAYING)
            continue;
        
        if(!d->character)
            continue;

        victim = d->character->getPC( );

        if(victim->switchedTo)
            continue;

        if (!ch->can_see( victim ))
            continue;

        if (!char_has_name( victim, arg ))
            continue;
        
        return victim;
    }

    return NULL;
}

Object * get_obj_world_unique( int vnum, Character *ch )
{
    Object *obj;
    
    for (obj = object_list; obj; obj = obj->next) 
        if (obj->pIndexData->vnum == vnum && obj->hasOwner( ch ))
            break;

    return obj;
}

std::list<Object *> get_objlist_world_unique( int vnum, Character *ch )
{
    Object *obj;
    std::list<Object *> list;
    
    for (obj = object_list; obj; obj = obj->next) 
        if (obj->pIndexData->vnum == vnum && obj->hasOwner( ch ))
            list.push_back( obj );

    return list;
}

Object * get_obj_room_unique( Room *room, int itype, Character *ch )
{
    for (Object *obj = room->contents; obj; obj = obj->next_content)
        if (obj->item_type == itype && obj->hasOwner( ch ))
            return obj;

    return NULL;
}

int count_obj_in_obj( Object *container )
{
    Object *objc;
    int count;

    for (count = 0, objc = container->contains; objc; objc = objc->next_content, count++)
        ;

    return count;
}

int count_obj_in_obj( Object *container, int itype )
{
    Object *objc;
    int count;

    for (count = 0, objc = container->contains; objc; objc = objc->next_content)
        if (objc->item_type == itype)
            count++;

    return count;
}


Object * get_obj_wear_carry( Character *ch, const DLString &cArgument )
{
    char arg[MAX_INPUT_LENGTH], argument[MAX_INPUT_LENGTH];
    Object *obj;
    int count = 0;
    int number;
    long long id = get_arg_id( cArgument );

    strcpy( argument, cArgument.c_str( ) );
    number = number_argument( argument, arg );

    for (obj = ch->carrying; obj != 0; obj = obj->next_content)
        if (obj->wear_loc != wear_none
            && ((id && obj->getID( ) == id) || (!id && obj_has_name( obj, arg, ch ))) )
        {
            if (id || ++count == number)
                return obj;
        }

    for (obj = ch->carrying; obj != 0; obj = obj->next_content)
        if (obj->wear_loc == wear_none
                && (ch->can_see( obj ) || ch->can_hear( obj ))
                && ((id && obj->getID( ) == id) || (!id && obj_has_name( obj, arg, ch ))) )
        {
            if (id || ++count == number)
                return obj;
        }

    return 0;
}

// return true if character is allowed to see object name hint in brackets
bool can_see_objname_hint( Character *ch, Object *obj )
{
    if (ch->is_immortal( ))
       return true;
    
    // See it in your eq and inv.
    if (obj->getCarrier( ) == ch)
       return true;

    // Can't see in other's inventory or eq.
    // Items on the floor should have a hint in long descr.
    if (!obj->in_obj)
       return false;

    // Take only from your own corpse.
    if (obj->in_obj->item_type == ITEM_CORPSE_PC)
       return obj->in_obj->hasOwner( ch );    

    // Other containers without owners - OK
    if (!obj->in_obj->getOwner( ))
        return true;

    // Hero bags and hunter armor - only for the owner.
    if (obj->in_obj->behavior && obj->in_obj->behavior->canLock( ch ))
        return true;

    return false;
}

// return true if character can access object by the name
bool obj_has_name( Object *obj, const DLString &arg, Character *ch )
{
    // If object is carried by or in a non-personal container:
    if (can_see_objname_hint( ch, obj )) {
        // First try matching by obj names.
        if (is_name( arg.c_str( ), obj->getName( ) ))
            return true;

        // Then try to match by short descr, all grammatical cases, no colours.
        if (is_name( arg.c_str( ), obj->getShortDescr( '7' ).colourStrip( ).c_str( ) ))
            return true;
        
        // No match.
        return false;
    }

    // For items inside PC corpses and the like: match only by names.
    return is_name( arg.c_str( ), obj->getName( ) );
}

// Return true if arg matches one of the character names or short descriptions.
// Doppel is already applied.
bool char_has_name(Character *target, const char *arg)
{
    return is_name(arg, target->getNameP('7').c_str());
}

bool obj_has_name_or_id( Object *obj, const DLString &arg, Character *ch, long long id )
{    
    if (id && obj->getID( ) == id) 
        return true;

    if (!id && obj_has_name( obj, arg, ch ))
        return true;

    return false;
}                             

Profession * find_prof_unstrict( const DLString &className) 
{    
    for (int i = 0; i < professionManager->size( ); i++) {
        Profession *p = professionManager->find( i );
        if (!p->isValid( ) || !p->isPlayed( )) 
            continue;

        if (className.strPrefix( p->getName( ) )) {
            return p;
        }

        if (className.strPrefix( p->getRusName( ).ruscase( '1' ) )
                    || className.strPrefix( p->getRusName( ).ruscase( '5' )))
            return p;
    }
    return NULL;
}

