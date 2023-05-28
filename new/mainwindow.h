#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSettings>
#include <QTableWidgetItem>
#include "whisper_wrapper.hpp"

Q_DECLARE_METATYPE(std::string)

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void text_transcribed(std::string);

private:
    Ui::MainWindow *ui;
    std::string model_folder;
    whisper_wrapper whisper;
    bool is_recording = false;
    std::string complete_text{};
    std::string previous_text{};
    QSettings settings{"Kenspeckle", "Dragon Clone"};


    void model_changed(int index);

    void microphone_combobox_activated(int index);

    void microphone_changed(int index);

    void record_button_pressed();

    void add_command_btn_pressed();

//    void remove_command_btn_clicked();

    void loadSettings();

    void save_settings();

    void on_table_cell_clicked(QTableWidgetItem *);
};

#endif // MAINWINDOW_H
