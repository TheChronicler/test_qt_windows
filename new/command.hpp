//
// Created by kenspeckle on 4/19/23.
//

#ifndef UNTITLED_COMMAND_HPP
#define UNTITLED_COMMAND_HPP


#include <string>
#include <vector>
#include <whisper.h>

class command {

	std::string command_str;
	std::vector<whisper_token> allowed_tokens;

	void print_command();

public:
	whisper_token allowed_token(size_t index) const;

	size_t nbr_allowed_tokens() const;

	std::string get_command_str() const;

	explicit command(whisper_context * ctx, const std::string &command_str_);
};

#endif //UNTITLED_COMMAND_HPP
