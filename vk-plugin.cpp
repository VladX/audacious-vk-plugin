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

#include "vk-plugin.hpp"
#include "https-request.hpp"
#include "json11.hpp"

namespace VK {

const unsigned kAppId = 4747736; // this is required for vk.com API
const char kName[] = "VK.com Plugin";
const char kDomain[] = "vk-plugin";
const char kAbout[] = "VK.com Plugin for Audacious\nCopyright © 2015 Vlad Samsonov";
const char * const PACKAGE = kDomain;
constexpr const char kHttpAuthFormat[] = "https://oauth.vk.com/authorize?"
"client_id=%u&" // APP_ID
"scope=%u&" // PERMISSIONS
"redirect_uri=https://oauth.vk.com/blank.html&" // REDIRECT_URI
"display=page&" // DISPLAY
"v=5.27&" // API_VERSION
"response_type=token";

enum { // incomplete... plugin only needs audio & offline access though
	PERM_AUDIO = 8,
	PERM_VIDEO = 16,
	PERM_STATUS = 1024,
	PERM_MESSAGES = 4096,
	PERM_OFFLINE = 65536
};

static ConfigAccessor * configAccessorInstance = nullptr;
static ErrorHandlerFn error_handler = nullptr;
static GtkWidget * box = nullptr;
static GtkWidget * entry = nullptr;
static char authUri[ARRAY_LENGTH(kHttpAuthFormat) + 24];
static std::string accessToken;

inline static void error (const char * fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	if (error_handler)
		error_handler(StringFormatter(fmt, ap).str());
	va_end(ap);
}

void set_config_accessor (ConfigAccessor * ac) {
	configAccessorInstance = ac;
}

void set_error_handler (ErrorHandlerFn handler) {
	error_handler = handler;
}

inline static void set_auth_uri () {
	snprintf(authUri, ARRAY_LENGTH(authUri), kHttpAuthFormat, kAppId, PERM_AUDIO | PERM_OFFLINE);
}

static void copy_to_clipboard (GtkWidget * widget, gpointer) {
	gtk_clipboard_set_text(gtk_widget_get_clipboard(widget, GDK_SELECTION_CLIPBOARD), authUri, -1);
}

static void update_token () {
	if (!configAccessorInstance)
		return;
	if (!configAccessorInstance->get("vk-access-token", accessToken))
		return;
	gtk_entry_set_text(GTK_ENTRY(entry), accessToken.c_str());
}

static gboolean update_token (GtkWidget * widget, GdkEvent *, gpointer) {
	accessToken = gtk_entry_get_text(GTK_ENTRY(widget));
	auto pos = accessToken.find("access_token=");
	if (pos != accessToken.npos) { // it seems like user instead of copying only token copied the whole URI... let's remove unnecessary parts
		accessToken.erase(0, pos + strlen("access_token="));
		gtk_entry_set_text(GTK_ENTRY(widget), accessToken.c_str());
	}
	pos = accessToken.find('&');
	if (pos != accessToken.npos) {
		accessToken.erase(pos);
		gtk_entry_set_text(GTK_ENTRY(widget), accessToken.c_str());
	}
	configAccessorInstance->set("vk-access-token", accessToken);
	return FALSE;
}

GtkWidget * get_widget () {
	if (box) {
		update_token();
		return box;
	}
	set_auth_uri();
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	GtkWidget * linkButton = gtk_link_button_new_with_label(authUri, _("Get access token"));
	GtkWidget * copyButton = gtk_button_new_with_label(_("Copy link to clipboard"));
	GtkWidget * tipLabel = gtk_label_new(_("1. Press link below to get access token.\n2. Allow this application to access to your account.\n3. Copy access token from the address bar to the entry above."));
	entry = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 64);
	gtk_entry_set_placeholder_text(GTK_ENTRY(entry), _("Type access token here..."));
	gtk_widget_set_halign(copyButton, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(tipLabel, GTK_ALIGN_START);
	gtk_box_pack_end(GTK_BOX(vbox), linkButton, TRUE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), copyButton, TRUE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), tipLabel, TRUE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), entry, TRUE, FALSE, 10);
	gtk_box_pack_end(GTK_BOX(box), vbox, TRUE, FALSE, 10);
	g_signal_connect(copyButton, "clicked", G_CALLBACK(copy_to_clipboard), nullptr);
	g_signal_connect(entry, "key-release-event", G_CALLBACK(static_cast<gboolean (*)(GtkWidget *, GdkEvent *, gpointer)>(update_token)), nullptr);
	update_token();
	return box;
}

static json11::Json parse_response (const char * response) {
	std::string err;
	auto dom = json11::Json::parse(response, err);
	if (!err.empty())
		throw StringFormatter(_("Failed to parse response: %s"), err.c_str());
	if (!dom["error"].is_null())
		throw StringFormatter("%s", dom["error"]["error_msg"].string_value().c_str());
	dom = dom["response"];
	if (dom.is_null() || !dom.is_array())
		throw StringFormatter(_("Failed to retrieve playlists from server: incorrect response"));
	return dom;
}

static void songs_response (bool errorFlag, const char * response, size_t, void * opaque) {
	ResponseState * state = (ResponseState *) opaque;
	if (errorFlag) {
		error(_("Failed to retrieve songs from server"));
		delete state;
		return;
	}
	json11::Json dom;
	try {
		dom = parse_response(response);
	}
	catch (const StringFormatter & e) {
		error("%s", e.str());
		return;
	}
	state->playlist->reserve(dom.array_items().size());
	for (auto song : dom.array_items())
		state->playlist->emplace_back(song["title"].string_value(), song["artist"].string_value(), song["url"].string_value(), song["duration"].int_value());
	state->inc();
	if (state->isComplete())
		state->readyCb(* state->playlists);
	delete state;
}

static void playlists_response (bool errorFlag, const char * response, size_t, void * opaque) {
	if (errorFlag) {
		error(_("Failed to retrieve playlists from server"));
		return;
	}
	json11::Json dom;
	try {
		dom = parse_response(response);
	}
	catch (const StringFormatter & e) {
		error("%s", e.str());
		return;
	}
	auto pl = dom.array_items();
	auto state = new ResponseState(pl.size(), (PlaylistsReadyFn) opaque);
	state->playlists->reserve(pl.size());
	state->playlists->emplace_back(0, _("My music"));
	state->playlist = &state->playlists->at(0);
	configAccessorInstance->get("vk-access-token", accessToken);
	HttpsRequest(StringFormatter("https://api.vk.com/method/audio.get?access_token=%s&need_user=0", accessToken.c_str()).str(), songs_response, state).send();
	for (size_t i = 1; i < pl.size(); ++i) {
		state->playlists->emplace_back(pl[i]["album_id"].int_value(), pl[i]["title"].string_value());
		HttpsRequest(StringFormatter("https://api.vk.com/method/audio.get?access_token=%s&need_user=0&album_id=%lu", accessToken.c_str(), state->playlists->at(i).getAlbumId()).str(), songs_response, new ResponseState(state, &state->playlists->at(i))).send();
	}
}

/*
This will request playlists from server and call it's argument. Callback will be called asynchronously.
It's safe to call this as many times as needed. Request is made for every call, and for every call there is a separate array of playlists.
*/
void request_user_playlists (PlaylistsReadyFn cb) {
	configAccessorInstance->get("vk-access-token", accessToken);
	HttpsRequest(StringFormatter("https://api.vk.com/method/audio.getAlbums?access_token=%s", accessToken.c_str()).str(), playlists_response, (void *) cb).send();
}

};
