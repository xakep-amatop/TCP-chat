#pragma once

#include <getopt.h>
#include <iostream>
#include <cstring>

typedef struct{
  unsigned was_host:    1;
  unsigned was_listen:  1;
  unsigned was_name:    1;
  unsigned was_port:    1;
  unsigned was_help:    1;
} was_arg_table;

typedef struct{
  std::string     name;
  std::string     port;
  std::string     host;
  was_arg_table   was_args;
} _info;

void ParseArguments(int argc, char * argv[], _info & Info);