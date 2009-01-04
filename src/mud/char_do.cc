/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/streams.h"
#include "common/string.h"
#include "mud/creature.h"
#include "mud/server.h"
#include "mud/room.h"
#include "mud/body.h"
#include "mud/player.h"
#include "mud/macro.h"
#include "mud/action.h"
#include "mud/object.h"
#include "mud/npc.h"

void Creature::do_emote(const std::string& action)
{
	if (get_room())
		*get_room() << "(" << StreamName(this, DEFINITE, true) << ") " << action << "\n";
}

void Creature::do_say(const std::string& text)
{
	// don't say nothing
	if (text.empty())
		return;

	// last creature of text
	char last_char = text[text.size() - 1];

	// you say...
	if (last_char == '?')
		*this << "You ask, \"" CTALK;
	else if (last_char == '!')
		*this << "You exclaim, \"" CTALK;
	else
		*this << "You say, \"" CTALK;
	*this << text << CNORMAL "\"\n";

	// blah says...
	{
		StreamControl stream(*get_room());  // ends at second bracket, sends text
		stream << StreamIgnore(this);
		if (is_dead())
			stream << "The ghostly voice of " << StreamName(this, INDEFINITE, false);
		else
			stream << StreamName(this, INDEFINITE, true);

		if (last_char == '?')
			stream << " asks, \"";
		else if (last_char == '!')
			stream << " exclaims, \"";
		else
			stream << " says, \"";

		stream << CTALK << text << CNORMAL "\"\n";
	}

	// FIXME EVENT
}

void Creature::do_sing(const std::string& text)
{
	// split into lines
	std::vector<std::string> lines;
	explode(lines, text, ';');
	if (lines.size() > 4) {
		*this << "You may only sing up to four lines at a time.\n";
		return;
	}

	// trim lines
	std::vector<std::string>::iterator li = lines.begin();
	while (li != lines.end()) {
		*li = strip(*li);
		if (li->empty())
			li = lines.erase(li);
		else
			++li;
	}

	// any lines at all?
	if (lines.empty()) {
		*this << "Sing what?\n";
		return;
	}

	// form output
	StringBuffer output;
	output << "  " CTALK << lines[0];
	for (size_t i = 1; i < lines.size(); ++i) {
		if (i % 2 == 0)
			output << "\n   ";
		else
			output << "\n     ";
		output << lines[i];
	}
	output << CNORMAL "\n";

	// you sing...
	*this << "You sing:\n" << output.str();

	// blah sings...
	StreamControl stream(*get_room());
	stream << StreamIgnore(this);
	if (is_dead())
		stream << "The ghostly voice of " << StreamName(this, INDEFINITE, false);
	else
		stream << StreamName(this, INDEFINITE, true);
	stream << " sings:\n" << output;
}

void Creature::do_look()
{
	// check
	if (!check_see()) return;
	if (!PLAYER(this)) return;

	get_room()->show(StreamControl(*this), this);
	Events::sendLook(get_room(), this, get_room());
}

void Creature::do_look(Creature *ch)
{
	assert(ch != NULL);

	// check
	if (!check_see()) return;

	// send message to receiver
	if (this != ch && PLAYER(ch) != NULL)
		*PLAYER(ch) << StreamName(*this, NONE, true) << " glances at you.\n";

	// description
	*this << StreamCreatureDesc(ch);

	// inventory
	ch->display_equip(StreamControl(this));

	// finish
	*this << "\n";

	Events::sendLook(get_room(), this, ch);
}

void Creature::do_look(Object *obj, ObjectLocation type)
{
	assert(obj != NULL);

	// check
	if (!check_see()) return;

	// specific container type
	if (type != 0) {
		if (obj->has_location(type))
			obj->show_contents(PLAYER(this), type);
		else if (type == ObjectLocation::IN)
			*this << StreamName(*obj, DEFINITE, true) << " cannot be looked inside of.\n";
		else if (type == ObjectLocation::ON)
			*this << StreamName(*obj, DEFINITE, true) << " cannot be looked ontop of.\n";
		else
			*this << StreamName(*obj, DEFINITE, true) << " cannot be looked at that way.\n";
	} else {
		// generic - description and on or in contents
		if (!obj->get_desc().empty())
			*this << StreamMacro(obj->get_desc(), "self", obj, "actor", this) << "  ";
		// on contents?
		if (obj->has_location(ObjectLocation::ON))
			obj->show_contents(PLAYER(this), ObjectLocation::ON);
		else
			*this << "\n";
	}

	Events::sendLook(get_room(), this, obj);
}

