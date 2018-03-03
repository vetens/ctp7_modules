/*! \file amc.cpp
 *  \brief AMC methods for RPC modules
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 */

#include "utils.h"

/*! \fn void getmonTTCmainLocal(localArgs * la)
 *  \brief Local version of getmonTTCmain
 *  \param la Local arguments
 */
void getmonTTCmainLocal(localArgs * la)
{
  LOGGER->log_message(LogManager::INFO, "Called getmonTTCmainLocal");
  la->response->set_word("MMCM_LOCKED",readReg(la,"GEM_AMC.TTC.STATUS.MMCM_LOCKED"));
  la->response->set_word("TTC_SINGLE_ERROR_CNT",readReg(la,"GEM_AMC.TTC.STATUS.TTC_SINGLE_ERROR_CNT"));
  la->response->set_word("BC0_LOCKED",readReg(la,"GEM_AMC.TTC.STATUS.BC0.LOCKED"));
  la->response->set_word("L1A_ID",readReg(la,"GEM_AMC.TTC.L1A_ID"));
  la->response->set_word("L1A_RATE",readReg(la,"GEM_AMC.TTC.L1A_RATE"));
}

/*! \fn void getmonTTCmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of TTC monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonTTCmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonTTCmainLocal(&la);
  rtxn.abort();
}

/*! \fn void getmonTRIGGERmainLocal(localArgs * la, int NOH)
 *  \brief Local version of getmonTRIGGERmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 */
void getmonTRIGGERmainLocal(localArgs * la, int NOH)
{
  std::string t1,t2;
  la->response->set_word("OR_TRIGGER_RATE",readReg(la,"GEM_AMC.TRIGGER.STATUS.OR_TRIGGER_RATE"));
  for (int i = 0; i < NOH; i++){
    t1 = stdsprintf("OH%s.TRIGGER_RATE",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.TRIGGER_RATE",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
  }
}

/*! \fn void getmonTRIGGERmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of trigger monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonTRIGGERmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonTRIGGERmainLocal(&la, NOH);
  rtxn.abort();
}

/*! \fn void getmonTRIGGEROHmainLocal(localArgs * la, int NOH)
 *  \brief Local version of getmonTRIGGEROHmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 */
void getmonTRIGGEROHmainLocal(localArgs * la, int NOH)
{
  std::string t1,t2;
  for (int i = 0; i < NOH; i++){
    t1 = stdsprintf("OH%s.LINK0_NOT_VALID_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_NOT_VALID_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_NOT_VALID_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_NOT_VALID_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
  }
}

/*! \fn void getmonTRIGGEROHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of trigger monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonTRIGGEROHmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonTRIGGEROHmainLocal(&la, NOH);
  rtxn.abort();
}

/*! \fn void getmonDAQmainLocal(localArgs * la)
 *  \brief Local version of getmonDAQmain
 *  \param la Local arguments
 */
void getmonDAQmainLocal(localArgs * la)
{
  la->response->set_word("DAQ_ENABLE",readReg(la,"GEM_AMC.DAQ.CONTROL.DAQ_ENABLE"));
  la->response->set_word("DAQ_LINK_READY",readReg(la,"GEM_AMC.DAQ.STATUS.DAQ_LINK_RDY"));
  la->response->set_word("DAQ_LINK_AFULL",readReg(la,"GEM_AMC.DAQ.STATUS.DAQ_LINK_AFULL"));
  la->response->set_word("DAQ_OFIFO_HAD_OFLOW",readReg(la,"GEM_AMC.DAQ.STATUS.DAQ_OUTPUT_FIFO_HAD_OVERFLOW"));
  la->response->set_word("L1A_FIFO_HAD_OFLOW",readReg(la,"GEM_AMC.DAQ.STATUS.L1A_FIFO_HAD_OVERFLOW"));
  la->response->set_word("L1A_FIFO_DATA_COUNT",readReg(la,"GEM_AMC.DAQ.EXT_STATUS.L1A_FIFO_DATA_CNT"));
  la->response->set_word("DAQ_FIFO_DATA_COUNT",readReg(la,"GEM_AMC.DAQ.EXT_STATUS.DAQ_FIFO_DATA_CNT"));
  la->response->set_word("EVENT_SENT",readReg(la,"GEM_AMC.DAQ.EXT_STATUS.EVT_SENT"));
  la->response->set_word("TTS_STATE",readReg(la,"GEM_AMC.DAQ.STATUS.TTS_STATE"));
}

/*! \fn void getmonDAQmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of DAQ monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonDAQmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonDAQmainLocal(&la);
  rtxn.abort();
}

/*! \fn void getmonDAQOHmainLocal(localArgs * la, int NOH)
 *  \brief Local version of getmonDAQOHmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 */
void getmonDAQOHmainLocal(localArgs * la, int NOH)
{
  std::string t1,t2;
  for (int i = 0; i < NOH; i++){
    t1 = stdsprintf("OH%s.STATUS.EVT_SIZE_ERR",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.EVENT_FIFO_HAD_OFLOW",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.INPUT_FIFO_HAD_OFLOW",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.INPUT_FIFO_HAD_UFLOW",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.VFAT_TOO_MANY",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.VFAT_NO_MARKER",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
  }
}

/*! \fn void getmonDAQOHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of DAQ monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonDAQOHmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonDAQOHmainLocal(&la, NOH);
  rtxn.abort();
}

/*! \fn void getmonOHmainLocal(localArgs * la, int NOH)
 *  \brief Local version of getmonOHmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 */
void getmonOHmainLocal(localArgs * la, int NOH)
{
  std::string t1,t2;
  for (int i = 0; i < NOH; i++){
    t1 = stdsprintf("OH%s.FW_VERSION",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.STATUS.FW.VERSION",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.EVENT_COUNTER",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.EVN",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.EVENT_RATE",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.EVT_RATE",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GTX.TRK_ERR",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GTX_LINK.TRK_ERR",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GTX.TRG_ERR",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GTX_LINK.TRG_ERR",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GBT.TRK_ERR",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GBT_LINK.TRK_ERR",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.CORR_VFAT_BLK_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.CORRUPT_VFAT_BLK_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
  }
}

/*! \fn void getmonOHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of OH monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonOHmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonOHmainLocal(&la, NOH);
  rtxn.abort();
}

extern "C" {
	const char *module_version_key = "amc v1.0.1";
	int module_activity_color = 4;
	void module_init(ModuleManager *modmgr) {
		if (memsvc_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("amc", "getmonTTCmain", getmonTTCmain);
		modmgr->register_method("amc", "getmonTRIGGERmain", getmonTRIGGERmain);
		modmgr->register_method("amc", "getmonTRIGGEROHmain", getmonTRIGGEROHmain);
		modmgr->register_method("amc", "getmonDAQmain", getmonDAQmain);
		modmgr->register_method("amc", "getmonDAQOHmain", getmonDAQOHmain);
		modmgr->register_method("amc", "getmonOHmain", getmonOHmain);
	}
}
