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

#include "common.hpp"

namespace VK {

extern const unsigned kAppId;
extern const char kName[];
extern const char kDomain[];
extern const char kAbout[];

class ConfigAccessor {
public:
	// Stores result into @value. Returns false if given key does not exist.
	virtual bool get (const std::string & key, std::string & value) = 0;
	// Sets config to @value.
	virtual void set (const std::string & key, const std::string & value) = 0;
};

class Song {
private:
	std::string title;
	std::string artist;
	std::string url;
	unsigned duration;
	
public:
	inline Song (const std::string & title, const std::string & artist, const std::string & url, unsigned duration) : title(title), artist(artist), url(url), duration(duration) {}
	
	inline const std::string & getTitle () const { return title; }
	inline const std::string & getArtist () const { return artist; }
	inline const std::string & getUrl () const { return url; }
	inline unsigned getDuration () const { return duration; }
};

class Playlist : public std::vector<Song> {
private:
	uint64_t albumId;
	std::string title;
	
public:
	inline Playlist (uint64_t albumId, const std::string & title) : albumId(albumId), title(title) {}
	inline uint64_t getAlbumId () const { return albumId; }
	inline const std::string & getTitle () const { return title; }
};

typedef void (* ErrorHandlerFn) (const char *);
typedef void (* PlaylistsReadyFn) (const std::vector<Playlist> &);

class ResponseState {
private:
	volatile size_t * counter;
	size_t completeCnt;
	
public:
	PlaylistsReadyFn readyCb;
	std::vector<Playlist> * playlists;
	Playlist * playlist;
	
	inline ResponseState (size_t completeCnt, PlaylistsReadyFn readyCb) {
		this->counter = new size_t(0);
		this->completeCnt = completeCnt;
		this->readyCb = readyCb;
		this->playlists = new std::vector<Playlist>;
	}
	inline ResponseState (const ResponseState * parent, Playlist * pl) { * this = * parent, playlist = pl; }
	// Atomic. Thread-safe.
	inline bool isComplete () const { return __sync_bool_compare_and_swap(counter, completeCnt, 0); }
	inline void inc () { ++(* counter); }
	inline ~ResponseState () {
		if (* counter == 0) {
			delete counter;
			delete playlists;
		}
	}
};

void set_config_accessor (ConfigAccessor *);

void set_error_handler (ErrorHandlerFn);

GtkWidget * get_widget ();

/*
This will request playlists from server and call it's argument. Callback will be called asynchronously.
It's safe to call this as many times as needed. Request is made for every call, and for every call there is a separate array of playlists.
*/
void request_user_playlists (PlaylistsReadyFn);

};
