/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef IDMAP_H
#define IDMAP_H

// NOTE:
// The funky static methods to retrieve the name list and intptr_t map
// are necessary but sickeningly ugly hacks to make sure that
// C++ actually initializes the containers.  Do not try to "clean
// up" or optimize the code by removing them, because it'll just
// cause some weird breakages.

class MID
{
public:
	typedef std::set<std::string> IDMap;

	MID();
	~MID();

	const std::string* lookup(const std::string& name);
	const std::string* create(const std::string& name);

	const IDMap& get_all() const { return id_map; }

private:
	IDMap id_map;
};

// must be inherited
template <typename tag>
class BaseID
{
private:
	explicit BaseID(const std::string* s_id) : id(s_id) {}

public:
	BaseID() : id(NULL) {}
	BaseID(const BaseID<tag>& s_id) : id(s_id.id) {}

	bool valid() const { return id != 0; }

	static BaseID<tag> lookup(const std::string& idname) { return BaseID<tag>(get_manager().lookup(idname)); }
	static BaseID<tag> create(const std::string& idname) { return BaseID<tag>(get_manager().create(idname)); }
	std::string name() const { return id != NULL ? *id : std::string(); }
	static std::string nameof(BaseID<tag> id) { return id.name(); }

	static const MID::IDMap& get_all() { return get_manager().get_all(); }

	bool operator< (const BaseID<tag>& cmp) const { return id < cmp.id; }
	bool operator== (const BaseID<tag>& cmp) const { return id == cmp.id; }

protected:
	const std::string* id;

	static MID& get_manager();
};

template <typename tag>
MID&
BaseID<tag>::get_manager()
{
	static MID manager;
	return manager;
}

#define DECLARE_IDMAP(name) \
	struct idmap_tag_ ## name ## _t {}; \
	class name##ID : public BaseID<idmap_tag_ ## name ## _t> { \
		public: \
		typedef BaseID<idmap_tag_##name##_t> base_t; \
		name##ID () : base_t() {} \
		name##ID (const base_t& s_id) : base_t(s_id) {} \
		name##ID (const name##ID& s_id) : base_t(s_id) {} \
	};

#endif
