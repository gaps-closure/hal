#include <iostream>
#include <zmqpp/zmqpp.hpp>
#include "message.h"

int main(int argc, char **argv) 
{
  assert(argc == 2);

  // initialize the 0MQ context
  zmqpp::context context;

  // generate a sub socket
  zmqpp::socket socket(context, zmqpp::socket_type::sub);

  std::string addr(argv[1]); 
  // connect to address given by argument
  socket.connect(addr);  
  socket.subscribe("");
  std::cout << "Connected to" << addr << std::endl; 

  // receive message from other enclave
  zmqpp::message recv;
  socket.receive(recv);

  std::cout << "Received message" << std::endl; 
  // deserialize what was received 
  message msg(recv);
  std::string contents(msg.payload.begin(), msg.payload.end());

  // print contents of message
  std::cout << contents << std::endl;

  return 0;
}