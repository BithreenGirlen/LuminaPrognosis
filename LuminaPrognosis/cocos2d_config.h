#ifndef COCOS2D_CONFIG_H_
#define COCOS2D_CONFIG_H_

#include <string>
#include <vector>
#include <unordered_map>

struct BundleVer
{
	std::string strRawName;
	std::string strVersion;
};

struct PathData
{
	std::string strPath;
	std::vector<int> iFileTypes;
};

struct PackData
{
	std::string strPackName;
	std::vector<size_t> nIndices;
};

struct ResourceInfo
{
	std::vector<std::string> uuids;
	std::unordered_map<size_t, PathData> paths;
	std::vector<PackData> packs;
	std::unordered_map<std::string, std::string> importVersion;
	std::unordered_map<std::string, std::string> nativeVersion;
	std::unordered_map<int, std::string> types;
};

void ReadConfig(char* src, ResourceInfo& r);
void ReadSetting(char* src, std::vector<BundleVer>& bundles);
long long SearchPackIndices(size_t nIndex, const std::vector<PackData>& packs);
std::string GetExtension(int iType, const std::unordered_map<int, std::string> &types);

#endif // COCOS2D_CONFIG_H_