void Creature::do_look(Portal *portal)
{
	assert(portal != NULL);

	// get target room
	Room* target_room = NULL;
	if (!portal->is_closed() && !portal->is_nolook())
		target_room = portal->get_relative_target(get_room());

	// basic description
	if (!portal->get_desc().empty())
		*this << StreamMacro(portal->get_desc(), "portal", portal, "actor", this) << "  ";
	else if (target_room == NULL)
		*this << "There is nothing remarkable about " << StreamName(*portal, DEFINITE) << ".  ";

	// show direction
	if (portal->get_dir().valid() && portal->get_name().get_text() != portal->get_dir().get_name())
		*this << StreamName(*portal, DEFINITE, true) << " heads " << portal->get_relative_dir(get_room()).get_name() << ".  ";

	// closed portal?
	if (portal->is_closed())
		*this << StreamName(*portal, DEFINITE, true) << " is closed.";
	// open and is a door?
	else if (portal->is_door())
		*this << StreamName(*portal, DEFINITE, true) << " is open.";

	// finish off line
	*this << "\n";

	Events::sendLook(get_room(), this, portal);

	// display target room if possible
	if (target_room) {
		target_room->show(*this, this);
		Events::sendLook(get_room(), this, target_room);
	}
}

class ActionChangePosition : public IAction
{
public:
	ActionChangePosition(Creature* s_ch, CreaturePosition s_position) : IAction(s_ch), position(s_position) {}

	virtual uint get_rounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << position.get_verbing(); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		if (position == get_actor()->get_pos()) {
			*get_actor() << "You are already " << position.get_verbing() << ".\n";
			return 1;
		}

		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " " << position.get_sverb() << ".\n";
		*get_actor() << "You " << position.get_verb() << ".\n";
		get_actor()->set_pos(position);

		return 0;
	}

private:
	CreaturePosition position;
};

void Creature::do_position(CreaturePosition position)
{
	add_action(new ActionChangePosition(this, position));
}

class ActionGet : public IAction
{
public:
	ActionGet(Creature* s_ch, Object* s_obj, Object* s_container, ObjectLocation s_type) :
			IAction(s_ch), obj(s_obj), container(s_container), type(s_type) {
	}

	virtual uint get_rounds() const { return 2; }
	virtual void describe(const StreamControl& stream) const { stream << "getting " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		if (!get_actor()->check_alive() || !get_actor()->check_move())
			return 1;

		// FIXME: check the container is still accessible

		if (!obj->is_touchable()) {
			*get_actor() << "You cannot reach " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		} else if (!obj->is_gettable()) {
			*get_actor() << "You cannot pickup " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		} else if (get_actor()->hold(obj) < 0) {
			*get_actor() << "Your hands are full.\n";
			return 1;
		} else {
			// get the object
			if (container) {
				*get_actor() << "You get " << StreamName(*obj, DEFINITE) << " from " << StreamName(container, DEFINITE) << ".\n";
				if (get_actor()->get_room()) *get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " gets " << StreamName(obj, INDEFINITE) << " from " << StreamName(container, INDEFINITE) << ".\n";
			} else {
				*get_actor() << "You pick up " << StreamName(*obj, DEFINITE) << ".\n";
				if (get_actor()->get_room()) *get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " picks up " << StreamName(obj, INDEFINITE) << ".\n";
			}

			// notification
			Events::sendTouchItem(get_actor()->get_room(), get_actor(), obj);
			Events::sendGraspItem(get_actor()->get_room(), get_actor(), obj);
			Events::sendPickupItem(get_actor()->get_room(), get_actor(), obj);
			return 0;
		}
	}

private:
	Object* obj;
	Object* container;
	ObjectLocation type;
};

void Creature::do_get(Object *obj, Object *contain, ObjectLocation type)
{
	assert(obj != NULL);

	add_action(new ActionGet(this, obj, contain, type));
}

class ActionPut : public IAction
{
public:
	ActionPut(Creature* s_ch, Object* s_obj, Object* s_container, ObjectLocation s_type) :
			IAction(s_ch), obj(s_obj), container(s_container), type(s_type) {
	}

	virtual uint get_rounds() const { return 2; }
	virtual void describe(const StreamControl& stream) const { stream << "putting " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		if (!get_actor()->check_alive() || !get_actor()->check_move())
			return 1;

		// FIXME: boy is this incomplete!!

		if (!container->has_location(type)) {
			*get_actor() << "You cannot do that with " << StreamName(*container) << ".\n";
			return 1;
		} else {
			// should force let go
			container->add_object(obj, type);
			*get_actor() << "You put " << StreamName(*obj, DEFINITE) << " " << type.name() << " " << StreamName(container, DEFINITE) << ".\n";
			return 0;
		}
	}

private:
	Object* obj;
	Object* container;
	ObjectLocation type;
};

