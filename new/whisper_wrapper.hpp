//
// Created by kenspeckle on 4/19/23.
//

#ifndef UNTITLED_WHISPER_WRAPPER_HPP
#define UNTITLED_WHISPER_WRAPPER_HPP

#include <thread>
#include <QObject>
#include "../common-sdl.h"
#include "command.hpp"


class whisper_wrapper : public QObject{
    Q_OBJECT
    signals:
        void text_transcribed(std::string);

private:
	audio_async audio{30*1000};
	std::string model_path;
    unsigned int microphone_idx = 0;
	struct whisper_context * ctx = nullptr;
	std::thread recording_thread;
	std::mutex m;
	bool is_running = false;

	std::vector<command> commands{};
	std::vector<whisper_token> k_tokens{};
	float vad_thold    = 0.6f;
	float freq_thold   = 100.0f;
	int max_tokens = 32;
	bool translate = false;
	bool speed_up = false;
	std::string language;
	int32_t step_ms = 3000;    //TODO
	int32_t length_ms = 10000;
	int32_t keep_ms = 200;
	bool keep_context = true;
	bool use_sliding_window = false;

	bool debug = false;


	unsigned int n_threads = std::min(8, (int32_t) std::thread::hardware_concurrency());


	void init_audio(int index);
	void loop();
	void handle_command_mode();
	void handle_stream_mode();
	void log_token_probabilities(const std::vector<float> &probs, std::vector<std::pair<float, int>> &probs_id) const;

public:
	explicit whisper_wrapper(const std::string & model_path);
	whisper_wrapper();

	virtual ~whisper_wrapper();

	void set_model_path(const std::string &model_path);

	void add_command(const std::string & new_command);

	void start_recording();

	void stop_recording();

    void set_microphone_index(unsigned int index);

};


#endif //UNTITLED_WHISPER_WRAPPER_HPP
