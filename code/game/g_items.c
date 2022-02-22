/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2018 borg

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "g_local.h"

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/

// Don't use these. Take values from g_respawn_* cvars
// #define  	RESPAWN_ARMOR           25
// #define      RESPAWN_HEALTH          35
// #define      RESPAWN_AMMO            40
// #define      RESPAWN_HOLDABLE        60
// #define      RESPAWN_MEGAHEALTH      35//120
// #define      RESPAWN_POWERUP         120


//======================================================================

int Pickup_Powerup( gentity_t *ent, gentity_t *other ) {
	int			quantity;
	int			i;
	gclient_t	*client;

	if ( !other->client->ps.powerups[ent->item->giTag] ) {
		// round timing to seconds to make multiple powerup timers
		// count in sync
		other->client->ps.powerups[ent->item->giTag] = 
			level.time - ( level.time % 1000 );
			
		if( ent->item->giTag == PW_QUAD ){
			other->client->quadKills = 0;
			other->client->stats[STATS_QUAD]++;
			trap_SendServerCommand( other-g_entities, va( "quadKill %i", other->client->quadKills ) );
		}
	
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	if (ent->powerupTimeLeft) {
		// If a powerup is picked up after it was dropped we use the time left.
		// We also remove 1 second from the time left as penalty.
		quantity = (ent->powerupTimeLeft - 1);
		if (quantity < 0) {
			quantity = 0;
		}
	}

	other->client->ps.powerups[ent->item->giTag] += quantity * 1000;

	// give any nearby players a "denied" anti-reward
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		vec3_t		delta;
		float		len;
		vec3_t		forward;
		trace_t		tr;

		client = &level.clients[i];
		if ( client == other->client ) {
			continue;
		}
		if ( client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
			continue;
		}

    // if same team in team game, no sound
    // cannot use OnSameTeam as it expects to g_entities, not clients
  	if ( g_gametype.integer >= GT_TEAM && g_ffa_gt==0 && other->client->sess.sessionTeam == client->sess.sessionTeam  ) {
      continue;
    }

		// if too far away, no sound
		VectorSubtract( ent->s.pos.trBase, client->ps.origin, delta );
		len = VectorNormalize( delta );
		if ( len > 192 ) {
			continue;
		}

		// if not facing, no sound
		AngleVectors( client->ps.viewangles, forward, NULL, NULL );
		if ( DotProduct( delta, forward ) < 0.4 ) {
			continue;
		}

		// if not line of sight, no sound
		trap_Trace( &tr, client->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID );
		if ( tr.fraction != 1.0 ) {
			continue;
		}

		// anti-reward
		client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD;
	}
	
	//TODO: Follow powerup - autoaction & 32
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR || level.clients[i].ps.pm_type == PM_SPECTATOR )
			&& level.clients[i].pers.autoaction & 32 ) {
		  
			level.clients[i].sess.spectatorClient = other->s.clientNum;
			level.clients[i].sess.spectatorState = SPECTATOR_FOLLOW;
		}
	}
	
	return g_respawn_powerup.integer;
}

//======================================================================

int Pickup_PersistantPowerup( gentity_t *ent, gentity_t *other ) {
	int		clientNum;
	char	userinfo[MAX_INFO_STRING];
	float	handicap;
	int		max;

	other->client->ps.stats[STAT_PERSISTANT_POWERUP] = ent->item - bg_itemlist;
	other->client->persistantPowerup = ent;

	switch( ent->item->giTag ) {
	case PW_GUARD:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
		handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
		if( handicap<=0.0f || handicap>100.0f) {
			handicap = 100.0f;
		}
		max = (int)(2 *  handicap);

		other->health = max;
		other->client->ps.stats[STAT_HEALTH] = max;
		other->client->ps.stats[STAT_MAX_HEALTH] = max;
		other->client->ps.stats[STAT_ARMOR] = max;
		other->client->pers.maxHealth = max;

		break;

	case PW_SCOUT:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
		handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
		if( handicap<=0.0f || handicap>100.0f) {
			handicap = 100.0f;
		}
		other->client->pers.maxHealth = handicap;
		other->client->ps.stats[STAT_ARMOR] = 0;
		break;

	case PW_DOUBLER:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
		handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
		if( handicap<=0.0f || handicap>100.0f) {
			handicap = 100.0f;
		}
		other->client->pers.maxHealth = handicap;
		break;
	case PW_AMMOREGEN:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
		handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
		if( handicap<=0.0f || handicap>100.0f) {
			handicap = 100.0f;
		}
		other->client->pers.maxHealth = handicap;
		memset(other->client->ammoTimes, 0, sizeof(other->client->ammoTimes));
		break;
	default:
		clientNum = other->client->ps.clientNum;
		trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
		handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
		if( handicap<=0.0f || handicap>100.0f) {
			handicap = 100.0f;
		}
		other->client->pers.maxHealth = handicap;
		break;
	}

	return -1;
}

