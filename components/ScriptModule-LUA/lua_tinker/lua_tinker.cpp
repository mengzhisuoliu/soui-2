﻿// lua_tinker.cpp
//
// LuaTinker - Simple and light C++ wrapper for Lua.
//
// Copyright (c) 2005-2007 Kwon-il Lee (zupet@hitel.net)
// 
// please check Licence.txt file for licence and legal issues. 


#include <iostream>
#include "lua_tinker.h"

#if defined(_MSC_VER)
#define I64_FMT "I64"
#elif defined(__APPLE__) 
#define I64_FMT "q"
#else
#define I64_FMT "ll"
#endif

#include <stack>
static std::stack<lua_State *> sCallerState;
lua_tinker::SaveState::SaveState(lua_State *L){
	sCallerState.push(L);
}
lua_tinker::SaveState::~SaveState(){
	sCallerState.pop();
}

lua_State * lua_tinker::get_state(){
	if(sCallerState.size()==0)
		return NULL;
	return sCallerState.top();
}


/*---------------------------------------------------------------------------*/
/* excution                                                                  */
/*---------------------------------------------------------------------------*/
void lua_tinker::dofile ( lua_State *L, const char *filename )
{
   lua_pushcclosure ( L, on_error, 0 );
   int errfunc = lua_gettop ( L );

   if ( luaL_loadfile ( L, filename ) == 0 )
   {
      lua_pcall ( L, 0, 1, errfunc );
   }
   else
   {
      print_error ( L, "%s", lua_tostring ( L, -1 ) );
   }

   lua_remove ( L, errfunc );
   lua_pop ( L, 1 );
}

/*---------------------------------------------------------------------------*/
void lua_tinker::dostring ( lua_State *L, const char* buff )
{
   lua_tinker::dobuffer ( L, buff, strlen ( buff ) );
}

/*---------------------------------------------------------------------------*/
void lua_tinker::dobuffer ( lua_State *L, const char* buff, size_t len )
{
   lua_pushcclosure ( L, on_error, 0 );
   int errfunc = lua_gettop ( L );

   if ( luaL_loadbuffer ( L, buff, len, "lua_tinker::dobuffer()" ) == 0 )
   {
      lua_pcall ( L, 0, 1, errfunc );
   }
   else
   {
      print_error ( L, "%s", lua_tostring ( L, -1 ) );
   }

   lua_remove ( L, errfunc );
   lua_pop ( L, 1 );
}

/*---------------------------------------------------------------------------*/
/* debug helpers                                                             */
/*---------------------------------------------------------------------------*/
static void call_stack ( lua_State* L, int n )
{
   lua_Debug ar;
   if ( lua_getstack ( L, n, &ar ) == 1 )
   {
      lua_getinfo ( L, "nSlu", &ar );

      const char* indent;
      if ( n == 0 )
      {
         indent = "->\t";
         lua_tinker::print_error ( L, "\t<call stack>" );
      }
      else
      {
         indent = "\t";
      }

      if ( ar.name )
         lua_tinker::print_error ( L, "%s%s() : line %d [%s : line %d]", indent, ar.name, ar.currentline, ar.source, ar.linedefined );
      else
         lua_tinker::print_error ( L, "%sunknown : line %d [%s : line %d]", indent, ar.currentline, ar.source, ar.linedefined );

      call_stack ( L, n + 1 );
   }
}

/*---------------------------------------------------------------------------*/
int lua_tinker::on_error ( lua_State *L )
{
   print_error ( L, "%s", lua_tostring ( L, -1 ) );

   call_stack ( L, 0 );

   return 0;
}

static FunPrint s_funPrint = NULL;
/*---------------------------------------------------------------------------*/
void lua_tinker::print_error ( lua_State *L, const char* fmt, ... )
{
   char text[4096];

   va_list args;
   va_start ( args, fmt );
   vsnprintf ( text, sizeof ( text ), fmt, args );
   va_end ( args );
   if(s_funPrint){
	   s_funPrint(text);
   }else{
	   lua_getglobal ( L, "_ALERT" );
	   if ( lua_isfunction ( L, -1 ) )
	   {
		  lua_pushstring ( L, text );
		  lua_call ( L, 1, 0 );
	   }
	   else
	   {
		  printf ( "%s\n", text );
		  lua_pop ( L, 1 );
	   }
   }
}


void lua_tinker::set_print_callback(FunPrint fun)
{
    s_funPrint=fun;
}

