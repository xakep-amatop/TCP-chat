#pragma once

#include <iostream>
#include <cstring>

#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>
#include <algorithm>

#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <common.h>

class Server{

  typedef void (Server::*MFP)(std::string);


  const std::string          fbanlist_name = "banlist.txt";

  struct sockaddr_in         socket_addr;
  int                        socket_fd;

  int                        max_connections;  // maximum queue of pending connections for listen function

  char                       buff[MAX_BUFFER_SIZE]; // buffer for receive messages

  size_t                     timeout;   // poll waiting time

  std::map<int, std::string> members;   // store client socket and client name, socket always unique (this can be used to support the same names in the chat)
  packet                     msg;

  std::vector<struct pollfd> cli_poll;
  std::vector<std::string>   cli_ip;    // place to store user`s IP

  std::vector< std::pair <std::string, bool> >   ban_list;

  std::string                additional_msg;

  std::thread                _threads[2];
  std::mutex                 m_send;

  std::fstream               file_banlist;

  int                        curr_fd;

  char                       str_time[0x100];
  bool                       was_client_name;

  void Init(int port, std::string _ip); // here perform creating socket, read banlist, bind and listen socket

  inline void GetStrTime(); // get current machine time and perform his to string, design for feature logs 

  inline void DisconnectClient(std::string & message, int i);

  void console_process();   // provide various commands, which we can enter
  void main_process();      // provide broadcast sending and receiving messages from clients

  void ShowHelp     (std::string);
  void ShowUsers    (std::string);
  void GetBanlist   (std::string);
  void KickUser     (std::string name);
  void BanUser      (std::string name);
  void BanHost      (std::string host);
  void UnbanUser    (std::string name);
  void UnbanHost    (std::string host);

  // commands that are maintained by the server, functions associated with them and their description
  std::map<const std::string, std::pair<MFP, const std::string>> 
                              commands  = {
                                            {"help",        {& Server::ShowHelp,   "\t\t\t            - show this help"}                                         },

                                            {"showusers",   {& Server::ShowUsers,  "\t\t            - show currently connected users, with their names and IPs"} },
                                            {"getbanlist",  {& Server::GetBanlist, "\t\t            - show the list of banned users and/or hosts"}               },
                                            {"kickuser",    {& Server::KickUser,   "\t\t<user_name> - kick the user from the chat"}                              },
                                            {"banuser",     {& Server::BanUser,    "\t\t<user_name> - ban the user from using the chat"}                         },
                                            {"banhost",     {& Server::BanHost,    "\t\t<host>      - ban all users from the specified host to be connected"}    },
                                            {"unbanuser",   {& Server::UnbanUser,  "\t\t<user_name> - unban the user from the chat"}                             },
                                            {"unbanhost",   {& Server::UnbanHost,  "\t\t<host>      - unban the host from the chat"}                             }
                                           };

  inline void pack_header     (std::string & message, int type); // function that packed int-type metadata to string from current position

  inline void get_message     (std::string & message); // handler of MESSAGE    type message
  inline void get_connection  (std::string & message); // handler of CONNECTION type message

  inline void WriteToFileBanlist();

  inline void UnicastSend     (std::string & message, int socket_fd);
  inline void BroadcastSend   (std::string & message);

public:

  Server() = delete;

  Server(int port,              std::string _ip);
  Server(std::string str_port,  std::string _ip);

  void Run();

  ~Server();
};
