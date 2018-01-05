#include <client.h>

int Client::init_socket(){
  int sock_fd       = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock_fd < 0) {
    std::cerr << "\n\033[1;31mError!!! Cannot create socket! \033[0m\033[1;35" << strerror(errno) << "\033[0m\n\n";
    exit(EXIT_FAILURE);
  }
  return sock_fd;
}

void Client::init_socket_addr(sockaddr_in & sock_addr, int port, std::string & _ip){
  memset(&sock_addr, 0, sizeof sock_addr);

  sock_addr.sin_family  = AF_INET;
  sock_addr.sin_port    = htons(port);

  int result            = inet_pton(AF_INET, _ip.c_str(), &sock_addr.sin_addr);

  if ( result <= 0 ){
    std::cerr << "\n\033[1;31mError!!! Function inet_pton! \033[0m\033[1;35" << strerror(errno) << "\033[0m\n\n";
    if(sock_fd != -1)
      close(sock_fd);
    exit(EXIT_FAILURE);
  }
}

void Client::Init(int port,  std::string & _ip, std::string & _name){
  sock_fd         = init_socket();

  init_socket_addr(sock_addr,       port, _ip);

  client_name     = _name;
  frame_title[0]  = "Chat";
  frame_title[1]  = "Type your message";
  frame_title[2]  = "Members";
  my_sock         = 0;
}

Client::Client(int port,  std::string _ip, std::string _name){
  Init(port, _ip, _name);
}

Client::Client(std::string str_port,  std::string _ip, std::string _name){
  int port        = atoi(str_port.c_str());
  if(!port){
    std::cerr << "\n\033[1;31mError!!! Wrong port number!! \033[0m\n\n";
    exit(EXIT_FAILURE);
  }

  Init(port, _ip, _name);
}

Client::~Client(){
  if(sock_fd != -1){
    close(sock_fd);
  }

  for (int i = 1; i < 3; i++){
    if(der_frame[i]){
      delwin(der_frame[i]);
    }
    if(frame[i]){
      delwin(frame[i]);
    }
  }
  endwin();
}

void Client::SetupGUI(){
  initscr();
  //curs_set(false);
  getmaxyx(stdscr, parent_y, parent_x);

  type_frame_y = 5;
  list_frame_x = (int)(parent_x / 5);

  start_color();

  init_color(30, RGB_TO_NC_COLOR(255,99,71));
  init_color(31, RGB_TO_NC_COLOR(240,128,128));

  init_pair(10, 30, COLOR_BLACK);
  init_pair(11, 31, COLOR_BLACK);

  init_color(32, RGB_TO_NC_COLOR(0,128,0));
  init_color(33, RGB_TO_NC_COLOR(0,128,128));
  init_color(34, RGB_TO_NC_COLOR(210,105,30));

  init_pair(12, 32, COLOR_BLACK);
  init_pair(13, 33, COLOR_BLACK);
  init_pair(14, 34, COLOR_BLACK);

  init_pair(1, COLOR_RED,   COLOR_BLACK);
  init_pair(2, COLOR_BLUE,  COLOR_BLACK);
  init_pair(3, COLOR_GREEN, COLOR_BLACK);

  frame[0] = newwin(parent_y - type_frame_y, parent_x - list_frame_x, 0,                       0);
  frame[1] = newwin(type_frame_y,            parent_x - list_frame_x, parent_y - type_frame_y, 0);
  frame[2] = newwin(parent_y,                list_frame_x,                0,                   parent_x - list_frame_x);

  der_frame[0] = derwin(frame[0], parent_y - type_frame_y - 2, parent_x - list_frame_x - 2,   1, 1);
  der_frame[1] = derwin(frame[1], type_frame_y - 2,            parent_x - list_frame_x - 2,   1, 1);
  der_frame[2] = derwin(frame[2], parent_y - 2,                list_frame_x - 2,              1, 1);

  for (std::size_t i = 0; i < 3; ++i){
    scrollok(der_frame[i], true);
    wattron(frame[i], A_BLINK);
    wattron(frame[i], COLOR_PAIR(i + 1));
    wborder(frame[i], 0, 0, 0, 0, 0, 0, 0, 0);
  }

  mvwprintw(frame[0], 0,((parent_x     - sizeof(frame_title[0]) - list_frame_x + 1) >> 1), "%s", frame_title[0]);
  mvwprintw(frame[1], 0, 1, "%s", frame_title[1]);
  mvwprintw(frame[2], 0,((list_frame_x - sizeof(frame_title[2])+ 1) >> 1),                 "%s", frame_title[2]);

  for(std::size_t i = 0; i < 3; ++i){
    wmove(der_frame[i], 0, 0);
    wrefresh(frame[i]);
  }
}

