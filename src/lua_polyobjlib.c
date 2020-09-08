// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Iestyn "Monster Iestyn" Jealous.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_polyobjlib.c
/// \brief polyobject library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "p_local.h"
#include "p_polyobj.h"
#include "lua_script.h"
#include "lua_libs.h"

enum polyobj_e {
	polyobj_valid = 0,
};
static const char *const polyobj_opt[] = {
	"valid",
	NULL};

static int polyobj_get(lua_State *L)
{
	polyobj_t *polyobj = *((polyobj_t **)luaL_checkudata(L, 1, META_POLYOBJ));
	enum polyobj_e field = luaL_checkoption(L, 2, NULL, polyobj_opt);

	if (!polyobj) {
		if (field == polyobj_valid) {
			lua_pushboolean(L, false);
			return 1;
		}
		return LUA_ErrInvalid(L, "polyobj_t");
	}

	switch (field)
	{
	case polyobj_valid:
		lua_pushboolean(L, true);
		break;
	}
	return 1;
};

static int polyobj_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("polyobj_t") " struct cannot be edited by Lua."); // this is just temporary
}

static int lib_iteratePolyObjects(lua_State *L)
{
	INT32 i = -1;
	if (lua_gettop(L) < 2)
	{
		//return luaL_error(L, "Don't call PolyObjects.iterate() directly, use it as 'for polyobj in PolyObjects.iterate do <block> end'.");
		lua_pushcfunction(L, lib_iteratePolyObjects);
		return 1;
	}
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (INT32)(*((polyobj_t **)luaL_checkudata(L, 1, META_POLYOBJ)) - PolyObjects);
	for (i++; i < numPolyObjects; i++)
	{
		LUA_PushUserdata(L, &PolyObjects[i], META_POLYOBJ);
		return 1;
	}
	return 0;
}

static int lib_getPolyObject(lua_State *L)
{
	const char *field;
	INT32 i;

	// find PolyObject by number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= numPolyObjects)
			return luaL_error(L, "PolyObjects[] index %d out of range (0 - %d)", i, numPolyObjects-1);
		LUA_PushUserdata(L, &PolyObjects[i], META_POLYOBJ);
		return 1;
	}

	field = luaL_checkstring(L, 2);
	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iteratePolyObjects);
		return 1;
	}
	return 0;
}

static int lib_numPolyObjects(lua_State *L)
{
	lua_pushinteger(L, numPolyObjects);
	return 1;
}

int LUA_PolyObjLib(lua_State *L)
{
	luaL_newmetatable(L, META_POLYOBJ);
		lua_pushcfunction(L, polyobj_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, polyobj_set);
		lua_setfield(L, -2, "__newindex");

		//lua_pushcfunction(L, polyobj_num);
		//lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getPolyObject);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numPolyObjects);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "PolyObjects");
	return 0;
}