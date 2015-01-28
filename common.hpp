/*
 * VK.com plugin for Audacious
 * Copyright (C) 2015 Vlad Samsonov <vvladxx@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <gtk/gtk.h>
#include <audacious/i18n.h>

#define ARRAY_LENGTH(A) (sizeof(A) / sizeof(A[0]))

class StringFormatter {
private:
	char * msg;
	char buf[512];
	
	void * operator new (size_t size) throw (std::bad_alloc) = delete; // this class is intended for stack allocations only
	void * operator new[] (size_t size) throw (std::bad_alloc) = delete;
	
public:
	inline StringFormatter (const char * fmt, va_list ap) {
		va_list aq;
		va_copy(aq, ap);
		int size = vsnprintf(buf, sizeof(buf), fmt, ap); // using fast stack allocation for small strings
		if (size >= (int) sizeof(buf)) { // stack allocated buffer is too small, fall back to heap
			msg = new char[size + 1];
			vsprintf(msg, fmt, aq);
		}
		else
			msg = buf;
		va_end(aq);
	}
	
	inline StringFormatter (const char * fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		int size = vsnprintf(buf, sizeof(buf), fmt, ap); // using fast stack allocation for small strings
		if (size >= (int) sizeof(buf)) { // stack allocated buffer is too small, fall back to heap
			msg = new char[size + 1];
			va_end(ap);
			va_start(ap, fmt);
			vsprintf(msg, fmt, ap);
		}
		else
			msg = buf;
		va_end(ap);
	}
	
	inline const char * str () const { return msg; }
	
	inline ~StringFormatter () {
		if (msg != buf)
			delete[] msg;
	}
};