void Client::init_connection(int _socket, sockaddr * _socket_addr){
  if (connect(_socket, _socket_addr, sizeof(* _socket_addr)) < 0) {
      std::cerr << "\n\033[1;31mError!!! Connect failed! \033[0m\033[1;35m" << strerror(errno) << "\033[0m\n\n";
      if(sock_fd != -1){
        close(sock_fd);
      }
      exit(EXIT_FAILURE);
  }

  std::string message = "";

  for(size_t i = 0; i < sizeof(int); ++i){
      message.push_back((char) ((CONNECT >> (i*8)) & 0xff));
  }

  for(size_t i = sizeof(int); i < 2 * sizeof(int); ++i){
      message.push_back((char) ((0 >> (i*8)) & 0xff));
  }

  message += client_name;

  if( send(sock_fd, message.c_str(), message.size(), 0) == -1){
        wattrset(der_frame[0], COLOR_PAIR(10));
        wprintw(der_frame[0], "\nError!!! Send name failed! ");

        wattrset(der_frame[0], COLOR_PAIR(11));
        wprintw(der_frame[0], "\n%s\n", strerror(errno));

        wrefresh(der_frame[0]);
  }
}

void Client::Run(){
  init_connection(sock_fd,       (sockaddr * ) & sock_addr);

  SetupGUI();

  {
    std::unique_lock<std::mutex> lk(m_ms);
    _threads[0] = std::thread([this]{this->ReadMsg   ();});
  }
  {
    std::unique_lock<std::mutex> lk(m_ms);
    cv.wait(lk, [this]{return my_sock;});
  }

  _threads[1] = std::thread([this]{this->WriteMsg  ();});

  _threads[0].join();
  _threads[1].join();
}

void Client::WriteMsg(){

  m_frame.lock();
  init_color(COLOR_CYAN, RGB_TO_NC_COLOR(255, 165, 0));
  init_pair(4, COLOR_CYAN,  COLOR_BLACK);

  noecho();
  m_frame.unlock();

  while (true){

    m_frame.unlock();
    werase(der_frame[1]);
    wattrset(der_frame[1], COLOR_PAIR(4));

    wmove(der_frame[1], 0, 0);
    wrefresh(der_frame[1]);
    m_frame.unlock();

    std::string msg;

    for(size_t i = 0; i < sizeof(int32_t); ++i){
      msg.push_back((char) ((MESSAGE >> (i*sizeof(int32_t))) & 0xff));
    }

    for(size_t i = 0; i < sizeof(int32_t); ++i){
      msg.push_back((char) ((my_sock >> (i*sizeof(int32_t))) & 0xff));
    }

    while(true){
      int sym =  wgetch(der_frame[1]);

      if (!rc) return;

      m_frame.lock();
      bool empty_str = msg.size() > 2 * sizeof(int32_t);
      if ((sym == KEY_BACKSPACE || sym == KEY_DC || sym == 127) && empty_str) {
        werase(der_frame[1]);
        wmove(der_frame[1], 0, 0);
        wrefresh(der_frame[1]);
        msg.pop_back();
        wprintw(der_frame[1], "%s", msg.c_str() + 2 * sizeof(int32_t));
      }
      else if (sym > '\037' && sym < '\177'){
        wprintw(der_frame[1], "%c", sym);
        msg.push_back((char) sym);
      }
      else if (sym == '\n' && empty_str){
        msg.push_back((char) sym);
        break;
      }
      wrefresh(der_frame[1]);
      m_frame.unlock();
    }

    if( send(sock_fd, msg.c_str(), msg.size(), 0) == -1){
      m_frame.lock();
      wattrset(der_frame[0], COLOR_PAIR(10));
      wprintw(der_frame[0], "\nError!!! Send failed! ");

      wattrset(der_frame[0], COLOR_PAIR(11));
      wprintw(der_frame[0], "\n%s\n", strerror(errno));

      wrefresh(der_frame[0]);
      m_frame.unlock();
    }
  }
}

