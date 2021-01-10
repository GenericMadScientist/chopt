/*
 * CHOpt - Star Power optimiser for Clone Hero
 * Copyright (C) 2020, 2021 Raymond Wright
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CHOPT_MAINWINDOW_HPP
#define CHOPT_MAINWINDOW_HPP

#include <memory>
#include <optional>

#include <QMainWindow>
#include <QString>
#include <QThread>

#include "settings.hpp"
#include "song.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    std::unique_ptr<Ui::MainWindow> m_ui;
    std::optional<Song> m_song;
    QThread* m_thread = nullptr;
    Settings get_settings() const;
    void load_file(const QString& file_name);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_earlyWhammySlider_valueChanged(int value);
    void on_findPathButton_clicked();
    void on_instrumentComboBox_currentIndexChanged(int value);
    void on_opacitySlider_valueChanged(int value);
    void on_selectFileButton_clicked();
    void on_squeezeSlider_valueChanged(int value);
    void on_videoLagSlider_valueChanged(int value);
    void parsing_failed(const QString& file_name);
    void path_found();
    void song_read(const Song& song, const QString& file_name);
    void write_message(const QString& message);
};

#endif