/*---------------------------------------------------------------------------*/
void lua_tinker::enum_stack ( lua_State *L )
{
   int top = lua_gettop ( L );
   print_error ( L, "%s", "----------stack----------" );
   print_error ( L, "Type:%d", top );
   for ( int i = 1; i <= lua_gettop ( L ); ++i )
   {
      switch ( lua_type ( L, i ) )
      {
      case LUA_TNIL:
         print_error ( L, "\t%s", lua_typename ( L, lua_type ( L, i ) ) );
         break;
      case LUA_TBOOLEAN:
         print_error ( L, "\t%s    %s", lua_typename ( L, lua_type ( L, i ) ), lua_toboolean ( L, i ) ? "true" : "false" );
         break;
      case LUA_TLIGHTUSERDATA:
         print_error ( L, "\t%s    0x%08p", lua_typename ( L, lua_type ( L, i ) ), lua_topointer ( L, i ) );
         break;
      case LUA_TNUMBER:
         print_error ( L, "\t%s    %f", lua_typename ( L, lua_type ( L, i ) ), lua_tonumber ( L, i ) );
         break;
      case LUA_TSTRING:
         print_error ( L, "\t%s    %s", lua_typename ( L, lua_type ( L, i ) ), lua_tostring ( L, i ) );
         break;
      case LUA_TTABLE:
         print_error ( L, "\t%s    0x%08p", lua_typename ( L, lua_type ( L, i ) ), lua_topointer ( L, i ) );
         break;
      case LUA_TFUNCTION:
         print_error ( L, "\t%s()  0x%08p", lua_typename ( L, lua_type ( L, i ) ), lua_topointer ( L, i ) );
         break;
      case LUA_TUSERDATA:
         print_error ( L, "\t%s    0x%08p", lua_typename ( L, lua_type ( L, i ) ), lua_topointer ( L, i ) );
         break;
      case LUA_TTHREAD:
         print_error ( L, "\t%s", lua_typename ( L, lua_type ( L, i ) ) );
         break;
      }
   }
   print_error ( L, "%s", "-------------------------" );
}

/*---------------------------------------------------------------------------*/
/* read                                                                      */
/*---------------------------------------------------------------------------*/
template<>
char* lua_tinker::read ( lua_State *L, int index )
{
   return ( char* ) lua_tostring ( L, index );
}

template<>
const char* lua_tinker::read ( lua_State *L, int index )
{
   return ( const char* ) lua_tostring ( L, index );
}

template<>
char lua_tinker::read ( lua_State *L, int index )
{
   return ( char ) lua_tonumber ( L, index );
}

template<>
unsigned char lua_tinker::read ( lua_State *L, int index )
{
   return ( unsigned char ) lua_tonumber ( L, index );
}

template<>
short lua_tinker::read ( lua_State *L, int index )
{
   return ( short ) lua_tonumber ( L, index );
}

template<>
unsigned short lua_tinker::read ( lua_State *L, int index )
{
   return ( unsigned short ) lua_tonumber ( L, index );
}

// long is different between i386 and X86_64 architecture
#if defined(__X86_64__) || defined(__X86_64) || defined(__amd_64) || defined(__amd_64__)
template<>
long lua_tinker::read(lua_State *L, int index)
{
   if(lua_isnumber(L,index))
      return (long)lua_tonumber(L, index);
   else
      return *(long*)lua_touserdata(L, index);
}

template<>
unsigned long lua_tinker::read(lua_State *L, int index)
{
   if(lua_isnumber(L,index))
      return (unsigned long)lua_tonumber(L, index);
   else
      return *( unsigned long* ) lua_touserdata ( L, index );
}

template<>
HANDLE lua_tinker::read ( lua_State *L, int index )
{
	if(lua_isnumber(L,index))
		return (HANDLE)(UINT)lua_tonumber(L, index);
	else
		return *( HANDLE* ) lua_touserdata ( L, index );
}
template<>
HWND lua_tinker::read(lua_State *L, int index)
{
	if(lua_isnumber(L,index))
		return (HWND)(UINT)lua_tonumber(L, index);
	else
		return *(HWND*)lua_touserdata(L, index);
}

template<>
HMENU lua_tinker::read(lua_State *L, int index)
{
	if(lua_isnumber(L,index))
		return (HMENU)(UINT)lua_tonumber(L, index);
	else
		return *(HMENU*)lua_touserdata(L, index);
}