void Client::GetStrTime(){
  time_t rawtime;
  tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(str_time, sizeof(str_time),"%F %r",timeinfo);
}

void Client::UpdateMembers(){
  m_frame.lock();
  werase(der_frame[2]);
  wmove(der_frame[2], 0, 0);
  for (std::map<int, std::string>::iterator it = members.begin(); it != members.end(); ++it){
    if(it->first == my_sock){
      wattrset(der_frame[2], COLOR_PAIR(1));
    }
    else{
      wattroff(der_frame[2], COLOR_PAIR(1));
    }
    wprintw(der_frame[2], "%s\n", it->second.c_str());
  }
  wrefresh(der_frame[2]);
  m_frame.unlock();
}

void Client::ReadMsg(){
  while(true){
    std::string message = "";
    bzero(str_time, sizeof str_time);
    do{
      bzero(buff,     sizeof buff);
      rc = recv(sock_fd, buff, sizeof(buff) - 1, 0);

      if (rc < 0){
        if (errno != EWOULDBLOCK){
           m_frame.lock();
           wattrset(der_frame[0], COLOR_PAIR(10));
           wprintw(der_frame[0], "\nWarning!!! Function recv failed! %s", strerror(errno));
           m_frame.unlock();
        }
      }else if (rc == 0){
        m_frame.lock();
        wattrset(der_frame[0], COLOR_PAIR(10));
        wprintw(der_frame[0], "\nServer closes connection!\n");
        wrefresh(der_frame[0]);
        m_frame.unlock();
        return;
      }
      else{
        packet * _packet = (packet * ) buff;
        switch(_packet->type){
          case MESSAGE:
            GetStrTime();
            cur_sock = _packet->socket;
            message.append(& _packet->data[0]);
            break;
          case CONNECT:{
            GetStrTime();

            members.clear();

            std::size_t user_count = 0;
            for(int i = sizeof(int32_t); i < rc; ){
              std::string _name(buff + sizeof(int32_t) + i);
              members[(int) *(buff + i)] = _name;
              i += _name.size() + sizeof(int32_t) + 1;
              ++user_count;
            }

            std::string new_name;

            for (std::map<int, std::string>::iterator it = members.begin(); it != members.end(); ++it){
              if(it->first < 0){
                new_name = it->second;
                int tmp_sock = - it->first;

                members.erase(it);
                members[tmp_sock] = new_name;

                if(!my_sock){
                  my_sock = tmp_sock;
                  cv.notify_one();
                }
                break;
              }
            }

            message = new_name + " is in the correspondence!\n";

            UpdateMembers();
            break;
          }
          case DISCONNECT:{
            GetStrTime();

            for (std::map<int, std::string>::iterator it = members.begin(); it != members.end(); ++it){
              if (it->first == _packet->socket){
                message = it->second;
                members.erase(it);
                break;
              }
            }

            message.append("  left the chat!\n");
            UpdateMembers();
            break;
          }
          case CLOSE_SESSION :
            GetStrTime();
            message.append(client_name + " ");
            message.append(buff + sizeof(int32_t));
            break;
          default:
            message.append(buff);
            break;
        }
        if ((unsigned) rc < sizeof(buff) - 1){
          break;
        }
      }
    } while(true);

    int _type = ((packet * ) buff)->type;

    m_frame.lock();
    if (_type == MESSAGE){
      wattrset(der_frame[0], COLOR_PAIR(12));
      wprintw(der_frame[0], "%s: ", members.find(cur_sock)->second.c_str());
    }

    wattrset(der_frame[0], COLOR_PAIR(13));
    wprintw(der_frame[0], "%s", message.c_str());

    wattrset(der_frame[0], COLOR_PAIR(14));
    wprintw(der_frame[0],  "%s\n\n", str_time);
    wrefresh(der_frame[0]);
    m_frame.unlock();

    message.erase();

    if (_type == CLOSE_SESSION){
      rc = 0;
      return;
    }
  }
}
