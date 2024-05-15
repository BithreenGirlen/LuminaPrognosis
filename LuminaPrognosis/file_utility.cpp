/*ファイル操作・コンソール出力*/

#include <Windows.h>
#include <urlmon.h>
#include <atlbase.h>

#include <string>

#include "file_utility.h"

#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "shlwapi.lib")


struct ComInit
{
	HRESULT m_hrComInit;
	ComInit() : m_hrComInit(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {}
	~ComInit() { if (SUCCEEDED(m_hrComInit)) ::CoUninitialize(); }
};

/*-----------------------  内部関数  -----------------------*/

void WriteMessage(const char* msg);

bool WriteStringToFile(const std::string& data, const char* file_path);

bool GetInternetResourceSize(const char* url, ULARGE_INTEGER& size);
std::string GetFileNameFromUrl(const std::string& url);
std::string GetDirectoryFromUrl(const std::string& url);


/*伝文出力*/
void WriteMessage(const char* msg)
{
	char stamp[16];
	SYSTEMTIME tm;
	::GetLocalTime(&tm);
	sprintf_s(stamp, "%02d:%02d:%02d:%03d ", tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
	std::string str = stamp + std::string(msg) + "\r\n";
	printf(str.c_str());
}

/*実行プロセスの階層取得*/
std::string GetBaseFolderPath()
{
	char pzPath[MAX_PATH]{};
	::GetModuleFileNameA(nullptr, pzPath, MAX_PATH);
	std::string::size_type pos = std::string(pzPath).find_last_of("\\/");
	return std::string(pzPath).substr(0, pos) + "\\/";
}
/*作業フォルダ作成*/
std::string CreateWorkFolder(const char* pzFolderName)
{
	std::string strFolder = GetBaseFolderPath();
	if (pzFolderName != nullptr)
	{
		strFolder += pzFolderName;
		strFolder += "\\/";
		::CreateDirectoryA(strFolder.c_str(), nullptr);
	}
	return strFolder;
}
/*入れ子作業フォルダ作成*/
std::string CreateNestedWorkFolder(const std::string &strRelativePath)
{
	if (!strRelativePath.empty())
	{
		std::string strFolder = GetBaseFolderPath();
		size_t nRead = 0;
		for (;;)
		{
			size_t nPos = strRelativePath.substr(nRead).find_first_of("\\/");
			if (nPos == std::string::npos)
			{
				strFolder += strRelativePath.substr(nRead) +"\\/";
				::CreateDirectoryA(strFolder.c_str(), nullptr);
				break;
			}

			strFolder += strRelativePath.substr(nRead, nPos) + "\\/";
			::CreateDirectoryA(strFolder.c_str(), nullptr);
			nRead += nPos + 1;
		}

		return strFolder;
	}

	return std::string();
}
/*相対URLを元にフォルダ作成*/
std::string CreateFolderBasedOnRelativeUrl(const std::string& strUrl, const std::string& strBaseFolder, int iDepth, bool bBeFilePath)
{
	if (!strUrl.empty())
	{
		std::string strFolder = strBaseFolder;
		size_t nRead = 0;
		int iOccurrence = 0;
		for (;;)
		{
			size_t nPos = strUrl.substr(nRead).find_first_of("/");
			if (nPos == std::string::npos)
			{
				if (bBeFilePath)
				{
					strFolder += strUrl.substr(nRead);
				}
				break;
			}
			++iOccurrence;
			if (iOccurrence > iDepth && nPos != 0)
			{
				strFolder += strUrl.substr(nRead, nPos) + "\\/";
				::CreateDirectoryA(strFolder.c_str(), nullptr);
			}
			nRead += nPos + 1;
		}

		return strFolder;
	}

	return std::string();
}

/*ファイルのメモリ展開*/
char* LoadExistingFile(const char* pzFilePath)
{
	HANDLE hFile = ::CreateFileA(pzFilePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwSize = ::GetFileSize(hFile, nullptr);
		if (dwSize != INVALID_FILE_SIZE)
		{
			char* buffer = static_cast<char*>(malloc(static_cast<size_t>(dwSize + 1ULL)));
			if (buffer != nullptr)
			{
				DWORD dwRead = 0;
				BOOL iRet = ::ReadFile(hFile, buffer, dwSize, &dwRead, nullptr);
				if (iRet)
				{
					::CloseHandle(hFile);
					*(buffer + dwRead) = '\0';
					return buffer;
				}
				else
				{
					free(buffer);
				}
			}
		}
		::CloseHandle(hFile);
	}

	return nullptr;
}
/*メモリのファイル出力*/
bool WriteStringToFile(const std::string& strData, const char* file_path)
{
	BOOL iRet = 0;

	if (file_path != nullptr)
	{
		HANDLE hFile = ::CreateFileA(file_path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			::SetFilePointer(hFile, NULL, nullptr, FILE_END);

			DWORD bytesWrite = 0;
			iRet = ::WriteFile(hFile, strData.data(), static_cast<DWORD>(strData.size()), &bytesWrite, nullptr);
			::CloseHandle(hFile);
		}
	}
	return iRet > 0;
}
/*保存処理メッセージ出力*/
bool SaveStringToFile(const std::string& strData, const char* pzFilePath)
{
	bool bRet = false;

	if (pzFilePath != nullptr)
	{
		std::string strFileName = GetFileNameFromUrl(pzFilePath);
		if (!strFileName.empty())
		{
			bRet = WriteStringToFile(strData, pzFilePath);
			if (bRet)
			{
				WriteMessage(std::string(strFileName).append(" success.").c_str());
			}
			else
			{
				WriteMessage(std::string(strFileName).append(" failed.").c_str());
			}
		}
	}

	return bRet;
}
/*ファイル有無確認*/
bool DoesFilePathExist(const char* pzFilePath)
{
	BOOL iRet = ::PathFileExistsA(pzFilePath);
	if (iRet)
	{
		std::string strFileName = GetFileNameFromUrl(pzFilePath);
		WriteMessage(std::string(strFileName).append(" already exists.").c_str());
	}

	return iRet == TRUE;
}

/*電子網資源の大きさ取得*/
bool GetInternetResourceSize(const char* url, ULARGE_INTEGER& size)
{
	ComInit init;
	CComPtr<IStream> pStream;
	HRESULT hr = ::URLOpenBlockingStreamA(nullptr, url, &pStream, 0, nullptr);

	if (hr == S_OK)
	{
		STATSTG stat;
		hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
		if (hr == S_OK)
		{
			size = stat.cbSize;
			return true;
		}
	}
	return false;
}
/*URLからファイル名取得*/
std::string GetFileNameFromUrl(const std::string &url)
{
	size_t pos = url.find_last_of("/");
	if (pos != std::string::npos)
	{
		return url.substr(pos + 1);
	}

	return std::string();
}
/*URLから階層取得*/
std::string GetDirectoryFromUrl(const std::string &url)
{
	size_t pos = url.find_last_of("/");
	if (pos != std::string::npos)
	{
		return url.substr(0, pos + 1);
	}

	return std::string();
}
/*電子網資源の保存*/
bool SaveInternetResourceToFile(const char* pzUrl, const char* pzFolder, const char* pzFileName, unsigned long nMinFileSize, bool bFolderCreation)
{
	std::string strFileName = pzFileName == nullptr ? GetFileNameFromUrl(pzUrl) : strchr(pzFileName, '/') == nullptr ? pzFileName : GetFileNameFromUrl(pzFileName);
	if (!strFileName.empty())
	{
		std::string strFilePath = pzFolder + strFileName;

		if (!::PathFileExistsA(strFilePath.c_str()))
		{
			ULARGE_INTEGER size{};
			bool bRet = GetInternetResourceSize(pzUrl, size);
			if (bRet)
			{
				if (size.LowPart > nMinFileSize)
				{
					if (bFolderCreation)
					{
						::CreateDirectoryA(pzFolder, nullptr);
					}
					ComInit init;
					HRESULT hr = ::URLDownloadToFileA(nullptr, pzUrl, strFilePath.c_str(), 0, nullptr);
					if (hr == S_OK)
					{
						WriteMessage(std::string(pzUrl).append(" success").c_str());
						return true;
					}
					else
					{
						WriteMessage(std::string(pzUrl).append(" failed").c_str());
					}
				}

			}
		}
		else
		{
			WriteMessage(std::string(strFileName).append(" already exists.").c_str());
			return true;
		}
	}
	else
	{
		WriteMessage("File path invalid.");
	}

	return false;
}
/*電子網資源のメモリ展開*/
bool LoadInternetResourceToBuffer(const char* url, char** dst, unsigned long* ulSize)
{
	if (url == nullptr || *dst != nullptr)return false;

	ComInit init;
	CComPtr<IStream> pStream;
	HRESULT hr = ::URLOpenBlockingStreamA(nullptr, url, &pStream, 0, nullptr);

	if (hr == S_OK)
	{
		STATSTG stat;
		hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
		if (hr == S_OK)
		{
			char* buffer = static_cast<char*>(malloc(stat.cbSize.LowPart + 1LL));
			if (buffer != nullptr)
			{
				DWORD dwReadBytes = 0;
				DWORD dwSize = 0;
				for (;;)
				{
					hr = pStream->Read(buffer + dwSize, stat.cbSize.LowPart - dwSize, &dwReadBytes);
					if (FAILED(hr))break;
					dwSize += dwReadBytes;
					if (dwSize >= stat.cbSize.LowPart)break;
				}
				*(buffer + dwSize) = '\0';
				*ulSize = dwSize;
				*dst = buffer;

				return true;
			}
		}
	}

	return false;
}

std::string TruncateFileName(const std::string& strFilePath)
{
	size_t nPos = strFilePath.rfind('/');
	if (nPos != std::string::npos)
	{
		return strFilePath.substr(nPos + 1);
	}
	else
	{
		return strFilePath;
	}
}

std::string RemoveExtension(const std::string& strFilePath)
{
	size_t nPos = strFilePath.rfind('.');
	return nPos == std::string::npos ? strFilePath : strFilePath.substr(0, nPos);
}

std::string ExtractExtension(const std::string& strFilePath)
{
	size_t nPos = strFilePath.rfind('.');
	return nPos == std::string::npos ? strFilePath : strFilePath.substr(nPos + 1);
}

bool SaveInternetResourceToFileCreatingNestedFolder(const char* pzUrl, const char* pzFileName, const char* pzBaseFolder)
{
	if (pzUrl == nullptr)return false;

	const auto AbsoluteToRelative = [](const std::string& strUrl)
		-> std::string
	{
			const char szKey[] = "https://";

			size_t nPos = strUrl.find(szKey);
			if (nPos == std::string::npos)nPos = 0;
			nPos += sizeof(szKey);
			if (nPos > strUrl.size())return strUrl;

			const char* p = strchr(&strUrl[nPos], '/');
			if (p == nullptr)return strUrl.substr(nPos);

			nPos += p - &strUrl[nPos] + 1;
			return strUrl.substr(nPos);
	};

	std::string strRelativeUrl = AbsoluteToRelative(pzUrl);

	/*
	* ファイル名が元の階層構造名を含む場合はファイル名を基にフォルダ生成
	*/
	std::string strFolderPath;
	if (pzFileName != nullptr && strchr(pzFileName, '/') != nullptr)
	{
		strFolderPath = CreateFolderBasedOnRelativeUrl(pzFileName, pzBaseFolder == nullptr ? GetBaseFolderPath() : pzBaseFolder, 0, false);
	}
	else
	{
		strFolderPath = CreateFolderBasedOnRelativeUrl(strRelativeUrl, pzBaseFolder == nullptr ? GetBaseFolderPath() : pzBaseFolder, 0, false);
	}

	return SaveInternetResourceToFile(pzUrl, strFolderPath.c_str(), pzFileName, 0, false);
}