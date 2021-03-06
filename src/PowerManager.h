/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

/**
 * class PowerManager
 *
 * Special code for handling spells, special powers, item effects, etc.
 */

#pragma once
#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "CommonIncludes.h"
#include "MapRenderer.h"
#include "Utils.h"

#include <queue>
#include <cassert>

class Animation;
class AnimationSet;
class Hazard;
class MapCollision;
class StatBlock;

const int POWER_COUNT = 1024;

const int POWTYPE_FIXED = 0;
const int POWTYPE_MISSILE = 1;
const int POWTYPE_REPEATER = 2;
const int POWTYPE_SPAWN = 3;
const int POWTYPE_TRANSFORM = 4;
const int POWTYPE_EFFECT = 5;

const int POWSTATE_BLOCK = 1;
const int POWSTATE_INSTANT = 2;
const int POWSTATE_ATTACK = 3;

const int BASE_DAMAGE_NONE = 0;
const int BASE_DAMAGE_MELEE = 1;
const int BASE_DAMAGE_RANGED = 2;
const int BASE_DAMAGE_MENT = 3;

// when casting a spell/power, the hazard starting position is
// either the source (the avatar or enemy), the target (mouse click position),
// or melee range in the direction that the source is facing
const int STARTING_POS_SOURCE = 0;
const int STARTING_POS_TARGET = 1;
const int STARTING_POS_MELEE = 2;

const int TRIGGER_BLOCK = 0;
const int TRIGGER_HIT = 1;
const int TRIGGER_HALFDEATH = 2;
const int TRIGGER_JOINCOMBAT = 3;
const int TRIGGER_DEATH = 4;

const int SPAWN_LIMIT_MODE_FIXED = 0;
const int SPAWN_LIMIT_MODE_STAT = 1;
const int SPAWN_LIMIT_MODE_UNLIMITED = 2;

const int SPAWN_LIMIT_STAT_PHYSICAL = 0;
const int SPAWN_LIMIT_STAT_MENTAL = 1;
const int SPAWN_LIMIT_STAT_OFFENSE = 2;
const int SPAWN_LIMIT_STAT_DEFENSE = 3;

const int STAT_MODIFIER_MODE_MULTIPLY = 0;
const int STAT_MODIFIER_MODE_ADD = 1;
const int STAT_MODIFIER_MODE_ABSOLUTE = 2;

class PostEffect {
public:
	int id;
	int magnitude;
	int duration;

	PostEffect() {
		id = 0;
		magnitude = 0;
		duration = 0;
	}
};

class Power {
public:
	// base info
	int type; // what kind of activate() this is
	std::string name;
	std::string description;
	std::string tag; // optional unique name used to get power id
	int icon; // just the number.  The caller menu will have access to the surface.
	int new_state; // when using this power the user (avatar/enemy) starts a new state
	std::string attack_anim; // name of the animation to play when using this power, if it is not block
	bool face; // does the user turn to face the mouse cursor when using this power?
	int source_type; //hero, neutral, or enemy
	bool beacon; //true if it's just an ememy calling its allies
	int count; // number of hazards/effects or spawns created
	bool passive; // if unlocked when the user spawns, automatically cast it
	int passive_trigger; // only activate passive powers under certain conditions (block, hit, death, etc)

	// power requirements
	std::set<std::string> requires_flags; // checked against equip_flags granted from items
	int requires_mp;
	int requires_hp;
	bool sacrifice;
	bool requires_los; // line of sight
	bool requires_empty_target; // target square must be empty
	int requires_item;
	int requires_equipped_item;
	bool consumable;
	bool requires_targeting; // power only makes sense when using click-to-target
	int cooldown; // milliseconds before you can use the power again

	// animation info
	std::string animation_name;
	int sfx_index;
	bool directional; // sprite sheet contains options for 8 directions, one per row
	int visual_random; // sprite sheet contains rows of random options
	int visual_option; // sprite sheet contains rows of similar effects.  use a specific option
	bool aim_assist;
	int speed; // for missile hazards, map units per frame
	int lifespan; // how long the hazard/animation lasts
	bool floor; // the hazard is drawn between the background and object layers
	bool complete_animation;

	// hazard traits
	bool use_hazard;
	bool no_attack;
	int radius;
	int base_damage; // enum.  damage is powered by melee, ranged, mental weapon
	int starting_pos; // enum. (source, target, or melee)
	bool multitarget;
	int range;
	bool target_party;
	std::vector<std::string> target_categories;

	int mod_accuracy_mode;
	int mod_accuracy_value;

	int mod_crit_mode;
	int mod_crit_value;

	int mod_damage_mode;
	int mod_damage_value_min;
	int mod_damage_value_max;//only used if mode is absolute

	//steal effects (in %, eg. hp_steal=50 turns 50% damage done into HP regain.)
	int hp_steal;
	int mp_steal;

	//missile traits
	int missile_angle;
	int angle_variance;
	int speed_variance;

	//repeater traits
	int delay;

	int trait_elemental; // enum. of elements
	bool trait_armor_penetration;
	int trait_crits_impaired; // crit bonus vs. movement impaired enemies (slowed, immobilized, stunned)
	bool trait_avoidance_ignore;

