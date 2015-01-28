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

#include <iostream>
#include <fstream>
#include <map>
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::map;

// Placeholder for Audacious config
class AudaciousConfigAccessor : public VK::ConfigAccessor {
private:
	const char * const kConfigFile = "/tmp/aud-config.tmp";
	map<const string, string> c;
	
public:
	AudaciousConfigAccessor () {
		std::ifstream f(kConfigFile);
		while (f.good()) {
			string key, val;
			f >> key >> val;
			if (key.length() && val.length())
				c[key] = val;
		}
	}
	
	~AudaciousConfigAccessor () {
		std::ofstream f(kConfigFile);
		if (!f.good())
			return;
		for (auto i : c)
			f << i.first << ' ' << i.second << endl;
	}
	
	virtual bool get (const std::string & key, std::string & value) {
		map<string, string>::const_iterator i = c.find(key);
		if (i == c.cend())
			return false;
		value = i->second;
		return true;
	}

	virtual void set (const std::string & key, const std::string & value) {
		c[key] = value;
	}
};

static void error_handler (const char * error) {
	cerr << "Error: " << error << endl;
}

static void print_playlists (const std::vector<VK::Playlist> & playlists) {
	for (auto p : playlists) {
		cout << "--- Playlist: " << p.getTitle() << endl;
		for (auto song : p)
			cout << song.getArtist() << " — " << song.getTitle() << ": " << song.getUrl() << endl;
	}
}

int main (int argc, char ** argv) {
	cout << "Plugin name: " << VK::kName << endl;
	cout << "Gettext domain: " << VK::kDomain << endl;
	cout << "About: " << VK::kAbout << endl;
	
	AudaciousConfigAccessor ac;
	VK::set_config_accessor(&ac);
	VK::set_error_handler(error_handler);
	
	gtk_init(&argc, &argv);
	
	auto window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	auto widget = VK::get_widget();
	
	VK::request_user_playlists(print_playlists);
	
	gtk_window_set_title(GTK_WINDOW(window), "Test");
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
	gtk_container_add(GTK_CONTAINER(window), widget);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_widget_show_all(window);
	gtk_main();
	
	return 0;
}