//======================================================================

int Pickup_Holdable( gentity_t *ent, gentity_t *other ) {

	other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;

	if( ent->item->giTag == HI_KAMIKAZE ) {
		other->client->ps.eFlags |= EF_KAMIKAZE;
	}

	return g_respawn_holdable.integer;
}


//======================================================================

void Add_Ammo (gentity_t *ent, int weapon, int count)
{
	if ( level.warmupTime == -1 && ent->practice == qfalse ) {
		ent->client->ps.ammo[weapon] = 999;
	} else {
		ent->client->ps.ammo[weapon] += count;
		if ( ent->client->ps.ammo[weapon] > 200 ) {
			ent->client->ps.ammo[weapon] = 200;
		}
	}
}

int Pickup_Ammo (gentity_t *ent, gentity_t *other)
{
	int		quantity;

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	Add_Ammo (other, ent->item->giTag, quantity);

	return g_respawn_ammo.integer;
}

//======================================================================


int Pickup_Weapon (gentity_t *ent, gentity_t *other) {
	int		quantity;
	
	if( ent->dropTime )
		quantity = ent->ammoCount;
	else 
	if ( ent->count < 0 ) {
		quantity = 0; // None for you, sir!
	} else {
		if ( ent->count ) {
			quantity = ent->count;
		} else {
			quantity = ent->item->quantity;
		}

    // dropped items and teamplay weapons always have full ammo
    if ( ! (ent->flags & FL_DROPPED_ITEM) && g_gametype.integer != GT_TEAM ) {
      if ( g_legacyWeaponAmmo.integer == 1 ) { 
        // respawning rules
        // drop the quantity if the already have over the minimum
        if ( other->client->ps.ammo[ ent->item->giTag ] < quantity ) {
          quantity = quantity - other->client->ps.ammo[ ent->item->giTag ];
        } else {
          quantity = 1;		// only add a single shot
        }
      }
    }
	}
	
	//G_Printf("%i \n", quantity );

	// add the weapon
	other->client->ps.stats[STAT_WEAPONS] |= ( 1 << ent->item->giTag );

	Add_Ammo( other, ent->item->giTag, quantity );

	if (ent->item->giTag == WP_GRAPPLING_HOOK)
		other->client->ps.ammo[ent->item->giTag] = -1; // unlimited ammo

	// team deathmatch has slow weapon respawns
	if ( g_gametype.integer == GT_TEAM ) {
		return g_weaponTeamRespawn.integer;
	}

	return g_weaponRespawn.integer;
}


//======================================================================

