/*

mime.hpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/

#ifndef MAILIO_EXPORT_H
#define MAILIO_EXPORT_H

#if defined(_WIN32)
# if defined(MAILIO_BUILDING_DLL) || defined(MAILIO_USING_DLL)
#  if defined(MAILIO_BUILDING_DLL)
#   define MAILIO_EXPORT __declspec(dllexport)
#  else
#   define MAILIO_EXPORT __declspec(dllimport)
#  endif
# else
#  define MAILIO_EXPORT 
# endif
#else
# define MAILIO_EXPORT
#endif

#endif
