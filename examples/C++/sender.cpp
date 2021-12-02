#include <iostream>
#include <zmqpp/zmqpp.hpp>
#include "message.h"

int main(int argc, char **argv) 
{
  assert(argc == 2);

  // initialize the 0MQ context
  zmqpp::context context;

  // generate a pub socket
  zmqpp::socket socket(context, zmqpp::socket_type::pub);

  std::string addr(argv[1]);
  // open the connection in address given by argument

  socket.connect(addr);  
  std::cout << "Connected to " << addr << std::endl; 

  // generate message payload
  std::string contents = "hello world!";
  std::vector<uint8_t> payload(contents.begin(), contents.end());

  // construct message with mux, sec, type = 2, 3, 3 
  message message(2, 3, 3, payload);

  /*
  * HAL subscriber binds to address (usually publisher would bind).
  * The APP-API cannot send immediately after a connect, as there
  * is a few msec before ZMQ creates outgoing pipe (so sleep 1)
  */
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  // send the message
  message.send(socket);

  std::cout << "Sent message: " << contents << std::endl; 

  return 0;
}