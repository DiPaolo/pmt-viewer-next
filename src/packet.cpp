/*******************************************************************************
 * File: Packet.cpp
 *
 * Description:
 *    CPacket class and corresponding structures implementation.
 *
 *    See ISO/IEC 13818-1 second edition (2000-12-01).
 *
 * Copyright (c) Ditenbir Pavel, 2007, 2024.
 *
 *******************************************************************************/

#include "packet.h"

CPacket::CPacket(void)
{
    m_pbData = NULL;
}

//
// Constructor
//
// Parameter pbData is a pointer to a buffer that contains packet data.
// Note that used only first PACKET_SIZE (188) bytes, because it's a
// length of transport stream packet.
CPacket::CPacket(const uint8_t* pbData)
{
    m_pbData = pbData;
}

CPacket::~CPacket(void)
{
    m_pbData = NULL;
}

//
// CPacket::Set
//
// Sets pbData pointer to a start of packet
void CPacket::Set(const uint8_t* pbData)
{
    m_pbData = pbData;

    m_header.Reset();
    m_PASection.Reset();
}

bool CPacket::CheckSyncByte(void) const
{
    if (m_pbData == NULL)
        return false;

    return (*m_pbData == SYNC_BYTE);
}

uint16_t CPacket::GetPID(void) const
{
    if (m_pbData == NULL)
        return NULL_PACKET;

    return (((m_pbData[1] & 0x1F) << 8) | m_pbData[2]);
}

//
// CPacket::GetPASection
//
// Parse packet and search PA Section. If some errors occurs, return FALSE.
bool CPacket::GetPASection(PA_SECTION* pPAS) const
{
    if (m_pbData == NULL)
        return false;

    const uint8_t* pb = m_pbData;

    PACKET_HEADER header(pb);

    if ((header.adaptation_field_control & 0x02) || (header.adaptation_field_control & 0x03)) {
        // adaptation_field_control is equal '10' or '11'
        // next bytes is adaptation field
    }

    if (header.payload_unit_start_indicator) {
        // read the pointer_field
        uint8_t pointer_field = *pb;
        pb++;
        pb += pointer_field;
    }

    if ((header.adaptation_field_control & 0x01) || (header.adaptation_field_control & 0x03)) {
        // adaptation_field_control is equal '01' or '11'
        // next bytes is payload

        if (header.PID == 0x0000) {
            // payload contains Program Association section
            PA_SECTION PAS(pb);
            *pPAS = PAS;
            return true;
        }
    }

    return false;
}

//
// CPacket::GetPMSection
//
// Parse packet and search PM Section. If some errors occurs, return FALSE.
bool CPacket::GetPMSection(PM_SECTION* pPMS, PATable PAT) const
{
    if (m_pbData == NULL)
        return false;

    const uint8_t* pb = m_pbData;

    PACKET_HEADER header(pb);

    if ((header.adaptation_field_control & 0x02) || (header.adaptation_field_control & 0x03)) {
        // adaptation_field_control is equal '10' or '11'
        // next bytes is adaptation field
    }

    if (header.payload_unit_start_indicator) {
        // read the pointer_field
        uint8_t pointer_field = *pb;
        pb++;
        pb += pointer_field;
    }

    if ((header.adaptation_field_control & 0x01) || (header.adaptation_field_control & 0x03)) {
        // adaptation_field_control is equal '01' or '11'
        // next bytes is payload

        std::list<PROGRAM_DESCRIPTOR>::const_iterator iter;
        for (iter = PAT.begin(); iter != PAT.end(); iter++)
            if (iter->PID == header.PID) {
                // payload contains Program Map section
                PM_SECTION PMS(pb);
                *pPMS = PMS;
                return true;
            }
    }

    return false;
}

//
// PACKET_HEADER implementation
//

PACKET_HEADER::PACKET_HEADER(void)
{
    Reset();
}

