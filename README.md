# Wirebot

## Introduction

Welcome to Wirebot. For installation instructions, please see the "Install Wirebot" section below.

Wirebot is an UNIX chat robot for the Wired 2.0 (b55) protocol entirely based on libwired. It is a client-side software able to trigger actions automatically following a dictionary of statements and by analyzing received events from the server.

The documentation is available at http://www.read-write.fr/wired/wiki.

## Features

* Support for Wired 2.0 protocol
* XML-based triggers dictionary
* Custom chat commands
* Many more...


## Install Wirebot (BSD-like systems)

This tutorial explains how to install and run Wirebot using FreeBSD (9.0) as operating system.

### Requirements

* Git
* libxml2
* GNU readline
* GNU libiconv
* OpenSSL

Be sure to have these components installed on your system, they are dependencies of libwired and Wirebot. 

### Install form sources

1. Clone Wiredbot repository:

		git clone https://nark@bitbucket.org/nark/wirebot.git wirebot/
	
2. Move into the clone directory:

		cd wirebot/
	
3. Add git submodules (here, libwired):

		git submodule init && git submodule update
	
4. Configure the package:

		./configure --enable-pthreads

	*The "--enable-pthreads" is currently needed.*
	
5. Compile both libwired and Wirebot

		gmake
		
	*The "make" on non-GNU systems*
	
6. Install Wirebot:

		sudo gmake install
		
### Using Wirebot
		
#### Run Wirebot
		
1. Run it a first time:

		rehash && wirebot -D -d
	
	*"-D" is to not daemonize the process*
	
	*"-d" is to enable debug mode*
	
	*"rehash" is used here to reload system commands before trying to calling a new one*
	
	This will launch Wirebot a firt time and it will try to connect to localhost:4875 with "admin" login and no password. Wirebot also created default config files (~/.wirebot/).  Now kill it (^C), and edit the Wirebot config:
	
		nano ~/.wirebot/wirebot.conf
		
	Edit the configuration file for your needs, mainly "hostname", "port", "login" and "password" field. 
	
2. Try to connect again:

		wirebot -D -d
		
	If everything is OK, you should see the following output in the shell:
		
		****% wirebot -D -d
		Info: Reading /home/****/.wirebot/wirebot.conf
		Debug:   nick = WireBot
		Debug:   status = Jedi in the Matrix
		Debug:   auto reconnect = yes
		Debug:   reconnect on kick = no
		Debug:   icon path = icon.png
		Debug:   hostname = localhost
		Debug:   port = 4871
		Debug:   login = admin
		Debug:   password = ********
		Info: Reading /home/****/.wirebot/wirebot.xml robot 
		Info: Writting PID file: /home/****/.wirebot/
		Info: Connecting to localhost...
		Info: Trying ::1 at port 4871...
		Info: Connected using AES/256 bits, logging in...
		Info: Logged in, welcome to Wired Server

#### Run Wirebot, as a daemon

1. Kill it again (^C) and now try to launch it as e daemon:

		/usr/local/bin/wirebot
		
2. To stop it, use the following command:

		kill `cat ~/.wirebot/wirebot.pid`
	
### Customize Wirebot

#### Change the icon

You can update the icon without having to restart the process. Add an "icon.png" file in `~/.wirebot/` directory, and type the following command in the public chat (or private message) where your bot is connected:

	!reload
		
#### Manage Wirebot dictionary

Wirebot chat-bot engine is based on a XML dictionary located at `~/.wirebot/wirebot.xml` by default. In this file, you can found every rules and triggers the bot know. The engine is based on input/output routing of Wired protocol messages, and currenty support two kind your meta-event named `rules` and `commands`.

If you modified the dictionary, you have to send a `!reload` to the bot for changes taking effect.

##### Rules

