#include <client.h>
#include <server.h>

#include <parse_args.h>

int main(int argc, char * argv[]){
  _info info;

  ParseArguments(argc, argv, info);

  if (info.was_args.was_listen){
    Server * _server = new Server(info.port, info.host);
    _server->Run();
    delete _server;
  }
  else{
    Client * _client  = new Client(info.port, info.host, info.name);
    _client->Run();
    delete _client;
  }
}
