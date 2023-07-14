#pragma once

#include <QPainter>
#include <QPushButton>
#include <QSoundEffect>
#include <chrono>
#include <ctime>
#include <thread>

#include "blocking_queue.h"
#include "omx_encoder.h"
#include "selfdrive/ui/ui.h"

constexpr auto PATH = "/data/media/0/videos";
constexpr float VOLUME = 0.5f;
constexpr int DST_HEIGHT = 720;
constexpr int SIZE = 190;
constexpr int MARGIN = 5;
constexpr int MARGIN_SMALL = 40;
constexpr int SRC_WIDTH = 2160;
constexpr int SRC_HEIGHT = 1080;

class ScreenRecorder : public QPushButton {
  Q_OBJECT

public:
  explicit ScreenRecorder(QWidget *parent = nullptr);
  ~ScreenRecorder() override;

  void publicUpdateScreen() {
    update_screen();
  }

protected:
  void paintEvent(QPaintEvent*) override;

private:
  bool recording = false;
  int src_width = SRC_WIDTH, src_height = SRC_HEIGHT;
  int dst_width = src_width * DST_HEIGHT / src_height + src_width % 2;
  int frame = 0;
  long long started = 0;
  QColor recording_color;

  BlockingQueue<QImage> image_queue;
  QSoundEffect soundStart, soundStop;
  QWidget* rootWidget = nullptr;
  std::unique_ptr<OmxEncoder> encoder;
  std::unique_ptr<uint8_t[]> rgb_buffer, rgb_scale_buffer;
  std::thread encoding_thread;

  void applyColor();
  void closeEncoder();
  void encoding_thread_func();
  void openEncoder(const std::string& filename);
  void start(bool sound);
  void stop(bool sound);
  void toggle();
  void update_screen();
};
