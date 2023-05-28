#include <whisper.h>
#include <algorithm>
#include <thread>
#include <QApplication>
#include <future>
#include <iostream>
#include <SDL.h>
#include <SDL_audio.h>
#include "common-sdl.h"
#include <mutex>


int main_gui(int argc, char *argv[]) {
    qRegisterMetaType<std::string>();
	SDL_Init( SDL_INIT_EVERYTHING );
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
    return QApplication::exec();
}
int main_test() {

	std::vector<std::string> command_strs{
		"enable",
		"disable",
		"cat",
		"dog",
		"apple",
		"red",
		"blue",
		"green",
		"lightblue"
	};


    return 0;

}



int main(int argc, char *argv[]) {
	auto recording_devices = audio_async::list_devices();
	for (const auto & recording_device : recording_devices) {
		std::cout << recording_device << std::endl;
	}

	std::cout << "================================================================" << std::endl;
    return main_gui(argc, argv);
}