template<>
HDC lua_tinker::read(lua_State *L, int index)
{
	if(lua_isnumber(L,index))
		return (HDC)(UINT)lua_tonumber(L, index);
	else
		return *(HDC*)lua_touserdata(L, index);
}

template<>
HICON lua_tinker::read(lua_State *L, int index)
{
	if(lua_isnumber(L,index))
		return (HICON)(UINT)lua_tonumber(L, index);
	else
		return *(HICON*)lua_touserdata(L, index);
}

template<>
HBITMAP lua_tinker::read(lua_State *L, int index)
{
	if(lua_isnumber(L,index))
		return (HBITMAP)(UINT)lua_tonumber(L, index);
	else
		return *(HBITMAP*)lua_touserdata(L, index);
}

template<>
HINSTANCE lua_tinker::read(lua_State *L, int index)
{
	if(lua_isnumber(L,index))
		return (HINSTANCE)(UINT)lua_tonumber(L, index);
	else
		return *(HINSTANCE*)lua_touserdata(L, index);
}

#else //__i386__ //32bit
template<>
long lua_tinker::read ( lua_State *L, int index )
{
   return ( long ) lua_tonumber ( L, index );
}

template<>
unsigned long lua_tinker::read ( lua_State *L, int index )
{
   return ( unsigned long ) lua_tonumber ( L, index );
}

#ifdef WIN32
template<>
HANDLE lua_tinker::read ( lua_State *L, int index )
{
	return ( HANDLE )(UINT_PTR) ( UINT )lua_tonumber ( L, index );
}

template<>
HWND lua_tinker::read(lua_State *L, int index)
{
	return (HWND)(UINT_PTR)(UINT)lua_tonumber(L, index);
}

template<>
HMENU lua_tinker::read(lua_State *L, int index)
{
	return (HMENU)(UINT_PTR)(UINT)lua_tonumber(L, index);
}

template<>
HDC lua_tinker::read(lua_State *L, int index)
{
	return (HDC)(UINT_PTR)(UINT)lua_tonumber(L, index);
}

template<>
HICON lua_tinker::read(lua_State *L, int index)
{
	return (HICON)(UINT_PTR)(UINT)lua_tonumber(L, index);
}

template<>
HBITMAP lua_tinker::read(lua_State *L, int index)
{
	return (HBITMAP)(UINT_PTR)(UINT)lua_tonumber(L, index);
}
#else
template<>
HGDIOBJ lua_tinker::read(lua_State *L, int index)
{
	return (HGDIOBJ)(UINT_PTR)(UINT)lua_tonumber(L, index);
}
#endif

template<>
HINSTANCE lua_tinker::read(lua_State *L, int index)
{
	if(lua_isnumber(L,index))
		return (HINSTANCE)(UINT_PTR)(UINT)lua_tonumber(L, index);
   else if(lua_isnil(L, index))
      return nullptr;
	else
		return *(HINSTANCE*)lua_touserdata(L, index);
}

#endif

template<>
int lua_tinker::read ( lua_State *L, int index )
{
   return ( int ) lua_tonumber ( L, index );
}

template<>
unsigned int lua_tinker::read ( lua_State *L, int index )
{
   return ( unsigned int ) lua_tonumber ( L, index );
}

template<>
float lua_tinker::read ( lua_State *L, int index )
{
   return ( float ) lua_tonumber ( L, index );
}

template<>
double lua_tinker::read ( lua_State *L, int index )
{
   return ( double ) lua_tonumber ( L, index );
}

template<>
bool lua_tinker::read ( lua_State *L, int index )
{
   if ( lua_isboolean ( L, index ) )
      return lua_toboolean ( L, index ) != 0;
   else
      return lua_tonumber ( L, index ) != 0;
}

template<>
void lua_tinker::read ( lua_State *L, int index )
{
   ( void ) L;
   ( void ) index;
   return;
}

template<>
long long lua_tinker::read ( lua_State *L, int index )
{
   if ( lua_isinteger ( L, index ) )
   {
      return ( long long ) lua_tointeger ( L, index );
   }
   if ( lua_isnumber ( L, index ) )
      return ( long long ) lua_tonumber ( L, index );
   else
      return *( long long* ) lua_touserdata ( L, index );
}
template<>
unsigned long long lua_tinker::read ( lua_State *L, int index )
{
   if ( lua_isinteger ( L, index ) )
   {
      return (unsigned long long ) lua_tointeger ( L, index );
   }
   if ( lua_isnumber ( L, index ) )
      return ( unsigned long long )lua_tonumber ( L, index );
   else
      return *( unsigned long long* )lua_touserdata ( L, index );
}

