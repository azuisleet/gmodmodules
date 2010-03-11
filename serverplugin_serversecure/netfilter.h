void NetFilter_Load();
void NetFilter_Unload();
void NetFilter_LevelShutdown();

void NetFilter_ClientConnect(INetChannel *channel, edict_t *pEntity);
void NetFilter_ClientDisconnect(INetChannel *channel);

void NetFilter_Tick(const time_t& now);