void Creature::do_put(Object *obj, Object *contain, ObjectLocation type)
{
	assert(obj != NULL);
	assert(contain != NULL);

	add_action(new ActionPut(this, obj, contain, type));
}

class ActionGiveCoins : public IAction
{
public:
	ActionGiveCoins(Creature* s_ch, Creature* s_target, uint s_amount) :
			IAction(s_ch), target(s_target), amount(s_amount) {}

	virtual uint get_rounds() const { return 2; }
	virtual void describe(const StreamControl& stream) const { stream << "giving coins to " << StreamName(target, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_alive() || !get_actor()->check_move())
			return 1;

		// have enough coins?
		if (get_actor()->get_coins() == 0) {
			*get_actor() << "You don't have any coins.\n";
			return 1;
		} else if (get_actor()->get_coins() < amount) {
			*get_actor() << "You only have " << get_actor()->get_coins() << " coins.\n";
			return 1;
		}

		// FIXME: not safe - what if give_coins overflows?
		// do give
		target->give_coins(amount);
		get_actor()->take_coins(amount);

		// messages
		*get_actor() << "You give " << amount << " coins to " << StreamName(*target, DEFINITE) << ".\n";
		*target << StreamName(*get_actor(), INDEFINITE, true) << " gives you " << amount << " coins.\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamIgnore(target) << StreamName(*get_actor(), INDEFINITE, true) << " gives some coins to " << StreamName(*target, INDEFINITE) << ".\n";
		return 0;
	}

private:
	Creature* target;
	uint amount;
};

void Creature::do_give_coins(Creature* target, uint amount)
{
	assert(target != NULL);
	assert(amount != 0);

	add_action(new ActionGiveCoins(this, target, amount));
}