template<>
lua_tinker::table lua_tinker::read ( lua_State *L, int index )
{
   return table ( L, index );
}

/*---------------------------------------------------------------------------*/
/* push                                                                      */
/*---------------------------------------------------------------------------*/
template<>
void lua_tinker::push ( lua_State *L, char ret )
{
   lua_pushnumber ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, unsigned char ret )
{
   lua_pushnumber ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, short ret )
{
   lua_pushnumber ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, unsigned short ret )
{
   lua_pushnumber ( L, ret );
}

#if defined(__X86_64__) || defined(__X86_64) || defined(__amd_64) || defined(__amd_64__)
template<>
void lua_tinker::push(lua_State *L, long ret)
{
   *(long*)lua_newuserdata(L, sizeof(long)) = ret;
   lua_getglobal(L, "__s64");
   lua_setmetatable(L, -2);
}

template<>
void lua_tinker::push(lua_State *L, unsigned long ret)
{
   *( unsigned long* ) lua_newuserdata ( L, sizeof ( unsigned long ) ) = ret;
   lua_getglobal ( L, "__u64" );
   lua_setmetatable ( L, -2 );
}

template<>
void lua_tinker::push(lua_State *L, HANDLE ret)
{
	*( HANDLE* ) lua_newuserdata ( L, sizeof ( HANDLE ) ) = ret;
	lua_getglobal ( L, "__hdl" );
	lua_setmetatable ( L, -2 );
}

template<>
void lua_tinker::push(lua_State *L, HWND ret)
{
	*(HWND*)lua_newuserdata(L, sizeof(HWND)) = ret;
	lua_getglobal(L,"__hwnd");
	lua_setmetatable(L, -2);
}

template<>
void lua_tinker::push(lua_State *L, HMENU ret)
{
	*(HMENU*)lua_newuserdata(L, sizeof(HMENU)) = ret;
	lua_getglobal(L,"__hmenu");
	lua_setmetatable(L, -2);
}

template<>
void lua_tinker::push(lua_State *L, HDC ret)
{
	*(HDC*)lua_newuserdata(L, sizeof(HDC)) = ret;
	lua_getglobal(L,"__hdc");
	lua_setmetatable(L, -2);
}

template<>
void lua_tinker::push(lua_State *L, HICON ret)
{
	*(HICON*)lua_newuserdata(L, sizeof(HICON)) = ret;
	lua_getglobal(L,"__hicon");
	lua_setmetatable(L, -2);
}

template<>
void lua_tinker::push(lua_State *L, HBITMAP ret)
{
	*(HBITMAP*)lua_newuserdata(L, sizeof(HBITMAP)) = ret;
	lua_getglobal(L,"__hbmp");
	lua_setmetatable(L, -2);
}

template<>
void lua_tinker::push(lua_State *L, HINSTANCE ret)
{
	*(HINSTANCE*)lua_newuserdata(L, sizeof(HINSTANCE)) = ret;
	lua_getglobal(L,"__hinst");
	lua_setmetatable(L, -2);
}

#else //__i386__ 
template<>
void lua_tinker::push ( lua_State *L, long ret )
{
   lua_pushnumber ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, unsigned long ret )
{
   lua_pushnumber ( L, ret );
}

#ifdef WIN32
template<>
void lua_tinker::push ( lua_State *L, HANDLE ret )
{
	lua_pushnumber ( L, ( UINT )(UINT_PTR)ret );
}

template<>
void lua_tinker::push(lua_State *L, HWND ret)
{
	lua_pushnumber ( L, ( UINT )(UINT_PTR)ret );
}

template<>
void lua_tinker::push(lua_State *L, HMENU ret)
{
	lua_pushnumber ( L, ( UINT )(UINT_PTR)ret );
}

template<>
void lua_tinker::push(lua_State *L, HDC ret)
{
	lua_pushnumber ( L, ( UINT )(UINT_PTR)ret );
}

template<>
void lua_tinker::push(lua_State *L, HICON ret)
{
	lua_pushnumber ( L, ( UINT )(UINT_PTR)ret );
}

template<>
void lua_tinker::push(lua_State *L, HBITMAP ret)
{
	lua_pushnumber ( L, ( UINT )(UINT_PTR)ret );
}
#else
template<>
void lua_tinker::push(lua_State *L, HGDIOBJ ret)
{
	lua_pushnumber ( L, ( UINT )(UINT_PTR)ret );
}
#endif

