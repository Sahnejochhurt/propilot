// MESSAGE PID PACKING

#define MAVLINK_MSG_ID_PID 34

typedef struct __mavlink_pid_t 
{
	uint8_t pid_id; ///< PID ID
	float k_p; ///< P
	float k_i; ///< I
	float k_d; ///< D

} mavlink_pid_t;



/**
 * @brief Pack a pid message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param pid_id PID ID
 * @param k_p P
 * @param k_i I
 * @param k_d D
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_pid_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t pid_id, float k_p, float k_i, float k_d)
{
	mavlink_pid_t *p = (mavlink_pid_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PID;

	p->pid_id = pid_id; // uint8_t:PID ID
	p->k_p = k_p; // float:P
	p->k_i = k_i; // float:I
	p->k_d = k_d; // float:D

	return mavlink_finalize_message(msg, system_id, component_id, sizeof(mavlink_pid_t));
}

/**
 * @brief Pack a pid message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param pid_id PID ID
 * @param k_p P
 * @param k_i I
 * @param k_d D
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_pid_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t pid_id, float k_p, float k_i, float k_d)
{
	mavlink_pid_t *p = (mavlink_pid_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PID;

	p->pid_id = pid_id; // uint8_t:PID ID
	p->k_p = k_p; // float:P
	p->k_i = k_i; // float:I
	p->k_d = k_d; // float:D

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, sizeof(mavlink_pid_t));
}

/**
 * @brief Encode a pid struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param pid C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_pid_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_pid_t* pid)
{
	return mavlink_msg_pid_pack(system_id, component_id, msg, pid->pid_id, pid->k_p, pid->k_i, pid->k_d);
}

/**
 * @brief Send a pid message
 * @param chan MAVLink channel to send the message
 *
 * @param pid_id PID ID
 * @param k_p P
 * @param k_i I
 * @param k_d D
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_pid_send(mavlink_channel_t chan, uint8_t pid_id, float k_p, float k_i, float k_d)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_pid_t *p = (mavlink_pid_t *)&msg.payload[0];

	msg.msgid = MAVLINK_MSG_ID_PID;
	msg.len = sizeof(mavlink_pid_t);
	p->pid_id = pid_id; // uint8_t:PID ID
	p->k_p = k_p; // float:P
	p->k_i = k_i; // float:I
	p->k_d = k_d; // float:D

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
// MESSAGE PID UNPACKING

/**
 * @brief Get field pid_id from pid message
 *
 * @return PID ID
 */
static inline uint8_t mavlink_msg_pid_get_pid_id(const mavlink_message_t* msg)
{
	mavlink_pid_t *p = (mavlink_pid_t *)&msg->payload[0];
	return (uint8_t)(p->pid_id);
}

/**
 * @brief Get field k_p from pid message
 *
 * @return P
 */
static inline float mavlink_msg_pid_get_k_p(const mavlink_message_t* msg)
{
	mavlink_pid_t *p = (mavlink_pid_t *)&msg->payload[0];
	return (float)(p->k_p);
}

/**
 * @brief Get field k_i from pid message
 *
 * @return I
 */
static inline float mavlink_msg_pid_get_k_i(const mavlink_message_t* msg)
{
	mavlink_pid_t *p = (mavlink_pid_t *)&msg->payload[0];
	return (float)(p->k_i);
}

/**
 * @brief Get field k_d from pid message
 *
 * @return D
 */
static inline float mavlink_msg_pid_get_k_d(const mavlink_message_t* msg)
{
	mavlink_pid_t *p = (mavlink_pid_t *)&msg->payload[0];
	return (float)(p->k_d);
}

/**
 * @brief Decode a pid message into a struct
 *
 * @param msg The message to decode
 * @param pid C-struct to decode the message contents into
 */
static inline void mavlink_msg_pid_decode(const mavlink_message_t* msg, mavlink_pid_t* pid)
{
	memcpy( pid, msg->payload, sizeof(mavlink_pid_t));
}