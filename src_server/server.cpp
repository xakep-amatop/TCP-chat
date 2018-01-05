#include "server.h"

void Server::Init(int port, std::string _ip){
  max_connections = MAX_CLIENTS;
  memset(&socket_addr, 0, sizeof(socket_addr));

  socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (socket_fd < 0) {
    std::cerr << "\n\033[1;31mError!!! Cannot create socket! \033[0m\033[1;35" << strerror(errno) << "\033[0m\n\n";
    exit(EXIT_FAILURE);
  }
  int rc, on = 1;

  rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  if (rc < 0){
    std::cerr << "\n\033[1;31mError!!! Function setsockopt! \033[0m\033[1;35" << strerror(errno) << "\033[0m\n\n";
    close(socket_fd);
    exit(EXIT_FAILURE);
  }

  rc = ioctl(socket_fd, FIONBIO, (char *)&on);
  if (rc < 0){
    std::cerr << "\n\033[1;31mError!!! Function ioctl! \033[0m\033[1;35" << strerror(errno) << "\033[0m\n\n";
    close(socket_fd);
    exit(EXIT_FAILURE);
  }

  socket_addr.sin_family = AF_INET;
  socket_addr.sin_port   = htons(port);
  rc                     = inet_pton(AF_INET, _ip.c_str(), &socket_addr.sin_addr);;

  if (rc <= 0 ){
    std::cerr << "\n\033[1;31mError!!! Function inet_pton! \033[0m\033[1;35" << strerror(errno) << "\033[0m\n\n";
    close(socket_fd);
    exit(EXIT_FAILURE);
  }

  if (bind(socket_fd,(sockaddr *)&socket_addr, sizeof(socket_addr)) == -1) {
    std::cerr << "\n\033[1;31mError!!! Bind failed! \033[0m\033[1;35m" << strerror(errno) << "\033[0m\n\n";
    close(socket_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(socket_fd, max_connections) == -1) {
    std::cerr << "\n\033[1;31mError!!! Listen failed! \033[0m\033[1;35m" << strerror(errno) << "\033[0m\n\n";
    close(socket_fd);
    exit(EXIT_FAILURE);
  }

  {
    pollfd tmp;
    tmp.fd        = socket_fd;
    tmp.events    = POLLIN;

    cli_poll.push_back(tmp);
    cli_ip.push_back(_ip);
  }
  timeout = MAX_TIMEOUT_POLL;

  std::cout << "\033[1;37mStart listening at: \033[0m\033[1;33m" << _ip << ":" << port << "\033[0m\n";

  // getting banlist

  file_banlist.open(fbanlist_name, std::fstream::app | std::fstream::in | std::fstream::out);

  if(file_banlist.is_open()){
    std::string item;
    while (std::getline (file_banlist, item)){
      std::istringstream it(item);

      bool is_host = false;
      std::string name_host;

      it >> is_host >> name_host;

      ban_list.push_back(std::make_pair(name_host, is_host));
   }
   file_banlist.clear();
  }
  else{
    std::cerr << "File with banlist doesn\'t open! File name: " << "banlist.txt" << std::endl;
  }
}

    Server::Server(int port, std::string _ip){

  Init(port, _ip);
}

     Server::Server(std::string str_port,  std::string _ip){
  int port = atoi(str_port.c_str());

  if(!port){
    std::cerr << "\n\033[1;31mError!!! Wrong port number!! \033[0m\n\n";
    exit(EXIT_FAILURE);
  }

  Init(port, _ip);
}

void Server::GetStrTime(){
  time_t rawtime;
  tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(str_time, sizeof(str_time),"%F %r",timeinfo);
}

void Server::DisconnectClient(std::string & message, int i){
  pack_header(message, DISCONNECT);
  pack_header(message, cli_poll[i].fd);

  message.append("  left the chat!\n");

  members.erase(cli_poll[i].fd);
  close(cli_poll[i].fd);

  cli_poll.erase(cli_poll.begin() + i);
  cli_ip.erase(cli_ip.begin() + i);
}

void Server::Run(){
  _threads[0] = std::thread([this]{this->main_process   ();});
  _threads[1] = std::thread([this]{this->console_process();});

  _threads[1].join();
  _threads[0].join();
}

void Server::ShowHelp     (std::string){
  std::cout << std::endl << std::endl;
  for(auto command = commands.begin(); command != commands.end(); ++command){
    std::cout << '\t' << command->first << " " << command->second.second << std::endl;
  }
  std::cout << std::endl << std::endl;
}

void Server::ShowUsers    (std::string){
  if (cli_poll.size() < 2){
    std::cout << "\nUser list is empty!\n\n";
    return;
  }

  std::size_t count = 0;
  for (std::size_t i = 1; i < cli_poll.size(); ++i){
    std::cout << ++count << ":\n\tName: " << members[cli_poll[i].fd] << "\n\tIP:   " << cli_ip[i] << std::endl << std::endl;
  }
}

void Server::GetBanlist   (std::string){
  if (ban_list.empty()){
    std::cout << "\nBanlist is empty\n\n";
    return;
  }

  std::size_t count = 0;

  for (auto _it = ban_list.begin(); _it != ban_list.end(); ++_it){
    std::cout << ++count << ":\n\t" << (_it->second ? "Host: ":"Name: ") << _it->first << std::endl << std::endl;
  }
}

void Server::KickUser     (std::string name){
  if (name.empty()){
    std::cout << "Bad argument!!! You must specify user-name!\n\n";
    return;
  }

  auto _it = std::find_if(members.begin(), members.end(),
             [name](const std::pair<int, std::string> & element){
                return element.second == name;
              });

  if (_it == members.end()){
    return;
  }

  int socket = _it->first;

  auto it = std::find_if(cli_poll.begin(), cli_poll.end(),
             [socket](const pollfd & element){
                return element.fd == socket;
              });

  std::size_t index = std::distance(cli_poll.begin(), it);
  std::string message;

  pack_header(message, CLOSE_SESSION);
  message.append(" You kick the server!\n");
  message += additional_msg;

  UnicastSend(message, index);

  message.erase();
  DisconnectClient(message, index);
  BroadcastSend(message);
}

void Server::BanUser      (std::string name){
  if (name.empty()){
    std::cout << "Bad argument!!! You must specify user-name!\n\n";
    return;
  }

  auto _it = std::find_if(ban_list.begin(), ban_list.end(),
     [name](const std::pair<std::string, bool> & element){
        return (element.first == name && element.second == false);
      });

  if(_it == ban_list.end()){
    ban_list.push_back(std::make_pair(name, false));
    std::cout << "User " << name << " was added to banlist\n\n";

    if(file_banlist.is_open()){
      file_banlist << 0 << " " << name << std::endl;
      file_banlist.flush();
    }

    additional_msg = " Are you banned\n";
    KickUser(name);
  }
  else{
    std::cout << "User " << name << " already exist in the banlist\n\n";
  }
}


void Server::BanHost      (std::string host){
  if (host.empty()){
    std::cout << "Bad argument!!! You must specify IP address!\n\n";
    return;
  }

  auto _it = std::find_if(ban_list.begin(), ban_list.end(),
     [host](const std::pair<std::string, bool> & element){
        return (element.first == host && element.second == true);
      });

  if(_it == ban_list.end()){
    ban_list.push_back(std::make_pair(host, true));
    std::cout << "IP address " << host << " was added to banlist\n\n";

    if(file_banlist.is_open()){
        file_banlist << 1 << " " << host << std::endl;
        file_banlist.flush();
    }

    additional_msg = " Your host banned\n";
    while(true){
      auto it_ip = std::find(cli_ip.begin() + 1, cli_ip.end(), host);
      if (it_ip == cli_ip.end()){
        std::cout << "No connected users with the host: " << host << std::endl;
        return;
      }
      std::size_t index = std::distance(cli_ip.begin(), it_ip);

      auto it_member = members.find(cli_poll[index].fd);

      if(it_member != members.end()){
        KickUser(it_member->second);
      }
    }
  }
  else{
    std::cout << "Host " << host << " already exist in the banlist\n\n";
  }
}

void Server::WriteToFileBanlist(){
  file_banlist.close();
  file_banlist.open(fbanlist_name, std::fstream::out);
  if(!file_banlist.is_open())
    return;

  for(auto item = ban_list.begin(); item != ban_list.end(); ++item){
    file_banlist << item->second << " " << item->first << std::endl;
  }

  file_banlist.close();
  file_banlist.open(fbanlist_name, std::fstream::out | std::fstream::app);
}

void Server::UnbanUser    (std::string name){
  if (name.empty()){
    std::cout << "Bad argument!!! You must specify user-name!\n\n";
    return;
  }

  auto _it = std::find_if(ban_list.begin(), ban_list.end(),
     [name](const std::pair<std::string, bool> & element){
        return element.first == name && element.second == false;
      });

  if( _it != ban_list.end()){
    ban_list.erase(_it);
    std::cout << "User " << name << " was deleted from banlist\n\n";
    WriteToFileBanlist();
  }
  else{
    std::cout << "User " << name << " does not exist in the banlist\n\n";
  }
}

void Server::UnbanHost    (std::string host){
    if (host.empty()){
      std::cout << "Bad argument!!! You must specify user-name!\n\n";
      return;
    }

    auto _it = std::find_if(ban_list.begin(), ban_list.end(),
       [host](const std::pair<std::string, bool> & element){
          return element.first == host && element.second == true;
        });

    if( _it != ban_list.end()){
      ban_list.erase(_it);
      std::cout << "Host " << host << " was deleted from banlist\n\n";
      WriteToFileBanlist();
    }
    else{
      std::cout << "Host " << host << " does not exist in the banlist\n\n";
    }
}

void Server::console_process(){
  std::string command;
  while(true){
    std::cout << '>';
    std::getline(std::cin, command);
    if (command.empty())
      continue;

    std::istringstream args(command);

    std::string arg;
    args >> arg;

    auto fp = commands.find(arg);

    if(fp == commands.end()){
      commands.erase(arg);
      std::cout << "You type wrong command: " << command << "\nEnter the `help` to see all available commands!\n\n";
    }
    else{
      m_send.lock();
      arg.clear();
      additional_msg.clear();
      args >> arg;
      MFP function = fp->second.first;
      (this->*function)(arg);
      m_send.unlock();
    }
  }
}

void Server::pack_header(std::string & message, int32_t _type){
  for(size_t i = 0; i < sizeof(_type); ++i){
    message.push_back((char) ((_type >> (i*8)) & 0xff));
  }
}

void Server::get_message(std::string & message){
  GetStrTime();

  pack_header(message, (int32_t) * buff                    );
  pack_header(message, (int32_t) * (buff + sizeof(int32_t)));

  message.append(buff + 2*sizeof(int32_t));
}

void Server::get_connection(std::string & message){
  GetStrTime();

  packet * _packet = (packet *) buff;
  std::string new_member(& _packet->data[0]);

  auto _it_ban = std::find_if(ban_list.begin(), ban_list.end(),
             [new_member](const std::pair<std::string, bool> & element){
                return (element.first == new_member && element.second == false);
              });

  auto _it = std::find_if( members.begin(), members.end(),
      [new_member](const std::pair<int, std::string> & element){
        return element.second == new_member;
      });

  if(_it == members.end() && _it_ban == ban_list.end()){

    members[- curr_fd] = new_member;

    pack_header(message, CONNECT);

    for (std::map<int, std::string>::iterator it = members.begin(); it != members.end(); ++it){
      pack_header(message, it->first);
      message.append(it->second);
      message.push_back((char) '\0');
    }

    members.erase(- curr_fd);
    members[curr_fd] = new_member;
  }
  else if (_it_ban != ban_list.end()){
    pack_header(message, CLOSE_SESSION);

    message.append(" You are banned!\n");
    was_client_name = true;
  }
  else{
    pack_header(message, CLOSE_SESSION);
    message.append("Name already exist\n");
    was_client_name = true;
  }
}

void Server::UnicastSend(std::string & message, int index_socket_fd){
  int rc = send(cli_poll[index_socket_fd].fd, message.c_str(), message.size(), 0);
  if (rc < 0){
    std::string _tmp;
    DisconnectClient(_tmp, index_socket_fd);
  }
}

void Server::BroadcastSend(std::string & message){
  for (std::size_t k = 1; k < cli_poll.size(); ++k){
    UnicastSend(message, k);
  }
}

void Server::main_process(){
  int rc;

  int new_sd;

  bool end_server     = false;
  do{
    rc = poll(& cli_poll[0], cli_poll.size(), timeout);
    m_send.lock();
    if (rc < 0){
      std::cerr << "\n\033[1;31mError!!! Function poll failed! \033[0m\033[1;35m" << strerror(errno) << "\033[0m\n";
      close(socket_fd);
      exit(EXIT_FAILURE);
    }

    if (rc == 0){
      std::cerr << "\n\033[1;35mpoll() timed out. End program.\033[0m\n\n";
      close(socket_fd);
      exit(EXIT_FAILURE);
    }

    bool was_new_connect = false;
    bool was_spam        = false;

    for (std::size_t i = 0; i < cli_poll.size(); ++i){

      curr_fd = cli_poll[i].fd;

      if(cli_poll[i].revents == 0 || cli_poll[i].revents == POLLNVAL)
        continue;

      if(cli_poll[i].revents & (POLLERR|POLLHUP)){
        std::cerr << "\n\033[1;35mWarning!!! Field revents not equal POLLIN! revents = " << cli_poll[i].revents <<" "<< strerror(errno) <<" \033[0m\n\n";
        close(cli_poll[i].fd);
        cli_poll.erase(cli_poll.begin() + i);
        cli_ip.erase(cli_ip.begin() + i);
        i -= 2;
        continue;
      }

      if(cli_poll[i].revents != POLLIN)
        continue;

      if (cli_poll[i].fd == socket_fd){
        do{
          sockaddr_in client_addr;
          socklen_t     len = sizeof(client_addr);

          new_sd = accept(socket_fd, (sockaddr * )& client_addr, &len);
          if (new_sd < 0){
            if (errno != EWOULDBLOCK){
              std::cerr << "\n\033[1;31mError!!! Function accept failed! \033[0m\033[1;35m" << strerror(errno) << "\033[0m\n\n";
              end_server = true;
            }
            break;
          }

          {
            in_addr ipAddr = client_addr.sin_addr;

            char str[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );

            std::string _ip(str);

            auto _it = std::find_if(ban_list.begin(), ban_list.end(),
                       [_ip](const std::pair<std::string, bool> & element){
                          return (element.first == _ip && element.second == true);
                        });

            if(_it == ban_list.end()){
              pollfd tmp;
              tmp.fd          = new_sd;
              tmp.events      = POLLIN;
              was_new_connect = true;
              cli_poll.push_back(tmp);
              cli_ip.push_back(_ip);
            }
            else{
              std::string message;
              pack_header(message, CLOSE_SESSION);
              message.append(" Your host is banned!\n");
              send(new_sd, message.c_str(), message.size(), 0);
              close(new_sd);
              was_client_name = true;
            }
          }
        } while (new_sd != -1);
      }
      else{
          std::string message   = "";
          bzero(str_time, sizeof str_time);
          int rc;
          do{
            bzero(buff,     sizeof buff);
            rc = recv(cli_poll[i].fd, buff, sizeof(buff) - 1, 0);
            if (rc < 0){
              if (errno != EWOULDBLOCK){
                  DisconnectClient(message, i);
              }
              break;
            }else if (rc == 0){
                DisconnectClient(message, i);
                break;
            }
            else{
              packet * _packet = (packet *) buff;
              switch(_packet->type){
                  case MESSAGE:
                    get_message(message);
                    break;
                  case CONNECT:
                    was_client_name  = false;
                    get_connection(message);
                    break;
                  case DISCONNECT:
                    DisconnectClient(message, i);
                    break;
                  case CLOSE_SESSION:
                    GetStrTime();
                    return;
                  default:
                    // here can be spam
                    auto it = members.find(cli_poll[i].fd);
                    if(it == members.end() || it->second.empty()){
                      DisconnectClient(message, i);
                      was_spam = true;
                    }
                    else{
                      message.append(buff);
                    }
                    break;
              }
              if ((unsigned) rc < sizeof(buff) - 1 || was_spam)
                  break;
            }
            was_spam = false;
          } while(true);

          if (was_client_name){
            UnicastSend(message, i);
            std::string _tmp;
            DisconnectClient(_tmp, i);
            was_client_name = false;
          }
          else{
            BroadcastSend(message);
          }
        message.erase();
      }  /* End of existing connection is readable             */

      if(was_new_connect){
        break;
      }
    } /* End of loop through pollable descriptors              */
    m_send.unlock();
  } while (!end_server); /* End of serving running.    */
}

     Server::~Server(){
  std::cout << "Closing all conections...\n";
  if(socket_fd != -1) close(socket_fd);
  for (std::size_t i = 0; i < cli_poll.size(); ++i){
    if(cli_poll[i].fd != -1) close(cli_poll[i].fd);
  }
  file_banlist.close();
}
