//
// Created by kenspeckle on 5/23/23.
//


#include <QProcess>

int main() {
    auto *process = new QProcess();
    process->start("/usr/bin/gnome-calculator");
    process->waitForStarted();
    process->waitForFinished();

}