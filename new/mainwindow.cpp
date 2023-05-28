#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <filesystem>
#include <QFileDialog>
#include <iostream>
#include <QProcess>
#include <QLineEdit>
#include <QSettings>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	whisper(){
	ui->setupUi(this);

    loadSettings();

    ui->transcribedTextEdit->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

	if (std::filesystem::is_directory(std::filesystem::current_path() / "models")) {
		this->model_folder = std::filesystem::absolute(std::filesystem::current_path() / "models");
	} else if (std::filesystem::is_directory(std::filesystem::current_path() / "../models/")) {
		this->model_folder = std::filesystem::absolute(std::filesystem::current_path() / "../models");
	} else {
		QString dir = QFileDialog::getExistingDirectory(
				this,
				tr("Open Model Folder"),
				QString::fromStdString(absolute(relative(std::filesystem::current_path(), "..")).string()),
				QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		this->model_folder = std::filesystem::absolute(dir.toStdString());
	}

	std::string init_model_path{};
	for (const auto & entry : std::filesystem::directory_iterator(model_folder)) {
		std::cout << entry.path() << std::endl;
		ui->model_combobox->addItem(QString::fromStdString(entry.path().filename()), QString::fromStdString(entry.path()));
		init_model_path = entry.path();
	}
	whisper.set_model_path(init_model_path);


    connect(&whisper, &whisper_wrapper::text_transcribed, this, &MainWindow::text_transcribed);
	connect(ui->model_combobox, &QComboBox::activated, this, &MainWindow::model_changed);
	connect(ui->model_combobox, &QComboBox::currentIndexChanged, this, &MainWindow::model_changed);

	auto recording_devices = audio_async::list_devices();
	for (size_t i = 0; i < recording_devices.size(); i++) {
		ui->microphone_comboxbox->addItem(QString::fromStdString(recording_devices[i]), QVariant::fromValue(i));
		std::cout << recording_devices[i] << std::endl;
	}
	connect(ui->model_combobox, &QComboBox::activated, this, &MainWindow::microphone_combobox_activated);
	connect(ui->microphone_comboxbox, &QComboBox::currentIndexChanged, this, &MainWindow::microphone_changed);
	connect(ui->recordBtn, &QPushButton::clicked, this, &MainWindow::record_button_pressed);

    connect(ui->newCommandBtn, &QPushButton::clicked, this, &MainWindow::add_command_btn_pressed);
    connect(ui->tableWidget, &QTableWidget::itemClicked, this, &MainWindow::on_table_cell_clicked);
    connect(ui->tableWidget, &QTableWidget::itemChanged, [this](){
        this->save_settings();
    });


    auto header = ui->tableWidget->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);

}

MainWindow::~MainWindow() {
	delete ui;
}


void MainWindow::model_changed(int index) {
	whisper.set_model_path(ui->model_combobox->itemData(index).toString().toStdString());
    save_settings();
}

void MainWindow::microphone_combobox_activated(int index) {
	ui->microphone_comboxbox->clear();
	auto recording_devices = audio_async::list_devices();
	for (size_t i = 0; i < recording_devices.size(); i++) {
		ui->microphone_comboxbox->addItem(QString::fromStdString(recording_devices[i]), QVariant::fromValue(i));
	}
    save_settings();
}

void MainWindow::microphone_changed(int index) {
	whisper.set_microphone_index(index);
    save_settings();
}

void MainWindow::record_button_pressed() {
    QPalette pal = ui->recordBtn->palette();

    is_recording = !is_recording;
	if (is_recording) {
        ui->recordBtn->setText("Stop");
        pal.setColor(QPalette::Button, QColor(Qt::green));    ui->recordBtn->setAutoFillBackground(true);
        ui->recordBtn->setPalette(pal);
        ui->recordBtn->update();
        whisper.start_recording();
	} else {
        ui->recordBtn->setText("Record");
        pal.setColor(QPalette::Button, QColor(Qt::red));
        ui->recordBtn->setAutoFillBackground(true);
        ui->recordBtn->setPalette(pal);
        ui->recordBtn->update();
        whisper.stop_recording();
    }
}

