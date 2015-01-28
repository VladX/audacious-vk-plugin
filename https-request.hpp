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

#include "vk-plugin.hpp"
#include <libsoup/soup.h>

namespace VK {

class HttpsRequest {
private:
	static SoupSession * session;
	SoupRequestHTTP * request;
	
	void initSession ();
	static void readyCallback (SoupSession *, SoupMessage *, gpointer);
	
public:
	typedef void (* ReadyFn) (bool, const char *, size_t, void *);
	
	const gchar * error;
	ReadyFn callback;
	void * opaque;
	
	HttpsRequest (const char *, ReadyFn, void *);
	inline HttpsRequest (HttpsRequest * clone) { * this = * clone; }
	inline const gchar * getError () const { return error; }
	void send ();
};

};
