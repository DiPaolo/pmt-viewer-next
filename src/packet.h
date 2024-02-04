/*******************************************************************************
 * File: Packet.h
 *
 * Description:
 *    CPacket class definition. This class implements transport packet in
 *    MPEG-2 Transport Stream. Also contains structures used by CPacket class
 *    for parsing packet.
 *
 *    See ISO/IEC 13818-1 second edition (2000-12-01).
 *
 * Copyright (c) Ditenbir Pavel, 2007, 2024.
 *
 *******************************************************************************/

#ifndef _PACKET_H_
#define _PACKET_H_

#include <list>

//
// #define directives
//
#define GET_BIT(byte, bit) (((byte) >> (bit)) & 0x01)

//
// Class and structures defined in this file
//
class CPacket;

struct PACKET_HEADER;
struct PA_SECTION;
struct PM_SECTION;

struct PROGRAM_DESCRIPTOR;
struct ES_INFO;
struct DESCRIPTOR;

//
// Typedefs
//
typedef const uint8_t* PCBYTE;

typedef std::list<PROGRAM_DESCRIPTOR> PATable;
typedef std::list<ES_INFO> PMTable;
typedef std::list<DESCRIPTOR> Descriptors;

//
// Class and structures definitions
//

// See table 2-2 in ISO/IEC 13818-1 second edition (2000-12-01).
struct PACKET_HEADER {
    PACKET_HEADER(void);
    PACKET_HEADER(PCBYTE& pb);

    void Reset(void);

    uint8_t sync_byte;
    uint8_t transport_error_indicator : 1;
    uint8_t payload_unit_start_indicator : 1;
    uint8_t transport_priority : 1;
    uint16_t PID : 13;
    uint8_t transport_scrambling_control : 2;
    uint8_t adaptation_field_control : 2;
    uint8_t continuity_counter : 4;
};

// See table 2-25 in ISO/IEC 13818-1 second edition (2000-12-01).
struct PA_SECTION {
    // constructors
    PA_SECTION(void);
    PA_SECTION(PCBYTE& pb);

    void Reset(void);

    uint8_t table_id;
    uint16_t section_syntax_indicator : 1;
    uint16_t bit_null : 1;
    uint16_t reserved_1 : 2;
    uint16_t section_length : 12;
    uint16_t transport_stream_id;
    uint8_t reserved_2 : 2;
    uint8_t version_number : 5;
    uint8_t current_next_indicator : 1;
    uint8_t section_number;
    uint8_t last_section_number;

    PATable m_PAT;

    uint32_t CRC_32;
};

// See table 2-28 in ISO/IEC 13818-1 second edition (2000-12-01).
struct PM_SECTION {
    PM_SECTION(void);
    PM_SECTION(PCBYTE& pb);

    void Reset(void);

    uint8_t table_id;
    uint16_t section_syntax_indicator : 1;
    uint16_t bit_null : 1;
    uint16_t reserved_1 : 2;
    uint16_t section_length : 12;
    uint16_t program_number;
    uint8_t reserved_2 : 2;
    uint8_t version_number : 5;
    uint8_t current_next_indicator : 1;
    uint8_t section_number;
    uint8_t last_section_number;
    uint16_t reserved_3 : 3;
    uint16_t PCR_PID : 13;
    uint16_t reserved_4 : 4;
    uint16_t program_info_length : 12;

    Descriptors program_descriptors;
    PMTable m_PMT;

    uint32_t CRC_32;
};

// Used by PA_SECTION. Associates Program Number and Program Map Table PID.
// See table 2-25 in ISO/IEC 13818-1 second edition (2000-12-01).
struct PROGRAM_DESCRIPTOR {
    uint16_t program_number;
    uint16_t reserved : 3;
    uint16_t PID : 13;
};

// Used by PM_SECTION.
// See table 2-28 in ISO/IEC 13818-1 second edition (2000-12-01).
struct ES_INFO {
    ES_INFO(void);
    ES_INFO(PCBYTE& pb);

    void Reset(void);

    uint8_t stream_type;
    uint16_t reserved_1 : 3;
    uint16_t elementary_PID : 13;
    uint16_t reserved_2 : 4;
    uint16_t ES_info_length : 12;

    Descriptors ES_descriptors;
};

// See section 2.6 in ISO/IEC 13818-1 second edition (2000-12-01).
struct DESCRIPTOR {
    DESCRIPTOR(void);
    DESCRIPTOR(PCBYTE& pb);
    DESCRIPTOR(const DESCRIPTOR& d);
    ~DESCRIPTOR(void);

    DESCRIPTOR& operator=(const DESCRIPTOR& d);

    void Reset(void);

    uint8_t tag;
    uint8_t length;
    uint8_t* pbData;
};

class CPacket {
public:
    // constants
    static const int PACKET_SIZE = 188; // size in bytes of TS packet
    static const uint8_t SYNC_BYTE = 0x47; // compare with first byte in packet
    static const uint16_t NULL_PACKET = 0x1FFF;

public:
    CPacket(void);
    CPacket(const uint8_t* pbData);
    ~CPacket(void);

    void Set(const uint8_t* pbData);

    bool CheckSyncByte(void) const;

    bool IsPMS(void) const;
    uint16_t GetPID(void) const;
    bool GetPASection(PA_SECTION* pPAS) const;
    bool GetPMSection(PM_SECTION* pPMS, PATable pat) const;

private:
    const uint8_t* m_pbData;
    PACKET_HEADER m_header;
    PA_SECTION m_PASection;
};

#endif // _PACKET_H_