//
// Constructor
//
// Parse packet header. Movement received reference.
PACKET_HEADER::PACKET_HEADER(PCBYTE& pb)
{
    sync_byte = *pb;
    if (sync_byte != CPacket::SYNC_BYTE)
        return;
    pb++;

    transport_error_indicator = GET_BIT(*pb, 7);
    payload_unit_start_indicator = GET_BIT(*pb, 6);
    transport_priority = GET_BIT(*pb, 5);
    PID = ((*pb & 0x1F) << 8) | pb[1];
    pb += 2;

    transport_scrambling_control = (*pb >> 6) & 0x03;
    adaptation_field_control = (*pb >> 4) & 0x03;
    continuity_counter = (*pb & 0x0F);
    pb++;
}

void PACKET_HEADER::Reset(void)
{
    memset(this, 0, sizeof(*this));
}

//
// PA_SECTION implementation
//

PA_SECTION::PA_SECTION(void)
{
    Reset();
}

//
// Constructor
//
// Parse PA Section. Movement received reference.
PA_SECTION::PA_SECTION(PCBYTE& pb)
{
    table_id = *pb;
    pb++;

    section_syntax_indicator = GET_BIT(*pb, 7);
    bit_null = GET_BIT(*pb, 6);
    reserved_1 = (*pb >> 4) & 0x03;
    section_length = ((uint16_t)(*pb & 0x0F) << 8) | pb[1];
    pb += 2;

    transport_stream_id = (((uint16_t)*pb << 8) | pb[1]);
    pb += 2;

    reserved_2 = (*pb >> 6) & 0x03;
    version_number = (*pb >> 1) & 0x1F;
    current_next_indicator = GET_BIT(*pb, 0);
    pb++;

    section_number = *pb;
    pb++;

    last_section_number = *pb;
    pb++;

    PROGRAM_DESCRIPTOR pd = { 0 };

    int nCount = (section_length - 9) / 4; // number of program descriptors
    for (int i = 0; i < nCount; i++) {
        pd.program_number = ((uint16_t)*pb << 8) | pb[1];
        pb += 2;

        pd.reserved = (*pb >> 5) | 0x07;
        pd.PID = ((uint16_t)(*pb & 0x1F) << 8) | pb[1];
        pb += 2;

        m_PAT.push_back(pd);
    }

    CRC_32 = (((uint32_t)pb[0] << 24) | ((uint32_t)pb[1] << 16) | ((uint32_t)pb[2] << 8) | ((uint32_t)pb[3]));
    pb += 4;
}

void PA_SECTION::Reset(void)
{
    table_id = 0;
    section_syntax_indicator = 0;
    bit_null = 0;
    reserved_1 = 0;
    section_length = 0;
    transport_stream_id = 0;
    reserved_2 = 0;
    version_number = 0;
    current_next_indicator = 0;
    section_number = 0;
    last_section_number = 0;
    CRC_32 = 0;
}

//
// PM_SECTION implementation
//

PM_SECTION::PM_SECTION(void)
{
    Reset();
}

//
// Constructor
//
// Parse PM Section. Movement received reference.
PM_SECTION::PM_SECTION(PCBYTE& pb)
{
    table_id = *pb;
    pb++;

    section_syntax_indicator = GET_BIT(*pb, 7);
    bit_null = GET_BIT(*pb, 6);
    reserved_1 = (*pb >> 4) & 0x03;
    section_length = ((uint16_t)(*pb & 0x0F) << 8) | pb[1];
    pb += 2;

    program_number = (((uint16_t)*pb << 8) | pb[1]);
    pb += 2;

    reserved_2 = (*pb >> 6) & 0x03;
    version_number = (*pb >> 1) & 0x1F;
    current_next_indicator = GET_BIT(*pb, 0);
    pb++;

    section_number = *pb;
    pb++;

    last_section_number = *pb;
    pb++;

    reserved_3 = (*pb >> 5) & 0x07;
    PCR_PID = ((uint16_t)(*pb & 0x1F) << 8) | pb[1];
    pb += 2;

    reserved_4 = (*pb >> 4) & 0x0F;
    program_info_length = ((uint16_t)(*pb & 0x0F) << 8) | pb[1];
    pb += 2;

    PCBYTE pbES_info = pb + program_info_length;
    while (pb < pbES_info) {
        DESCRIPTOR d(pb);
        program_descriptors.push_back(d);
    }

    PCBYTE pbCRC_32 = pb + (section_length - 13 - program_info_length);
    while (pb < pbCRC_32) {
        ES_INFO ESInfo(pb);
        m_PMT.push_back(ESInfo);
    }

    CRC_32 = (((uint32_t)pb[0] << 24) | ((uint32_t)pb[1] << 16) | ((uint32_t)pb[2] << 8) | ((uint32_t)pb[3]));
    pb += 4;
}

