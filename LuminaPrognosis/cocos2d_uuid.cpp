
#include "cocos2d_uuid.h"

/*UUID解号*/
std::string DecodeUuid(const std::string& src)
{
	const std::string strBase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	const char HexDigits[] = "0123456789abcdef";

	std::string strEncoded;
	size_t nPos = src.find('@');
	if (nPos != std::string::npos)
	{
		strEncoded = src.substr(0, nPos);
	}
	else
	{
		strEncoded = src;
	}
	if (strEncoded.size() != 22)return std::string();

	std::string strDecoded;

	for (size_t i = 2; i < 22; i += 2)
	{
		size_t l = strBase64.find(strEncoded.at(i));
		size_t r = strBase64.find(strEncoded.at(i + 1));
		if (l == std::string::npos || r == std::string::npos)
		{
			return std::string();
		}

		strDecoded += HexDigits[l >> 2];
		strDecoded += HexDigits[((l & 3) << 2) | r >> 4];
		strDecoded += HexDigits[r & 0xF];
	}

	std::string strResult;
	strResult.reserve(36);
	strResult += strEncoded.substr(0, 2);
	strResult += strDecoded.substr(0, 6);
	strResult += '-';
	strResult += strDecoded.substr(6, 4);
	strResult += '-';
	strResult += strDecoded.substr(10, 4);
	strResult += '-';
	strResult += strDecoded.substr(14, 4);
	strResult += '-';
	strResult += strDecoded.substr(18);

	return strResult;
}

/*UUID編号*/
std::string EncodeUuid(const std::string& src)
{
	const char Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	const std::string strHexDigits = "0123456789abcdef";

	std::string strRaw;
	size_t nPos = src.find('@');
	if (nPos != std::string::npos)
	{
		strRaw = src.substr(0, nPos);
	}
	else
	{
		strRaw = src;
	}
	if (strRaw.size() != 36)return std::string();

	std::string strTrimmed;
	size_t nRead = 0;
	for (;;)
	{
		nPos = strRaw.substr(nRead).find('-');
		if (nPos == std::string::npos)
		{
			strTrimmed += strRaw.substr(nRead);
			break;
		}

		strTrimmed += strRaw.substr(nRead, nPos);
		nRead += nPos + 1;
	}

	std::string strResult;
	strResult.reserve(22);

	strResult += strTrimmed.substr(0, 2);

	for (size_t i = 2; i < 32; i += 3)
	{
		size_t l = strHexDigits.find(strTrimmed.at(i));
		size_t m = strHexDigits.find(strTrimmed.at(i + 1));
		size_t r = strHexDigits.find(strTrimmed.at(i + 2));

		if (l == std::string::npos || m == std::string::npos || r == std::string::npos)
		{
			return std::string();
		}

		strResult += Base64[(l << 2) | (m >> 2)];
		strResult += Base64[((m & 3) << 4) | r];
	}

	return strResult;
}