int Pickup_Health (gentity_t *ent, gentity_t *other) {
	int			max;
	int			quantity;
	
	other->client->stats[STATS_HEALTH] += ent->item->quantity;
	
	if( ent->item->quantity == 100 )
		other->client->stats[STATS_MH]++;

	// small and mega healths will go over the max
	if( other->client && bg_itemlist[other->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	}
	else
	if ( ent->item->quantity != 5 && ent->item->quantity != 100 ) {
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	} else {
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->health += quantity;

	if (other->health > max ) {
		other->health = max;
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if ( ent->item->quantity == 100 ) {		// mega health respawns slow
		return g_respawn_megahealth.integer;
	}
	
	// return g_respawn_health.integer;
	return g_respawn_health.integer;
}

//======================================================================

int Pickup_Armor( gentity_t *ent, gentity_t *other ) {
	int		upperBound;
	
	other->client->stats[STATS_ARMOR] += ent->item->quantity;
	
	if( ent->item->quantity == 50 )
		other->client->stats[STATS_YA]++;
	else if( ent->item->quantity == 100 )
		other->client->stats[STATS_RA]++;

	other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;

	if( other->client && bg_itemlist[other->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
		upperBound = other->client->ps.stats[STAT_MAX_HEALTH];
	}
	else {
		upperBound = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if ( other->client->ps.stats[STAT_ARMOR] > upperBound ) {
		other->client->ps.stats[STAT_ARMOR] = upperBound;
	}

	return g_respawn_armor.integer;
}

//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent ) {
        //Don't spawn quad if quadfactor are 1.0 or less
        if(ent->item->giType == IT_POWERUP && ent->item->giTag == PW_QUAD && g_quadfactor.value <= 1.0)
            return;

	// randomly select from teamed entities
	if (ent->team) {
		gentity_t	*master;
		int	count;
		int choice;
		qboolean is_powerup = ent->item->giType == IT_POWERUP;
		qboolean choice_is_powerup = is_powerup;

		if ( !ent->teammaster ) {
			G_Error( "RespawnItem: bad teammaster");
		}
		master = ent->teammaster;

		// Find size of teamchain
		ent = master;
		for(count = 0; ent; count++) {
			ent = ent->teamchain;
		}

		// Pick random entity of teamchain
		//
		// Here we make sure that if it is a powerup teamchain that the picked
		// ent is also a powerup. On some maps there are non-powerups in the
		// teamchain (for example ps37ctf-mmp). In this case we can't use the
		// non-powerup teamchain item.
		do {
			choice = rand() % count;

			ent = master;
			for(count = 0; count < choice; count++) {
				ent = ent->teamchain;
			}

			if(is_powerup) {
				choice_is_powerup = ent->item->giType == IT_POWERUP;
			}
		} while(is_powerup && !choice_is_powerup);
	}

	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity (ent);

	if ( ent->item->giType == IT_POWERUP ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if (ent->speed) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		}
		else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/poweruprespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	if ( ent->item->giType == IT_HOLDABLE && ent->item->giTag == HI_KAMIKAZE ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if (ent->speed) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		}
		else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/kamikazerespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;
}


/*
===============
Touch_Item
===============
*/
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace) {
	int			respawn;
	qboolean	predict;

	//instant gib
	if ((g_instantgib.integer || g_rockets.integer || g_gametype.integer == GT_CTF_ELIMINATION || g_elimination_allgametypes.integer)
                && ent->item->giType != IT_TEAM)
		return;

	//Cannot touch flag before round starts
	if(g_gametype.integer == GT_CTF_ELIMINATION && level.roundNumber != level.roundNumberStarted)
		return;

	//Cannot take ctf elimination oneway
	if(g_gametype.integer == GT_CTF_ELIMINATION && g_elimination_ctf_oneway.integer!=0 && (
			(other->client->sess.sessionTeam==TEAM_BLUE && (level.eliminationSides+level.roundNumber)%2 == 0 ) ||
			(other->client->sess.sessionTeam==TEAM_RED && (level.eliminationSides+level.roundNumber)%2 != 0 ) ))
		return;

	if (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_LMS)
		return;		//nothing to pick up in elimination

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup

	// the same pickup rules are used for client side and server side
	if ( !BG_CanItemBeGrabbed( g_gametype.integer, &ent->s, &other->client->ps ) ) {
		return;
	}

	//In double DD we cannot "pick up" a flag we already got
	if(g_gametype.integer == GT_DOUBLE_D) {
		if( strcmp(ent->classname, "team_CTF_redflag") == 0 )
			if(other->client->sess.sessionTeam == level.pointStatusA)
				return;
		if( strcmp(ent->classname, "team_CTF_blueflag") == 0 )
			if(other->client->sess.sessionTeam == level.pointStatusB)
				return;
	}
	
	/*G_Printf( "%i   vs   %i\n", ent->item->lastDrop, level.time);
	*/
	if( /*ent->item->giType == IT_WEAPON &&*/ ent->dropTime + 500 > level.time ){
		//G_Printf( "%i   vs   %i\n", ent->dropTime, level.time);
		return;
	}
	else if( ent->item->lastDrop + 500 > level.time )
		return;
	

	G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );

	predict = other->client->pers.predictItemPickup;

	// call the item-specific pickup function
	switch( ent->item->giType ) {
	case IT_WEAPON:
		respawn = Pickup_Weapon(ent, other);
//		predict = qfalse;
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
//		predict = qfalse;
		break;
	case IT_ARMOR:
		respawn = Pickup_Armor(ent, other);
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup(ent, other);
		predict = qfalse;
		break;
	case IT_PERSISTANT_POWERUP:
		respawn = Pickup_PersistantPowerup(ent, other);
		break;
	case IT_TEAM:
		respawn = Pickup_Team(ent, other);
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	default:
		return;
	}
	
	other->client->lastPickup = ent->item->shortPickup_name;

	if ( !respawn ) {
		return;
	}

	// play the normal pickup sound
	if (predict) {
		G_AddPredictableEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
	} else {
		G_AddEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
	}

	// powerup pickups are global broadcasts
	if ( ent->item->giType == IT_POWERUP || ent->item->giType == IT_TEAM) {
		// if we want the global sound to play
		if (!ent->speed) {
			gentity_t	*te;

			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
			te->s.eventParm = ent->s.modelindex;
			te->r.svFlags |= SVF_BROADCAST;
		} else {
			gentity_t	*te;

			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
			te->s.eventParm = ent->s.modelindex;
			// only send this temp entity to a single client
			te->r.svFlags |= SVF_SINGLECLIENT;
			te->r.singleClient = other->s.number;
		}
	}

	// fire item targets
	G_UseTargets (ent, other);

	// wait of -1 will not respawn
	if ( ent->wait == -1 ) {
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	// non zero wait overrides respawn time
	if ( ent->wait ) {
		respawn = ent->wait;
	}

	// random can be used to vary the respawn time
	if ( ent->random ) {
		respawn += crandom() * ent->random;
		if ( respawn < 1 ) {
			respawn = 1;
		}
	}

	// dropped items will not respawn
	if ( ent->flags & FL_DROPPED_ITEM ) {
		ent->freeAfterEvent = qtrue;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;

	// ZOID
	// A negative respawn times means to never respawn this item (but don't 
	// delete it).  This is used by items that are respawned by third party 
	// events such as ctf flags
	if ( respawn <= 0 ) {
		ent->nextthink = 0;
		ent->think = 0;
	} else {
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}
	trap_LinkEntity( ent );

	if( g_allowRespawnTimer.integer && !(ent->flags & FL_DROPPED_ITEM) )
		G_SendRespawnTimer( ent->s.number, ent->item->giType, ent->item->quantity, level.time + respawn * 1000, G_FindNearestItemSpawn( ent ), other->s.clientNum);
}


//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t	*dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	if ((g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_DOUBLE_D)			&& item->giType == IT_TEAM) { // Special case for CTF flags
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;
		Team_CheckDroppedItem( dropped );
	} else { // auto-remove after 30 seconds
		dropped->think = G_FreeEntity;
		dropped->nextthink = level.time + 30000;
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity (dropped);

	return dropped;
}

gentity_t *LaunchItemPowerup( gitem_t *item, vec3_t origin, vec3_t velocity, int powerupTimeLeft ) {
	gentity_t	*dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	dropped->think = G_FreeEntity;
	dropped->nextthink = level.time + 30000;

	dropped->flags = FL_DROPPED_ITEM;

	dropped->powerupTimeLeft = powerupTimeLeft;

	trap_LinkEntity (dropped);

	return dropped;
}

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItemWeapon( gitem_t *item, vec3_t origin, vec3_t velocity, int ammoCount, int dropTime ) {
	gentity_t	*dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	
	dropped->think = G_FreeEntity;
	dropped->nextthink = level.time + 30000;

	dropped->flags = FL_DROPPED_ITEM;
	
	dropped->dropTime = dropTime;
	dropped->ammoCount = ammoCount;

	trap_LinkEntity(dropped);

	return dropped;
}

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItemTime( gitem_t *item, vec3_t origin, vec3_t velocity, int dropTime ) {
	gentity_t	*dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	
	dropped->think = G_FreeEntity;
	dropped->nextthink = level.time + 30000;

	dropped->flags = FL_DROPPED_ITEM;
	
	dropped->dropTime = dropTime;

	trap_LinkEntity(dropped);

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;
	
	return LaunchItem( item, ent->s.pos.trBase, velocity );
}

gentity_t *Drop_Item_Powerup( gentity_t *ent, gitem_t *item, float angle, int time ) {
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t position;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 0, velocity );
	VectorAdd( velocity, ent->s.pos.trBase, position );

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;

	item->lastDrop = level.time;

	ent->client->lastDrop = item->shortPickup_name;

	return LaunchItemPowerup( item, position, velocity, time );
}

gentity_t *Drop_Item_Armor( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t position;
	int	dropTime;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 0, velocity );
	VectorAdd( velocity, ent->s.pos.trBase, position );
	
	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;
	
	if( ent->client->ps.stats[STAT_ARMOR] < item->quantity )
	    return NULL;
	
	ent->client->ps.stats[STAT_ARMOR] -= item->quantity;
	
	dropTime = level.time;
	
	ent->client->lastDrop = item->shortPickup_name;
	
	return LaunchItemTime( item, position, velocity, dropTime );
}

gentity_t *Drop_Item_Health( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t position;
	int	dropTime;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 0, velocity );
	VectorAdd( velocity, ent->s.pos.trBase, position );
	
	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;
	
	if( ent->client->ps.stats[STAT_HEALTH] <= item->quantity || ent->health <= item->quantity)
	    return NULL;
	
	ent->client->ps.stats[STAT_HEALTH] -= item->quantity;
	ent->health -= item->quantity;
	
	dropTime = level.time;
	
	ent->client->lastDrop = item->shortPickup_name;
	
	return LaunchItemTime( item, position, velocity, dropTime );
}

