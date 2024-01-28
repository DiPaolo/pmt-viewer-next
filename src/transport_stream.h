/*******************************************************************************
* File: TransportStream.h
*
* Description: CTransportStream class definition. This class implements
*              MPEG-2 Transport Stream according to ISO/IEC 13818-1 second
*              edition (2000-12-01) specification.
*
* Copyright (c) Ditenbir Pavel, 2007.
*
*******************************************************************************/

#ifndef _TRANSPORT_STREAM_H_
#define _TRANSPORT_STREAM_H_


#include <Windows.h>
#include "Packet.h"

class CTransportStream
{
public:
	CTransportStream(void);
	CTransportStream(LPCTSTR pszFileName);
	~CTransportStream(void);

	BOOL Open(LPCTSTR pszFileName);
	void Close(void);

	BOOL IsMPEG2TS(void) const;

	LPCTSTR GetFileName(void) const;
	DWORD   GetFileSize(void) const;
	UINT    GetPMSCount(void);
	UINT    GetPacketsCount(void) const;

	// random access to PM Sections in a TS
	UINT GetPMSection(UINT uNum, PM_SECTION* pPMS) const;

	// functions for sequential access to PM Sections in a TS
	UINT GetFirstPMSection(PM_SECTION* pPMS, UINT* uPMSNum = NULL);
	UINT GetLastPMSection (PM_SECTION* pPMS, UINT* uPMSNum = NULL);
	UINT GetNextPMSection (PM_SECTION* pPMS, UINT* uPMSNum = NULL);
	UINT GetPrevPMSection (PM_SECTION* pPMS, UINT* uPMSNum = NULL);

private:
    HANDLE m_hFile;
	TCHAR  m_szFileName[MAX_PATH];
	UINT   m_uPMSCount; // count of PM Section in TS

	// zero-based variables used by functions for sequential access to PM Sections
	UINT m_uCurPMS;       // number of current PMS
	UINT m_uCurPMSPacket; // number of current packet that contains PM Section
};


#endif // _TRANSPORT_STREAM_H_