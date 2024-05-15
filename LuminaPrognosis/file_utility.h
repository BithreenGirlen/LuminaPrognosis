#ifndef FILE_UTILITY_H_
#define FILE_UTILITY_H_

#include <string>


/*ファイル操作系*/

std::string GetBaseFolderPath();
std::string CreateWorkFolder(const char* pzFolderName);
std::string CreateNestedWorkFolder(const std::string& strRelativePath);
std::string CreateFolderBasedOnRelativeUrl(const std::string& strUrl, const std::string& strBaseFolder, int iDepth, bool bBeFilePath);

char* LoadExistingFile(const char* pzFilePath);
bool SaveStringToFile(const std::string& strData, const char* pzFilePath);
bool DoesFilePathExist(const char* pzFilePath);

/*電子網系*/
bool SaveInternetResourceToFile(const char* pzUrl, const char* pzFolder, const char* pzFileName, unsigned long nMinFileSize, bool bFolderCreation);
bool LoadInternetResourceToBuffer(const char* url, char** dst, unsigned long *ulSize);

std::string TruncateFileName(const std::string& strFilePath);
std::string RemoveExtension(const std::string& strFilePath);
std::string ExtractExtension(const std::string& strFilePath);

bool SaveInternetResourceToFileCreatingNestedFolder(const char* pzUrl, const char* pzFileName, const char* pzBaseFolder);

#endif //FILE_UTILITY_H_
