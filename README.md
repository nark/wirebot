# Wirebot for UNIX

## Introduction

Wirebot is a chat robot for the Wired 2.0 protocol. It connects to a Wired server and interacts with other users following rules and command setup by the administrator. Wirebot is a kind of mix between Wire and Wired, mainly because it is a client program running as a daemon.

## Features

* Support for Wired 2.0 protocol
* XML-based triggers dictionary
* Custom chat commands
* Many more...

## How it works

Wirebot is basically a client, that receives/sends/replies messages from/to the server. With Wirebot you can define rules that trigger on received messages to execute a corresponding operation, like for example sending a message back. This I/O system is combined to features like multiple-random-outputs, delays or repeats to create an interactive engine for a chat robot. 

The bot also respond to chat commands, thus users can directly execute some operations on-demand. See "Commands " below in the Customize section.

## Install Wirebot (UNIX-like systems)

This tutorial explains how to install and run Wirebot on an UNIX-like operating system.

### Requirements

* Git
* libxml2
* GNU readline
* GNU libiconv
* OpenSSL

Be sure to have these components installed on your system, they are dependencies of libwired and Wirebot. 

### Install form binary

1. Clone Wiredbot repository:

		wget http://master.dl.sourceforge.net/project/wired2/wirebot/wirebot.tar.gz
	
2. Move into the clone directory:

		cd wirebot/
	
3. Add git submodules (here, libwired):

		git submodule init && git submodule update
	
4. Configure the package:

		./configure --enable-pthreads

	*The "--enable-pthreads" is currently needed.*
	
5. Compile both libwired and Wirebot

		make
		
	*Use "gmake" on non-GNU systems*
	
6. Install Wirebot:

		sudo make install
		
### Using Wirebot
		
#### Run Wirebot
		
1. Run it a first time:

		wirebot -D -d
	
	*"-D" is to not daemonize the process*
	
	*"-d" is to enable debug mode*
		
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

The shorter way is to take an example:

 	<rules>
		<rule permissions="admin,guest" activated="true">
		 	<input message="wired.chat.say" comparison="contains" sensitive="false">hello</input>
		
		 	<output message="wired.chat.say" delay="1">Hey @INPUT_NICK. :-)</output>
		 	<output message="wired.chat.say" delay="1">Hello @INPUT_NICK. :-)</output>
		 	<output message="wired.chat.say" repeat="3">:-)</output>
		 </rule>
	</rules>

**Definition:**	

* permissions: User logins able to trigger this rule, use "any" for all users.
* activated: Use "true" if the rule is activated, "false" if not.
* inputs:
	* message: The input message name referring to the Wired specifications. 
	See "currently supported messages" below.
	* comparison: The method used to match the input string.
	* sensitive: Use "true" for sensitive matching, "false" otherwise.
* outputs:
	* message: The output message name referring to the Wired specifications.
	* delay: Set a delay before executing the output.
	* repeat: Repeat output action as many time as specified.
	
##### List of currently supported input messages 
	
* wired.chat.say
* wired.chat.me
* wired.message.message
* wired.message.broadcast
* wired.chat.user_join
* wired.chat.user_leave

##### List of currently supported output messages 
	
* wired.chat.say
* wired.chat.me
* wired.message.message
* wired.message.broadcast


##### Commands

TBD

### Mac Users

If you are a Mac user, have a look to [WireBot for Mac](http://wired.read-write.fr/products/wire-bot/). It provides a binary version for OSX and a graphical user interface to edit dictionnary file. 

## Licence

Copyright (c) 2011-2012 Rafaël Warnault. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
