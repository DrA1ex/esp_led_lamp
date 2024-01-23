#include "storage.cpp"
#include "config.h"

bool StorageGlobal::storage_initialized = false;

template class Storage<Config>;
template class Storage<PresetNames>;
template class Storage<PresetConfigs>;