class ActionWear : public IAction
{
public:
	ActionWear(Creature* s_ch, Object* s_obj) :
			IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds() const { return 5; }
	virtual void describe(const StreamControl& stream) const { stream << "putting on " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		if (!get_actor()->check_move())
			return 1;

		if (!get_actor()->is_held(obj)) {
			*get_actor() << "You must be holding " << StreamName(*obj, DEFINITE) << " to put it on.\n";
			return 1;
		}

		if (get_actor()->wear(obj) < 0) {
			*get_actor() << "You can't wear " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		*get_actor() << "You wear " << StreamName(*obj, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " equips " << StreamName(obj) << ".\n";
		return 0;
	}

private:
	Object* obj;
};

void Creature::do_wear(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionWear(this, obj));
}

class ActionRemove : public IAction
{
public:
	ActionRemove(Creature* s_ch, Object* s_obj) :
			IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds() const { return 5; }
	virtual void describe(const StreamControl& stream) const { stream << "removing " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		if (!get_actor()->check_move())
			return 1;

		if (!get_actor()->is_worn(obj)) {
			*get_actor() << "You must be wearing " << StreamName(*obj, DEFINITE) << " to take it off.\n";
			return 1;
		}

		if (get_actor()->hold(obj) < 0) {
			*get_actor() << "Your hands are full.\n";
			return 1;
		}

		*get_actor() << "You remove " << StreamName(*obj, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " removes " << StreamName(obj) << ".\n";
		return 0;
	}

private:
	Object* obj;
};

void Creature::do_remove(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionRemove(this, obj));
}

class ActionDrop : public IInstantAction
{
public:
	ActionDrop(Creature* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform() {
		if (!get_actor()->check_alive() || !get_actor()->check_move())
			return;

		if (!get_actor()->is_held(obj)) {
			*get_actor() << "You are not holding " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		if (!obj->is_dropable()) {
			*get_actor() << "You cannot drop " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		// do drop
		*get_actor() << "You drop " << StreamName(*obj, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " drops " << StreamName(obj) << ".\n";

		get_actor()->get_room()->add_object(obj);

		// send notification
		Events::sendReleaseItem(get_actor()->get_room(), get_actor(), obj);
		Events::sendDropItem(get_actor()->get_room(), get_actor(), obj);
		return;
	}

private:
	Object* obj;
};

void Creature::do_drop(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionDrop(this, obj));
}

class ActionRead : public IInstantAction
{
public:
	ActionRead(Creature* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform() {
		// checks
		if (!get_actor()->check_see())
			return;

		// FIXME: add back text property, and an event
		*get_actor() << StreamName(*obj, DEFINITE, true) << " cannot be read.\n";
	}

private:
	Object* obj;
};

void Creature::do_read(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionRead(this, obj));
}

class ActionEat : public IAction
{
public:
	ActionEat(Creature* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds() const { return 4; }
	virtual void describe(const StreamControl& stream) const { stream << "eating " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// FIXME: text and event
		*get_actor() << "You can't eat " << StreamName(*obj, DEFINITE) << ".\n";
		return 1;
	}

private:
	Object* obj;
};

void Creature::do_eat(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionEat(this, obj));
}

class ActionDrink : public IAction
{
public:
	ActionDrink(Creature* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds() const { return 4; }
	virtual void describe(const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// FIXME: text and event
		*get_actor() << "You can't drink " << StreamName(*obj, DEFINITE) << ".\n";
		return 1;
	}

private:
	Object* obj;
};

void Creature::do_drink(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionDrink(this, obj));
}

class ActionRaise : public IAction
{
public:
	ActionRaise(Creature* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds() const { return 2; }
	virtual void describe(const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// held?
		if (!get_actor()->is_held(obj)) {
			*get_actor() << "You are not holding " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		// output
		*get_actor() << "You raise " << StreamName(*obj, DEFINITE) << " into the air.\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " raises " << StreamName(obj) << " into the air.\n";

		// FIXME EVENT
		return 0;
	}

private:
	Object* obj;
};

void Creature::do_raise(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionRaise(this, obj));
}

class ActionTouch : public IInstantAction
{
public:
	ActionTouch(Creature* s_ch, Object* s_obj) : IInstantAction(s_ch), obj(s_obj) {}

	virtual void perform() {
		// checks
		if (!get_actor()->check_move())
			return;

		// touchable?
		if (!obj->is_touchable()) {
			*get_actor() << "You cannot touch " << StreamName(*obj, DEFINITE) << ".\n";
			return;
		}

		// output
		*get_actor() << "You touch " << StreamName(*obj, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " touches " << StreamName(obj) << ".\n";

		// FIXME EVENT
		return;
	}

private:
	Object* obj;
};

void Creature::do_touch(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionTouch(this, obj));
}

class ActionKick : public IAction
{
public:
	ActionKick(Creature* s_ch, Object* s_obj) : IAction(s_ch), obj(s_obj) {}

	virtual uint get_rounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "drinking " << StreamName(obj, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		if (!obj->is_touchable()) {
			*get_actor() << "You cannot kick " << StreamName(*obj, DEFINITE) << ".\n";
			return 1;
		}

		// output
		*get_actor() << "You kick " << StreamName(*obj, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " kickes " << StreamName(obj) << ".\n";

		// FIXME EVENT
		return 0;
	}

private:
	Object* obj;
};

void Creature::do_kick(Object *obj)
{
	assert(obj != NULL);

	add_action(new ActionKick(this, obj));
}

class ActionOpenPortal : public IAction
{
public:
	ActionOpenPortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint get_rounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "opening " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// door?
		if (!portal->is_door()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// already open?
		if (!portal->is_closed()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is already open.\n";
			return 1;
		}

		// locked?
		if (portal->is_locked()) {
			*get_actor() << "You try to open " << StreamName(*portal, DEFINITE, true) << ", but it is locked.\n";
			if (get_actor()->get_room())
				*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " tries to open " << StreamName(portal, DEFINITE) << ", but it appears to be locked.\n";

			// FIXME EVENT (Touch event)
			return 0;
		}

		// open it
		portal->open(get_actor()->get_room(), get_actor());
		*get_actor() << "You open " << StreamName(*portal, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " opens " << StreamName(portal, DEFINITE) << ".\n";

		// FIXME EVENT (Touch and Open)

		return 0;
	}

private:
	Portal* portal;
};

void Creature::do_open(Portal *portal)
{
	assert(portal != NULL);

	add_action(new ActionOpenPortal(this, portal));
}

class ActionClosePortal : public IAction
{
public:
	ActionClosePortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint get_rounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "closing " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// door?
		if (!portal->is_door()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// already closed?
		if (portal->is_closed()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is already closed.\n";
			return 1;
		}

		// close it
		portal->close(get_actor()->get_room(), get_actor());
		*get_actor() << "You close " << StreamName(*portal, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " closes " << StreamName(portal, DEFINITE) << ".\n";

		// FIXME EVENT (Touch and Close)

		return 0;
	}

private:
	Portal* portal;
};

void Creature::do_close(Portal *portal)
{
	assert(portal != NULL);

	add_action(new ActionClosePortal(this, portal));
}

class ActionLockPortal : public IAction
{
public:
	ActionLockPortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint get_rounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "locking " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// door?
		if (!portal->is_door()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!portal->is_closed()) {
			*get_actor() << "You cannot lock " << StreamName(*portal, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// lock it
		portal->lock(get_actor()->get_room(), get_actor());
		*get_actor() << "You lock " << StreamName(*portal, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " locks " << StreamName(portal, DEFINITE) << ".\n";

		// FIXME EVENT (Touch and Lock)

		return 0;
	}

private:
	Portal* portal;
};

void Creature::do_lock(Portal *portal)
{
	assert(portal != NULL);

	add_action(new ActionLockPortal(this, portal));
}

class ActionUnlockPortal : public IAction
{
public:
	ActionUnlockPortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint get_rounds() const { return 1; }
	virtual void describe(const StreamControl& stream) const { stream << "unlocking " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// door?
		if (!portal->is_door()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!portal->is_closed()) {
			*get_actor() << "You cannot unlock " << StreamName(*portal, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// unlock it
		portal->unlock(get_actor()->get_room(), get_actor());
		*get_actor() << "You unlock " << StreamName(*portal, DEFINITE) << ".\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " unlocks " << StreamName(portal, DEFINITE) << ".\n";

		// FIXME EVENT (Touch and Unlock)
		return 0;
	}

private:
	Portal* portal;
};

void Creature::do_unlock(Portal *portal)
{
	assert(portal != NULL);

	add_action(new ActionUnlockPortal(this, portal));
}

class ActionUsePortal : public IAction
{
public:
	ActionUsePortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal), rounds(0) {}

	virtual uint get_rounds() const { return rounds; }
	virtual void describe(const StreamControl& stream) const { stream << "kicking " << StreamName(portal, INDEFINITE); }

	virtual void finish() {
		// checks
		if (!get_actor()->check_move())
			return;

		// closed?  drat
		if (portal->is_closed()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is closed.\n";
			return;
		}

		// get target room
		Room *new_room = portal->get_relative_target(get_actor()->get_room());
		if (new_room == NULL) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " does not lead anywhere.\n";
			return;
		}

		// set rounds
		// one round plus one per every 20% below max health
		// add five if an NPC
		rounds = 1 + (100 - (get_actor()->get_hp() * 100 / get_actor()->get_max_hp())) / 20 + (NPC(get_actor()) ? 5 : 0);

		// do go
		get_actor()->enter(new_room, portal);
	}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// closed?  drat
		if (portal->is_closed()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is closed.\n";
			return 1;
		}

		// get target room
		Room *new_room = portal->get_relative_target(get_actor()->get_room());
		if (new_room == NULL) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " does not lead anywhere.\n";
			return 1;
		}

		// set rounds
		// one round plus one per every 20% below max health
		// add five if an NPC
		rounds = 1 + (100 - (get_actor()->get_hp() * 100 / get_actor()->get_max_hp())) / 20 + (NPC(get_actor()) ? 5 : 0);

		return 0;
	}

private:
	Portal* portal;
	uint rounds;
};

void Creature::do_go(Portal *portal)
{
	assert(portal != NULL);

	add_action(new ActionUsePortal(this, portal));
}

class ActionKickPortal : public IAction
{
public:
	ActionKickPortal(Creature* s_ch, Portal* s_portal) : IAction(s_ch), portal(s_portal) {}

	virtual uint get_rounds() const { return 3; }
	virtual void describe(const StreamControl& stream) const { stream << "kicking " << StreamName(portal, INDEFINITE); }
	virtual void finish() {}

	virtual int start() {
		// checks
		if (!get_actor()->check_move())
			return 1;

		// door?
		if (!portal->is_door()) {
			*get_actor() << StreamName(*portal, DEFINITE, true) << " is not a door.\n";
			return 1;
		}

		// open?
		if (!portal->is_closed()) {
			*get_actor() << "You cannot kick " << StreamName(*portal, DEFINITE) << " while it's open.\n";
			return 1;
		}

		// kick it
		portal->open(get_actor()->get_room(), get_actor());
		*get_actor() << "You kick " << StreamName(*portal, DEFINITE) << " open.\n";
		if (get_actor()->get_room())
			*get_actor()->get_room() << StreamIgnore(get_actor()) << StreamName(get_actor(), INDEFINITE, true) << " kicks " << StreamName(portal, DEFINITE) << " open.\n";

		// FIXME EVENT (Touch and Kick)

		return 0;
	}

private:
	Portal* portal;
};

void Creature::do_kick(Portal *portal)
{
	assert(portal != NULL);

	add_action(new ActionKickPortal(this, portal));
}