template<>
void lua_tinker::push(lua_State *L, HINSTANCE ret)
{
	lua_pushnumber ( L, ( UINT )(UINT_PTR)ret );
}

#endif

template<>
void lua_tinker::push ( lua_State *L, int ret )
{
   lua_pushnumber ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, unsigned int ret )
{
   lua_pushnumber ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, float ret )
{
   lua_pushnumber ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, double ret )
{
   lua_pushnumber ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, char* ret )
{
   lua_pushstring ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, const char* ret )
{
   lua_pushstring ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, bool ret )
{
   lua_pushboolean ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, lua_value* ret )
{
   if ( ret ) ret->to_lua ( L ); else lua_pushnil ( L );
}

template<>
void lua_tinker::push ( lua_State *L, long long ret )
{
   //    *(long long*)lua_newuserdata(L, sizeof(long long)) = ret;
   //    lua_getglobal(L, "__s64");
   //    lua_setmetatable(L, -2);
   lua_pushinteger ( L, ret );
}
template<>
void lua_tinker::push ( lua_State *L, unsigned long long ret )
{
  // *( unsigned long long* )lua_newuserdata ( L, sizeof ( unsigned long long ) ) = ret;
   //lua_getglobal ( L, "__u64" );
   //lua_setmetatable ( L, -2 );
   lua_pushinteger ( L, ret );
}

template<>
void lua_tinker::push ( lua_State *L, lua_tinker::table ret )
{
   lua_pushvalue ( L, ret.m_obj->m_index );
}

/*---------------------------------------------------------------------------*/
/* pop                                                                       */
/*---------------------------------------------------------------------------*/
template<>
void lua_tinker::pop ( lua_State *L )
{
   lua_pop ( L, 1 );
}

template<>
lua_tinker::table lua_tinker::pop ( lua_State *L )
{
   return table ( L, lua_gettop ( L ) );
}

/*---------------------------------------------------------------------------*/
/* Tinker Class Helper                                                       */
/*---------------------------------------------------------------------------*/
static void invoke_parent ( lua_State *L )
{
   lua_pushstring ( L, "__parent" );
   lua_rawget ( L, -2 );
   if ( lua_istable ( L, -1 ) )
   {
      lua_pushvalue ( L, 2 );
      lua_rawget ( L, -2 );
      if ( !lua_isnil ( L, -1 ) )
      {
         lua_remove ( L, -2 );
      }
      else
      {
         lua_remove ( L, -1 );
         invoke_parent ( L );
         lua_remove ( L, -2 );
      }
   }
}

/*---------------------------------------------------------------------------*/
int lua_tinker::meta_get ( lua_State *L )
{
   // 传入表 和 索引参数
   // stack: 1.类(userdata) 2.变量(string) 
   lua_getmetatable ( L, 1 );
   // stack: 1.类(userdata) 2.变量(string) 3.meta(table)
   lua_pushvalue ( L, 2 );
   // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.变量(string)
   lua_rawget ( L, -2 );
   // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.meta[变量]value值(userdata)

   // 如果存在userdata 存在该变量
   if ( lua_isuserdata ( L, -1 ) )
   {
      user2type<var_base*>::invoke ( L, -1 )->get ( L );
      // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.meta[变量]value值(userdata) 5.实际值
      lua_remove ( L, -2 );
      // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.实际值
   }
   else if ( lua_isnil ( L, -1 ) )
   {
      // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.nil
      lua_remove ( L, -1 );
      // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 
      invoke_parent ( L );
      // fix bug by fergus
      // 调用父类也需调用get
      if ( lua_isuserdata ( L, -1 ) )
      {
         // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.父类中的变量(userdata) 
         user2type<var_base*>::invoke ( L, -1 )->get ( L );
         // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.父类中的变量(userdata) 5.实际值
         lua_remove ( L, -2 );
         // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.实际值
      }
      else if ( lua_isnil ( L, -1 ) )
      {
         // stack: 1.类(userdata) 2.变量(string) 3.meta(table) 4.nil
         lua_pushfstring ( L, "can't find '%s' class variable. (forgot registering class variable ?)", lua_tostring ( L, 2 ) );
         lua_error ( L );
      }
   }

   lua_remove ( L, -2 );
   // stack: 1.类(userdata) 2.变量(string) 3.实际值

   return 1;
}