void PM_SECTION::Reset(void)
{
    table_id = 0;
    section_syntax_indicator = 0;
    bit_null = 0;
    reserved_1 = 0;
    section_length = 0;
    program_number = 0;
    reserved_2 = 0;
    version_number = 0;
    current_next_indicator = 0;
    section_number = 0;
    last_section_number = 0;
    reserved_3 = 0;
    PCR_PID = 0;
    reserved_4 = 0;
    program_info_length = 0;
    CRC_32 = 0;
}

//
// ES_INFO implementation
//

ES_INFO::ES_INFO(void)
{
    Reset();
}

//
// Constructor
//
// Parse ES info, which is a part of PM Section. Movement received reference.
ES_INFO::ES_INFO(PCBYTE& pb)
{
    stream_type = *pb;
    pb++;

    reserved_1 = (*pb >> 5) & 0x07;
    elementary_PID = ((uint16_t)(*pb & 0x1F) << 8) | pb[1];
    pb += 2;

    reserved_2 = (*pb >> 4) & 0x0F;
    ES_info_length = ((uint16_t)(*pb & 0x0F) << 8) | pb[1];
    pb += 2;

    PCBYTE pbEnd = pb + ES_info_length;
    while (pb < pbEnd) {
        DESCRIPTOR d(pb);
        ES_descriptors.push_back(d);
    }
}

void ES_INFO::Reset(void)
{
    stream_type = 0;
    reserved_1 = 0;
    elementary_PID = 0;
    reserved_2 = 0;
    ES_info_length = 0;
    ES_descriptors.clear();
}

//
// DESCRIPTOR implementation
//

DESCRIPTOR::DESCRIPTOR(void)
{
    tag = 0;
    length = 0;
    pbData = NULL;
}

//
// Constructor
//
// Parse descriptor. Gets following fields: tag, length and data. Don't
// determine descriptor type and don't parse data field accroding to type.
// Instead, gets data field as array of bytes.
//
// Movement received reference.
DESCRIPTOR::DESCRIPTOR(PCBYTE& pb)
{
    tag = *pb;
    pb++;

    length = *pb;
    pb++;

    pbData = new uint8_t[length];
    for (uint8_t i = 0; i < length; i++)
        pbData[i] = *pb++;
}

//
// Copy constructor
//
DESCRIPTOR::DESCRIPTOR(const DESCRIPTOR& d)
{
    tag = d.tag;
    length = d.length;
    pbData = new uint8_t[length];
    for (uint8_t i = 0; i < length; i++)
        pbData[i] = d.pbData[i];
}

DESCRIPTOR::~DESCRIPTOR(void)
{
    Reset();
}

DESCRIPTOR& DESCRIPTOR::operator=(const DESCRIPTOR& d)
{
    if (this != &d) {
        tag = d.tag;
        length = d.length;

        if (pbData != NULL)
            delete[] pbData;

        pbData = new uint8_t[length];
        for (uint8_t i = 0; i < length; i++)
            pbData[i] = d.pbData[i];
    }

    return *this;
}

void DESCRIPTOR::Reset(void)
{
    tag = 0;
    length = 0;

    if (pbData != NULL) {
        delete[] pbData;
        pbData = NULL;
    }
}
