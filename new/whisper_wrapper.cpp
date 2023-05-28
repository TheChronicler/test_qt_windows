//
// Created by kenspeckle on 4/19/23.
//

#include <whisper.h>
#include <iostream>
#include <thread>
#include <algorithm>
#include <future>
#include <sstream>
#include "whisper_wrapper.hpp"
#include "../common.h"

whisper_wrapper::whisper_wrapper(const std::string &model_path) : audio(audio_async(30 * 1000)) {
	set_model_path(model_path);

}

whisper_wrapper::whisper_wrapper() : audio(audio_async(30 * 1000)) {}

void whisper_wrapper::set_model_path(const std::string &model_path_) {
	this->model_path = model_path_;
	this->ctx = whisper_init_from_file(this->model_path.c_str());
}

void whisper_wrapper::init_audio(int index) {
    std::cerr << "Audio index: " << std::to_string(index) << std::endl;
	const unsigned int nbr_recording_devices = audio_async::get_number_recording_devices();
	if (index >= nbr_recording_devices || !audio.init(index, WHISPER_SAMPLE_RATE)) {
		throw std::runtime_error{std::string{__func__} + ": audio.init() failed!"};
	}
}

whisper_wrapper::~whisper_wrapper() {
	if (ctx != nullptr) {
		whisper_print_timings(ctx);
	}
	whisper_free(ctx);
}

void whisper_wrapper::start_recording() {
	keep_ms = std::min(keep_ms, step_ms);
	length_ms = std::max(length_ms, step_ms);
	init_audio((int) microphone_idx);
	audio.resume();

	// wait for 1 second to avoid any buffered noise
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	audio.clear();

	is_running = true;

    std::cout << "Starting the whisper thread" << std::endl;
//	loop();

    this->recording_thread = std::thread(&whisper_wrapper::loop, this);
//    std::future<int> fi = std::async(std::launch::async, loop, std::ref(on));

//	this->recording_thread.join();asdfasdf
//	process_command_list(ctx, audio);
//	always_prompt_transcription(ctx, audio, params);
//	process_general_transcription(ctx, audio, params);

}

void whisper_wrapper::stop_recording() {
    std::cout << "Stopping the whisper thread ..." << std::endl;
	is_running = false;
	audio.pause();
    this->recording_thread.join();

    std::cout << "Stopped" << std::endl;
}


void whisper_wrapper::loop() {
	bool sdl_poll_successfull = true;
	while (is_running && sdl_poll_successfull) {
        std::cout << "Looping luis" << std::endl;
		// handle Ctrl + C
		sdl_poll_successfull = sdl_poll_events();

		// delay
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

//		handle_command_mode();
		handle_stream_mode();

	}
}

void whisper_wrapper::log_token_probabilities(const std::vector<float> &probs,
											  std::vector<std::pair<float, int>> &probs_id) const {
	for (const auto &cmd: probs_id) {
//		std::cout << __func__ << ": \033[1m" << commands[cmd.second].get_command_str() << "\033[0m = " << std::to_string(cmd.first) << std::endl;
		std::cout << __func__ << ": " << commands[cmd.second].get_command_str() << " = "
				  << std::to_string(cmd.first);// << std::endl;
		for (size_t i = 0; i < commands[cmd.second].nbr_allowed_tokens(); i++) {
			auto token = commands[cmd.second].allowed_token(i);
			std::cout << "'" << whisper_token_to_str(ctx, token) << "' " << std::to_string(probs[token]);
		}
		std::cout << std::endl;
	}
}

