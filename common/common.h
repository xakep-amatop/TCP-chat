#ifndef __COMMON_H__
#define __COMMON_H__

  #define MAX_BUFFER_SIZE     (8192)
  #define MAX_CLIENTS         (1000)

  #define NO_BLOCKING_POLL    (0)      //non-blocking poll
  #define INFTIM              (-1)     //blocking poll to infinity time
  #define MAX_TIMEOUT_POLL    (INFTIM) //in ms or above constants

   enum PACKET_TYPES{
       MESSAGE       = 1,
       CONNECT       = 2,
       DISCONNECT    = 3,
       CLOSE_SESSION = 4
   };

   typedef struct{
       int32_t      type;
       int32_t      socket;
       char         data[1];
   } packet;

#endif

