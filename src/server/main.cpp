#include <sys/socket.h>

#include <iostream>

#include "udp_server.h"

#include <pgw_utils/config_loader.h>

int main(int argc, char *argv[])
{

  try
  {
    protei::Server server(9641);
  }
  catch (...)
  {
  }

  // server.start();

  return 0;
}