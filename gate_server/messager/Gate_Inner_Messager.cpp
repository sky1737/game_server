/*
 *  Created on: Dec 16, 2015
 *      Author: zhangyalei
 */


#include "Msg_Define.h"
#include "Gate_Inner_Messager.h"
#include "Gate_Manager.h"
#include "Gate_Player.h"
#include "Log.h"

Gate_Inner_Messager::Gate_Inner_Messager(void) { }

Gate_Inner_Messager::~Gate_Inner_Messager(void) { }

Gate_Inner_Messager *Gate_Inner_Messager::instance_;

Gate_Inner_Messager *Gate_Inner_Messager::instance(void) {
	if (! instance_)
		instance_ = new Gate_Inner_Messager;
	return instance_;
}

int Gate_Inner_Messager::process_login_block(Block_Buffer &buf) {
	int32_t gate_cid = 0;
	uint16_t len = 0;
	uint32_t msg_id = 0;
	int32_t status = 0;
	int32_t player_cid = 0;

	buf.read_int32(gate_cid);
	buf.read_uint16(len);
	buf.read_uint32(msg_id);
	buf.read_int32(status);
	buf.read_int32(player_cid);

	Perf_Mon perf_mon(msg_id);
	GATE_MANAGER->inner_msg_count(msg_id);

	if (msg_id == SYNC_LOGIN_GATE_PLAYER_ACCOUNT) {
		MSG_112001 msg;
		msg.deserialize(buf);
		Block_Buffer player_buf;
		player_buf.make_message(RES_CONNECT_GATE, status);
		MSG_511001 player_msg;
		player_msg.account = msg.account;
		player_msg.serialize(player_buf);
		player_buf.finish_message();
		GATE_MANAGER->send_to_client(player_cid, player_buf);

		if (status == 0) {
			//玩家成功登录
			process_success_login(player_cid, msg.account);
		}
	}
	return 0;
}

int Gate_Inner_Messager::process_game_block(Block_Buffer &buf) {
	int32_t gate_cid = 0;
	uint16_t len = 0;
	uint32_t msg_id = 0;
	int32_t  status = 0;
	int32_t player_cid = 0;

	buf.read_int32(gate_cid);
	buf.read_uint16(len);
	buf.read_uint32(msg_id);
	buf.read_int32(status);
	buf.read_int32(player_cid);

	Perf_Mon perf_mon(msg_id);
	GATE_MANAGER->inner_msg_count(msg_id);

	Block_Buffer player_buf;
	player_buf.make_message(msg_id, status);
	player_buf.copy(&buf);
	player_buf.finish_message();
	GATE_MANAGER->send_to_client(player_cid, player_buf);

	//玩家登录game成功，刷新gate信息
	if (msg_id == RES_FETCH_ROLE_INFO) {
		Gate_Player *player = GATE_MANAGER->find_cid_gate_player(player_cid);
		if (player) {
			MSG_520001 msg;
			msg.deserialize(buf);
			player->set_role_id(msg.role_info.role_id);
			player->set_account(msg.role_info.account);
		}
	} else if (msg_id == ACTIVE_DISCONNECT) {
		GATE_MANAGER->close_client(player_cid);
	}

	return 0;
}

int Gate_Inner_Messager::process_master_block(Block_Buffer &buf) {
	int32_t gate_cid = 0;
	uint16_t len = 0;
	uint32_t msg_id = 0;
	int32_t  status = 0;
	int32_t player_cid = 0;

	buf.read_int32(gate_cid);
	buf.read_uint16(len);
	buf.read_uint32(msg_id);
	buf.read_int32(status);
	buf.read_int32(player_cid);

	Perf_Mon perf_mon(msg_id);
	GATE_MANAGER->inner_msg_count(msg_id);

	Block_Buffer player_buf;
	player_buf.make_message(msg_id, status);
	player_buf.copy(&buf);
	player_buf.finish_message();
	GATE_MANAGER->send_to_client(player_cid, player_buf);

	return 0;
}

int Gate_Inner_Messager::process_self_loop_block(Block_Buffer &buf) {
	uint16_t len = 0;
	uint32_t msg_id = 0;
	int32_t status = 0;

	buf.read_uint16(len);
	buf.read_uint32(msg_id);
	buf.read_int32(status);

	Perf_Mon perf_mon(msg_id);
	switch (msg_id) {
	case SYNC_INNER_TIMER_TICK: {
		GATE_MANAGER->tick();
		break;
	}
	default:
		break;
	}

	return 0;
}

int Gate_Inner_Messager::process_success_login(int player_cid, std::string &account) {
	Gate_Player *player = GATE_MANAGER->pop_gate_player();
	if (! player) {
		MSG_USER("game_player_pool_.pop() return 0.");
		return -1;
	}

	player->reset();
	player->set_cid(player_cid);
	player->set_account(account);
	GATE_MANAGER->bind_cid_gate_player(player_cid, *player);

	return 0;
}
