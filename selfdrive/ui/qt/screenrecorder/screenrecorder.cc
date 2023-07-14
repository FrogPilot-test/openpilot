#include <QPixmap>
#include <QUrl>

#include <libyuv.h>
#include <sys/time.h>

#include "selfdrive/ui/qt/screenrecorder/screenrecorder.h"
#include "selfdrive/ui/qt/util.h"

static long long milliseconds() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return ((long long)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

ScreenRecorder::ScreenRecorder(QWidget *parent) 
: QPushButton(parent), 
  image_queue(UI_FREQ),
  encoder(std::make_unique<OmxEncoder>(PATH, dst_width, DST_HEIGHT, UI_FREQ, 2 * 1024 * 1024, false, false)),
  rgb_buffer(std::make_unique<uint8_t[]>(src_width * src_height * 4)),
  rgb_scale_buffer(std::make_unique<uint8_t[]>(dst_width * DST_HEIGHT * 4))
{
  setFixedSize(SIZE, SIZE);
  setFocusPolicy(Qt::NoFocus);
  connect(this, &QPushButton::clicked, this, &ScreenRecorder::toggle);
  connect(uiState(), &UIState::offroadTransition, this, &ScreenRecorder::stop);

  soundStart.setSource(QUrl::fromLocalFile("../assets/sounds/start_record.wav"));
  soundStop.setSource(QUrl::fromLocalFile("../assets/sounds/stop_record.wav"));
  soundStart.setVolume(VOLUME);
  soundStop.setVolume(VOLUME);
}

ScreenRecorder::~ScreenRecorder() { stop(false); }

void ScreenRecorder::applyColor() {
  if (frame % (UI_FREQ / 2) == 0) {
    recording_color = (frame % UI_FREQ < UI_FREQ / 2) 
      ? QColor::fromRgbF(1, 0, 0, 0.6) 
      : QColor::fromRgbF(0, 0, 0, 0.3);
    update();
  }
}

void ScreenRecorder::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);
  p.setPen(QPen(QColor::fromRgbF(1, 1, 1, 0.4), MARGIN, Qt::SolidLine, Qt::FlatCap));
  p.setBrush(QBrush(QColor::fromRgbF(0, 0, 0, 0)));
  p.drawEllipse(QRect(MARGIN, MARGIN, width() - 2 * MARGIN, height() - 2 * MARGIN));
  p.setPen(Qt::NoPen);
  p.setBrush(QBrush(recording ? recording_color : QColor::fromRgbF(0, 0, 0, 0.3)));
  p.drawEllipse(QRect(MARGIN_SMALL, MARGIN_SMALL, width() - 2 * MARGIN_SMALL, height() - 2 * MARGIN_SMALL));
}

void ScreenRecorder::openEncoder(const std::string &filename) { encoder->encoder_open(filename.c_str()); }

void ScreenRecorder::closeEncoder() { if (encoder) encoder->encoder_close(); }

void ScreenRecorder::toggle() { recording ? stop(true) : start(true); }

void ScreenRecorder::start(bool sound) {
  if (recording) return;
  recording = true;
  frame = 0;
  rootWidget = this;
  while (rootWidget->parentWidget() != nullptr) rootWidget = rootWidget->parentWidget();

  time_t t = std::time(nullptr);
  struct tm tm = *std::localtime(&t);
  char filename[64];
  std::snprintf(filename, sizeof(filename), "%04d%02d%02d-%02d%02d%02d.mp4", 
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  openEncoder(filename);

  encoding_thread = std::thread([=] { encoding_thread_func(); });

  update();
  started = milliseconds();
  if (sound) soundStart.play();
}

void ScreenRecorder::encoding_thread_func() {
  uint64_t start_time = nanos_since_boot() - 1;
  while (recording && encoder) {
    QImage popImage;
    if (!image_queue.pop_wait_for(popImage, std::chrono::milliseconds(10))) continue;

    QImage image = popImage.convertToFormat(QImage::Format_RGBA8888);
    libyuv::ARGBScale(image.bits(), image.width() * 4,
            image.width(), image.height(),
            rgb_scale_buffer.get(), dst_width * 4,
            dst_width, DST_HEIGHT,
            libyuv::kFilterLinear);
    encoder->encode_frame_rgba(rgb_scale_buffer.get(), dst_width, DST_HEIGHT, (uint64_t)nanos_since_boot() - start_time);
  }
}

void ScreenRecorder::stop(bool sound) {
  if (!recording) return;
  recording = false;
  update();
  closeEncoder();
  image_queue.clear();
  if (encoding_thread.joinable()) encoding_thread.join();
  if (sound) soundStop.play();
}

void ScreenRecorder::update_screen() {
  if (!recording) return;
  if (milliseconds() - started > 1000 * 60 * 3) {
    stop(false);
    start(false);
    return;
  }
  applyColor();
  if (rootWidget != nullptr) {
    QPixmap pixmap = rootWidget->grab();
    image_queue.push(pixmap.toImage());
  }
  frame++;
}
