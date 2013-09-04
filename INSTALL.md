# Install Wirebot (BSD-like systems)

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
    
  *Use "make" on non-GNU systems*
  
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

