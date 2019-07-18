#include <ctime>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <limits.h>
#include <thread>
#include <chrono>
#include <errno.h>
#include <xhal/rpc/wiscrpcsvc.h>

using namespace wisc;

#define STANDARD_CATCH                                                  \
  catch (RPCSvc::NotConnectedException &e) {                            \
    std::stringstream errmsg;                                           \
    errmsg << "Caught NotConnectedException: " <<  e.message.c_str();   \
    std::cerr << errmsg.str();                                          \
    return 1;                                                           \
  } catch (RPCSvc::RPCErrorException &e) {                              \
    std::stringstream errmsg;                                           \
    errmsg << "Caught RPCErrorException: " << e.message.c_str();        \
    std::cerr << errmsg.str();                                          \
    return 1;                                                           \
  } catch (RPCSvc::RPCException &e) {                                   \
    std::stringstream errmsg;                                           \
    errmsg << "Caught RPCException: " <<  e.message.c_str();            \
    std::cerr << errmsg.str();                                          \
    return 1;                                                           \
  }

#define ASSERT(x) do {                                                  \
    if (!(x)) {                                                         \
      std::stringstream errmsg;                                         \
      errmsg << "Assertion Failed on line " << __LINE__                 \
             << ": " << #x << std::endl;                                \
      std::cerr << errmsg.str();                                        \
      return 1;                                                         \
    }                                                                   \
  } while (0)

int process(RPCSvc& rpc, RPCMsg& req, std::string const& lfname)
{
  RPCMsg rsp;
  std::time_t timestamp = std::time(nullptr);
  try {
    rsp = rpc.call_method(req);
    if (rsp.get_key_exists("error")) {
      std::stringstream errmsg;
      errmsg << "RPC ERROR:" << rsp.get_string("error") << std::endl;
      std::cerr << errmsg.str();
      // syslog(LOG_ERR, errmsg.str().c_str());
      return 0;
    }

    std::vector<std::string> keynames = rsp.get_string_array("keynames");
    // std::vector<uint32_t> keyvalues = rsp.get_word_array("keyvalues");

    std::ofstream ofs;
    ofs.open(lfname, std::ofstream::out | std::ofstream::app);

    for (auto const& key : keynames) {
      uint32_t value = rsp.get_word(key);
      std::stringstream msg;
      ofs << timestamp << '\t' << key << '\t' << value << std::endl;
      // std::cout << msg.str();
      // syslog(LOG_INFO, msg.str().c_str());
    }
    ofs.close();
  } STANDARD_CATCH; // catch and continue? or fail?
}

int main(int argc, char *argv[])
{
  RPCSvc rpc;
  std::string hostname = argv[1];
  try {
    rpc.connect(hostname);
  } catch (RPCSvc::ConnectionFailedException &e) {
    std::stringstream errmsg;
    errmsg << "Caught ConnectionFailedException: " <<  e.message.c_str();
    std::cerr << errmsg.str();
    return 1;
  } catch (RPCSvc::RPCException &e) {
    std::stringstream errmsg;
    errmsg << "Caught RPCException: " <<  e.message.c_str();
    std::cerr << errmsg.str();
    return 1;
  }

  RPCMsg req;

  try {
    ASSERT(rpc.load_module("daq_monitor", "daq_monitor v1.0.1"));
  } STANDARD_CATCH;

  req = RPCMsg("daq_monitor.getmonCTP7dump");

  if (argc < 3) {
    std::stringstream errmsg;
    errmsg << "No remote register file specified, module default will be used." << std::endl;
    std::cerr << errmsg.str();
   // return 1;
  } else {
    std::string regList = argv[2];
    req.set_string("fname", regList);
  }

  // get the sample frequency
  uint32_t delay = 15;

  if (argc < 4) {
    std::stringstream errmsg;
    errmsg << "No delay specified, default will be 15 seconds." << std::endl;
    std::cerr << errmsg.str();
  } else {
    errno = 0;
    delay = strtoul(argv[3], nullptr, 10);
    if (errno != 0 /*|| *p != '\0'*/ || delay > INT_MAX) {
      delay = 15; // default sleep is 15 seconds
    }
  }

  std::stringstream lfname;
  std::stringstream logbase;
  logbase <<  std::getenv("CTP7_DUMP_PATH");
  if (logbase.str().empty())
    lfname << "/tmp/" << hostname << "-monitoring.log";
  else
    lfname << logbase.str() << "/monitoring.log";
  std::cout << "Logging register dumps to " << lfname.str() << std::endl;
  while (true) {
    process(rpc, req, lfname.str());
    std::this_thread::sleep_for(std::chrono::seconds(delay));
  };
}
