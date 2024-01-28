/*******************************************************************************
* File: TransportStream.cpp
*
* Description: CTransportStream class implementation.
*              See ISO/IEC 13818-1 second edition (2000-12-01).
*
* Copyright (c) Ditenbir Pavel, 2007.
*
*******************************************************************************/

#include <list>
#include "TransportStream.h"


CTransportStream::CTransportStream(void)
{
	m_hFile = INVALID_HANDLE_VALUE;

	lstrcpy(m_szFileName, TEXT(""));

	m_uCurPMS       = 0;
	m_uCurPMSPacket = 0;
	m_uPMSCount     = 0;
}


CTransportStream::~CTransportStream(void)
{
	Close();
}


BOOL CTransportStream::Open(LPCTSTR pszFileName)
{
	if (m_hFile != INVALID_HANDLE_VALUE)
		Close();

	if (lstrlen(pszFileName) > (sizeof(m_szFileName) / sizeof(m_szFileName[0]) - 1))
		return FALSE;

	lstrcpy(m_szFileName, pszFileName);

	// open file
	m_hFile = CreateFile(m_szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		lstrcpy(m_szFileName, TEXT(""));
		return FALSE;
	}

	return TRUE;
}


void CTransportStream::Close(void)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	lstrcpy(m_szFileName, TEXT(""));

	m_uCurPMS       = 0;
	m_uCurPMSPacket = 0;
	m_uPMSCount     = 0;
}

//
// CTransportStream::IsMPEG2TS
//
// In this function checks only first byte of each packet in a file. The value
// of this byte must be equal SYNC_BYTE value.
BOOL CTransportStream::IsMPEG2TS(void) const
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if (SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return FALSE;

	CPacket	packet;
	BYTE    bPacket[CPacket::PACKET_SIZE] = {0};
	DWORD   dwReaded  = 0; // number of bytes readed from file

	while (TRUE)
	{
		if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
			return 0;

		if (dwReaded == 0)
			// reached end of file
			break;

		packet.Set(bPacket);
		if (!packet.CheckSyncByte())
			return FALSE;
	}

	return TRUE;
}


LPCTSTR CTransportStream::GetFileName(void) const
{
	return m_szFileName;
}

DWORD CTransportStream::GetFileSize(void) const
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;

	DWORD dwSize = ::GetFileSize(m_hFile, NULL);
	if (dwSize == 0xFFFFFFFF)
		return 0;

	return dwSize;
}

//
// GetPMTCount
//
// Returns a count of Program Map Sections in Transport Stream.
UINT CTransportStream::GetPMSCount(void)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;

	if (m_uPMSCount)
		// m_uPMSCount is already known
		return m_uPMSCount;

	if (SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return 0;

	CPacket	   packet;
	PA_SECTION PAS;
	PM_SECTION PMS;
	PATable    PAT;

	BYTE  bPacket[CPacket::PACKET_SIZE] = {0};
	DWORD dwReaded  = 0; // number of bytes readed from file
	UINT  uPMSCount = 0;

	while (TRUE)
	{
		if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
			return 0;

		if (dwReaded == 0)
			// reached end of file
			break;

		packet.Set(bPacket);

		USHORT uPID = packet.GetPID();
		if (uPID == 0)
		{
			// packet contains PA Section
			if (packet.GetPASection(&PAS))
				PAT.assign(PAS.m_PAT.begin(), PAS.m_PAT.end());
		}
		else
		{
			PATable::const_iterator iter;
			for (iter = PAT.begin(); iter != PAT.end(); iter++)
				if (iter->PID == uPID)
					if (packet.GetPMSection(&PMS, PAT))
						uPMSCount++;
		}
	}

	m_uPMSCount = uPMSCount;
	return m_uPMSCount;
}

UINT CTransportStream::GetPacketsCount(void) const
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;

	if (SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return 0;

	CPacket	packet;
	BYTE    bPacket[CPacket::PACKET_SIZE] = {0};
	DWORD   dwReaded      = 0; // number of bytes readed from file
	UINT    uPacketsCount = 0;

	while (TRUE)
	{
		if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
			return 0;

		if (dwReaded == 0)
			// reached end of file
			break;

		uPacketsCount++;
	}

	return uPacketsCount;
}

