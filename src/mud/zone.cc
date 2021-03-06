/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common.h"
#include "common/rand.h"
#include "common/streams.h"
#include "common/file.h"
#include "common/string.h"
#include "mud/room.h"
#include "mud/zone.h"
#include "mud/settings.h"
#include "mud/npc.h"
#include "mud/clock.h"
#include "mud/gametime.h"
#include "mud/weather.h"
#include "mud/object.h"
#include "mud/hooks.h"

_MZone MZone;

bool Spawn::check(const Zone* zone) const
{
	if (!tag.valid())
		return false;
	size_t count = MEntity.tagCount(tag);
	return count < min;
}

bool Spawn::heartbeat()
{
	// ready for update?
	if (++dcount >= delay) {
		dcount = 0;
		return true;
	}

	return false;
}

void Spawn::spawn(Zone* zone) const
{
	// FIXME: print warnings for the returns below;
	//        builders should know about this stuff...

	// any blueprints to spawn?
	if (blueprints.empty())
		return;
	// any rooms to spawn in?
	if (rooms.empty())
		return;

	// select random room
	std::string roomname = rooms[Random::get(rooms.size())];

	// find room
	Room* room = zone->getRoom(roomname);
	if (room == NULL)
		return;

	// select random blueprint id
	std::string tempname = blueprints[Random::get(blueprints.size())];

	// try to spawn as NPC
	Npc* npc = Npc::loadBlueprint(tempname);
	if (npc != NULL) {
		// make sure NPC has the tag
		npc->addTag(tag);

		// zone lock the NPC
		npc->setZoneLocked(true);

		// add to room
		npc->enter(room, NULL);
	} else {
		// try to spawn as object
		Object* object = Object::loadBlueprint(tempname);
		if (object != NULL) {
			// make sure object has the tag
			object->addTag(tag);

			// add to room
			room->addObject(object);
		}
	}
}

int Spawn::load(File::Reader& reader)
{
	min = 1;
	tag = TagID();
	blueprints.resize(0);
	rooms.resize(0);
	delay = 1;
	dcount = 0;

	FO_READ_BEGIN
	FO_ATTR("spawn", "count")
	min = node.getInt();
	FO_ATTR("spawn", "tag")
	tag = TagID::create(node.getString());
	FO_ATTR("spawn", "delay")
	delay = node.getInt();
	FO_ATTR("spawn", "blueprint")
	blueprints.push_back(node.getString());
	FO_ATTR("spawn", "room")
	rooms.push_back(node.getString());
	FO_READ_ERROR
	return -1;
	FO_READ_END

	return 0;
}


void Spawn::save(File::Writer& writer) const
{
	writer.begin("zone", "spawn");

	writer.attr("spawn", "tag", TagID::nameof(tag));
	writer.attr("spawn", "count", min);
	writer.attr("spawn", "delay", delay);
	for (std::vector<std::string>::const_iterator i = blueprints.begin(); i != blueprints.end(); ++i) {
		writer.attr("spawn", "blueprint", *i);
	}
	for (std::vector<std::string>::const_iterator i = rooms.begin(); i != rooms.end(); ++i) {
		writer.attr("spawn", "room", *i);
	}

	writer.end();
}

Zone::Zone()
{}

Room* Zone::getRoom(const std::string& id) const
{
	for (RoomList::const_iterator i = rooms.begin(); i != rooms.end(); ++i)
		if (strEq((*i)->getId(), id))
			return (*i);

	return NULL;
}

Room* Zone::getRoomAt(size_t index) const
{
	for (RoomList::const_iterator i = rooms.begin(); i != rooms.end(); ++i)
		if (index-- == 0)
			return (*i);

	return NULL;
}

size_t Zone::getRoomCount() const
{
	return rooms.size();
}

int Zone::load(const std::string& path)
{
	File::Reader reader;
	if (reader.open(path))
		return -1;

	FO_READ_BEGIN
	FO_ATTR("zone", "name")
	setName(node.getString());
	FO_ATTR("zone", "id")
	id = node.getString();
	FO_ENTITY("zone", "child")
	if (ROOM(entity) == NULL) throw File::Error("Zone child is not a Room");
	addRoom(ROOM(entity));
	FO_OBJECT("zone", "spawn")
	Spawn spawn;
	if (!spawn.load(reader))
		spawns.push_back(spawn);
	else
		throw File::Error("Failed to load room");
	FO_READ_ERROR
	return -1;
	FO_READ_END

	return 0;
}

void Zone::save()
{
	std::string path = MSettings.getZonePath() + "/" + getId() + ".zone";

	/* backup zone file */
	if (MSettings.getBackupZones()) {
		char time_buffer[15];
		time_t base_t;
		time(&base_t);
		strftime(time_buffer, sizeof(time_buffer), "%Y%m%d%H%M%S", localtime(&base_t));
		std::string backup = path + "." + std::string(time_buffer) + "~";
		if (File::rename(path, backup.c_str())) /* move file */
			Log::Error << "Backup of zone '" << getId() << "' to " << backup << " failed: " << strerror(errno);
	}

	File::Writer writer;
	if (writer.open(path)) {
		Log::Error << "Failed to open " << path << " for writing";
		return;
	}

	// header
	writer.comment("Zone: " + getId());

	// basics
	writer.bl();
	writer.comment("--- BASICS ---");
	writer.attr("zone", "id", id);
	writer.attr("zone", "name", name);

	// spawns
	writer.bl();
	writer.comment("--- SPAWNS ---");
	for (SpawnList::const_iterator i = spawns.begin(); i != spawns.end(); ++i)
		i->save(writer);

	// rooms
	writer.bl();
	writer.comment("--- ROOMS ---");
	for (RoomList::iterator i = rooms.begin(); i != rooms.end(); ++i)
		(*i)->save(writer, "zone", "child");

	writer.bl();
	writer.comment(" --- EOF ---");
}