gentity_t *Drop_Item_Ammo( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t position;
	int	dropTime;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 0, velocity );
	VectorAdd( velocity, ent->s.pos.trBase, position );
	
	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;
	
	if( ent->client->ps.ammo[item->giTag] < item->quantity )
	    return NULL;
	
	ent->client->ps.ammo[item->giTag/*ent->s.weapon*/] -= item->quantity;
	
	dropTime = level.time;
	
	ent->client->lastDrop = item->shortPickup_name;
	
	return LaunchItemTime( item, position, velocity, dropTime );
}

//TODO: Switch to next weapon

gentity_t *Drop_Item_Weapon( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t position;
	int 	ammoCount;
	int	dropTime;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 0, velocity );
	VectorAdd( velocity, ent->s.pos.trBase, position );
	
	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;
	
	ammoCount = ent->client->ps.ammo[item->giTag /*ent->s.weapon*/];
	
	ent->client->ps.ammo[item->giTag/*ent->s.weapon*/] = 0;
	ent->client->ps.stats[STAT_WEAPONS] &= ~( 1 << item->giTag );
	
	dropTime = level.time;
	
	ent->client->lastDrop = item->shortPickup_name;
	
	return LaunchItemWeapon( item, position, velocity, ammoCount, dropTime );
}

gentity_t *Drop_Item_Flag( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t position;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 0, velocity );
	VectorAdd( velocity, ent->s.pos.trBase, position );
	
	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;
	
	item->lastDrop = level.time;
	
	ent->client->lastDrop = item->shortPickup_name;
	
	
	return LaunchItem( item, position, velocity );
}