/*
UINT CTransportStream::GetPMSection(UINT uNum, PM_SECTION* pPMS) const
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;

	if (SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return 0;

	CPacket	   packet;
	PA_SECTION PAS;
	PM_SECTION PMS;
	PATable    PAT;

	BYTE  bPacket[CPacket::PACKET_SIZE] = {0};
	DWORD dwReaded   = 0;
	UINT  uPASNum    = 1; // number of current PA Section
	UINT  uPMSNum    = 1; // number of current PM Section
	UINT  uPacketNum = 1; // number of current packet

	while (TRUE)
	{
		if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
			return 0;

		if (dwReaded == 0)
			// reached end of file
			break;

		packet.Set(bPacket);

		USHORT uPID = packet.GetPID();
		if (uPID == 0)
		{
			// packet contains PA Section
			uPASNum++;
			if (packet.GetPASection(&PAS))
				PAT.assign(PAS.m_PAT.begin(), PAS.m_PAT.end());
		}
		else
		{
			std::list<PROGRAM_DESCRIPTOR>::const_iterator iter;
			for (iter = PAT.begin(); iter != PAT.end(); iter++)
				if (iter->PID == uPID)
					if (packet.GetPMSection(&PMS, PAT))
						// TODO: ckeck != < >
						if (uPMSNum == uNum)
						{
							*pPMS = PMS;
							return uPacketNum;
						}
						else
							uPMSNum++; // TODO: break; ?
		}

		uPacketNum++;
	}

	return 0;
}
*/

UINT CTransportStream::GetFirstPMSection(PM_SECTION* pPMS, UINT* uPMSNum /* = NULL */)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;

	if (SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return 0;

	CPacket	   packet;
	PA_SECTION PAS;
	PM_SECTION PMS;
	PATable    PAT;

	BYTE  bPacket[CPacket::PACKET_SIZE] = {0};
	DWORD dwReaded   = 0; // number of bytes readed from file
	UINT  uPacketNum = 0; // zero-based number of current packet

	while (TRUE)
	{
		if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
			return 0;

		if (dwReaded == 0)
			// reached end of file
			break;

		packet.Set(bPacket);

		USHORT uPID = packet.GetPID();
		if (uPID == 0)
		{
			// packet contains PA Section
			if (packet.GetPASection(&PAS))
				PAT.assign(PAS.m_PAT.begin(), PAS.m_PAT.end());
		}
		else
		{
			std::list<PROGRAM_DESCRIPTOR>::const_iterator iter;
			for (iter = PAT.begin(); iter != PAT.end(); iter++)
				if (iter->PID == uPID)
					if (packet.GetPMSection(&PMS, PAT))
					{
						*pPMS           = PMS;
						m_uCurPMS       = 0; // this is a first PM Section in TS
						m_uCurPMSPacket = uPacketNum;

						if (uPMSNum != NULL)
							*uPMSNum = m_uCurPMS + 1;

						return (m_uCurPMSPacket + 1);
					}
		}

		uPacketNum++;
	}

	// there is no PM Sections in file
	return 0;
}

UINT CTransportStream::GetLastPMSection(PM_SECTION* pPMS, UINT* uPMSNum /* = NULL */)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;

	if (SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return 0;

	CPacket	   packet;
	PA_SECTION PAS;
	PM_SECTION PMS;
	PATable    PAT;

	BYTE  bPacket[CPacket::PACKET_SIZE] = {0};
	DWORD dwReaded      = 0; // number of bytes readed from file
	UINT  uPMSCount     = 0;
	UINT  uCurPMSPacket = 0;
	UINT  uPacketNum    = 0; // zero-based number of current packet

	while (TRUE)
	{
		if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
			return 0;

		if (dwReaded == 0)
			// reached end of file
			break;

		packet.Set(bPacket);

		USHORT uPID = packet.GetPID();
		if (uPID == 0)
		{
			// packet contains PA Section
			if (packet.GetPASection(&PAS))
				PAT.assign(PAS.m_PAT.begin(), PAS.m_PAT.end());
		}
		else
		{
			std::list<PROGRAM_DESCRIPTOR>::const_iterator iter;
			for (iter = PAT.begin(); iter != PAT.end(); iter++)
				if (iter->PID == uPID)
					if (packet.GetPMSection(&PMS, PAT))
					{
						uPMSCount++;
						uCurPMSPacket = uPacketNum;
					}
		}

		uPacketNum++;
	}

	if (uPMSCount == 0)
		// PM Section not found
		return 0;

	*pPMS           = PMS;
	m_uCurPMS       = uPMSCount - 1; // m_uCurPMS is zero-based
	m_uCurPMSPacket = uCurPMSPacket;

	if (uPMSNum != NULL)
		*uPMSNum = m_uCurPMS + 1;

	return (m_uCurPMSPacket + 1);
}

