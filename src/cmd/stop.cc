/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "mud/creature.h"
#include "mud/command.h"

/* BEGIN COMMAND
 *
 * name: stop
 *
 * format: stop
 *
 * END COMMAND */

void command_stop(Creature* ch, std::string argv[])
{
	ch->cancelAction();
}
