#pragma once

#include <iostream>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <error.h>

#include <time.h>
#include <common.h>

#include <readline/readline.h>
#include <ncurses.h>

#define RGB_TO_NC_COLOR(r, g, b) (r * 1000 / 255), (g * 1000 / 255), (b * 1000 / 255)

class Client {
  struct sockaddr_in      sock_addr;

  int                     sock_fd;
  int                     cur_sock;
  int                     my_sock; // socket in server side, because client name can not be primary key

  std::string             client_name;

  // two buffers in order not to make a critical section
  char                    buff      [MAX_BUFFER_SIZE]; // buffer to read messages from server

  char                    str_time[0x100];

  int                     parent_x;
  int                     parent_y;

  int                     type_frame_y;
  int                     list_frame_x;

  int                     rc;

  std::thread             _threads[2];

  WINDOW *                frame[3];
  WINDOW *                der_frame[3];

  std::map<int, std::string>  members;

  std::mutex                 m_ms;    // for my_sock
  std::mutex                 m_frame;

  std::condition_variable    cv;

  const char *            frame_title[3];

  inline void Init(int port, std::string & _ip, std::string & _name);
  inline void SetupGUI();
  inline int  init_socket();
  inline void init_socket_addr(struct sockaddr_in & sock_addr, int port, std::string & _ip);
  inline void init_connection(int _socket, struct sockaddr * _socket_addr);

  inline void UpdateMembers();

  inline void GetStrTime();

  void ReadMsg();
  void WriteMsg();

public:

  Client() = delete;

  Client(int          port,      std::string _ip, std::string _name);
  Client(std::string  str_port,  std::string _ip, std::string _name);

  void Run();

  ~Client();
};
