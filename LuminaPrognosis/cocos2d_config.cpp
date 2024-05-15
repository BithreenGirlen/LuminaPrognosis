
#include <memory>

#include "json_minimal.h"
#include "cocos2d_config.h"

void ReadPath(char* src, std::unordered_map<size_t, PathData>& pathMap)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	char* q = nullptr;
	char* qq = nullptr;

	bool bRet = ExtractJsonObject(&src, "paths", &*pBuffer);
	if (!bRet)return;
	qq = *pBuffer + strlen("paths\":[");

	std::vector<char> vBuffer;
	vBuffer.resize(256);

	for (;;)
	{
		std::unique_ptr<char*> pBuffer2 = std::make_unique<char*>();
		PathData path;
		q = strchr(qq, '"');
		if (q == nullptr)break;

		++q;
		qq = strchr(q, '"');
		if (qq == nullptr)break;
		size_t nLen = qq - q;

		std::string strIndex = std::string(q, nLen);
		int iIndex = strtol(strIndex.c_str(), nullptr, 10);

		bRet = ExtractJsonArray(&q, strIndex.c_str(), &*pBuffer2);
		if (!bRet)break;
		qq = *pBuffer2 + nLen + 2;

		for (size_t i = 0;;++i)
		{
			bRet = ReadNextArrayValue(&qq, vBuffer.data(), vBuffer.size());
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
		qq = q + 1;
	}
}

void ReadPack(char* src, std::vector<PackData>& packs)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	char* q = nullptr;
	char* qq = nullptr;

	bool bRet = ExtractJsonObject(&src, "packs", &*pBuffer);
	if (!bRet)return;
	qq = *pBuffer + strlen("packs\":[");

	for (;;)
	{
		std::unique_ptr<char*> pBuffer2 = std::make_unique<char*>();
		PackData pack;
		q = strchr(qq, '"');
		if (q == nullptr)break;

		++q;
		qq = strchr(q, '"');
		if (qq == nullptr)break;
		size_t nLen = qq - q;
		pack.strPackName = std::string(q, nLen);

		bRet = ExtractJsonArray(&q, pack.strPackName.c_str(), &*pBuffer2);
		qq = *pBuffer2 + nLen + 2;
		if (!bRet)break;

		for (;;)
		{
			char sBuffer[32]{};
			bRet = ReadNextArrayValue(&qq, sBuffer, sizeof(sBuffer));
			if (!bRet)break;
			size_t nIndex = strtol(sBuffer, nullptr, 10);
			pack.nIndices.push_back(nIndex);
		}

		packs.push_back(pack);
		qq = q + 1;
	}
}

void ReadVersion(char* src, std::unordered_map<std::string, std::string>& versionMap, bool bIsNative)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	std::unique_ptr<char*> pBuffer2 = std::make_unique<char*>();
	char* q = nullptr;
	char* qq = nullptr;

	bool bRet = ExtractJsonObject(&src, "versions", &*pBuffer2);
	if (!bRet)return;

	bRet = bIsNative ? ExtractJsonArray(&*pBuffer2, "native", &*pBuffer) : ExtractJsonArray(&*pBuffer2, "import", &*pBuffer);
	if (!bRet)return;

	qq = bIsNative ? *pBuffer + strlen("native:\"[") : *pBuffer + strlen("import:\"[");

	for (;;)
	{
		char sBuffer[256]{};
		std::string strName;
		std::string strVersion;
		q = strchr(qq, ',');
		if (q == nullptr)break;
		size_t nLen = q - qq;
		if (nLen > sizeof(sBuffer))break;
		memcpy(sBuffer, qq, nLen);
		*(sBuffer + nLen) = '\0';
		size_t nIndex = strtol(sBuffer, nullptr, 10);
		if (sBuffer[0] == '"')
		{
			*(sBuffer + nLen - 1LL) = '\0';
			strName = &sBuffer[1];
		}
		else
		{
			strName = sBuffer;
		}

		qq = strchr(q, '"');
		if (qq == nullptr)break;
		++qq;

		q = strchr(qq, '"');
		if (q == nullptr)break;
		nLen = q - qq;
		if (nLen > sizeof(sBuffer))break;
		memcpy(sBuffer, qq, nLen);
		*(sBuffer + nLen) = '\0';

		strVersion = sBuffer;
		qq = q + 2;
		versionMap.insert({ strName, strVersion });
	}
}

void ReadUuid(char* src, std::vector<std::string>& uuids)
{
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	bool bRet = ExtractJsonArray(&src, "uuids", &*pBuffer);
	if (!bRet)return;

	char *p = *pBuffer + strlen("uuids\":[");
	std::vector<char> vBuffer;
	vBuffer.resize(256);

	for (;;)
	{
		bRet = ReadNextArrayValue(&p, vBuffer.data(), vBuffer.size());
		if (!bRet)break;
		uuids.emplace_back(vBuffer.data());
		++p;
	}
}

void ReadTypes(char* src, std::unordered_map<int, std::string>& types)
{
	if (src == nullptr)return;

	constexpr char szKey[] = R"("types":)";
	std::unique_ptr<char*> pBuffer = std::make_unique<char*>();
	bool bRet = ExtractJsonObject(&src, szKey, &*pBuffer);
	if (!bRet)return;

	char* p = *pBuffer + sizeof(szKey)/sizeof(char) - 1;
	std::vector<char> vBuffer;
	vBuffer.resize(256);
	for (int i = 0;;++i)
	{
		bRet = ReadNextArrayValue(&p, vBuffer.data(), vBuffer.size());
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
	char* p = nullptr;
	bool bRet = false;

	std::vector<char> nameBuffer;
	nameBuffer.resize(256);
	std::vector<char> valueBuffer;
	valueBuffer.resize(256);

	bRet = ExtractJsonObject(&src, "bundleVers", &*pBuffer);
	if (!bRet)return;
	p = strchr(*pBuffer, ':');

	for (;;)
	{
		BundleVer a;
		bRet = ReadNextKey(&p, nameBuffer.data(), nameBuffer.size(), valueBuffer.data(), valueBuffer.size());
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
