#include <parse_args.h>

#define _DEBUG

static void PrintHelp(char * path_name){
  char * program_name = strchr(path_name, '/');
  std::cout <<  "\n\tUsage: " << std::string( program_name ? ++program_name : path_name) << " [OPTIONS]\n"
                "\n\tOPTIONS:\n"
                "\t\t    --help  \t\tdisplay this help and exit\n"
                "\t\t-h, --host  \t\tIP to listen in server mode, or to connect to in client mode\n"
                "\t\t-l, --listen\t\tserver mode, if not specified - client mode\n"
                "\t\t-n, --name  \t\tnickname to use in chatting, if specified - client mode\n"
                "\t\t-p, --port  \t\tTCP port to listen in server mode, or to connect to in client mode\n\n";
}

void ParseArguments(int argc, char * argv[], _info & Info){

  Info.was_args = {0};

  const char * short_options = "h:p:n:l";
  struct option long_options[] =
        {
          {"host",    required_argument,  0,  'h' },
          {"port",    required_argument,  0,  'p' },
          {"name",    required_argument,  0,  'n' },
          {"help",    no_argument,        0,  0   },
          {"listen",  no_argument,        0,  0   },
          {0, 0, 0, 0}
        };

  while(true){
    int option_index = 0;
    int c = getopt_long(argc,argv,short_options, long_options, & option_index);

    if(c == -1){
      break;
    }

    switch (c){
      case 0:
        if (!strcmp(long_options[option_index].name, "help")){
          PrintHelp(argv[0]);
          Info.was_args.was_help = 1;
        }
        break;
      case 'h':
        if (optarg){
          Info.was_args.was_host  = 1;
          Info.host               = std::string(optarg);
        }
        break;
      case 'p':
        if (optarg){
          Info.was_args.was_port  = 1;
          Info.port               = std::string(optarg);
        }
        break;
      case 'l':
        Info.was_args.was_listen = 1;
        break;
      case 'n':
        if (optarg){
          Info.was_args.was_name = 1;
          Info.name              = std::string(optarg);
        }
        break;
      case '?':
        /* getopt_long already printed an error message. */
        break;

      default:
        std::cerr << "\n\033[1;31mTyped wrong!!!\033[0m\n";
        abort ();
      }
  }

  size_t number_args = Info.was_args.was_host + Info.was_args.was_port;

  if( Info.was_args.was_listen){
    if (number_args != 2){
      if (!Info.was_args.was_help){
        std::cerr << "\n\033[1;31mError!!! You must specified all options required in\033[0m \033[1;34mSERVER\033[0m \033[1;31mmode!!!\033[0m\n";
        PrintHelp(argv[0]);
      }
      exit(EXIT_FAILURE);
    }
  }
  else{
    number_args += Info.was_args.was_name;
    if (number_args != 3){
      if (!Info.was_args.was_help){
        std::cerr << "\n\033[1;31mError!!! You must specified all options required in\033[0m \033[1;34mCLIENT\033[0m \033[1;31mmode!!!\033[0m\n";
        PrintHelp(argv[0]);
      }
      exit(EXIT_FAILURE);
    }
  }
}