/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	RespawnItem( ent );
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		dest;

	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;		// store item number in modelindex
	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	// useing an item causes it to respawn
	ent->use = Use_Item;

	if ( ent->spawnflags & 1 ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			G_Printf ("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	
	// powerups don't spawn in for a while (but not in elimination)
	if(g_gametype.integer != GT_ELIMINATION && g_gametype.integer != GT_CTF_ELIMINATION && g_gametype.integer != GT_LMS 
                && !g_instantgib.integer && !g_elimination_allgametypes.integer && !g_rockets.integer )
	if ( ent->item->giType == IT_POWERUP ) {
		float	respawn;

		respawn = 45 + crandom() * 15;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
		return;
	}

	trap_LinkEntity (ent);
	
	if( g_allowRespawnTimer.integer && !(ent->flags & FL_DROPPED_ITEM) )
		G_SendRespawnTimer( ent->s.number, ent->item->giType, ent->item->quantity, ent->nextthink , G_FindNearestItemSpawn( ent ), -1 );
}


qboolean	itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems( void ) {

	// Set up team stuff
	Team_InitGame();

	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_DOUBLE_D) {
		gitem_t	*item;

		// check for the two flags
		item = BG_FindItem( "Red Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map" );
		}
		item = BG_FindItem( "Blue Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map" );
		}
	}
	if( g_gametype.integer == GT_1FCTF ) {
		gitem_t	*item;

		// check for all three flags
		item = BG_FindItem( "Red Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map" );
		}
		item = BG_FindItem( "Blue Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map" );
		}
		item = BG_FindItem( "Neutral Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_neutralflag in map" );
		}
	}

	if( g_gametype.integer == GT_OBELISK ) {
		gentity_t	*ent;

		// check for the two obelisks
		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_redobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_redobelisk in map" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_blueobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_blueobelisk in map" );
		}
	}

	if( g_gametype.integer == GT_HARVESTER ) {
		gentity_t	*ent;

		// check for all three obelisks
		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_redobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_redobelisk in map" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_blueobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_blueobelisk in map" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_neutralobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_neutralobelisk in map" );
		}
	}
}

/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems( void ) {
	memset( itemRegistered, 0, sizeof( itemRegistered ) );

	if(g_instantgib.integer) {
		//Always load Gauntlet and machine gun because g_instantgib might suddenly change
		RegisterItem( BG_FindItemForWeapon( WP_GAUNTLET ) );
		RegisterItem( BG_FindItemForWeapon( WP_MACHINEGUN ) );
		RegisterItem( BG_FindItemForWeapon( WP_RAILGUN ) );
	}
	if(g_rockets.integer) {
		RegisterItem( BG_FindItemForWeapon( WP_GAUNTLET ) );
		RegisterItem( BG_FindItemForWeapon( WP_MACHINEGUN ) );
		RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ) );
	}
	else
	{
		// players always start with the base weapon
		RegisterItem( BG_FindItemForWeapon( WP_MACHINEGUN ) );
		RegisterItem( BG_FindItemForWeapon( WP_GAUNTLET ) );
		if(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION 
                        || g_gametype.integer == GT_LMS || g_elimination_allgametypes.integer)
		{
			RegisterItem( BG_FindItemForWeapon( WP_SHOTGUN ) );
			RegisterItem( BG_FindItemForWeapon( WP_GRENADE_LAUNCHER ) );
			RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ) );
			RegisterItem( BG_FindItemForWeapon( WP_LIGHTNING ) );
			RegisterItem( BG_FindItemForWeapon( WP_RAILGUN ) );
			RegisterItem( BG_FindItemForWeapon( WP_PLASMAGUN ) );
