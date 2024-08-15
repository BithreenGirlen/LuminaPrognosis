
#include <memory>

#include "json_minimal.h"
#include "cocos2d_config.h"

void ReadPath(char* src, std::unordered_map<size_t, PathData>& pathMap)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	bool bRet = json_minimal::ExtractJsonObject(&src, "paths", &*pBuffer);
	if (!bRet)return;

	char* p = *pBuffer;
	json_minimal::ReadUpToNameEnd(&p, nullptr);
	std::vector<char> vBuffer(1024, '\0');
	for (;;)
	{
		PathData path;

		bRet = json_minimal::ReadUpToNameEnd(&p, nullptr, vBuffer.data(), vBuffer.size());
		if (!bRet)break;
		int iIndex = strtol(vBuffer.data(), nullptr, 10);

		pBuffer = std::make_unique<char*>();
		bRet = json_minimal::ExtractJsonArray(&p, nullptr, &*pBuffer);
		if (!bRet)break;

		char* pp = *pBuffer;
		for (size_t i = 0;;++i)
		{
			bRet = json_minimal::ReadNextArrayValue(&pp, vBuffer.data(), vBuffer.size());
			if (!bRet)break;
			if (i == 0)
			{
				path.strPath = vBuffer.data();
			}
			else
			{
				path.iFileTypes.push_back(strtol(vBuffer.data(), nullptr, 10));
			}
		}
		pathMap.insert({ iIndex, path });
	}
}

void ReadPack(char* src, std::vector<PackData>& packs)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	bool bRet = json_minimal::ExtractJsonObject(&src, "packs", &*pBuffer);
	if (!bRet)return;

	char* p = *pBuffer;
	json_minimal::ReadUpToNameEnd(&p, nullptr);
	std::vector<char> vBuffer(1024, '\0');
	for (;;)
	{
		PackData pack;

		bRet = json_minimal::ReadUpToNameEnd(&p, nullptr, vBuffer.data(), vBuffer.size());
		if (!bRet)break;
		pack.strPackName = vBuffer.data();

		pBuffer = std::make_unique<char*>();
		bRet = json_minimal::ExtractJsonArray(&p, nullptr, &*pBuffer);
		if (!bRet)break;

		char* pp = *pBuffer;
		for (;;)
		{
			bRet = json_minimal::ReadNextArrayValue(&pp, vBuffer.data(), vBuffer.size());
			if (!bRet)break;
			size_t nIndex = strtol(vBuffer.data(), nullptr, 10);
			pack.nIndices.push_back(nIndex);
		}

		packs.push_back(pack);
	}
}

void ReadVersion(char* src, std::unordered_map<std::string, std::string>& versionMap, bool bIsNative)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	bool bRet = json_minimal::ExtractJsonObject(&src, "versions", &*pBuffer);
	if (!bRet)return;

	char* p = *pBuffer;
	pBuffer = std::make_unique<char*>();

	bRet = bIsNative ? json_minimal::ExtractJsonArray(&p, "native", &*pBuffer) : json_minimal::ExtractJsonArray(&p, "import", &*pBuffer);
	if (!bRet)return;

	p = *pBuffer;
	json_minimal::ReadUpToNameEnd(&p, nullptr);

	std::vector<std::string> tempArray;
	std::vector<char> vBuffer(1024, '\0');
	for (;;)
	{
		bRet = json_minimal::ReadNextArrayValue(&p, vBuffer.data(), vBuffer.size());
		if (!bRet)break;
		tempArray.push_back(vBuffer.data());
	}

	for (size_t i = 0; i < tempArray.size() - 1; i += 2)
	{
		std::string strName;
		std::string strVersion;

		if (tempArray.at(i).at(0) == '"')
		{
			strName = &tempArray.at(i)[1];
		}
		else
		{
			strName = tempArray.at(i);
		}
		strVersion = tempArray.at(i + 1);
		versionMap.insert({ strName, strVersion });
	}
}

