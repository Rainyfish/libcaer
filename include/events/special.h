/**
 * @file special.h
 *
 * Special Events format definition and handling functions.
 * This event type encodes special occurrences, such as
 * timestamp related notifications or external input events.
 */

#ifndef LIBCAER_EVENTS_SPECIAL_H_
#define LIBCAER_EVENTS_SPECIAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/**
 * Shift and mask values for the type and data portions
 * of a special event.
 * Up to 128 types, with 24 bits of data each, are possible.
 * Bit 0 is the valid mark, see 'common.h' for more details.
 */
#define TYPE_SHIFT 1
#define TYPE_MASK 0x0000007F
#define DATA_SHIFT 8
#define DATA_MASK 0x00FFFFFF

/**
 * Special event type identifiers.
 * Used to interpret the special event type field.
 */
enum caer_special_event_types {
	TIMESTAMP_WRAP = 0,              //!< A 32 bit timestamp wrap occurred.
	TIMESTAMP_RESET = 1,             //!< A timestamp reset occurred.
	EXTERNAL_INPUT_RISING_EDGE = 2,  //!< A rising edge was detected (External Input module on device).
	EXTERNAL_INPUT_FALLING_EDGE = 3, //!< A falling edge was detected (External Input module on device).
	EXTERNAL_INPUT_PULSE = 4,        //!< A pulse was detected (External Input module on device).
	DVS_ROW_ONLY = 5,    //!< A DVS row-only event was detected (a row address without any following column addresses).
};

struct caer_special_event {
	uint32_t data; // First because of valid mark.
	int32_t timestamp;
}__attribute__((__packed__));

typedef struct caer_special_event *caerSpecialEvent;

struct caer_special_event_packet {
	struct caer_event_packet_header packetHeader;
	struct caer_special_event events[];
}__attribute__((__packed__));

typedef struct caer_special_event_packet *caerSpecialEventPacket;

caerSpecialEventPacket caerSpecialEventPacketAllocate(int32_t eventCapacity, int16_t eventSource, int32_t tsOverflow);

static inline caerSpecialEvent caerSpecialEventPacketGetEvent(caerSpecialEventPacket packet, int32_t n) {
	// Check that we're not out of bounds.
	if (n < 0 || n >= caerEventPacketHeaderGetEventCapacity(&packet->packetHeader)) {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(CAER_LOG_CRITICAL, "Special Event",
			"Called caerSpecialEventPacketGetEvent() with invalid event offset %" PRIi32 ", while maximum allowed value is %" PRIi32 ".",
			n, caerEventPacketHeaderGetEventCapacity(&packet->packetHeader));
#endif
		return (NULL);
	}

	// Return a pointer to the specified event.
	return (packet->events + n);
}

static inline int32_t caerSpecialEventGetTimestamp(caerSpecialEvent event) {
	return (le32toh(event->timestamp));
}

static inline int64_t caerSpecialEventGetTimestamp64(caerSpecialEvent event, caerSpecialEventPacket packet) {
	return (I64T(
		(U64T(caerEventPacketHeaderGetEventTSOverflow(&packet->packetHeader)) << TS_OVERFLOW_SHIFT) | U64T(caerSpecialEventGetTimestamp(event))));
}

// Limit Timestamp to 31 bits for compatibility with languages that have no unsigned integer (Java).
static inline void caerSpecialEventSetTimestamp(caerSpecialEvent event, int32_t timestamp) {
	if (timestamp < 0) {
		// Negative means using the 31st bit!
#if !defined(LIBCAER_LOG_NONE)
		caerLog(CAER_LOG_CRITICAL, "Special Event", "Called caerSpecialEventSetTimestamp() with negative value!");
#endif
		return;
	}

	event->timestamp = htole32(timestamp);
}

static inline bool caerSpecialEventIsValid(caerSpecialEvent event) {
	return ((le32toh(event->data) >> VALID_MARK_SHIFT) & VALID_MARK_MASK);
}

static inline void caerSpecialEventValidate(caerSpecialEvent event, caerSpecialEventPacket packet) {
	if (!caerSpecialEventIsValid(event)) {
		event->data |= htole32(U32T(1) << VALID_MARK_SHIFT);

		// Also increase number of events and valid events.
		// Only call this on (still) invalid events!
		caerEventPacketHeaderSetEventNumber(&packet->packetHeader,
			caerEventPacketHeaderGetEventNumber(&packet->packetHeader) + 1);
		caerEventPacketHeaderSetEventValid(&packet->packetHeader,
			caerEventPacketHeaderGetEventValid(&packet->packetHeader) + 1);
	}
	else {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(CAER_LOG_CRITICAL, "Special Event", "Called caerSpecialEventValidate() on already valid event.");
#endif
	}
}

static inline void caerSpecialEventInvalidate(caerSpecialEvent event, caerSpecialEventPacket packet) {
	if (caerSpecialEventIsValid(event)) {
		event->data &= htole32(~(U32T(1) << VALID_MARK_SHIFT));

		// Also decrease number of valid events. Number of total events doesn't change.
		// Only call this on valid events!
		caerEventPacketHeaderSetEventValid(&packet->packetHeader,
			caerEventPacketHeaderGetEventValid(&packet->packetHeader) - 1);
	}
	else {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(CAER_LOG_CRITICAL, "Special Event", "Called caerSpecialEventInvalidate() on already invalid event.");
#endif
	}
}

static inline uint8_t caerSpecialEventGetType(caerSpecialEvent event) {
	return U8T((le32toh(event->data) >> TYPE_SHIFT) & TYPE_MASK);
}

static inline void caerSpecialEventSetType(caerSpecialEvent event, uint8_t type) {
	event->data |= htole32((U32T(type) & TYPE_MASK) << TYPE_SHIFT);
}

static inline uint32_t caerSpecialEventGetData(caerSpecialEvent event) {
	return U32T((le32toh(event->data) >> DATA_SHIFT) & DATA_MASK);
}

static inline void caerSpecialEventSetData(caerSpecialEvent event, uint32_t data) {
	event->data |= htole32((U32T(data) & DATA_MASK) << DATA_SHIFT);
}

#ifdef __cplusplus
}
#endif

#endif /* LIBCAER_EVENTS_SPECIAL_H_ */