//			RegisterItem( BG_FindItemForWeapon( WP_BFG ) );
//			RegisterItem( BG_FindItemForWeapon( WP_NAILGUN ) );
//			RegisterItem( BG_FindItemForWeapon( WP_PROX_LAUNCHER ) );
//			RegisterItem( BG_FindItemForWeapon( WP_CHAINGUN ) );
		}
	}
	if( g_gametype.integer == GT_HARVESTER ) {
		RegisterItem( BG_FindItem( "Red Cube" ) );
		RegisterItem( BG_FindItem( "Blue Cube" ) );
	}
        
	if(g_gametype.integer == GT_DOUBLE_D ) {
		RegisterItem( BG_FindItem( "Point A (Blue)" ) );
		RegisterItem( BG_FindItem( "Point A (Red)" ) );
		RegisterItem( BG_FindItem( "Point A (White)" ) );
		RegisterItem( BG_FindItem( "Point B (Blue)" ) );
		RegisterItem( BG_FindItem( "Point B (Red)" ) );
		RegisterItem( BG_FindItem( "Point B (White)" ) );
	}

	if(g_gametype.integer == GT_DOMINATION ) {
		RegisterItem( BG_FindItem( "Neutral domination point" ) );
		RegisterItem( BG_FindItem( "Red domination point" ) );
		RegisterItem( BG_FindItem( "Blue domination point" ) );
	}
	
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Error( "RegisterItem: NULL" );
	}
#ifndef MISSIONPACK
	if ( item->giType == IT_WEAPON && ( item->giTag == WP_NAILGUN || item->giTag == WP_CHAINGUN || item->giTag == WP_PROX_LAUNCHER ) )
		return;
#endif
	itemRegistered[ item - bg_itemlist ] = qtrue;
}


/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
	char	string[MAX_ITEMS+1];
	int		i;
	int		count;

	count = 0;
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( itemRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ bg_numItems ] = 0;
        G_Printf( "%i items registered\n", count );
	trap_SetConfigstring(CS_ITEMS, string);
}

qboolean G_WeaponRegistered( int weapon ){
	return itemRegistered[ITEM_INDEX(BG_FindItemForWeapon( weapon ))];
}

/*
============
G_ItemDisabled
============
*/
int G_ItemDisabled( gitem_t *item ) {

	char name[128];

	Com_sprintf(name, sizeof(name), "disable_%s", item->classname);
	return trap_Cvar_VariableIntegerValue( name );
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem (gentity_t *ent, gitem_t *item) {
	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	//Load all items in instantgib anyway or we will be in trouble if it is suddenly disabled!
	//if((item->giType == IT_TEAM && g_instantgib.integer) || !g_instantgib.integer)
	{
		//Don't load pickups in Elimination (or maybe... gives warnings)
		if (g_gametype.integer != GT_ELIMINATION /*&& g_gametype.integer != GT_CTF_ELIMINATION */&& g_gametype.integer != GT_LMS)
			RegisterItem( item );
		//Registrer flags anyway in CTF Elimination:
		if (g_gametype.integer == GT_CTF_ELIMINATION && item->giType == IT_TEAM){
			/*if( strcmp(ent->classname, "team_CTF_blueflag") && (level.eliminationSides+level.roundNumber)%2 != 0 )
				item = BG_FindItem("Neutral Flag");
			else if( strcmp(ent->classname, "team_CTF_redflag") && (level.eliminationSides+level.roundNumber)%2 == 0 ) 
				item = BG_FindItem("Neutral Flag");
			*/	
			RegisterItem( item );
		}
		if ( G_ItemDisabled(item) )
			return;
	}
        if(!g_persistantpowerups.integer && item->giType == IT_PERSISTANT_POWERUP)
            return;

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;		// items are bouncy

	if (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_LMS || 
			( item->giType != IT_TEAM && (g_instantgib.integer || g_rockets.integer || g_elimination_allgametypes.integer || g_gametype.integer==GT_CTF_ELIMINATION) ) ) {
		ent->s.eFlags |= EF_NODRAW; //Invisible in elimination
                ent->r.svFlags |= SVF_NOCLIENT;  //Don't broadcast
        }

	if(g_gametype.integer == GT_DOUBLE_D && (strcmp(ent->classname, "team_CTF_redflag")==0 || strcmp(ent->classname, "team_CTF_blueflag")==0 || strcmp(ent->classname, "team_CTF_neutralflag") == 0 || item->giType == IT_PERSISTANT_POWERUP  ))
		ent->s.eFlags |= EF_NODRAW; //Don't draw the flag models/persistant powerups

	if((g_gametype.integer != GT_1FCTF) && strcmp(ent->classname, "team_CTF_neutralflag") == 0)
		ent->s.eFlags |= EF_NODRAW; // Don't draw the neutralflag except in 1 flag ctf 

        if(strcmp(ent->classname, "domination_point") == 0)
                ent->s.eFlags |= EF_NODRAW; // Don't draw domination_point. It is just a pointer to where the Domination points should be placed
	if ( item->giType == IT_POWERUP ) {
		G_SoundIndex( "sound/items/poweruprespawn.wav" );
		G_SpawnFloat( "noglobalsound", "0", &ent->speed);
	}

	if ( item->giType == IT_PERSISTANT_POWERUP ) {
		ent->s.generic1 = ent->spawnflags;
	}
	
}


/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	// cut the velocity to keep from bouncing forever
	VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

	// check for stop
	if ( trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 ) {
		trace->endpos[2] += 1.0;	// make sure it is off ground
		SnapVector( trace->endpos );
		G_SetOrigin( ent, trace->endpos );
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			contents;
	int			mask;

	// if groundentity has been set to -1, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == -1 ) {
		if ( ent->s.pos.trType != TR_GRAVITY ) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY ) {
		// check think function
		G_RunThink( ent );
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position
	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;//MASK_SOLID;
	}
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, 
		ent->r.ownerNum, mask );

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	trap_LinkEntity( ent );	// FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 ) {
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents( ent->r.currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		if (ent->item && ent->item->giType == IT_TEAM) {
			Team_FreeEntity(ent);
		} else {
			G_FreeEntity( ent );
		}
		return;
	}

	G_BounceItem( ent, &tr );
}