void MainWindow::text_transcribed(std::string new_text) {
    this->complete_text += new_text;
    this->previous_text = new_text;
    ui->transcribedTextEdit->setPlainText(QString::fromStdString(complete_text));

    auto tmp_text = this->previous_text + new_text;
    std::transform(tmp_text.begin(), tmp_text.end(), tmp_text.begin(), [](unsigned char c){ return std::tolower(c); });

    for (size_t i = 0; i < ui->tableWidget->rowCount(); i++) {
        auto current_command_text = ui->tableWidget->item(i, 0)->text();
        auto current_path = ui->tableWidget->item(i, 1)->toolTip();
        if (current_command_text.isEmpty() || current_command_text.contains("Auswahl Programm")) {
            std::cout << "Skipping command \"" << current_command_text.toStdString() << "\" because no executable was selected" << std::endl;
            continue;
        }

        if (tmp_text.find(current_command_text.toLower().toStdString()) != std::string::npos) {
            std::cout << "Its a match: \"" << current_command_text.toStdString() << "\"" << std::endl;
            std::cout << "Trying to execute \"" << current_path.toStdString() << "\"" << std::endl;
            QProcess::startDetached(current_path);
//            return;
        }
    }

}


void MainWindow::add_command_btn_pressed() {
    std::cout << "Add new row" << std::endl;
    const auto rowCount = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(rowCount);

    auto *commandItem = new QTableWidgetItem();
    commandItem->setFlags(commandItem->flags() | Qt::ItemIsEditable);
    ui->tableWidget->setItem(rowCount, 0, commandItem);

    auto *valueItem = new QTableWidgetItem();
    valueItem->setFlags(valueItem->flags() ^ Qt::ItemIsEditable);
    ui->tableWidget->setItem(rowCount, 1, valueItem);
//    std::cout << "Done" << std::endl;
}



void MainWindow::on_table_cell_clicked(QTableWidgetItem * item) {
    if (item->column() != 1) {
        return;
    }
    auto fileName = QFileDialog::getOpenFileName(
            this,
            "Open Program",
            "/usr/bin");
    QFileInfo fileInfo{fileName};
    item->setText(fileInfo.fileName());
    item->setToolTip(fileName);
    this->save_settings();

}


void MainWindow::loadSettings() {

    QMap<QString, QVariant> tmp_command_map = settings.value("commands").toMap();
    for (QMap<QString, QVariant>::iterator it = tmp_command_map.begin(); it != tmp_command_map.end(); it++) {
        const auto rowCount = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(rowCount);

        auto *commandItem = new QTableWidgetItem();
        commandItem->setFlags(commandItem->flags() | Qt::ItemIsEditable);
        commandItem->setText(it.key());
        ui->tableWidget->setItem(rowCount, 0, commandItem);

        auto *valueItem = new QTableWidgetItem();
        valueItem->setFlags(valueItem->flags() ^ Qt::ItemIsEditable);
        commandItem->setText(it.value().toString());
        ui->tableWidget->setItem(rowCount, 1, valueItem);

    }

    for (size_t i = 0; i < ui->tableWidget->rowCount(); i++) {
        auto command = ui->tableWidget->item(i, 0)->text();
        auto path = ui->tableWidget->item(i, 1)->toolTip();
        tmp_command_map.insert(command, QVariant{path});
    }


    auto last_used_model_name = settings.value("last_used_model").toString();

    for (size_t i = 0; i < ui->model_combobox->count(); i++) {
        if (ui->model_combobox->itemData(i).toString() == last_used_model_name) {
            ui->model_combobox->setCurrentIndex(i);
            break;
        }
    }


    auto last_used_microphone_string = settings.value("last_used_microphone").toString();
    for (size_t i = 0; i < ui->microphone_comboxbox->count(); i++) {
        if (ui->microphone_comboxbox->itemData(i).toString() == last_used_microphone_string) {
            ui->microphone_comboxbox->setCurrentIndex(i);
            break;
        }
    }

}

void MainWindow::save_settings() {
    QMap<QString, QVariant> tmp_command_map;
    for (size_t i = 0; i < ui->tableWidget->rowCount(); i++) {
        auto command_item = ui->tableWidget->item(i, 0);
        auto path_item = ui->tableWidget->item(i, 1);
        if (command_item == nullptr || path_item == nullptr) {
            continue;
        }
        tmp_command_map.insert(command_item->text(), QVariant{path_item->toolTip()});
    }

    settings.setValue("commands", QVariant{tmp_command_map});
    settings.setValue("last_used_model", ui->model_combobox->itemData(ui->model_combobox->currentIndex()).toString());
    settings.setValue("last_used_microphone", ui->microphone_comboxbox->itemData(ui->microphone_comboxbox->currentIndex()).toString());

    settings.sync();
}