/*---------------------------------------------------------------------------*/
int lua_tinker::meta_set ( lua_State *L )
{
   // stack: 1.类(userdata) 2.变量(string) 3.要赋的值
   lua_getmetatable ( L, 1 );
   // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table)
   lua_pushvalue ( L, 2 );
   // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table) 5.变量(string)
   lua_rawget ( L, -2 );
   // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table) 5.meta[变量](userdata mem_var指针)

   if ( lua_isuserdata ( L, -1 ) )
   {
      user2type<var_base*>::invoke ( L, -1 )->set ( L );
      // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table) 5.meta[变量](userdata mem_var指针)
   }
   else if ( lua_isnil ( L, -1 ) )
   {
      // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table) 5.nil
      lua_remove ( L, -1 );
      // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table)
      lua_pushvalue ( L, 2 );
      // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table) 5.变量(string)
      lua_pushvalue ( L, 4 );
      // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table) 5.变量(string) 6.类meta(table)
      invoke_parent ( L );
      // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table) 5.变量(string) 6.类meta(table) 7.获取到父类的变量(userdata mem_var指针)
      if ( lua_isuserdata ( L, -1 ) )
      {
         user2type<var_base*>::invoke ( L, -1 )->set ( L );
      }
      else if ( lua_isnil ( L, -1 ) )
      {
         // stack: 1.类(userdata) 2.变量(string) 3.要赋的值 4.类meta(table) 5.变量(string) 6.类meta(table) 7.nil
         lua_pushfstring ( L, "can't find '%s' class variable. (forgot registering class variable ?)", lua_tostring ( L, 2 ) );
         lua_error ( L );
      }
   }
   lua_settop ( L, 3 );
   // stack: 1.类(userdata) 2.变量(string) 3.要赋的值
   return 0;
}

/*---------------------------------------------------------------------------*/
void lua_tinker::push_meta ( lua_State *L, const char* name )
{
   lua_getglobal ( L, name );
}

/*---------------------------------------------------------------------------*/
/* table object on stack                                                     */
/*---------------------------------------------------------------------------*/
lua_tinker::table_obj::table_obj ( lua_State* L, int index )
   :m_L ( L )
   , m_index ( index )
   , m_ref ( 0 )
{
   if ( lua_isnil ( m_L, m_index ) )
   {
      m_pointer = NULL;
      lua_remove ( m_L, m_index );
   }
   else
   {
      m_pointer = lua_topointer ( m_L, m_index );
   }
}

lua_tinker::table_obj::~table_obj ( )
{
   if ( validate ( ) )
   {
      lua_remove ( m_L, m_index );
   }
}

void lua_tinker::table_obj::inc_ref ( )
{
   ++m_ref;
}

void lua_tinker::table_obj::dec_ref ( )
{
   if ( --m_ref == 0 )
      delete this;
}

bool lua_tinker::table_obj::validate ( )
{
   if ( m_pointer != NULL )
   {
      if ( m_pointer == lua_topointer ( m_L, m_index ) )
      {
         return true;
      }
      else
      {
         int top = lua_gettop ( m_L );

         for ( int i = 1; i <= top; ++i )
         {
            if ( m_pointer == lua_topointer ( m_L, i ) )
            {
               m_index = i;
               return true;
            }
         }

         m_pointer = NULL;
         return false;
      }
   }
   else
   {
      return false;
   }
}

/*---------------------------------------------------------------------------*/
/* Table Object Holder                                                       */
/*---------------------------------------------------------------------------*/
lua_tinker::table::table ( lua_State* L )
{
   lua_newtable ( L );

   m_obj = new table_obj ( L, lua_gettop ( L ) );

   m_obj->inc_ref ( );
}

lua_tinker::table::table ( lua_State* L, const char* name )
{
   lua_getglobal ( L, name );

   if ( lua_istable ( L, -1 ) == 0 )
   {
      lua_pop ( L, 1 );

      lua_newtable ( L );
      lua_setglobal ( L, name );
      lua_getglobal ( L, name );
   }

   m_obj = new table_obj ( L, lua_gettop ( L ) );

   m_obj->inc_ref ( );
}

lua_tinker::table::table ( lua_State* L, int index )
{
   if ( index < 0 )
   {
      index = lua_gettop ( L ) + index + 1;
   }

   m_obj = new table_obj ( L, index );

   m_obj->inc_ref ( );
}

lua_tinker::table::table ( const table& input )
{
   m_obj = input.m_obj;

   m_obj->inc_ref ( );
}

lua_tinker::table::~table ( )
{
   m_obj->dec_ref ( );
}

/*---------------------------------------------------------------------------*/