int G_FindNearestItem( gentity_t *ent ){
	int mindist = -1;
	int dist;
	int count;
	int minnumber = -1;
	
	for( count = 0; count < MAX_GENTITIES; count++ ){
	  
		if( !g_entities[count].inuse || g_entities[count].s.eType != ET_ITEM )
			continue;
		
		if( ( g_entities[count].item->giType == IT_ARMOR && g_entities[count].item->quantity >= 50 ) || ( g_entities[count].item->giType == IT_HEALTH && g_entities[count].item->quantity >= 100 )
		  || g_entities[count].item->giType == IT_WEAPON || g_entities[count].item->giType == IT_POWERUP || g_entities[count].item->giType == IT_PERSISTANT_POWERUP || g_entities[count].item->giType == IT_HOLDABLE 
		  || g_entities[count].item->giType == IT_TEAM ){
			dist = ( ent->r.currentOrigin[0] - g_entities[count].s.origin[0] ) * ( ent->r.currentOrigin[0] - g_entities[count].s.origin[0] )
			     + ( ent->r.currentOrigin[1] - g_entities[count].s.origin[1] ) * ( ent->r.currentOrigin[1] - g_entities[count].s.origin[1] )
			     + ( ent->r.currentOrigin[2] - g_entities[count].s.origin[2] ) * ( ent->r.currentOrigin[2] - g_entities[count].s.origin[2] );
			if( dist < mindist || mindist == -1 ){
				mindist = dist;
				minnumber = g_entities[count].s.number;
			}
		}
	}
	return minnumber;
}

int G_FindNearestItemSpawn( gentity_t *ent ){
	int mindist = -1;
	int dist;
	int count;
	int minnumber = -1;
	
	for( count = 0; count < MAX_GENTITIES; count++ ){
	  
		if( !g_entities[count].inuse || g_entities[count].s.eType != ET_ITEM || g_entities[count].flags == FL_DROPPED_ITEM )
			continue;
		
		if( ent->s.number == g_entities[count].s.number )
			continue;
		
		if( ( g_entities[count].item->giType == IT_ARMOR && g_entities[count].item->quantity >= 50 ) || ( g_entities[count].item->giType == IT_HEALTH && g_entities[count].item->quantity >= 100 )
		  || g_entities[count].item->giType == IT_WEAPON || g_entities[count].item->giType == IT_POWERUP || g_entities[count].item->giType == IT_PERSISTANT_POWERUP || g_entities[count].item->giType == IT_HOLDABLE 
		  || g_entities[count].item->giType == IT_TEAM ){
			dist = ( ent->r.currentOrigin[0] - g_entities[count].r.currentOrigin[0] ) * ( ent->r.currentOrigin[0] - g_entities[count].r.currentOrigin[0] )
			     + ( ent->r.currentOrigin[1] - g_entities[count].r.currentOrigin[1] ) * ( ent->r.currentOrigin[1] - g_entities[count].r.currentOrigin[1] )
			     + ( ent->r.currentOrigin[2] - g_entities[count].r.currentOrigin[2] ) * ( ent->r.currentOrigin[2] - g_entities[count].r.currentOrigin[2] );
			if( dist < mindist || mindist == -1 ){
				mindist = dist;
				minnumber = g_entities[count].s.number;
			}
		}
	}
	return minnumber;
}