void Zone::addRoom(Room *room)
{
	assert(room != NULL);

	room->setZone(this);
	rooms.push_back(room);
}

void Zone::heartbeat()
{
	// spawn systems
	for (SpawnList::iterator i = spawns.begin(); i != spawns.end(); ++i) {
		if (i->heartbeat())
			if (i->check(this))
				i->spawn(this);
	}
}

void Zone::activate()
{
	for (RoomList::iterator i = rooms.begin(); i != rooms.end(); ++i)
		(*i)->activate();
}

void Zone::deactivate()
{
	for (RoomList::iterator i = rooms.begin(); i != rooms.end(); ++i)
		(*i)->deactivate();
}

void Zone::destroy()
{
	// save and backup
	save();
	std::string path = MSettings.getZonePath() + "/" + getId() + ".zone";
	File::rename(path, path + "~");

	// remove from zone list
	_MZone::ZoneList::iterator i = find(MZone.zones.begin(), MZone.zones.end(), this);
	if (i != MZone.zones.end())
		MZone.zones.erase(i);

	// shut down zone
	deactivate();
}

void Zone::broadcastEvent(const Event& event)
{
	for (RoomList::iterator i = rooms.begin(); i != rooms.end(); ++i)
		MEvent.resend(event, *i);
}

int _MZone::initialize()
{
	// modules we need
	if (require(MEntity) != 0)
		return 1;
	if (require(MNpcBP) != 0)
		return 1;
	if (require(MObjectBP) != 0)
		return 1;
	if (require(MTime) != 0)
		return 1;
	if (require(MWeather) != 0)
		return 1;
	return 0;
}

// load the world
int _MZone::loadWorld()
{
	// read zones dir
	std::vector<std::string> files = File::dirlist(MSettings.getZonePath());
	File::filter(files, "*.zone");
	for (std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i) {
		Zone* zone = new Zone();
		if (zone->load(*i))
			return -1;
		zones.push_back(zone); // don't call addZone(), we don't want to activate it yet
	}

	// now activate all zones
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		(*i)->activate();

	return 0;
}

// close down zone manager
void _MZone::shutdown()
{
	// deactive all zones, which deactives all entities.
	// we don't delete them until a collection is run, in order to
	// protect against any portals/rooms/whatever that might link
	// back to the zone
	for (ZoneList::iterator i = zones.begin(), e = zones.end(); i != e; ++i)
		(*i)->deactivate();

	// collect entities
	MEntity.collect();

	// delete zones
	for (ZoneList::iterator i = zones.begin(), e = zones.end(); i != e; ++i)
		delete *i;
	zones.clear();
}

// save zones
void _MZone::save()
{
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		(*i)->save();
}

/* find a Zone */
Zone* _MZone::getZone(const std::string& id)
{
	assert(!id.empty() && "id must not be empty");

	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		if (strEq((*i)->getId(), id))
			return (*i);

	return NULL;
}

/* get a Zone  by index */
Zone* _MZone::getZoneAt(size_t index)
{
	if (index >= zones.size())
		return NULL;

	return zones[index];
}

/* find a Room */
Room* _MZone::getRoom(const std::string& id)
{
	if (id.empty())
		return NULL;

	Room *room;
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i) {
		room = (*i)->getRoom(id);
		if (room != NULL)
			return room;
	}

	return NULL;
}

void Zone::announce(const std::string& str, AnnounceFlags flags) const
{
	for (RoomList::const_iterator i = rooms.begin(); i != rooms.end(); ++i) {
		if (!flags ||
		        (flags & ANFL_OUTDOORS && (*i)->isOutdoors()) ||
		        (flags & ANFL_INDOORS && !(*i)->isOutdoors())
		   )
			**i << str << "\n";
	}
}

/* announce to all rooms in a Room */
void _MZone::announce(const std::string& str, AnnounceFlags flags)
{
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		(*i)->announce(str, flags);
}

void _MZone::addZone(Zone *zone)
{
	assert(zone != NULL);

	// make sure we don't already have the zone
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i)
		if (*i == zone)
			return;

	// push the zone
	zones.push_back(zone);

	// activate it
}

void _MZone::listRooms(const StreamControl& stream)
{
	for (ZoneList::iterator i = zones.begin(); i != zones.end(); ++i) {
		stream << " " << (*i)->getName() << " <" << (*i)->getId() << ">\n";
		for (Zone::RoomList::iterator ii = (*i)->rooms.begin(); ii != (*i)->rooms.end(); ++ii)
			stream << "   " << StreamName(*ii) << " <" << (*ii)->getId() << ">\n";
	}
}
