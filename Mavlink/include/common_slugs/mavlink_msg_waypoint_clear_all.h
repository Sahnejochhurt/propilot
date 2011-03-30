// MESSAGE WAYPOINT_CLEAR_ALL PACKING

#define MAVLINK_MSG_ID_WAYPOINT_CLEAR_ALL 45
#define MAVLINK_MSG_45_LEN 2

typedef struct __mavlink_waypoint_clear_all_t 
{
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID
} mavlink_waypoint_clear_all_t;

//#if sizeof(mavlink_waypoint_clear_all_t) != 2 // sadly this fails 
//#warning struct and size do not match
//#endif


/**
 * @brief Pack a waypoint_clear_all message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_clear_all_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component)
{
	mavlink_waypoint_clear_all_t *p = (mavlink_waypoint_clear_all_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_CLEAR_ALL;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID

	return mavlink_finalize_message(msg, system_id, component_id, sizeof(mavlink_waypoint_clear_all_t));
}

/**
 * @brief Pack a waypoint_clear_all message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_clear_all_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component)
{
	mavlink_waypoint_clear_all_t *p = (mavlink_waypoint_clear_all_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_CLEAR_ALL;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, sizeof(mavlink_waypoint_clear_all_t));
}

/**
 * @brief Encode a waypoint_clear_all struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param waypoint_clear_all C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_waypoint_clear_all_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_waypoint_clear_all_t* waypoint_clear_all)
{
	return mavlink_msg_waypoint_clear_all_pack(system_id, component_id, msg, waypoint_clear_all->target_system, waypoint_clear_all->target_component);
}

/**
 * @brief Send a waypoint_clear_all message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_waypoint_clear_all_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_waypoint_clear_all_t *p = (mavlink_waypoint_clear_all_t *)&msg.payload[0];

	msg.msgid = MAVLINK_MSG_ID_WAYPOINT_CLEAR_ALL;
	msg.len = sizeof(mavlink_waypoint_clear_all_t);
	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID

	msg.sysid = mavlink_system.sysid;
	msg.compid = mavlink_system.compid;
	msg.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = mavlink_get_channel_status(chan)->current_tx_seq+1;
	checksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);
	msg.ck_a = (uint8_t)(checksum & 0xFF); ///< High byte
	msg.ck_b = (uint8_t)(checksum >> 8); ///< Low byte

	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE WAYPOINT_CLEAR_ALL UNPACKING

/**
 * @brief Get field target_system from waypoint_clear_all message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_waypoint_clear_all_get_target_system(const mavlink_message_t* msg)
{
	mavlink_waypoint_clear_all_t *p = (mavlink_waypoint_clear_all_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from waypoint_clear_all message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_waypoint_clear_all_get_target_component(const mavlink_message_t* msg)
{
	mavlink_waypoint_clear_all_t *p = (mavlink_waypoint_clear_all_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Decode a waypoint_clear_all message into a struct
 *
 * @param msg The message to decode
 * @param waypoint_clear_all C-struct to decode the message contents into
 */
static inline void mavlink_msg_waypoint_clear_all_decode(const mavlink_message_t* msg, mavlink_waypoint_clear_all_t* waypoint_clear_all)
{
	memcpy( waypoint_clear_all, msg->payload, sizeof(mavlink_waypoint_clear_all_t));
}