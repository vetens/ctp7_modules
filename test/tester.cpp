#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xhal/rpc/wiscrpcsvc.h>

using namespace wisc;

int main(int argc, char *argv[])
{
  RPCSvc rpc;
  try {
    rpc.connect(argv[1]);
  } catch (RPCSvc::ConnectionFailedException &e) {
    printf("Caught RPCErrorException: %s\n", e.message.c_str());
    return 1;
  } catch (RPCSvc::RPCException &e) {
    printf("Caught exception: %s\n", e.message.c_str());
    return 1;
  }

#define STANDARD_CATCH                                                  \
  catch (RPCSvc::NotConnectedException &e) {                            \
    printf("Caught NotConnectedException: %s\n", e.message.c_str());    \
    return 1;                                                           \
  } catch (RPCSvc::RPCErrorException &e) {                              \
    printf("Caught RPCErrorException: %s\n", e.message.c_str());        \
    return 1;                                                           \
  } catch (RPCSvc::RPCException &e) {                                   \
    printf("Caught exception: %s\n", e.message.c_str());                \
    return 1;                                                           \
  }

#define ASSERT(x) do {                                                  \
    if (!(x)) {                                                         \
      printf("Assertion Failed on line %u: %s\n", __LINE__, #x);        \
      return 1;                                                         \
    }                                                                   \
  } while (0)

  RPCMsg req, rsp;

  try {
    ASSERT(rpc.load_module("amc", "amc v1.0.1"));
  } STANDARD_CATCH;

  req = RPCMsg("amc.ttcMMCMPhaseShift");
  req.set_word("shiftOutOfLockFirst", 1);
  req.set_word("useBC0Locked",        0);
  req.set_word("doScan",              0);
  try {
    rsp = rpc.call_method(req);
    // std::cout << rsp.get_string("error") << std::endl;
  } STANDARD_CATCH;

  return 0;
}
