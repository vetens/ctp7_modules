#include "optohybrid.h"

extern "C" {
	const char *module_version_key = "optohybrid v1.0.1";
	int module_activity_color = 4;
	void module_init(ModuleManager *modmgr) {
		if (memsvc_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("optohybrid", "broadcastWrite", broadcastWrite);
		modmgr->register_method("optohybrid", "loadVT1", loadVT1);
		modmgr->register_method("optohybrid", "loadTRIMDAC", loadTRIMDAC);
		modmgr->register_method("optohybrid", "configureVFATs", configureVFATs);
	}
}
