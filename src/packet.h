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
* Copyright (c) Ditenbir Pavel, 2007.
*
*******************************************************************************/

#ifndef _PACKET_H_
#define _PACKET_H_


#include <Windows.h>
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
typedef const BYTE* PCBYTE;

typedef std::list<PROGRAM_DESCRIPTOR> PATable;
typedef std::list<ES_INFO>            PMTable;
typedef std::list<DESCRIPTOR>         Descriptors;


//
// Class and structures definitions
//

// See table 2-2 in ISO/IEC 13818-1 second edition (2000-12-01).
struct PACKET_HEADER
{
	PACKET_HEADER(void);
	PACKET_HEADER(PCBYTE& pb);

	void Reset(void);

	BYTE   sync_byte;
	BYTE   transport_error_indicator    : 1;
	BYTE   payload_unit_start_indicator : 1;
	BYTE   transport_priority           : 1;
	USHORT PID                          : 13;
	BYTE   transport_scrambling_control : 2;
	BYTE   adaptation_field_control     : 2;
	BYTE   continuity_counter           : 4;
};

// See table 2-25 in ISO/IEC 13818-1 second edition (2000-12-01).
struct PA_SECTION
{
	// constructors
	PA_SECTION(void);
	PA_SECTION(PCBYTE& pb);

	void Reset(void);

	BYTE    table_id;
	USHORT  section_syntax_indicator : 1;
	USHORT  bit_null                 : 1;
	USHORT  reserved_1               : 2;
	USHORT  section_length           : 12;
	USHORT  transport_stream_id;
	BYTE    reserved_2               : 2;
	BYTE    version_number           : 5;
	BYTE    current_next_indicator   : 1;
	BYTE    section_number;
	BYTE    last_section_number;

	PATable m_PAT;

	UINT    CRC_32;
};

// See table 2-28 in ISO/IEC 13818-1 second edition (2000-12-01).
struct PM_SECTION
{
	PM_SECTION(void);
	PM_SECTION(PCBYTE& pb);

	void Reset(void);

	BYTE    table_id;
	USHORT  section_syntax_indicator : 1;
	USHORT  bit_null                 : 1;
	USHORT  reserved_1               : 2;
	USHORT  section_length           : 12;
	USHORT  program_number;
	BYTE    reserved_2               : 2;
	BYTE    version_number           : 5;
	BYTE    current_next_indicator   : 1;
	BYTE    section_number;
	BYTE    last_section_number;
	USHORT  reserved_3          : 3;
	USHORT  PCR_PID             : 13;
	USHORT  reserved_4          : 4;
	USHORT  program_info_length : 12;

	Descriptors program_descriptors;
	PMTable     m_PMT;

	UINT    CRC_32;
};

// Used by PA_SECTION. Associates Program Number and Program Map Table PID.
// See table 2-25 in ISO/IEC 13818-1 second edition (2000-12-01).
struct PROGRAM_DESCRIPTOR
{
	USHORT program_number;
	USHORT reserved : 3;
	USHORT PID      : 13;
};

// Used by PM_SECTION.
// See table 2-28 in ISO/IEC 13818-1 second edition (2000-12-01).
struct ES_INFO
{
	ES_INFO(void);
	ES_INFO(PCBYTE& pb);

	void Reset(void);

	BYTE   stream_type;
	USHORT reserved_1     : 3;
	USHORT elementary_PID : 13;
	USHORT reserved_2     : 4;
	USHORT ES_info_length : 12;

	Descriptors ES_descriptors;
};

// See section 2.6 in ISO/IEC 13818-1 second edition (2000-12-01).
struct DESCRIPTOR
{
	DESCRIPTOR(void);
	DESCRIPTOR(PCBYTE& pb);
	DESCRIPTOR(const DESCRIPTOR& d);
	~DESCRIPTOR(void);

	DESCRIPTOR& operator=(const DESCRIPTOR& d);

	void Reset(void);

	BYTE  tag;
	BYTE  length;
	BYTE* pbData;
};


class CPacket
{
public:
	// constants
	static const int    PACKET_SIZE = 188;  // size in bytes of TS packet
	static const BYTE   SYNC_BYTE   = 0x47; // compare with first byte in packet
	static const USHORT NULL_PACKET = 0x1FFF;

public:
	CPacket(void);
	CPacket(const BYTE* pbData);
	~CPacket(void);

	void Set(const BYTE* pbData);

	BOOL CheckSyncByte(void) const;

	BOOL	IsPMS(void) const;
	USHORT	GetPID(void) const;
	BOOL	GetPASection(PA_SECTION* pPAS) const;
	BOOL	GetPMSection(PM_SECTION* pPMS, PATable pat) const;

private:
	const BYTE*   m_pbData;
	PACKET_HEADER m_header;
	PA_SECTION    m_PASection;
};


#endif // _PACKET_H_