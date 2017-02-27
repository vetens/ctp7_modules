#include "moduleapi.h"
#include <libmemsvc.h>

memsvc_handle_t memsvc;

void mblockread(const RPCMsg *request, RPCMsg *response) {
  uint32_t count = request->get_word("count");
  uint32_t addr = request->get_word("address");
  uint32_t data[count];
  uint32_t* tmp;

  for (unsigned int i=0; i<count; i++){
    if (memsvc_read(memsvc, addr, 1, tmp) == 0) {
      data[i] = *tmp;
    }
    else {
      response->set_string("error", memsvc_get_last_error(memsvc));
      LOGGER->log_message(LogManager::INFO, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
      break;
    }
  }
	response->set_word_array("data", data, count);
}

extern "C" {
	const char *module_version_key = "extras v1.0.1";
	int module_activity_color = 4;
	void module_init(ModuleManager *modmgr) {
		if (memsvc_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("extras", "blockread", mblockread);
	}
}
