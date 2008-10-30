/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_COMMON_GCMAP_H
#define SOURCEMUD_COMMON_GCMAP_H

#include "common/gcbase.h"

#include <map>

namespace GCType
{
	// GC traced version of std::map
	template <typename K, typename T>
	struct map : public std::map<const K, T, std::less<const K>, gc_allocator<std::pair<const K, T> > >, public GC { };
	//
	// GC traced version of std::multimap
	template <typename K, typename T>
	struct multimap : public std::multimap<const K, T, std::less<const K>, gc_allocator<std::pair<const K, T> > >, public GC { };
}

#endif