	int transform_duration;
	bool manual_untransform; // true binds to the power another recurrence power
	bool keep_equipment;

	// special effects
	bool buff;
	bool buff_teleport;
	bool buff_party;
	int buff_party_power_id;

	std::vector<PostEffect> post_effects;
	std::string effect_type;
	bool effect_additive;
	bool effect_render_above;

	int post_power;
	int wall_power;
	bool allow_power_mod;

	// spawn info
	std::string spawn_type;
	int target_neighbor;
	int spawn_limit_mode;
	int spawn_limit_qty;
	int spawn_limit_every;
	int spawn_limit_stat;

	Power()
		: type(-1)
		, name("")
		, description("")
		, tag("")
		, icon(-1)
		, new_state(-1)
		, attack_anim("")
		, face(false)
		, source_type(-1)
		, beacon(false)
		, count(1)
		, passive(false)
		, passive_trigger(-1)

		, requires_mp(0)
		, requires_hp(0)
		, sacrifice(false)
		, requires_los(false)
		, requires_empty_target(false)
		, requires_item(-1)
		, requires_equipped_item(-1)
		, consumable(false)
		, requires_targeting(false)
		, cooldown(0)

		, animation_name("")
		, sfx_index(-1)
		, directional(false)
		, visual_random(0)
		, visual_option(0)
		, aim_assist(false)
		, speed(0)
		, lifespan(0)
		, floor(false)
		, complete_animation(false)

		, use_hazard(false)
		, no_attack(false)
		, radius(0)
		, base_damage(BASE_DAMAGE_NONE)
		, starting_pos(STARTING_POS_SOURCE)
		, multitarget(false)
		, range(0)
		, target_party(false)
		, mod_accuracy_mode(STAT_MODIFIER_MODE_MULTIPLY)
		, mod_accuracy_value(100)
		, mod_crit_mode(STAT_MODIFIER_MODE_MULTIPLY)
		, mod_crit_value(100)
		, mod_damage_mode(STAT_MODIFIER_MODE_MULTIPLY)
		, mod_damage_value_min(100)
		, mod_damage_value_max(0)

		, hp_steal(0)
		, mp_steal(0)

		, missile_angle(0)
		, angle_variance(0)
		, speed_variance(0)

		, delay(0)

		, trait_elemental(-1)
		, trait_armor_penetration(false)
		, trait_crits_impaired(0)
		, trait_avoidance_ignore(false)

		, transform_duration(0)
		, manual_untransform(false)
		, keep_equipment(false)

		, buff(false)
		, buff_teleport(false)
		, buff_party(false)
		, buff_party_power_id(0)

		, effect_type("")
		, effect_additive(false)
		, effect_render_above(false)

		, post_power(0)
		, wall_power(0)

		, allow_power_mod(false)
		, spawn_type("")
		, target_neighbor(0)
		, spawn_limit_mode(SPAWN_LIMIT_MODE_UNLIMITED)
		, spawn_limit_qty(1)
		, spawn_limit_every(1)
		, spawn_limit_stat(SPAWN_LIMIT_STAT_MENTAL)
	{}

};

class PowerManager {
private:

	MapCollision *collider;

	void loadPowers();

	int loadSFX(const std::string& filename);

	Point limitRange(int range, Point src, Point target);
	Point targetNeighbor(Point target, int range);
	Point targetNeighbor(Point target, int range, bool ignore_blocked);
	void initHazard(int powernum, StatBlock *src_stats, Point target, Hazard *haz);
	void buff(int power_index, StatBlock *src_stats, Point target);
	void playSound(int power_index, StatBlock *src_stats);

	bool fixed(int powernum, StatBlock *src_stats, Point target);
	bool missile(int powernum, StatBlock *src_stats, Point target);
	bool repeater(int powernum, StatBlock *src_stats, Point target);
	bool spawn(int powernum, StatBlock *src_stats, Point target);
	bool transform(int powernum, StatBlock *src_stats, Point target);

	void payPowerCost(int power_index, StatBlock *src_stats);

public:
	PowerManager();
	~PowerManager();

	std::string log_msg;

	void handleNewMap(MapCollision *_collider);
	bool activate(int power_index, StatBlock *src_stats, Point target);
	const Power &getPower(unsigned id) 	{assert(id < powers.size()); return powers[id];}
	bool canUsePower(unsigned id) const;
	bool hasValidTarget(int power_index, StatBlock *src_stats, Point target);
	bool spawn(const std::string& enemy_type, Point target);
	bool effect(StatBlock *src_stats, StatBlock *caster_stats, int power_index, int source_type);
	void activatePassives(StatBlock *src_stats);
	void activateSinglePassive(StatBlock *src_stats, int id);
	int getIdFromTag(std::string tag);

	std::vector<Power> powers;
	std::queue<Hazard *> hazards; // output; read by HazardManager
	std::queue<Map_Enemy> enemies; // output; read by PowerManager
	std::queue<int> party_buffs;

	// shared sounds for power special effects
	std::vector<SoundManager::SoundID> sfx;

	std::vector<int> used_items;
	std::vector<int> used_equipped_items;
};

#endif