void whisper_wrapper::handle_stream_mode() {
	const int n_samples_30s = (1e-3 * 30000.0) * WHISPER_SAMPLE_RATE;
	std::vector<float> pcmf32(n_samples_30s, 0.0f);
	std::vector<float> pcmf32_old;
	std::vector<float> pcmf32_new(n_samples_30s, 0.0f);


	const int n_samples_step = (1e-3 * step_ms) * WHISPER_SAMPLE_RATE;
	const int n_samples_len = (1e-3 * length_ms) * WHISPER_SAMPLE_RATE;
	const int n_samples_keep = (1e-3 * keep_ms) * WHISPER_SAMPLE_RATE;
	int n_iter = 0;
	std::vector<whisper_token> prompt_tokens;
	const int n_new_line = std::max(1, length_ms / step_ms - 1);

	while (true) {
		audio.get(step_ms, pcmf32_new);
		if ((int) pcmf32_new.size() > 2 * n_samples_step) {
			std::cerr << __func__ << ": WARNING: cannot process audio fast enough, dropping audio ..." << std::endl;
			audio.clear();
			continue;
		}
		if ((int) pcmf32_new.size() >= n_samples_step) {
			audio.clear();
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

    std::cout << "End of endless loop" << std::endl;

	const int n_samples_new = pcmf32_new.size();
	// take up to params.length_ms audio from previous iteration
	const int n_samples_take = std::min((int) pcmf32_old.size(),
										std::max(0, n_samples_keep + n_samples_len - n_samples_new));
	pcmf32.resize(n_samples_new + n_samples_take);
	for (int i = 0; i < n_samples_take; i++) {
		pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
	}
	std::copy(pcmf32_new.cbegin(), pcmf32_new.cend(), pcmf32.begin() + n_samples_take);
	memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(), n_samples_new * sizeof(float));


	// run the inference
	whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

	wparams.print_progress = debug;
	wparams.print_special = debug;
	wparams.print_realtime = debug;
	wparams.print_timestamps = debug;
	wparams.translate = translate;
	wparams.single_segment = !use_sliding_window;
	wparams.max_tokens = max_tokens;
	wparams.language = "de";
	wparams.n_threads = n_threads;
	wparams.audio_ctx = microphone_idx;
	wparams.speed_up = speed_up;

	wparams.prompt_tokens = keep_context ? prompt_tokens.data() : nullptr;
	wparams.prompt_n_tokens = keep_ms ? prompt_tokens.size() : 0;

    std::cerr << "Just before whisper_full" << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
	if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
        std::cerr << "failed to process audio" << std::endl;
        return;
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::cerr << "Just after whisper_full. This took " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms" << std::endl;

    std::cerr << "Just before whisper_full_n_segments" << std::endl;
	const int n_segments = whisper_full_n_segments(ctx);
    std::cerr << "Just after whisper_full_n_segments" << std::endl;

    std::cerr << "Just before whisper_full_get_segment_text" << std::endl;
    std::stringstream ss;
    for (int i = 0; i < n_segments; ++i) {
		const char *text = whisper_full_get_segment_text(ctx, i);
        ss << text;
	}
    std::string new_text = ss.str();
    std::cout << std::endl << new_text << std::endl;

    emit text_transcribed(new_text);
    std::cerr << std::endl << "Just after whisper_full_get_segment_text" << std::endl;



    ++n_iter;
	if (!use_sliding_window && (n_iter % n_new_line) == 0) {
		printf("\n");
		std::cout << std::endl;

		// keep part of the audio for next iteration to try to mitigate word boundary issues
		pcmf32_old = std::vector<float>(pcmf32.end() - n_samples_keep, pcmf32.end());

		// Add tokens of the last full length segment as the prompt
		if (keep_context) {
			prompt_tokens.clear();
			const int n_segments = whisper_full_n_segments(ctx);
			for (int i = 0; i < n_segments; ++i) {
				const int token_count = whisper_full_n_tokens(ctx, i);
				for (int j = 0; j < token_count; ++j) {
					prompt_tokens.push_back(whisper_full_get_token_id(ctx, i, j));
				}
			}
		}
	}


}

void whisper_wrapper::handle_command_mode() {
	std::vector<float> pcmf32_cur;
	audio.get(2000, pcmf32_cur);

	if (::vad_simple(pcmf32_cur, WHISPER_SAMPLE_RATE, 1000, vad_thold, freq_thold, debug)) {
		return;
	}
	std::cout << __func__ << ": Speech detected! Processing ..." << std::endl;

	const auto t_start = std::chrono::high_resolution_clock::now();

	whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

	wparams.print_progress = false;
	wparams.print_special = false;        // TODO debug logging also option //;params.print_special;
	wparams.print_realtime = false;
	wparams.print_timestamps = false;        // TODO debug logging !params.no_timestamps;
	wparams.translate = translate;
	wparams.no_context = true;
	wparams.single_segment = true;
	wparams.max_tokens = 1;
	wparams.language = language.c_str();
	wparams.n_threads = n_threads;

	wparams.audio_ctx = microphone_idx;    // TODO rausfinden was das hier macht
	wparams.speed_up = speed_up;

	wparams.prompt_tokens = k_tokens.data();
	wparams.prompt_n_tokens = (int) k_tokens.size();

	// run the transformer and a single decoding pass
	if (whisper_full(ctx, wparams, pcmf32_cur.data(), (int) pcmf32_cur.size()) != 0) {
		fprintf(stderr, "%s: ERROR: whisper_full() failed\n", __func__);
		return;
	}

	// estimate command probability
	// NOTE: not optimal

	const auto *logits = whisper_get_logits(ctx);
	std::vector<float> probs(whisper_n_vocab(ctx), 0.0f);

	// compute probs from logits via softmax
	float max = -1e9;
	for (size_t i = 0; i < probs.size(); ++i) {
		max = std::max(max, logits[i]);
	}
	float sum = 0.0f;
	for (size_t i = 0; i < probs.size(); ++i) {
		probs[i] = expf(logits[i] - max);
		sum += probs[i];
	}
	for (float &prob: probs) {
		prob /= sum;
	}

	std::vector<std::pair<float, int>> probs_id;
	double psum = 0.0;
	for (size_t i = 0; i < commands.size(); ++i) {
		probs_id.emplace_back(probs[commands[i].allowed_token(0)], i);
		for (size_t j = 1; j < commands[i].nbr_allowed_tokens(); ++j) {
			probs_id.back().first += probs[commands[i].allowed_token(j)];
		}
		probs_id.back().first /= commands[i].nbr_allowed_tokens();
		psum += probs_id.back().first;
	}

	// normalize
	for (auto &p: probs_id) {
		p.first /= psum;
	}

	// sort descending
	std::sort(probs_id.begin(), probs_id.end(), [](const std::pair<float, int> &a, const std::pair<float, int> &b) {
		return a.first > b.first;
	});

	// print the commands and the respective probabilities
	log_token_probabilities(probs, probs_id);

	// best command
	const float prob = probs_id[0].first;

	if (prob < 0.5) {
		std::cout << "Detected a command (" << commands[probs_id[0].second].get_command_str()
				  << ") but the probability is below 0.5: " << std::to_string(prob) << std::endl;
	} else {
		const int index = probs_id[0].second;
		auto duration = (int) std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::high_resolution_clock::now() - t_start).count();
		std::cout << std::endl << __func__ << ": detected command: \033[1m" << commands[index].get_command_str()
				  << "\033[0m | p = " << std::to_string(prob) << " | calc_time = " << std::to_string(duration)
				  << std::endl;
	}

	audio.clear();
	std::cout << std::endl << std::endl;
}

void whisper_wrapper::add_command(const std::string &new_command) {
	commands.emplace_back(ctx, new_command);
}

void whisper_wrapper::set_microphone_index(unsigned int index) {
	this->microphone_idx = index;
}


