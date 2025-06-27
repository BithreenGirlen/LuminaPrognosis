

#include <string>
#include <vector>
#include <unordered_map>

#include "file_utility.h"
#include "cocos2d_uuid.h"
#include "cocos2d_config.h"

namespace mirai_girl
{
	const char szHostUrl[] = "https://cdn-app.miraigirl.net/";

	/*csvファイル群*/
	//const char szMasterCsv[] = "https://cdn-app.miraigirl.net/master/master.binary";
	//const char szPassword[] = "DgT2WT0lHEKbaCmF";

	/*URL連結*/
	std::string ChainHostUrl(const std::string& strRelativeUrl)
	{
		std::string strUrl = szHostUrl;

		size_t nPos = 0;
		if (!strRelativeUrl.empty())
		{
			if (strRelativeUrl.at(0) == '.')++nPos;

			if (nPos < strRelativeUrl.size())
			{
				const char cHostEnd = strUrl.back();
				const char cRelativeBegin = strRelativeUrl.at(nPos);

				if (cHostEnd != '/' && cRelativeBegin != '/')
				{
					strUrl += "/";
				}
				else if (cRelativeBegin == '/')++nPos;
			}
		}

		strUrl += &strRelativeUrl[nPos];
		return strUrl;
	}
	/*設定ファイルURL生成*/
	std::string CreateConfigFileUrl(const BundleVer& bundle)
	{
		std::string strUrl = szHostUrl;
		strUrl += "assets/";
		strUrl += bundle.strRawName + "/config." + bundle.strVersion + ".json";
		return strUrl;
	}
	/*素材ファイルURL生成*/
	std::string CreateAssetsFileUrl(const std::string& configName, const std::string& uuid, const std::string strVer, bool bIsNative)
	{
		std::string strUrl = szHostUrl;
		strUrl += "assets/";
		strUrl += configName;
		strUrl += "/";
		strUrl += bIsNative ? "native/" : "import/";

		std::string strUuid = DecodeUuid(uuid);
		strUrl += strUuid.substr(0, 2) + '/' + strUuid + '.';
		strUrl += strVer;
		return strUrl;
	}
	/*梱包ファイルURL生成*/
	std::string CreatePackJsonUrl(const std::string& configName, const std::string& packName, const std::string strVer)
	{
		std::string strUrl = szHostUrl;
		strUrl += "assets/";
		strUrl += configName;
		strUrl += "/import/";

		strUrl += packName.substr(0, 2) + '/' + packName + '.';
		strUrl += strVer;
		strUrl += ".json";
		return strUrl;
	}

	/*画像復号*/
	void DecryptImage(std::string& encrypted)
	{
		const std::string strSalt = "2fjaykPFd6bAJn59beX5TWDQzsEW";
		std::string strTable = std::to_string(encrypted.size()) + strSalt;
		std::string strKey;

		char c = 0x00;
		for (size_t i = 0; i < 32; ++i)
		{
			strKey.push_back(c ^ strTable[i % strTable.size()]);
			c = strKey.back();
		}

		for (size_t i = 0; i < encrypted.size(); ++i)
		{
			encrypted[i] ^= strKey[i % strKey.size()];
		}
	}
}

/*電子網資源先からファイル名取り出し*/
std::string ExtractFileNameFromInternetResource(const std::string& strRelativePath, const char* start, const char end, size_t nOffset)
{
	std::string strResult;
	std::string strUrl = mirai_girl::ChainHostUrl(strRelativePath.c_str());

	char* pBuffer = nullptr;
	unsigned long ulSize = 0;
	bool bRet = LoadInternetResourceToBuffer(strUrl.c_str(), &pBuffer, &ulSize);
	if (bRet)
	{
		char* p = strstr(pBuffer, start);
		if (p != nullptr)
		{
			p += nOffset;
			char* pp = strchr(p, end);
			if (pp != nullptr)
			{
				size_t nLen = pp - p;
				strResult = std::string(p, nLen);
			}
		}
		free(pBuffer);
	}
	return strResult;
}