UINT CTransportStream::GetNextPMSection(PM_SECTION* pPMS, UINT* uPMSNum /* = NULL */)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;

	CPacket	   packet;
	PA_SECTION PAS;
	PM_SECTION PMS;
	PATable    PAT;

	BYTE  bPacket[CPacket::PACKET_SIZE] = {0};
	DWORD dwReaded   = 0; // number of bytes readed from file
	UINT  uPacketNum = 0; // zero-based number of current packet

	// May be there are no PA Sections between current PM Section and next PM Section.
	// Therefore, if the current PM Section is not a first PM Section in TS,
	// then make a backward search of PA Section from current PM Section.
	if (m_uCurPMSPacket > 0)
	{
		uPacketNum = m_uCurPMSPacket - 1;
		while (TRUE)
		{
			if (SetFilePointer(m_hFile, 188 * uPacketNum, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
				return 0;

			if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
				return 0;

			if (dwReaded == 0)
				// reached end of file
				return 0;

			packet.Set(bPacket);
			if (packet.GetPID() == 0)
				if (packet.GetPASection(&PAS))
				{
					PAT.assign(PAS.m_PAT.begin(), PAS.m_PAT.end());
					break;
				}

			if (uPacketNum)
                uPacketNum--;
			else
				break;
		}
	}

	// Go to packet following after the current PM Section and then search PA Section and PM Section

	uPacketNum = m_uCurPMSPacket + 1;
	if (SetFilePointer(m_hFile, 188 * uPacketNum, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return 0;

	while (TRUE)
	{
		if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
			return 0;

		if (dwReaded == 0)
			// reached end of file
			break;

		packet.Set(bPacket);

		USHORT uPID = packet.GetPID();
		if (uPID == 0)
		{
			// packet contains PA Section
			if (packet.GetPASection(&PAS))
				PAT.assign(PAS.m_PAT.begin(), PAS.m_PAT.end());
		}
		else
		{
			std::list<PROGRAM_DESCRIPTOR>::const_iterator iter;
			for (iter = PAT.begin(); iter != PAT.end(); iter++)
				if (iter->PID == uPID)
					if (packet.GetPMSection(&PMS, PAT))
					{
						*pPMS = PMS;
						m_uCurPMS++;
						m_uCurPMSPacket = uPacketNum;

						if (uPMSNum != NULL)
							*uPMSNum = m_uCurPMS + 1;

						return (m_uCurPMSPacket + 1);
					}
		}

		uPacketNum++;
	}

	// there is no PM Sections in file
	return 0;
}

UINT CTransportStream::GetPrevPMSection(PM_SECTION* pPMS, UINT* uPMSNum /* = NULL */)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return 0;

	if (m_uCurPMSPacket <= 1)
		// Current PMS packet is first or second in TS, so there are no
		// PM Sections before this PMS packet
		return 0;

	CPacket	   packet;
	PA_SECTION PAS;
	PM_SECTION PMS;
	PATable    PAT;

	BYTE  bPacket[CPacket::PACKET_SIZE] = {0};
	DWORD dwReaded   = 0; // number of bytes readed from file
	UINT  uPacketNum = 0; // zero-based number of current packet
	UINT  uPAS       = 0;

	uPacketNum = m_uCurPMSPacket - 1;
	while (uPacketNum > 0)
	{
		// backward search of PA Section
		while (TRUE)
		{
			if (SetFilePointer(m_hFile, 188 * uPacketNum, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
				return 0;

			if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
				return 0;

			if (dwReaded == 0)
				// reached end of file
				return 0;

			packet.Set(bPacket);
			if (packet.GetPID() == 0)
				if (packet.GetPASection(&PAS))
				{
					PAT.assign(PAS.m_PAT.begin(), PAS.m_PAT.end());
					break;
				}

			if (uPacketNum)
                uPacketNum--;
			else
				break;
		}

		if (PAT.empty())
			// PA Section not found
			return 0;

		uPAS = uPacketNum;

		// Search a PM Section from PA Section to current PM Section
		// ... PA (uPAS) ... search -> ... PM (m_uCurPMSPacket) ...

		uPacketNum++;
		if (SetFilePointer(m_hFile, 188 * uPacketNum, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			return 0;

		UINT uCurPMSPacket = 0;
		while (uPacketNum < m_uCurPMSPacket)
		{
			if (!ReadFile(m_hFile, &bPacket, CPacket::PACKET_SIZE, &dwReaded, NULL))
				return 0;

			if (dwReaded == 0)
				// reached end of file
				break;

			packet.Set(bPacket);

			USHORT uPID = packet.GetPID();
			if (uPID == 0)
			{
				// packet contains PA Section
				if (packet.GetPASection(&PAS))
					PAT.assign(PAS.m_PAT.begin(), PAS.m_PAT.end());
			}
			else
			{
				std::list<PROGRAM_DESCRIPTOR>::const_iterator iter;
				for (iter = PAT.begin(); iter != PAT.end(); iter++)
					if (iter->PID == uPID)
						if (packet.GetPMSection(&PMS, PAT))
							uCurPMSPacket = uPacketNum;
			}

			uPacketNum++;
		}

		if (uCurPMSPacket > 0)
		{
			*pPMS = PMS;
			m_uCurPMS--;
			m_uCurPMSPacket = uCurPMSPacket;

			if (uPMSNum != NULL)
				*uPMSNum = m_uCurPMS + 1;

			return (m_uCurPMSPacket + 1);
		}

		// There are no packets with PM Section from PA Section to current PM Section,
		// so continue to search a PA Section in reverse order
		// <- search ... PA (uPAS) ... PM (m_uCurPMSPacket) ...

		if (uPAS == 0)
			return 0;

		uPacketNum = uPAS - 1;
	}

	// there is no PM Sections in file
	return 0;
}