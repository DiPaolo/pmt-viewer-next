/*******************************************************************************
 * File: TransportStream.h
 *
 * Description: CTransportStream class definition. This class implements
 *              MPEG-2 Transport Stream according to ISO/IEC 13818-1 second
 *              edition (2000-12-01) specification.
 *
 * Copyright (c) Ditenbir Pavel, 2007, 2024.
 *
 *******************************************************************************/

#ifndef _TRANSPORT_STREAM_H_
#define _TRANSPORT_STREAM_H_

#include <string>

#include "packet.h"

class CTransportStream {
public:
    CTransportStream(void);
    CTransportStream(const std::string& pszFileName);
    ~CTransportStream(void);

    bool Open(const std::string& pszFileName);
    void Close(void);

    bool IsMPEG2TS(void) const;

    std::string GetFileName(void) const;
    int GetFileSize(void) const;
    uint32_t GetPMSCount(void);
    uint32_t GetPacketsCount(void) const;

    // random access to PM Sections in a TS
    uint32_t GetPMSection(uint32_t uNum, PM_SECTION* pPMS) const;

    // functions for sequential access to PM Sections in a TS
    uint32_t GetFirstPMSection(PM_SECTION* pPMS, uint32_t* uPMSNum = NULL);
    uint32_t GetLastPMSection(PM_SECTION* pPMS, uint32_t* uPMSNum = NULL);
    uint32_t GetNextPMSection(PM_SECTION* pPMS, uint32_t* uPMSNum = NULL);
    uint32_t GetPrevPMSection(PM_SECTION* pPMS, uint32_t* uPMSNum = NULL);

private:
    std::FILE* m_hFile = nullptr;
    std::string m_szFileName = "";
    uint32_t m_uPMSCount = 0; // count of PM Section in TS

    // zero-based variables used by functions for sequential access to PM Sections
    uint32_t m_uCurPMS = 0; // number of current PMS
    uint32_t m_uCurPMSPacket = 0; // number of current packet that contains PM Section
};

#endif // _TRANSPORT_STREAM_H_