/*JavaScript文字列整理*/
std::string TidyUpJsString(const std::string& strSrc, bool bSingleQuote)
{
	const char cQuote = bSingleQuote ? '\'' : '"';
	std::string strResult;

	int iCount = 0;
	for (size_t nRead = 0; nRead < strSrc.size(); ++nRead)
	{
		if (strSrc.at(nRead) == cQuote)
		{
			++iCount;
			continue;
		}

		if (iCount & 0x01)
		{
			strResult.push_back(strSrc.at(nRead));
		}
	}

	return iCount & 0x01 ? std::string() : strResult;
}

/*設定ファイル名取得*/
std::string GetSettingJsonFileName()
{
	/*
	* Trace the following files.
	*
	* 1. (HostUrl)/
	* => System.import('./index.XXXXX.js')
	* 
	* 2. (HostUrl)/index.XXXXX.js
	* => System.register(["./application.XXXXX.js"]
	* 
	* 3. (HostUrl)/application.XXXXX.js
	* => this.settingsPath = 'src/settings.XXXXX.json';
	*/

	std::string strKey = "System.import(";
	std::string strTemp = ExtractFileNameFromInternetResource("", strKey.c_str(), ')', strKey.size());
	if (strTemp.empty())return std::string();

	strTemp = TidyUpJsString(strTemp, true);
	if (strTemp.empty())return std::string();

	strKey = "System.register([";
	strTemp = ExtractFileNameFromInternetResource(strTemp, strKey.c_str(), ']', strKey.size());
	if (strTemp.empty())return std::string();

	strTemp = TidyUpJsString(strTemp, false);
	if (strTemp.empty())return std::string();

	strKey = "this.settingsPath";
	strTemp = ExtractFileNameFromInternetResource(strTemp, strKey.c_str(), ';', strKey.size());
	if (strTemp.empty())return std::string();

	strTemp = TidyUpJsString(strTemp, true);
	if (strTemp.empty())return std::string();

	return strTemp;
}

std::string GetSettingJson()
{
	std::string strJsonFileName = GetSettingJsonFileName();
	if (strJsonFileName.empty())return std::string();

	std::string strResult;

	std::string strUrl = mirai_girl::ChainHostUrl(strJsonFileName);
	char* pBuffer = nullptr;
	unsigned long ulSize = 0;
	LoadInternetResourceToBuffer(strUrl.c_str(), &pBuffer, &ulSize);
	if (pBuffer != nullptr)
	{
		strResult = pBuffer;
		free(pBuffer);
	}
	return strResult;
}
/*設定ファイルから必要情報抜粋*/
void PickupDataFromBundleConfigFile(const BundleVer& bundle, ResourceInfo &r)
{
	std::string strUrl = mirai_girl::CreateConfigFileUrl(bundle);
	unsigned long ulSize = 0;
	char* pBuffer = nullptr;
	bool bRet = LoadInternetResourceToBuffer(strUrl.c_str(), &pBuffer, &ulSize);
	if (bRet)
	{
		ReadConfig(pBuffer, r);
		free(pBuffer);
		pBuffer = nullptr;
	}
}
/*画像を復号して保存*/
bool DecryptAndSaveImage(const char* pzUrl, const char* pzFileName, const char* pzBaseFolder)
{
	if (pzUrl == nullptr || pzFileName == nullptr)return false;

	std::string strFileData;
	unsigned long ulSize = 0;
	char* pBuffer = nullptr;
	LoadInternetResourceToBuffer(pzUrl, &pBuffer, &ulSize);
	if (pBuffer != nullptr)
	{
		strFileData.resize(ulSize);
		memcpy(&strFileData[0], pBuffer, ulSize);
		mirai_girl::DecryptImage(strFileData);
		free(pBuffer);
	}
	else
	{
		return false;
	}

	std::string strFilePath = CreateFolderBasedOnRelativeUrl(pzFileName, pzBaseFolder == nullptr ? GetBaseFolderPath() : pzBaseFolder, 0, true);

	if (DoesFilePathExist(strFilePath.c_str()))return true;

	return SaveStringToFile(strFileData, strFilePath.c_str());
}

