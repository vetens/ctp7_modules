#include "utils.h"

void thresholdScanLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, RPCMsg *response)
{
}

void thresholdScan(const RPCMsg *request, RPCMsg *response)
{
}

void latencyScanLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, RPCMsg *response)
{
}

void latencyScan(const RPCMsg *request, RPCMsg *response)
{
}

void scurveScanLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, RPCMsg *response)
{
}

void scurveScan(const RPCMsg *request, RPCMsg *response)
{
}

extern "C" {
	const char *module_version_key = "calibration_routines v1.0.1";
	int module_activity_color = 4;
	void module_init(ModuleManager *modmgr) {
		if (memsvc_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("calibration_routines", "thresholdScan", thresholdScan);
		modmgr->register_method("calibration_routines", "latencyScan", latencyScan);
		modmgr->register_method("calibration_routines", "scurveScan", scurveScan);
	}
}
