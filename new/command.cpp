//
// Created by kenspeckle on 4/19/23.
//

#include <iostream>
#include "command.hpp"

command::command(whisper_context * ctx, const std::string &command_str_) {
	whisper_token tokens[1024];

	for (size_t l = 0; l < command_str_.size(); ++l) {
		// NOTE: very important to add the whitespace !
		//       the reason is that the first decoded token starts with a whitespace too!
		std::string ss = std::string(" ") + command_str_.substr(0, l + 1);

		const int n = whisper_tokenize(ctx, ss.c_str(), tokens, 1024);
		if (n < 0) {
			std::cerr << __func__ << ": error: failed to tokenize command '" << command_str_.c_str() << "'" << std::endl;
			throw std::runtime_error{""};
		}

		if (n == 1) {
			allowed_tokens.push_back(tokens[0]);
		}
	}
	command_str = command_str_;
}

void command::print_command() {
	std::cerr << "  - \033[1m" << command_str << "\033[0m = [";
	for (const auto & token : allowed_tokens) {
		fprintf(stderr, " %5d", token);
	}
	std::cerr << " ]" << std::endl;
}

whisper_token command::allowed_token(size_t index) const {
	return allowed_tokens[index];
}

size_t command::nbr_allowed_tokens() const{
	return allowed_tokens.size();
}

std::string command::get_command_str() const {
	return command_str;
}