bool SwitchImageExtension(std::string& strExtension)
{
	const char* p1 = strstr(strExtension.c_str(), ".jpg");
	const char* p2 = strstr(strExtension.c_str(), ".png");
	if (p1 == nullptr && p2 == nullptr)
	{
		return false;
	}
	strExtension = p1 != nullptr ? ".png" : ".jpg";
	return true;
}

void DownloadBundleResources(const BundleVer& bundle)
{
	ResourceInfo r;
	PickupDataFromBundleConfigFile(bundle, r);

	std::string strBaseFolder = CreateNestedWorkFolder(std::string("assets/").append(bundle.strRawName));

	for (size_t i = 0; i < r.uuids.size(); ++i)
	{
		auto path = r.paths.find(i);
		if (path == r.paths.end())continue;

		std::string strRawName = TruncateFileName(path->second.strPath);
		std::string strExtension = GetExtension(path->second.iFileTypes.at(0), r.types);

		/*一般資源、並びSpine/HCGは暗号化されていない*/
		bool bToBeDecrypted = strstr(path->second.strPath.c_str(), "Texture/HCG") != nullptr;

		/*import側は脚本ファイル・skeletonファイルのみ保存*/
		if (strstr(path->second.strPath.c_str(), "Scenario_csv/") != nullptr ||
			(strstr(path->second.strPath.c_str(), "Spine/") != nullptr && path->second.iFileTypes.at(0) == 9)
			)
		{
			auto iter = r.importVersion.find(std::to_string(i));
			if (iter != r.importVersion.end())
			{
				std::string strUrl = mirai_girl::CreateAssetsFileUrl(bundle.strRawName, r.uuids.at(i), iter->second, false);
				strExtension = ".json";
				strUrl += strExtension;
				std::string strFileName = path->second.strPath;
				strFileName += strExtension;
				bool bRet = SaveInternetResourceToFileCreatingNestedFolder(strUrl.c_str(), strFileName.c_str(), strBaseFolder.c_str());
				continue;
			}
		}

		auto iter2 = r.nativeVersion.find(std::to_string(i));
		if (iter2 != r.nativeVersion.end())
		{
			std::string strUrlName = mirai_girl::CreateAssetsFileUrl(bundle.strRawName, r.uuids.at(i), iter2->second, true);
			std::string strUrl = strUrlName + strExtension;
			std::string strFileName = path->second.strPath;
			strFileName += strExtension;
			if (bToBeDecrypted)
			{
				bool bRet = DecryptAndSaveImage(strUrl.c_str(), strFileName.c_str(), strBaseFolder.c_str());
				if (!bRet)
				{
					bRet = SwitchImageExtension(strExtension);
					if (!bRet)
					{
						continue;
					}

					strUrl = strUrlName + strExtension;
					strFileName = path->second.strPath + strExtension;
					DecryptAndSaveImage(strUrl.c_str(), strFileName.c_str(), strBaseFolder.c_str());
				}
			}
			else
			{
				bool bRet = SaveInternetResourceToFileCreatingNestedFolder(strUrl.c_str(), strFileName.c_str(), strBaseFolder.c_str());
				if (!bRet)
				{
					bRet = SwitchImageExtension(strExtension);
					if (!bRet)
					{
						continue;
					}
					strUrl = strUrlName + strExtension;
					strFileName = path->second.strPath + strExtension;

					SaveInternetResourceToFileCreatingNestedFolder(strUrl.c_str(), strFileName.c_str(), strBaseFolder.c_str());
				}
			}
		}

	}
}

void GetResources()
{
	std::vector<BundleVer> bundles;
	std::string strSettingJson = GetSettingJson();
	if (strSettingJson.empty())return;

	ReadSetting(&strSettingJson[0], bundles);

	for (const auto& bundle : bundles)
	{
		if (strstr(bundle.strRawName.c_str(), "resources") == nullptr)continue;
		DownloadBundleResources(bundle);
	}
}

int main()
{
	GetResources();
}