/*
================
G_ItemTeam
Returns on wich side the item is
(-1 when mid or game is not ctf)
================
*/

int G_ItemTeam( int entityNum ){
	gentity_t *ent;
	int blue = -1;
	int red = -1;
	int i;
	float distRed, distBlue;
	
	if( g_gametype.integer != GT_CTF )
		return -1;
	
	ent = &g_entities[ entityNum ];
	
	if( ent->s.eType != ET_ITEM )
		return -1;
	
	for( i = 0; i < MAX_GENTITIES; i++ ){
		if( !g_entities[ i ].inuse )
			continue;
		if( g_entities[i].s.eType != ET_ITEM )
			continue;
		if( g_entities[i].flags == FL_DROPPED_ITEM )
			continue;
		if( g_entities[i].item->giType != IT_TEAM )
			continue;
		if( g_entities[i].item->giTag == PW_REDFLAG /*&& !red*/ ){
			//G_Printf("Found RED %i\n", i);
			red =  i;
		}
		if( g_entities[i].item->giTag == PW_BLUEFLAG /*&& !blue*/ ){
			//G_Printf("Found BLUE %i\n", i);
			blue = i;
		}
	}
	
	if( ( red == -1 ) || ( blue == -1 ) )
		return -1;
	
	distRed = /*sqrt*/( ent->r.currentOrigin[0] - g_entities[red].r.currentOrigin[0] ) * ( ent->r.currentOrigin[0] - g_entities[red].r.currentOrigin[0] )
		+ ( ent->r.currentOrigin[1] - g_entities[red].r.currentOrigin[1] ) * ( ent->r.currentOrigin[1] - g_entities[red].r.currentOrigin[1] )
		+ ( ent->r.currentOrigin[2] - g_entities[red].r.currentOrigin[2] ) * ( ent->r.currentOrigin[2] - g_entities[red].r.currentOrigin[2] );
		
	distBlue = /*sqrt*/( ent->r.currentOrigin[0] - g_entities[blue].r.currentOrigin[0] ) * ( ent->r.currentOrigin[0] - g_entities[blue].r.currentOrigin[0] )
		 + ( ent->r.currentOrigin[1] - g_entities[blue].r.currentOrigin[1] ) * ( ent->r.currentOrigin[1] - g_entities[blue].r.currentOrigin[1] )
		 + ( ent->r.currentOrigin[2] - g_entities[blue].r.currentOrigin[2] ) * ( ent->r.currentOrigin[2] - g_entities[blue].r.currentOrigin[2] );
		 
	//G_Printf("RED: %f,   BLUE: %f    Diff: %f\n", distRed, distBlue, distBlue - distRed );
		 
	if( ( distBlue - distRed ) > 20.0f ){
		//G_Printf("RED\n");
		return TEAM_RED;
	}
	else if( ( distBlue - distRed ) < -20.0f ){
		//G_Printf("BLUE\n");
		return TEAM_BLUE;
	}
	else
		return -1;
	
}

qboolean G_CheckDeniedReward( gentity_t *attacker, gentity_t *other ){
	int dist = -1;
	int i;
	gentity_t *ent;
	vec3_t delta;
	for( i = 0; i < MAX_GENTITIES; i++ ){
		ent = &g_entities[i];
		if( !ent->inuse || ent->s.eType != ET_ITEM )
			continue;
		if( ent->s.eFlags & EF_NODRAW )
			continue;
		if( ent->item->giType != IT_ARMOR && ent->item->giType != IT_HEALTH && ent->item->giType != IT_POWERUP )
			continue;
		if( ent->item->quantity < 50 && ent->item->giType == IT_ARMOR )
			continue;
		if( ent->item->quantity < 100 && ent->item->giType == IT_HEALTH )
			continue;
		
		VectorSubtract( ent->s.pos.trBase, other->client->ps.origin, delta );
		dist = VectorNormalize( delta );
		if ( dist < 128 ) {
			attacker->client->rewards[REWARD_ITEMDENIED]++;
			RewardMessage(attacker, REWARD_ITEMDENIED, attacker->client->rewards[REWARD_ITEMDENIED] );
			return qtrue;
			//break;
		}
		
		
	}
	return qfalse;
}

