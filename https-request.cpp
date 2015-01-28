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

#include "https-request.hpp"

namespace VK {

SoupSession * HttpsRequest::session = nullptr;

inline void HttpsRequest::initSession () {
	if (session)
		return;
	session = soup_session_new_with_options(SOUP_SESSION_USER_AGENT, kName, NULL);
}

HttpsRequest::HttpsRequest (const char * uri, ReadyFn cb, void * opaque) : callback(cb), opaque(opaque) {
	initSession();
	GError * error = nullptr;
	request = soup_session_request_http(session, "GET", uri, &error);
	this->error = (error) ? error->message : nullptr;
}

void HttpsRequest::readyCallback (SoupSession *, SoupMessage * msg, gpointer thisptr) {
	reinterpret_cast<HttpsRequest *>(thisptr)->callback(msg->status_code != 200, msg->response_body->data, msg->response_body->length, reinterpret_cast<HttpsRequest *>(thisptr)->opaque);
	delete reinterpret_cast<HttpsRequest *>(thisptr);
}

void HttpsRequest::send () {
	if (!request)
		return;
	soup_session_queue_message(session, soup_request_http_get_message(request), readyCallback, new HttpsRequest(this));
}

};