void ReadUuid(char* src, std::vector<std::string>& uuids)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	bool bRet = json_minimal::ExtractJsonArray(&src, "uuids", &*pBuffer);
	if (!bRet)return;

	char *p = *pBuffer;
	json_minimal::ReadUpToNameEnd(&p, nullptr);
	std::vector<char> vBuffer(1024, '\0');
	for (;;)
	{
		bRet = json_minimal::ReadNextArrayValue(&p, vBuffer.data(), vBuffer.size());
		if (!bRet)break;
		uuids.emplace_back(vBuffer.data());
	}
}

void ReadTypes(char* src, std::unordered_map<int, std::string>& types)
{
	if (src == nullptr)return;

	constexpr char szKey[] = R"("types":)";
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	bool bRet = json_minimal::ExtractJsonObject(&src, szKey, &*pBuffer);
	if (!bRet)return;

	char* p = *pBuffer;
	json_minimal::ReadUpToNameEnd(&p, nullptr);
	std::vector<char> vBuffer(1024, '\0');
	for (int i = 0;;++i)
	{
		bRet = json_minimal::ReadNextArrayValue(&p, vBuffer.data(), vBuffer.size());
		if (!bRet)break;
		types.insert({ i, vBuffer.data()});
	}

}
/*資源経路情報読み取り*/
void ReadConfig(char* src, ResourceInfo& r)
{
	ReadUuid(src, r.uuids);
	ReadPath(src, r.paths);
	ReadVersion(src, r.importVersion, false);
	ReadVersion(src, r.nativeVersion, true);
	ReadPack(src, r.packs);
	ReadTypes(src, r.types);
}
/*設定ファイル読み取り*/
void ReadSetting(char* src, std::vector<BundleVer>& bundles)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	std::vector<char> nameBuffer(1024, '\0');
	std::vector<char> valueBuffer(1024, '\0');

	bool bRet = json_minimal::ExtractJsonObject(&src, "bundleVers", &*pBuffer);
	if (!bRet)return;
	char* p = *pBuffer;
	json_minimal::ReadUpToNameEnd(&p, nullptr);

	for (;;)
	{
		BundleVer a;
		bRet = json_minimal::ReadNextKey(&p, nameBuffer.data(), nameBuffer.size(), valueBuffer.data(), valueBuffer.size());
		if (!bRet)break;

		a.strRawName = nameBuffer.data();
		a.strVersion = valueBuffer.data();
		bundles.push_back(a);
	}
}
/*梱包ファイル索引番号探索*/
long long SearchPackIndices(size_t nIndex, const std::vector<PackData>& packs)
{
	for (size_t i = 0; i < packs.size(); ++i)
	{
		auto& pack = packs.at(i);
		bool bFound = false;
		for (size_t ii = 0; ii < pack.nIndices.size(); ++ii)
		{
			if (nIndex == pack.nIndices.at(ii))
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)continue;
		return i;
	}
	return -1;
}

std::string GetExtension(int iType, const std::unordered_map<int, std::string>& types)
{
	const std::unordered_map<std::string, std::string> extensionMap =
	{
		{"cc.AudioClip", ".mp3"},
		{"cc.Asset", ".atlas"},
		{"cc.BitmapFont", ""},
		{"cc.BufferAsset", ""},
		{"cc.ImageAsset", ".jpg"},
		{"cc.JsonAsset", ""},
		{"cc.Material", ""},
		{"cc.ParticleAsset", ".plist"},
		{"cc.Prefab, ", ".prefab"},
		{"cc.RenderTexture", ""},
		{"cc.SpriteFrame", ""},
		{"cc.SpriteAtlas", ".atlas"},
		{"cc.TextAsset", ""},
		{"cc.Texture2D", ""},
		{"cc.TiledMapAsset", ""},
		{"cc.TTFFont", ""},
		{"cc.VideoClip", ".mp4"},
		{"sp.SkeletonData", ".skel"}
	};

	const auto &iter = types.find(iType);
	if (iter != types.cend())
	{
		const auto& iter2 = extensionMap.find(iter->second);
		if (iter2 != extensionMap.cend())
		{
			return iter2->second;
		}
	}
	return std::string();
}
