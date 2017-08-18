#include <pthread.h>
#include "optohybrid.h"

struct scan_args {
  lmdb::txn & rtxn;
  lmdb::dbi & dbi;
  RPCMsg *response;
};

void *thresholdScanLocal(void * args)
{
  struct scan_args *local_args = (struct scan_args *) args; 
}

void thresholdScan(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);

  pthread_t thread;
  configureScanModule(rtxn, dbi, request, response);
  struct scan_args args = {.rtxn = rtxn, .dbi = dbi, .response = response};
  pthread_create(&thread,NULL,&thresholdScanLocal,(void *)&args);

}

void *latencyScanLocal(void * args)
{
  struct scan_args *local_args = (struct scan_args *) args; 
  startScanModule(local_args->rtxn, local_args->dbi, local_args->response);
}

void latencyScan(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);

  pthread_t thread;
  configureScanModule(rtxn, dbi, request, response);
  struct scan_args args = {.rtxn = rtxn, .dbi = dbi, .response = response};
  pthread_create(&thread,NULL,&latencyScanLocal,(void *)&args);

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
