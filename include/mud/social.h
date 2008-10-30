/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_SOCIAL_H
#define SOURCEMUD_MUD_SOCIAL_H

#include "common/string.h"
#include "common/types.h"
#include "mud/server.h"
#include "common/imanager.h"

// details of a particular social action
struct SocialDetails : public GC
{
	SocialDetails ();

	String adverb; // empty for no adverb
	String self; // display to the actor
	String room; // displayed to the room
	String target; // displayed to the target if one given

	struct SocialFlags {
		int speech:1, move:1, touch:1, target:1;
	} flags;
};

// hold a social
class Social : public GC
{
	public:
	Social ();

	// basic info
	String get_name () const { return name; }

	// perform the social
	int perform (class Creature* actor, class Entity* target, String adverb) const;

	private:
	// basic info
	String name;
	GCType::vector<SocialDetails> details;
	Social* next;

	// for list management...
	friend class SSocialManager;
};

class SSocialManager : public IManager
{
	public:
	inline SSocialManager() : socials(NULL) {}

	int initialize ();

	void shutdown ();

	Social* find_social (String name);

	private:
	Social* socials;
};
extern SSocialManager SocialManager;

#endif
