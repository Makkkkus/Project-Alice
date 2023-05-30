#pragma once

#include "gui_common_elements.hpp"
#include "gui_element_types.hpp"
#include "diplomatic_messages.hpp"

namespace ui {

struct diplo_reply_taken_notification { int a = 0; };

template<bool Left>
class msg_lr_button : public button_element_base {
public:
	void on_create(sys::state& state) noexcept override {
		button_element_base::on_create(state);
		frame = Left ? 0 : 1;
	}

	void button_action(sys::state& state) noexcept override {
		if(parent) {
			Cyto::Any payload = element_selection_wrapper<bool>{ Left };
			parent->impl_get(state, payload);
		}
	}
};

template<bool B>
class msg_reply_button : public button_element_base {
public:
	void button_action(sys::state& state) noexcept override {
		if(parent) {
			Cyto::Any payload = diplomatic_message::message{};
			parent->impl_get(state, payload);
			diplomatic_message::message m = any_cast<diplomatic_message::message>(payload);
			command::respond_to_diplomatic_message(state, state.local_player_nation, m.from, m.type, B);

			Cyto::Any n_payload = diplo_reply_taken_notification{};
			parent->impl_get(state, n_payload);
		}
	}
};

class msg_window : public window_element_base {
	simple_text_element_base* count_text = nullptr;
	int32_t index = 0;
public:
	std::vector<diplomatic_message::message> messages{};

	void on_create(sys::state& state) noexcept override {
		window_element_base::on_create(state);
		xy_pair cur_pos{ 0, 0 };
		{
			auto ptr = make_element_by_type<event_lr_button<false>>(state, state.ui_state.defs_by_name.find("alice_left_right_button")->second.definition);
			cur_pos.x = base_data.size.x - (ptr->base_data.size.x * 2);
			cur_pos.y = ptr->base_data.size.y * 1;
			ptr->base_data.position = cur_pos;
			add_child_to_front(std::move(ptr));
		}
		{
			auto ptr = make_element_by_type<simple_text_element_base>(state, state.ui_state.defs_by_name.find("alice_page_count")->second.definition);
			cur_pos.x -= ptr->base_data.size.x;
			ptr->base_data.position = cur_pos;
			count_text = ptr.get();
			add_child_to_front(std::move(ptr));
		}
		{
			auto ptr = make_element_by_type<event_lr_button<true>>(state, state.ui_state.defs_by_name.find("alice_left_right_button")->second.definition);
			cur_pos.x -= ptr->base_data.size.x;
			ptr->base_data.position = cur_pos;
			add_child_to_front(std::move(ptr));
		}
		set_visible(state, false);
	}
	
	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "title") {
			return make_element_by_type<simple_text_element_base>(state, id);
		} else if(name == "description") {
			return make_element_by_type<simple_text_element_base>(state, id);
		} else if(name == "agreebutton") {
			return make_element_by_type<msg_reply_button<true>>(state, id);
		} else if(name == "declinebutton") {
			return make_element_by_type<msg_reply_button<false>>(state, id);
		} else if(name == "leftshield") {
			return make_element_by_type<nation_player_flag>(state, id);
		} else if(name == "rightshield") {
			return make_element_by_type<flag_button>(state, id);
		} else if(name == "background") {
			auto ptr = make_element_by_type<draggable_target>(state, id);
			ptr->base_data.size = base_data.size;
			return ptr;
		} else {
			return nullptr;
		}
	}

	void on_update(sys::state& state) noexcept override {
		auto it = std::remove_if(messages.begin(), messages.end(), [&](auto& m){
			return m.when + diplomatic_message::expiration_in_days <= state.current_date;
		});
		auto r = std::distance(it, messages.end());
		messages.erase(it, messages.end());

		if(messages.empty()) {
			set_visible(state, false);
		}

		count_text->set_text(state, std::to_string(int32_t(index)) + "/" + std::to_string(int32_t(messages.size())));
	}
	message_result get(sys::state& state, Cyto::Any& payload) noexcept override {
		if(index >= int32_t(messages.size()))
			index = 0;
		else if(index < 0)
			index = int32_t(messages.size()) - 1;

		if(payload.holds_type<dcon::nation_id>()) {
			if(messages.empty()) {
				payload.emplace<dcon::nation_id>(dcon::nation_id{});
			} else {
				payload.emplace<dcon::nation_id>(messages[index].from);
			}
			return message_result::consumed;
		} else if(payload.holds_type<element_selection_wrapper<bool>>()) {
			bool b = any_cast<element_selection_wrapper<bool>>(payload).data;
			index += b ? -1 : +1;
			impl_on_update(state);
			return message_result::consumed;
		} else if(payload.holds_type<diplomatic_message::message>()) {
			if(messages.empty()) {
				payload.emplace<diplomatic_message::message>(diplomatic_message::message{});
			} else {
				payload.emplace<diplomatic_message::message>(messages[index]);
			}
			return message_result::consumed;
		} else if(payload.holds_type<diplo_reply_taken_notification>()) {
			if(!messages.empty())
				messages.erase(messages.begin() + size_t(index));
			return message_result::consumed;
		}
		return window_element_base::get(state, payload);
	}
};

}