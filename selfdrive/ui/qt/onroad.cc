#include "selfdrive/ui/qt/onroad.h"

#include <chrono>
#include <cmath>

#include <QDebug>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QTimer>

#include "common/timing.h"
#include "selfdrive/ui/qt/util.h"
#ifdef ENABLE_MAPS
#include "selfdrive/ui/qt/maps/map_helpers.h"
#include "selfdrive/ui/qt/maps/map_panel.h"
#endif

OnroadWindow::OnroadWindow(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout  = new QVBoxLayout(this);
  main_layout->setMargin(UI_BORDER_SIZE);
  QStackedLayout *stacked_layout = new QStackedLayout;
  stacked_layout->setStackingMode(QStackedLayout::StackAll);
  main_layout->addLayout(stacked_layout);

  nvg = new AnnotatedCameraWidget(VISION_STREAM_ROAD, this);

  QWidget * split_wrapper = new QWidget;
  split = new QHBoxLayout(split_wrapper);
  split->setContentsMargins(0, 0, 0, 0);
  split->setSpacing(0);
  split->addWidget(nvg);

  if (getenv("DUAL_CAMERA_VIEW")) {
    CameraWidget *arCam = new CameraWidget("camerad", VISION_STREAM_ROAD, true, this);
    split->insertWidget(0, arCam);
  }

  if (getenv("MAP_RENDER_VIEW")) {
    CameraWidget *map_render = new CameraWidget("navd", VISION_STREAM_MAP, false, this);
    split->insertWidget(0, map_render);
  }

  stacked_layout->addWidget(split_wrapper);

  alerts = new OnroadAlerts(this);
  alerts->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  stacked_layout->addWidget(alerts);

  // setup stacking order
  alerts->raise();

  setAttribute(Qt::WA_OpaquePaintEvent);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &OnroadWindow::updateState);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &OnroadWindow::offroadTransition);
}

void OnroadWindow::updateState(const UIState &s) {
  QColor bgColor = bg_colors[s.status];
  Alert alert = Alert::get(*(s.sm), s.scene.started_frame);
  alerts->updateAlert(alert);

  if (s.scene.map_on_left) {
    split->setDirection(QBoxLayout::LeftToRight);
  } else {
    split->setDirection(QBoxLayout::RightToLeft);
  }

  nvg->updateState(s);

  if (bg != bgColor) {
    // repaint border
    bg = bgColor;
    update();
  }
}

void OnroadWindow::mousePressEvent(QMouseEvent* e) {
  // FrogPilot clickable widgets
  const auto &scene = uiState()->scene;
  const SubMaster &sm = *uiState()->sm;
  static Params params;
  const bool isToyotaCar = scene.toyota_car;
  static bool recentlyTapped = false;
  const int x_offset = scene.mute_dm ? 50 : 250;
  bool rightHandDM = sm["driverMonitoringState"].getDriverMonitoringState().getIsRHD();

  // Driving personalities button
  int x = rightHandDM ? rect().right() - (btn_size - 24) / 2 - (UI_BORDER_SIZE * 2) - x_offset : (btn_size - 24) / 2 + (UI_BORDER_SIZE * 2) + x_offset;
  const int y = rect().bottom() - (scene.conditional_experimental || scene.always_on_lateral ? 25 : 0) - 140;
  // Give the button a 25% offset so it doesn't need to be clicked on perfectly
  const bool isDrivingPersonalitiesClicked = (e->pos() - QPoint(x, y)).manhattanLength() <= btn_size * 1.25 && !isToyotaCar;

  // Change cruise control increments button
  const QRect maxSpeedRect(0, 0, 350, 350);
  const bool isMaxSpeedClicked = maxSpeedRect.contains(e->pos()) && isToyotaCar;

  // Hide speed button
  const QRect speedRect(rect().center().x() - 175, 50, 350, 350);
  const bool isSpeedClicked = speedRect.contains(e->pos());

  // Check if the driving personality button was clicked
  if (isDrivingPersonalitiesClicked) {
    personalityProfile = (params.getInt("LongitudinalPersonality") + 2) % 3;
    params.putInt("LongitudinalPersonality", personalityProfile);
    propagateEvent = false;
  // Check if the click was within the max speed area
  } else if (isMaxSpeedClicked) {
    const bool currentReverseCruiseIncrease = params.getBool("ReverseCruiseIncrease");
    reverseCruiseIncrease = !currentReverseCruiseIncrease;
    params.putBool("ReverseCruiseIncrease", reverseCruiseIncrease);
    params.putBool("FrogPilotTogglesUpdated", true);
    propagateEvent = false;
  // Check if the click was within the speed text area
  } else if (isSpeedClicked) {
    const bool currentVisibility = params.getBool("HideSpeed");
    speedVisible = !currentVisibility;
    params.putBool("HideSpeed", speedVisible);
    propagateEvent = false;
  // If the click wasn't for anything specific, change the value of "ExperimentalMode" and "ConditionalStatus"
  } else if (recentlyTapped && scene.experimental_mode_via_wheel) {
    if (scene.conditional_experimental) {
      const int override_value = (scene.conditional_status == 1 || scene.conditional_status == 2) ? 0 : scene.conditional_status >= 2 ? 1 : 2;
      params.putInt("ConditionalStatus", override_value);
    } else {
      const bool experimentalMode = params.getBool("ExperimentalMode");
      params.putBool("ExperimentalMode", !experimentalMode);
    }
    recentlyTapped = false;
    propagateEvent = true;
  } else {
    recentlyTapped = true;
    propagateEvent = true;
  }

  const bool clickedOnWidget = isDrivingPersonalitiesClicked || isMaxSpeedClicked || isSpeedClicked;

#ifdef ENABLE_MAPS
  if (map != nullptr && !clickedOnWidget) {
    // Switch between map and sidebar when using navigate on openpilot
    bool sidebarVisible = geometry().x() > 0;
    bool show_map = uiState()->scene.navigate_on_openpilot ? sidebarVisible : !sidebarVisible;
    map->setVisible(show_map && !map->isVisible() && propagateEvent);
    map_open = map->isVisible();
  }
#endif
  // propagation event to parent(HomeWindow)
  if (propagateEvent) {
    QWidget::mousePressEvent(e);
  }
}

void OnroadWindow::offroadTransition(bool offroad) {
#ifdef ENABLE_MAPS
  if (!offroad) {
    if (map == nullptr && (uiState()->primeType() || !MAPBOX_TOKEN.isEmpty())) {
      auto m = new MapPanel(get_mapbox_settings());
      map = m;

      QObject::connect(m, &MapPanel::mapPanelRequested, this, &OnroadWindow::mapPanelRequested);

      m->setFixedWidth(topWidget(this)->width() / 2 - UI_BORDER_SIZE);
      split->insertWidget(0, m);

      // hidden by default, made visible when navRoute is published
      m->setVisible(false);
    }
  }
#endif

  alerts->updateAlert({});
}

void OnroadWindow::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.fillRect(rect(), QColor(bg.red(), bg.green(), bg.blue(), 255));
}

// ***** onroad widgets *****

// OnroadAlerts
void OnroadAlerts::updateAlert(const Alert &a) {
  if (!alert.equal(a)) {
    alert = a;
    update();
  }
}

void OnroadAlerts::paintEvent(QPaintEvent *event) {
  const auto &scene = uiState()->scene;
  if (alert.size == cereal::ControlsState::AlertSize::NONE) {
    return;
  }
  static std::map<cereal::ControlsState::AlertSize, const int> alert_heights = {
    {cereal::ControlsState::AlertSize::SMALL, 271},
    {cereal::ControlsState::AlertSize::MID, 420},
    {cereal::ControlsState::AlertSize::FULL, height()},
  };
  int h = alert_heights[alert.size];

  int margin = 40;
  int radius = 30;
  if (alert.size == cereal::ControlsState::AlertSize::FULL) {
    margin = 0;
    radius = 0;
  }
  QRect r = QRect(0 + margin, height() - h + margin - (scene.conditional_experimental || scene.always_on_lateral && alert.size != cereal::ControlsState::AlertSize::FULL ? 25 : 0), width() - margin*2, h - margin*2);

  QPainter p(this);

  // draw background + gradient
  p.setPen(Qt::NoPen);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);
  p.setBrush(QBrush(alert_colors[alert.status]));
  p.drawRoundedRect(r, radius, radius);

  QLinearGradient g(0, r.y(), 0, r.bottom());
  g.setColorAt(0, QColor::fromRgbF(0, 0, 0, 0.05));
  g.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0.35));

  p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
  p.setBrush(QBrush(g));
  p.drawRoundedRect(r, radius, radius);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  // text
  const QPoint c = r.center();
  p.setPen(QColor(0xff, 0xff, 0xff));
  p.setRenderHint(QPainter::TextAntialiasing);
  if (alert.size == cereal::ControlsState::AlertSize::SMALL) {
    p.setFont(InterFont(74, QFont::DemiBold));
    p.drawText(r, Qt::AlignCenter, alert.text1);
  } else if (alert.size == cereal::ControlsState::AlertSize::MID) {
    p.setFont(InterFont(88, QFont::Bold));
    p.drawText(QRect(0, c.y() - 125, width(), 150), Qt::AlignHCenter | Qt::AlignTop, alert.text1);
    p.setFont(InterFont(66));
    p.drawText(QRect(0, c.y() + 21, width(), 90), Qt::AlignHCenter, alert.text2);
  } else if (alert.size == cereal::ControlsState::AlertSize::FULL) {
    bool l = alert.text1.length() > 15;
    p.setFont(InterFont(l ? 132 : 177, QFont::Bold));
    p.drawText(QRect(0, r.y() + (l ? 240 : 270), width(), 600), Qt::AlignHCenter | Qt::TextWordWrap, alert.text1);
    p.setFont(InterFont(88));
    p.drawText(QRect(0, r.height() - (l ? 361 : 420), width(), 300), Qt::AlignHCenter | Qt::TextWordWrap, alert.text2);
  }
}

// ExperimentalButton
ExperimentalButton::ExperimentalButton(QWidget *parent) : experimental_mode(false), engageable(false), QPushButton(parent) {
  setFixedSize(btn_size, btn_size);

  params = Params();
  engage_img = loadPixmap("../assets/img_chffr_wheel.png", {img_size, img_size});
  experimental_img = loadPixmap("../assets/img_experimental.svg", {img_size, img_size});
  QObject::connect(this, &QPushButton::clicked, this, &ExperimentalButton::changeMode);

  // Custom steering wheel images
  wheel_images = {
    {0, loadPixmap("../assets/img_chffr_wheel.png", {img_size, img_size})},
    {1, loadPixmap("../assets/lexus.png", {img_size, img_size})},
    {2, loadPixmap("../assets/toyota.png", {img_size, img_size})},
    {3, loadPixmap("../assets/frog.png", {img_size, img_size})},
    {4, loadPixmap("../assets/rocket.png", {img_size, img_size})}
  };
}

void ExperimentalButton::changeMode() {
  if (params.getBool("ExperimentalModeConfirmed")) {
    params.putBool("ExperimentalMode", !experimental_mode);
  }
}

void ExperimentalButton::updateState(const UIState &s) {
  const auto cs = (*s.sm)["controlsState"].getControlsState();
  bool eng = cs.getEngageable() || cs.getEnabled();
  if ((cs.getExperimentalMode() != experimental_mode) || (eng != engageable)) {
    engageable = eng;
    experimental_mode = cs.getExperimentalMode();
    update();
  }

  // FrogPilot properties
  setProperty("steeringWheel", s.scene.steering_wheel);
}

void ExperimentalButton::paintEvent(QPaintEvent *event) {
  // If the rotating steering wheel toggle is on hide the icon
  static auto &scene = uiState()->scene;
  if (!scene.rotating_wheel) {
    // Custom steering wheel icon
    engage_img = wheel_images[steeringWheel];

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPoint center(btn_size / 2, btn_size / 2);
    QPixmap img = steeringWheel ? engage_img : experimental_mode ? experimental_img : engage_img;

    p.setOpacity(1.0);
    p.setPen(Qt::NoPen);
    p.setBrush(scene.always_on_lateral_active ? QColor(10, 186, 181, 255) : scene.conditional_status == 1 ? QColor(255, 246, 0, 255) : steeringWheel && experimental_mode ? QColor(218, 111, 37, 241) : QColor(0, 0, 0, 166));
    p.drawEllipse(center, btn_size / 2, btn_size / 2);
    p.setOpacity((isDown() || !engageable) ? 0.6 : 1.0);
    p.drawPixmap((btn_size - img_size) / 2, (btn_size - img_size) / 2, img);
  }
}


// Window that shows camera view and variety of info drawn on top
AnnotatedCameraWidget::AnnotatedCameraWidget(VisionStreamType type, QWidget* parent) : fps_filter(UI_FREQ, 3, 1. / UI_FREQ), CameraWidget("camerad", type, true, parent) {
  pm = std::make_unique<PubMaster, const std::initializer_list<const char *>>({"uiDebug"});

  QVBoxLayout *main_layout  = new QVBoxLayout(this);
  main_layout->setMargin(UI_BORDER_SIZE);
  main_layout->setSpacing(0);

  experimental_btn = new ExperimentalButton(this);
  main_layout->addWidget(experimental_btn, 0, Qt::AlignTop | Qt::AlignRight);

  dm_img = loadPixmap("../assets/img_driver_face.png", {img_size + 5, img_size + 5});

  // FrogPilot variable checks
  static auto params = Params();
  if (params.getBool("HideSpeed")) {
    speedVisible = false;
  }

  if (params.getBool("ReverseCruiseIncrease")) {
    reverseCruiseIncrease = true;
  }

  // FrogPilot images
  compass_inner_img = loadPixmap("../assets/images/compass_inner.png", {img_size, img_size});
  engage_img = loadPixmap("../assets/img_chffr_wheel.png", {img_size, img_size});
  experimental_img = loadPixmap("../assets/img_experimental.svg", {img_size, img_size});

  // Custom steering wheel images
  wheel_images = {
    {0, loadPixmap("../assets/img_chffr_wheel.png", {img_size, img_size})},
    {1, loadPixmap("../assets/lexus.png", {img_size, img_size})},
    {2, loadPixmap("../assets/toyota.png", {img_size, img_size})},
    {3, loadPixmap("../assets/frog.png", {img_size, img_size})},
    {4, loadPixmap("../assets/rocket.png", {img_size, img_size})}
  };

  // Driving personalities profiles
  profile_data = {
    {QPixmap("../assets/aggressive.png"), "Aggressive"},
    {QPixmap("../assets/standard.png"), "Standard"},
    {QPixmap("../assets/relaxed.png"), "Relaxed"}
  };

  // Turn signal images
  const QStringList imagePaths = {
    "../assets/images/frog_turn_signal_1.png",
    "../assets/images/frog_turn_signal_2.png",
    "../assets/images/frog_turn_signal_3.png",
    "../assets/images/frog_turn_signal_4.png"
  };
  signalImgVector.reserve(2 * imagePaths.size() + 1);
  for (int i = 0; i < 2; ++i) {
    for (const QString& path : imagePaths) {
      signalImgVector.push_back(QPixmap(path));
    }
  }
  // Add the blindspot signal image to the vector
  signalImgVector.push_back(QPixmap("../assets/images/frog_turn_signal_1_red.png"));

  // Initialize the timer for the turn signal animation
  auto animationTimer = new QTimer(this);
  connect(animationTimer, &QTimer::timeout, this, [this] {
    animationFrameIndex = (animationFrameIndex + 1) % totalFrames;
    update();
  });
  animationTimer->start(totalFrames * 11); // 50 milliseconds per frame; syncs up perfectly with my 2019 Lexus ES 350 turn signal clicks
}

void AnnotatedCameraWidget::updateState(const UIState &s) {
  const int SET_SPEED_NA = 255;
  const SubMaster &sm = *(s.sm);

  const bool cs_alive = sm.alive("controlsState");
  const bool nav_alive = sm.alive("navInstruction") && sm["navInstruction"].getValid();

  const auto cs = sm["controlsState"].getControlsState();

  // Handle older routes where vCruiseCluster is not set
  float v_cruise =  cs.getVCruiseCluster() == 0.0 ? cs.getVCruise() : cs.getVCruiseCluster();
  float set_speed = cs_alive ? v_cruise : SET_SPEED_NA;
  bool cruise_set = set_speed > 0 && (int)set_speed != SET_SPEED_NA;
  if (cruise_set && !s.scene.is_metric) {
    set_speed *= KM_TO_MILE;
  }

  // Handle older routes where vEgoCluster is not set
  float v_ego;
  if (sm["carState"].getCarState().getVEgoCluster() == 0.0 && !v_ego_cluster_seen) {
    v_ego = sm["carState"].getCarState().getVEgo();
  } else {
    v_ego = sm["carState"].getCarState().getVEgoCluster();
    v_ego_cluster_seen = true;
  }
  float cur_speed = cs_alive ? std::max<float>(0.0, v_ego) : 0.0;
  cur_speed *= s.scene.is_metric ? MS_TO_KPH : MS_TO_MPH;

  auto speed_limit_sign = sm["navInstruction"].getNavInstruction().getSpeedLimitSign();
  float speed_limit = nav_alive ? sm["navInstruction"].getNavInstruction().getSpeedLimit() : 0.0;
  speed_limit *= (s.scene.is_metric ? MS_TO_KPH : MS_TO_MPH);

  setProperty("speedLimit", speed_limit);
  setProperty("has_us_speed_limit", nav_alive && speed_limit_sign == cereal::NavInstruction::SpeedLimitSign::MUTCD);
  setProperty("has_eu_speed_limit", nav_alive && speed_limit_sign == cereal::NavInstruction::SpeedLimitSign::VIENNA);

  setProperty("is_cruise_set", cruise_set);
  setProperty("is_metric", s.scene.is_metric);
  setProperty("speed", cur_speed);
  setProperty("setSpeed", set_speed);
  setProperty("speedUnit", s.scene.is_metric ? tr("km/h") : tr("mph"));
  setProperty("hideDM", (cs.getAlertSize() != cereal::ControlsState::AlertSize::NONE));
  setProperty("status", s.status);

  // update engageability/experimental mode button
  experimental_btn->updateState(s);

  // update DM icon
  auto dm_state = sm["driverMonitoringState"].getDriverMonitoringState();
  setProperty("dmActive", dm_state.getIsActiveMode());
  setProperty("rightHandDM", dm_state.getIsRHD());
  // DM icon transition
  dm_fade_state = std::clamp(dm_fade_state+0.2*(0.5-dmActive), 0.0, 1.0);

  // FrogPilot properties
  setProperty("alwaysOnLateral", s.scene.always_on_lateral_active);
  setProperty("bearingDeg", s.scene.bearing_deg);
  setProperty("blindSpotLeft", s.scene.blind_spot_left);
  setProperty("blindSpotRight", s.scene.blind_spot_right);
  setProperty("compass", s.scene.compass);
  setProperty("conditionalExperimental", s.scene.conditional_experimental);
  setProperty("conditionalSpeed", s.scene.conditional_speed);
  setProperty("conditionalSpeedLead", s.scene.conditional_speed_lead);
  setProperty("conditionalStatus", s.scene.conditional_status);
  setProperty("desiredFollow", s.scene.desired_follow);
  setProperty("developerUI", s.scene.developer_ui);
  setProperty("experimentalMode", s.scene.experimental_mode);
  setProperty("frogColors", s.scene.frog_colors);
  setProperty("frogSignals", s.scene.frog_signals);
  setProperty("muteDM", s.scene.mute_dm);
  setProperty("obstacleDistance", s.scene.obstacle_distance);
  setProperty("onroadAdjustableProfiles", s.scene.driving_personalities_ui_wheel && !s.scene.toyota_car);
  setProperty("rotatingWheel", s.scene.rotating_wheel);
  setProperty("steeringAngleDeg", s.scene.steering_angle_deg);
  setProperty("steeringWheel", s.scene.steering_wheel);
  setProperty("stoppedEquivalence", s.scene.stopped_equivalence);
  setProperty("toyotaCar", s.scene.toyota_car);
  setProperty("turnSignalLeft", s.scene.turn_signal_left);
  setProperty("turnSignalRight", s.scene.turn_signal_right);
}

void AnnotatedCameraWidget::drawHud(QPainter &p) {
  p.save();

  // Header gradient
  QLinearGradient bg(0, UI_HEADER_HEIGHT - (UI_HEADER_HEIGHT / 2.5), 0, UI_HEADER_HEIGHT);
  bg.setColorAt(0, QColor::fromRgbF(0, 0, 0, 0.45));
  bg.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0));
  p.fillRect(0, 0, width(), UI_HEADER_HEIGHT, bg);

  QString speedLimitStr = (speedLimit > 1) ? QString::number(std::nearbyint(speedLimit)) : "–";
  QString speedStr = QString::number(std::nearbyint(speed));
  QString setSpeedStr = is_cruise_set ? QString::number(std::nearbyint(setSpeed)) : "–";

  // Draw outer box + border to contain set speed and speed limit
  const int sign_margin = 12;
  const int us_sign_height = 186;
  const int eu_sign_size = 176;

  const QSize default_size = {172, 204};
  QSize set_speed_size = default_size;
  if (is_metric || has_eu_speed_limit) set_speed_size.rwidth() = 200;
  if (has_us_speed_limit && speedLimitStr.size() >= 3) set_speed_size.rwidth() = 223;

  if (has_us_speed_limit) set_speed_size.rheight() += us_sign_height + sign_margin;
  else if (has_eu_speed_limit) set_speed_size.rheight() += eu_sign_size + sign_margin;

  int top_radius = 32;
  int bottom_radius = has_eu_speed_limit ? 100 : 32;

  QRect set_speed_rect(QPoint(60 + (default_size.width() - set_speed_size.width()) / 2, 45), set_speed_size);
  p.setPen(QPen(whiteColor(75), 6));
  if (reverseCruiseIncrease) {
    p.setPen(QPen(QColor(0, 150, 255), 6));
  }
  p.setBrush(blackColor(166));
  drawRoundedRect(p, set_speed_rect, top_radius, top_radius, bottom_radius, bottom_radius);

  // Draw MAX
  QColor max_color = QColor(0x80, 0xd8, 0xa6, 0xff);
  QColor set_speed_color = whiteColor();
  if (is_cruise_set) {
    if (status == STATUS_DISENGAGED) {
      max_color = whiteColor();
    } else if (status == STATUS_OVERRIDE) {
      max_color = QColor(0x91, 0x9b, 0x95, 0xff);
    } else if (speedLimit > 0) {
      auto interp_color = [=](QColor c1, QColor c2, QColor c3) {
        return speedLimit > 0 ? interpColor(setSpeed, {speedLimit + 5, speedLimit + 15, speedLimit + 25}, {c1, c2, c3}) : c1;
      };
      max_color = interp_color(max_color, QColor(0xff, 0xe4, 0xbf), QColor(0xff, 0xbf, 0xbf));
      set_speed_color = interp_color(set_speed_color, QColor(0xff, 0x95, 0x00), QColor(0xff, 0x00, 0x00));
    }
  } else {
    max_color = QColor(0xa6, 0xa6, 0xa6, 0xff);
    set_speed_color = QColor(0x72, 0x72, 0x72, 0xff);
  }
  p.setFont(InterFont(40, QFont::DemiBold));
  p.setPen(max_color);
  p.drawText(set_speed_rect.adjusted(0, 27, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("MAX"));
  p.setFont(InterFont(90, QFont::Bold));
  p.setPen(set_speed_color);
  p.drawText(set_speed_rect.adjusted(0, 77, 0, 0), Qt::AlignTop | Qt::AlignHCenter, setSpeedStr);

  const QRect sign_rect = set_speed_rect.adjusted(sign_margin, default_size.height(), -sign_margin, -sign_margin);
  // US/Canada (MUTCD style) sign
  if (has_us_speed_limit) {
    p.setPen(Qt::NoPen);
    p.setBrush(whiteColor());
    p.drawRoundedRect(sign_rect, 24, 24);
    p.setPen(QPen(blackColor(), 6));
    p.drawRoundedRect(sign_rect.adjusted(9, 9, -9, -9), 16, 16);

    p.setFont(InterFont(28, QFont::DemiBold));
    p.drawText(sign_rect.adjusted(0, 22, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("SPEED"));
    p.drawText(sign_rect.adjusted(0, 51, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("LIMIT"));
    p.setFont(InterFont(70, QFont::Bold));
    p.drawText(sign_rect.adjusted(0, 85, 0, 0), Qt::AlignTop | Qt::AlignHCenter, speedLimitStr);
  }

  // EU (Vienna style) sign
  if (has_eu_speed_limit) {
    p.setPen(Qt::NoPen);
    p.setBrush(whiteColor());
    p.drawEllipse(sign_rect);
    p.setPen(QPen(Qt::red, 20));
    p.drawEllipse(sign_rect.adjusted(16, 16, -16, -16));

    p.setFont(InterFont((speedLimitStr.size() >= 3) ? 60 : 70, QFont::Bold));
    p.setPen(blackColor());
    p.drawText(sign_rect, Qt::AlignCenter, speedLimitStr);
  }

  // current speed
  if (speedVisible) {
    p.setFont(InterFont(176, QFont::Bold));
    drawText(p, rect().center().x(), 210, speedStr);
    p.setFont(InterFont(66));
    drawText(p, rect().center().x(), 290, speedUnit, 200);
  }

  p.restore();

  // Driving personalities button - Hide the button when the turn signal animation is on
  if (onroadAdjustableProfiles && (!frogSignals || (frogSignals && !turnSignalLeft && !turnSignalRight))) {
    drawDrivingPersonalities(p);
  }

  // Compass - Hide the compass when the turn signal animation is on
  if (compass && (!frogSignals || (frogSignals && !turnSignalLeft && !turnSignalRight))) {
    drawCompass(p);
  }

  // Developer UI
  if (developerUI) {
    drawDeveloperUI(p);
  }

  // Frog turn signal animation
  if (frogSignals && (turnSignalLeft || turnSignalRight)) {
    drawFrogSignals(p);
  }

  // Rotating steering wheel
  if (rotatingWheel) {
    drawRotatingWheel(p, rect().right() - btn_size / 2 - UI_BORDER_SIZE * 2 + 25, btn_size / 2 + int(UI_BORDER_SIZE * 1.5) - 5);
  }

  // FrogPilot status bar
  if (conditionalExperimental || alwaysOnLateral) {
    drawStatusBar(p);
  }
}

void AnnotatedCameraWidget::drawText(QPainter &p, int x, int y, const QString &text, int alpha) {
  QRect real_rect = p.fontMetrics().boundingRect(text);
  real_rect.moveCenter({x, y - real_rect.height() / 2});

  p.setPen(QColor(0xff, 0xff, 0xff, alpha));
  p.drawText(real_rect.x(), real_rect.bottom(), text);
}

void AnnotatedCameraWidget::drawIcon(QPainter &p, int x, int y, QPixmap &img, QBrush bg, float opacity) {
  p.setOpacity(1.0);  // bg dictates opacity of ellipse
  p.setPen(Qt::NoPen);
  p.setBrush(bg);
  p.drawEllipse(x - btn_size / 2, y - btn_size / 2, btn_size, btn_size);
  p.setOpacity(opacity);
  p.drawPixmap(x - img.size().width() / 2, y - img.size().height() / 2, img);
  p.setOpacity(1.0);
}

void AnnotatedCameraWidget::initializeGL() {
  CameraWidget::initializeGL();
  qInfo() << "OpenGL version:" << QString((const char*)glGetString(GL_VERSION));
  qInfo() << "OpenGL vendor:" << QString((const char*)glGetString(GL_VENDOR));
  qInfo() << "OpenGL renderer:" << QString((const char*)glGetString(GL_RENDERER));
  qInfo() << "OpenGL language version:" << QString((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

  prev_draw_t = millis_since_boot();
  setBackgroundColor(bg_colors[STATUS_DISENGAGED]);
}

void AnnotatedCameraWidget::updateFrameMat() {
  CameraWidget::updateFrameMat();
  UIState *s = uiState();
  int w = width(), h = height();

  s->fb_w = w;
  s->fb_h = h;

  // Apply transformation such that video pixel coordinates match video
  // 1) Put (0, 0) in the middle of the video
  // 2) Apply same scaling as video
  // 3) Put (0, 0) in top left corner of video
  s->car_space_transform.reset();
  s->car_space_transform.translate(w / 2 - x_offset, h / 2 - y_offset)
      .scale(zoom, zoom)
      .translate(-intrinsic_matrix.v[2], -intrinsic_matrix.v[5]);
}

void AnnotatedCameraWidget::drawLaneLines(QPainter &painter, const UIState *s) {
  painter.save();

  const UIScene &scene = s->scene;
  SubMaster &sm = *(s->sm);

  // lanelines
  for (int i = 0; i < std::size(scene.lane_line_vertices); ++i) {
    painter.setBrush(QColor::fromRgbF(1.0, 1.0, 1.0, std::clamp<float>(scene.lane_line_probs[i], 0.0, 0.7)));
    if (frogColors) {
      painter.setBrush(QColor(23, 134, 68, 242));
    }
    painter.drawPolygon(scene.lane_line_vertices[i]);
  }

  // road edges
  for (int i = 0; i < std::size(scene.road_edge_vertices); ++i) {
    painter.setBrush(QColor::fromRgbF(1.0, 0, 0, std::clamp<float>(1.0 - scene.road_edge_stds[i], 0.0, 1.0)));
    if (frogColors) {
      painter.setBrush(QColor(23, 134, 68, 242));
    }
    painter.drawPolygon(scene.road_edge_vertices[i]);
  }

  // paint path
  const bool show_e2e_path = (sm["controlsState"].getControlsState().getExperimentalMode() &&
                              scene.longitudinal_control) || frogColors;
  QLinearGradient bg(0, height(), 0, 0);
  if (show_e2e_path) {
    // The first half of track_vertices are the points for the right side of the path
    // and the indices match the positions of accel from uiPlan
    const auto &acceleration_const = sm["uiPlan"].getUiPlan().getAccel();
    const int max_len = std::min<int>(scene.track_vertices.length() / 2, acceleration_const.size());

    // Copy of the acceleration vector for the "frogColors" path
    std::vector<float> acceleration;
    for (int i = 0; i < acceleration_const.size(); i++) {
      acceleration.push_back(acceleration_const[i]);
    }

    for (int i = 0; i < max_len; ++i) {
      // Some points are out of frame
      if (scene.track_vertices[i].y() < 0 || scene.track_vertices[i].y() > height()) continue;

      // Flip so 0 is bottom of frame
      float lin_grad_point = (height() - scene.track_vertices[i].y()) / height();

      // If acceleration is between -0.25 and 0.25 and frogColors is True, set acceleration to 2 to give it a consistent green color
      if (frogColors && acceleration[i] > -0.25 && acceleration[i] < 0.25) {
        acceleration[i] = 2;
      }

      // speed up: 120, slow down: 0
      float path_hue = fmax(fmin(60 + acceleration[i] * 35, 120), 0);
      // FIXME: painter.drawPolygon can be slow if hue is not rounded
      path_hue = int(path_hue * 100 + 0.5) / 100;

      float saturation = fmin(fabs(acceleration[i] * 1.5), 1);
      float lightness = util::map_val(saturation, 0.0f, 1.0f, 0.95f, 0.62f);  // lighter when grey
      float alpha = util::map_val(lin_grad_point, 0.75f / 2.f, 0.75f, 0.4f, 0.0f);  // matches previous alpha fade
      bg.setColorAt(lin_grad_point, QColor::fromHslF(path_hue / 360., saturation, lightness, alpha));

      // Skip a point, unless next is last
      i += (i + 2) < max_len ? 1 : 0;
    }

  } else {
    bg.setColorAt(0.0, QColor::fromHslF(148 / 360., 0.94, 0.51, 0.4));
    bg.setColorAt(0.5, QColor::fromHslF(112 / 360., 1.0, 0.68, 0.35));
    bg.setColorAt(1.0, QColor::fromHslF(112 / 360., 1.0, 0.68, 0.0));
  }

  painter.setBrush(bg);
  painter.drawPolygon(scene.track_vertices);

  // Create new path with track vertices and track edge vertices
  QPainterPath path;
  path.addPolygon(scene.track_vertices);
  path.addPolygon(scene.track_edge_vertices);

  // Paint path edges
  QLinearGradient pe(0, height(), 0, 0);
  if (alwaysOnLateral) {
    pe.setColorAt(0.0, QColor::fromHslF(178 / 360., 0.90, 0.38, 1.0));
    pe.setColorAt(0.5, QColor::fromHslF(178 / 360., 0.90, 0.38, 0.5));
    pe.setColorAt(1.0, QColor::fromHslF(178 / 360., 0.90, 0.38, 0.1));
  } else if (conditionalStatus == 1) {
    pe.setColorAt(0.0, QColor::fromHslF(58 / 360., 1.00, 0.50, 1.0));
    pe.setColorAt(0.5, QColor::fromHslF(58 / 360., 1.00, 0.50, 0.5));
    pe.setColorAt(1.0, QColor::fromHslF(58 / 360., 1.00, 0.50, 0.1));
  } else if (experimentalMode) {
    pe.setColorAt(0.0, QColor::fromHslF(25 / 360., 0.71, 0.50, 1.0));
    pe.setColorAt(0.5, QColor::fromHslF(25 / 360., 0.71, 0.50, 0.5));
    pe.setColorAt(1.0, QColor::fromHslF(25 / 360., 0.71, 0.50, 0.1));
  } else if (scene.navigate_on_openpilot) {
    pe.setColorAt(0.0, QColor::fromHslF(225 / 360., 0.75, 0.50, 1.0));
    pe.setColorAt(0.5, QColor::fromHslF(225 / 360., 0.75, 0.50, 0.5));
    pe.setColorAt(1.0, QColor::fromHslF(225 / 360., 0.75, 0.50, 0.1));
  } else if (frogColors) {
    pe.setColorAt(0.0, QColor::fromHslF(150 / 360., 0.75, 0.25, 1.0));
    pe.setColorAt(0.5, QColor::fromHslF(150 / 360., 0.75, 0.25, 0.5));
    pe.setColorAt(1.0, QColor::fromHslF(150 / 360., 0.75, 0.25, 0.1));
  } else {
    pe.setColorAt(0.0, QColor::fromHslF(148 / 360., 0.94, 0.51, 1.0));
    pe.setColorAt(0.5, QColor::fromHslF(112 / 360., 1.00, 0.68, 0.5));
    pe.setColorAt(1.0, QColor::fromHslF(112 / 360., 1.00, 0.68, 0.1));
  }

  painter.setBrush(pe);
  painter.drawPath(path);

  // Paint blindspot path
  QLinearGradient bs(0, height(), 0, 0);
  if (blindSpotLeft || blindSpotRight) {
    bs.setColorAt(0.0, QColor::fromHslF(0 / 360., 0.75, 0.50, 0.6));
    bs.setColorAt(0.5, QColor::fromHslF(0 / 360., 0.75, 0.50, 0.4));
    bs.setColorAt(1.0, QColor::fromHslF(0 / 360., 0.75, 0.50, 0.2));
  }

  painter.setBrush(bs);
  if (blindSpotLeft) {
    painter.drawPolygon(scene.track_left_blindspot_vertices);
  }
  if (blindSpotRight) {
    painter.drawPolygon(scene.track_right_blindspot_vertices);
  }

  painter.restore();
}

void AnnotatedCameraWidget::drawDriverState(QPainter &painter, const UIState *s) {
  const UIScene &scene = s->scene;

  painter.save();

  // base icon
  int offset = UI_BORDER_SIZE + btn_size / 2;
  int x = rightHandDM ? width() - offset : offset;
  int y = height() - offset - (conditionalExperimental || alwaysOnLateral ? 25 : 0);
  float opacity = dmActive ? 0.65 : 0.2;
  drawIcon(painter, x, y, dm_img, blackColor(70), opacity);

  // face
  QPointF face_kpts_draw[std::size(default_face_kpts_3d)];
  float kp;
  for (int i = 0; i < std::size(default_face_kpts_3d); ++i) {
    kp = (scene.face_kpts_draw[i].v[2] - 8) / 120 + 1.0;
    face_kpts_draw[i] = QPointF(scene.face_kpts_draw[i].v[0] * kp + x, scene.face_kpts_draw[i].v[1] * kp + y);
  }

  painter.setPen(QPen(QColor::fromRgbF(1.0, 1.0, 1.0, opacity), 5.2, Qt::SolidLine, Qt::RoundCap));
  painter.drawPolyline(face_kpts_draw, std::size(default_face_kpts_3d));

  // tracking arcs
  const int arc_l = 133;
  const float arc_t_default = 6.7;
  const float arc_t_extend = 12.0;
  QColor arc_color = QColor::fromRgbF(0.545 - 0.445 * s->engaged(),
                                      0.545 + 0.4 * s->engaged(),
                                      0.545 - 0.285 * s->engaged(),
                                      0.4 * (1.0 - dm_fade_state));
  float delta_x = -scene.driver_pose_sins[1] * arc_l / 2;
  float delta_y = -scene.driver_pose_sins[0] * arc_l / 2;
  painter.setPen(QPen(arc_color, arc_t_default+arc_t_extend*fmin(1.0, scene.driver_pose_diff[1] * 5.0), Qt::SolidLine, Qt::RoundCap));
  painter.drawArc(QRectF(std::fmin(x + delta_x, x), y - arc_l / 2, fabs(delta_x), arc_l), (scene.driver_pose_sins[1]>0 ? 90 : -90) * 16, 180 * 16);
  painter.setPen(QPen(arc_color, arc_t_default+arc_t_extend*fmin(1.0, scene.driver_pose_diff[0] * 5.0), Qt::SolidLine, Qt::RoundCap));
  painter.drawArc(QRectF(x - arc_l / 2, std::fmin(y + delta_y, y), arc_l, fabs(delta_y)), (scene.driver_pose_sins[0]>0 ? 0 : 180) * 16, 180 * 16);

  painter.restore();
}

void AnnotatedCameraWidget::drawLead(QPainter &painter, const cereal::RadarState::LeadData::Reader &lead_data, const QPointF &vd) {
  painter.save();

  const float speedBuff = frogColors ? 25. : 10.; // The center of the chevron appears sooner when frogColors is "true"
  const float leadBuff = frogColors ? 100. : 40.; // The center of the chevron appears sooner when frogColors is "true"
  const float d_rel = lead_data.getDRel();
  const float v_rel = lead_data.getVRel();

  float fillAlpha = 0;
  if (d_rel < leadBuff) {
    fillAlpha = 255 * (1.0 - (d_rel / leadBuff));
    if (v_rel < 0) {
      fillAlpha += 255 * (-1 * (v_rel / speedBuff));
    }
    fillAlpha = (int)(fmin(fillAlpha, 255));
  }

  float sz = std::clamp((25 * 30) / (d_rel / 3 + 30), 15.0f, 30.0f) * 2.35;
  float x = std::clamp((float)vd.x(), 0.f, width() - sz / 2);
  float y = std::fmin(height() - sz * .6, (float)vd.y());

  float g_xo = sz / 5;
  float g_yo = sz / 10;

  QPointF glow[] = {{x + (sz * 1.35) + g_xo, y + sz + g_yo}, {x, y - g_yo}, {x - (sz * 1.35) - g_xo, y + sz + g_yo}};
  painter.setBrush(QColor(218, 202, 37, 255));
  painter.drawPolygon(glow, std::size(glow));

  // chevron
  QPointF chevron[] = {{x + (sz * 1.25), y + sz}, {x, y}, {x - (sz * 1.25), y + sz}};
  painter.setBrush(redColor(fillAlpha));
  if (frogColors) {
    painter.setBrush(frogColor(fillAlpha));
  }
  painter.drawPolygon(chevron, std::size(chevron));

  // Add developer UI if enabled
  if (developerUI) {
    // Ensure vLead speed doesn't go under 0 m/s and round it to 2 decimals
    const float v_lead = std::round(std::max(lead_data.getVLead(), 0.0f) * 100) / 100;

    // Declare the variables
    static float distance = d_rel;
    static float speed = v_lead;
    static QString unit_d = "meters";
    static QString unit_s = "m/s";

    // Conduct any necessary conversions
    if (developerUI == 1) {
      // Convert to US imperial
      distance = d_rel * 3.28084;
      speed = v_lead * 2.23694;
      unit_d = "feet"; 
      unit_s = "mph";
    } else if (developerUI == 2) {
      // Convert to metric
      speed = v_lead * 3.6;
      unit_s = "km/h";
    }

    // Form the combined_text
    QString combined_text = QString::number(distance, 'f', 2) + " " + unit_d + " | " + QString::number(speed, 'f', 2) + " " + unit_s;
    painter.setPen(Qt::white);
    painter.setFont(InterFont(35, QFont::Bold));

    // Calculate the start position for drawing
    const QFontMetrics metrics(painter.font());
    const int middle_x = (chevron[2].x() + chevron[0].x()) / 2;
    painter.drawText(middle_x - metrics.horizontalAdvance(combined_text) / 2, chevron[0].y() + metrics.height() + 5, combined_text);
  }

  painter.restore();
}

void AnnotatedCameraWidget::paintGL() {
  UIState *s = uiState();
  SubMaster &sm = *(s->sm);
  const double start_draw_t = millis_since_boot();
  const cereal::ModelDataV2::Reader &model = sm["modelV2"].getModelV2();
  const cereal::RadarState::Reader &radar_state = sm["radarState"].getRadarState();

  // draw camera frame
  {
    std::lock_guard lk(frame_lock);

    if (frames.empty()) {
      if (skip_frame_count > 0) {
        skip_frame_count--;
        qDebug() << "skipping frame, not ready";
        return;
      }
    } else {
      // skip drawing up to this many frames if we're
      // missing camera frames. this smooths out the
      // transitions from the narrow and wide cameras
      skip_frame_count = 5;
    }

    // Wide or narrow cam dependent on speed
    bool has_wide_cam = available_streams.count(VISION_STREAM_WIDE_ROAD) && !s->scene.wide_camera_disabled;
    if (has_wide_cam) {
      float v_ego = sm["carState"].getCarState().getVEgo();
      if ((v_ego < 10) || available_streams.size() == 1) {
        wide_cam_requested = true;
      } else if (v_ego > 15) {
        wide_cam_requested = false;
      }
      wide_cam_requested = wide_cam_requested && sm["controlsState"].getControlsState().getExperimentalMode();
      // for replay of old routes, never go to widecam
      wide_cam_requested = wide_cam_requested && s->scene.calibration_wide_valid;
    }
    CameraWidget::setStreamType(wide_cam_requested ? VISION_STREAM_WIDE_ROAD : VISION_STREAM_ROAD);

    s->scene.wide_cam = CameraWidget::getStreamType() == VISION_STREAM_WIDE_ROAD;
    if (s->scene.calibration_valid) {
      auto calib = s->scene.wide_cam ? s->scene.view_from_wide_calib : s->scene.view_from_calib;
      CameraWidget::updateCalibration(calib);
    } else {
      CameraWidget::updateCalibration(DEFAULT_CALIBRATION);
    }
    CameraWidget::setFrameId(model.getFrameId());
    CameraWidget::paintGL();
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(Qt::NoPen);

  if (s->worldObjectsVisible()) {
    if (sm.rcv_frame("modelV2") > s->scene.started_frame) {
      update_model(s, model, sm["uiPlan"].getUiPlan());
      if (sm.rcv_frame("radarState") > s->scene.started_frame) {
        update_leads(s, radar_state, model.getPosition());
      }
    }

    drawLaneLines(painter, s);

    if (s->scene.longitudinal_control) {
      auto lead_one = radar_state.getLeadOne();
      auto lead_two = radar_state.getLeadTwo();
      if (lead_one.getStatus()) {
        drawLead(painter, lead_one, s->scene.lead_vertices[0]);
      }
      if (lead_two.getStatus() && (std::abs(lead_one.getDRel() - lead_two.getDRel()) > 3.0)) {
        drawLead(painter, lead_two, s->scene.lead_vertices[1]);
      }
    }
  }

  // DMoji - Hide the icon when the turn signal animation is on
  if (!hideDM && (sm.rcv_frame("driverStateV2") > s->scene.started_frame) && !muteDM && (!frogSignals || (frogSignals && !turnSignalLeft && !turnSignalRight))) {
    update_dmonitoring(s, sm["driverStateV2"].getDriverStateV2(), dm_fade_state, rightHandDM);
    drawDriverState(painter, s);
  }

  drawHud(painter);

  double cur_draw_t = millis_since_boot();
  double dt = cur_draw_t - prev_draw_t;
  double fps = fps_filter.update(1. / dt * 1000);
  if (fps < 15) {
    LOGW("slow frame rate: %.2f fps", fps);
  }
  prev_draw_t = cur_draw_t;

  // publish debug msg
  MessageBuilder msg;
  auto m = msg.initEvent().initUiDebug();
  m.setDrawTimeMillis(cur_draw_t - start_draw_t);
  pm->send("uiDebug", msg);
}

void AnnotatedCameraWidget::showEvent(QShowEvent *event) {
  CameraWidget::showEvent(event);

  ui_update_params(uiState());
  prev_draw_t = millis_since_boot();
}

// FrogPilot widgets

void AnnotatedCameraWidget::drawCompass(QPainter &p) {
  p.save();

  // Variable declarations
  constexpr int circle_size = 250;
  constexpr int circle_offset = circle_size / 2;
  constexpr int degreeLabelOffset = circle_offset + 25;
  constexpr int inner_compass = btn_size / 2;
  int x = !rightHandDM ? rect().right() - btn_size / 2 - (UI_BORDER_SIZE * 2) - 10 : btn_size / 2 + (UI_BORDER_SIZE * 2) + 10;
  const int y = rect().bottom() - 20 - (conditionalExperimental || alwaysOnLateral ? 50 : 0) - 140;

  // Enable Antialiasing
  p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

  // Configure the circles
  p.setPen(QPen(Qt::white, 2));
  const auto drawCircle = [&](const int offset, const QBrush brush = Qt::NoBrush) {
    p.setOpacity(1.0);
    p.setBrush(brush);
    p.drawEllipse(x - offset, y - offset, offset * 2, offset * 2);
  };

  // Draw the circle background and white inner circle
  drawCircle(circle_offset, blackColor(100));

  // Rotate and draw the compass_inner_img image
  p.save();
  p.translate(x, y);
  p.rotate(bearingDeg);
  p.drawPixmap(-compass_inner_img.width() / 2, -compass_inner_img.height() / 2, compass_inner_img);
  p.restore();

  // Draw the cardinal directions
  const auto drawDirection = [&](const QString &text, const int from, const int to, const int align) {
    // Move the "E" and "W" directions a bit closer to the middle so they're more uniform
    const int offset = (text == "E") ? -5 : ((text == "W") ? 5 : 0);
    // Set the opacity based on whether the direction label is currently being pointed at
    p.setOpacity((bearingDeg >= from && bearingDeg < to) ? 1.0 : 0.2);
    p.drawText(QRect(x - inner_compass + offset, y - inner_compass, btn_size, btn_size), align, text);
  };
  p.setFont(InterFont(25, QFont::Bold));
  drawDirection("N", 0, 68, Qt::AlignTop | Qt::AlignHCenter);
  drawDirection("E", 23, 158, Qt::AlignRight | Qt::AlignVCenter);
  drawDirection("S", 113, 248, Qt::AlignBottom | Qt::AlignHCenter);
  drawDirection("W", 203, 338, Qt::AlignLeft | Qt::AlignVCenter);
  drawDirection("N", 293, 360, Qt::AlignTop | Qt::AlignHCenter);

  // Draw the white circle outlining the cardinal directions
  drawCircle(inner_compass + 5);

  // Draw the white circle outlining the bearing degrees
  drawCircle(degreeLabelOffset);

  // Draw the black background for the bearing degrees
  QPainterPath outerCircle, innerCircle;
  outerCircle.addEllipse(x - degreeLabelOffset, y - degreeLabelOffset, degreeLabelOffset * 2, degreeLabelOffset * 2);
  innerCircle.addEllipse(x - circle_offset, y - circle_offset, circle_size, circle_size);
  p.setOpacity(1.0);
  p.fillPath(outerCircle.subtracted(innerCircle), Qt::black);

  // Draw the degree lines and bearing degrees
  const auto drawCompassElements = [&](const int angle) {
    const bool isCardinalDirection = angle % 90 == 0;
    const int lineLength = isCardinalDirection ? 15 : 10;
    const bool isBold = abs(angle - static_cast<int>(bearingDeg)) <= 7;

    // Set the current bearing degree value to bold
    p.setFont(QFont("Inter", 8, isBold ? QFont::Bold : QFont::Normal));
    p.setPen(QPen(Qt::white, isCardinalDirection ? 3 : 1));

    // Place the elements in their respective spots around their circles
    p.save();
    p.translate(x, y);
    p.rotate(angle);
    p.drawLine(0, -(circle_size / 2 - lineLength), 0, -(circle_size / 2));
    p.translate(0, -(circle_size / 2 + 12));
    p.rotate(-angle);
    p.drawText(QRect(-20, -10, 40, 20), Qt::AlignCenter, QString::number(angle));
    p.restore();
  };

  for (int i = 0; i < 360; i += 15) {
    drawCompassElements(i);
  }

  p.restore();
}

void AnnotatedCameraWidget::drawDrivingPersonalities(QPainter &p) {
  p.save();

  // Declare the variables
  static QElapsedTimer timer;
  static bool displayText = false;
  static int lastProfile = -1;
  constexpr int fadeDuration = 1000; // 1 second
  constexpr int textDuration = 3000; // 3 seconds
  int x = rightHandDM ? rect().right() - (btn_size - 24) / 2 - (UI_BORDER_SIZE * 2) - (muteDM ? 50 : 250) : (btn_size - 24) / 2 + (UI_BORDER_SIZE * 2) + (muteDM ? 50 : 250);
  const int y = rect().bottom() - (conditionalExperimental || alwaysOnLateral ? 25 : 0) - 100;

  // Enable Antialiasing
  p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

  // Select the appropriate profile image/text
  int index = qBound(0, personalityProfile, 2);
  QPixmap &profile_image = profile_data[index].first;
  QString profile_text = profile_data[index].second;

  // Display the profile text when the user changes profiles
  if (lastProfile != personalityProfile) {
    displayText = true;
    lastProfile = personalityProfile;
    timer.restart();
  }

  // Set the text display
  displayText = !timer.hasExpired(textDuration);

  // Set the elapsed time since the profile switch
  int elapsed = timer.elapsed();

  // Calculate the opacity for the text and image based on the elapsed time
  qreal textOpacity = qBound(0.0, (1.0 - static_cast<qreal>(elapsed - textDuration) / fadeDuration), 1.0);
  qreal imageOpacity = qBound(0.0, (static_cast<qreal>(elapsed - textDuration) / fadeDuration), 1.0);

  // Draw the profile text with the calculated opacity
  if (textOpacity > 0.0) {
    p.setFont(InterFont(50, QFont::Bold));
    p.setPen(QColor(255, 255, 255));
    // Calculate the center position for text
    QFontMetrics fontMetrics(p.font());
    int textWidth = fontMetrics.horizontalAdvance(profile_text);
    // Apply opacity to the text
    p.setOpacity(textOpacity);
    p.drawText(x - textWidth / 2, y + fontMetrics.height() / 2, profile_text);
  }

  // Draw the profile image with the calculated opacity
  if (imageOpacity > 0.0) {
    drawIcon(p, x, y, profile_image, blackColor(0), imageOpacity);
  }

  p.restore();
}

void AnnotatedCameraWidget::drawDeveloperUI(QPainter &p) {
  // Declare the variables
  const SubMaster &sm = *uiState()->sm;
  const double currentAcceleration = std::round(sm["carState"].getCarState().getAEgo() * 100) / 100;
  static const std::vector<std::string> DistanceUnits = {"", " feet", " meters", " meters"};
  static const std::vector<std::string> NavDistanceUnits = {"", " ft", " m", " m"};
  static const std::vector<std::string> SpeedUnits = {"", " mph", " km/h", " m/s"};
  static const double DistanceConversions[] = {0, 3.28084, 1.0, 1.0};
  static const double SpeedConversions[] = {0, 2.23694, 3.6, 1.0};
  static double maxAcceleration = 0.0;
  static auto lastUpdated = std::chrono::steady_clock::now();

  // Update maxAcceleration
  if (currentAcceleration > maxAcceleration && status == STATUS_ENGAGED) {
    maxAcceleration = currentAcceleration;
    lastUpdated = std::chrono::steady_clock::now();
  }

  // Check if less than 5 seconds have passed since last max acceleration
  const auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastUpdated).count();

  // Configure the insights
  const auto createText = [&](const QString &title, double data){
    return title + QString::number(data * DistanceConversions[developerUI]) +
    (map_open ? QString::fromStdString(NavDistanceUnits[developerUI]) : QString::fromStdString(DistanceUnits[developerUI]));
  };

  // Create the insights
  const QString maxAccText = "Accel: " + QString::number((timeDiff > 5 ? currentAcceleration : maxAcceleration) * SpeedConversions[developerUI]) + QString::fromStdString(SpeedUnits[developerUI]) + 
                             (map_open ? "" : " - Max: " + QString::number(maxAcceleration * SpeedConversions[developerUI]) + QString::fromStdString(SpeedUnits[developerUI]));

  const QString insightsText = createText((map_open ? QString(" | Obstacle: ") : QString("  |  Obstacle Factor: ")), obstacleDistance) +
                               (map_open ? " - " : "  -  ") +
                               createText((map_open ? QString("Stop: ") : QString("Stop Factor: ")), stoppedEquivalence) +
                               " = " +
                               createText((map_open ? QString("Follow: ") : QString("Follow Distance: ")), desiredFollow);

  // Prepare and draw insights rectangle
  const QRect insightsRect(rect().left() - 1, rect().top() - 60, rect().width() + 2, 100);
  p.save();
  p.setBrush(QColor(0, 0, 0, 150));
  p.setOpacity(1.0);
  p.drawRoundedRect(insightsRect, 30, 30);

  // Set up and draw the text
  p.setFont(InterFont(30, QFont::DemiBold));
  p.setRenderHint(QPainter::TextAntialiasing);

  // Calculate the baseline to center the text vertically
  const QRect adjustedRect = insightsRect.adjusted(0, 27, 0, 27);
  const int totalTextWidth = p.fontMetrics().width(maxAccText + insightsText);
  const int textStartPos = adjustedRect.x() + (adjustedRect.width() - totalTextWidth) / 2;
  const int textHeight = p.fontMetrics().height();
  const int textBaseLine = adjustedRect.y() + (adjustedRect.height() + textHeight) / 2 - p.fontMetrics().descent();

  // Draw maxAccText with appropriate color
  p.setPen(timeDiff < 5 ? Qt::red : Qt::white);
  p.drawText(textStartPos, textBaseLine, maxAccText);

  // Draw insightsText in white color
  p.setPen(Qt::white);
  p.drawText(textStartPos + p.fontMetrics().width(maxAccText), textBaseLine, insightsText);

  p.restore();
}

void AnnotatedCameraWidget::drawFrogSignals(QPainter &p) {
  // Declare the turn signal size
  constexpr int signalHeight = 480;
  constexpr int signalWidth = 360;

  // Calculate the vertical position for the turn signals
  const int baseYPosition = (height() - signalHeight) / 2 + (conditionalExperimental || alwaysOnLateral ? 225 : 300);
  // Calculate the x-coordinates for the turn signals
  int leftSignalXPosition = 75 + width() - signalWidth - 300 * (blindSpotLeft ? 0 : animationFrameIndex);
  int rightSignalXPosition = -75 + 300 * (blindSpotRight ? 0 : animationFrameIndex);

  // Enable Antialiasing
  p.setRenderHint(QPainter::Antialiasing);

  // Draw the turn signals
  if (animationFrameIndex < static_cast<int>(signalImgVector.size())) {
    const auto drawSignal = [&](const bool signalActivated, const int xPosition, const bool flip, const bool blindspot) {
      if (signalActivated) {
        // Get the appropriate image from the signalImgVector
        QPixmap signal = signalImgVector[(blindspot ? signalImgVector.size()-1 : animationFrameIndex % totalFrames)].transformed(QTransform().scale(flip ? -1 : 1, 1));
        // Draw the image
        p.drawPixmap(xPosition, baseYPosition, signalWidth, signalHeight, signal);
      }
    };

    // Display the animation based on which signal is activated
    drawSignal(turnSignalLeft, leftSignalXPosition, false, blindSpotLeft);
    drawSignal(turnSignalRight, rightSignalXPosition, true, blindSpotRight);
  }
}

void AnnotatedCameraWidget::drawRotatingWheel(QPainter &p, int x, int y) {
  // Variable declarations
  const auto &scene = uiState()->scene;

  // Custom steering wheel icon
  engage_img = wheel_images[steeringWheel];

  // Enable Antialiasing
  p.setRenderHint(QPainter::Antialiasing);

  // Set the icon according to the current status of "Experimental Mode"
  QPixmap img = steeringWheel ? engage_img : (experimentalMode ? experimental_img : engage_img);

  // Draw the icon and rotate it alongside the steering wheel
  p.setOpacity(1.0);
  p.setPen(Qt::NoPen);
  p.setBrush(alwaysOnLateral ? QColor(10, 186, 181, 255) : conditionalStatus == 1 ? QColor(255, 246, 0, 255) : steeringWheel && experimentalMode ? QColor(218, 111, 37, 241) : scene.navigate_on_openpilot ? QColor(32, 80, 223, 255) : QColor(0, 0, 0, 166));
  p.drawEllipse(x - btn_size / 2, y - btn_size / 2, btn_size, btn_size);
  p.save();
  p.translate(x, y);
  p.rotate(-steeringAngleDeg);
  p.drawPixmap(-img.size().width() / 2, -img.size().height() / 2, img);
  p.restore();
}

void AnnotatedCameraWidget::drawStatusBar(QPainter &p) {
  p.save();

  // List all of the Conditional Experimental Mode statuses
  const QMap<int, QString> conditionalStatusMap = {
    {1, "Conditional Experimental overridden"},
    {2, "Experimental Mode manually activated"},
    {3, "Experimental Mode activated for turn" + (map_open ? "" : QString(" / lane change"))},
    {4, "Experimental Mode activated for stop" + (map_open ? "" : QString(" sign / stop light"))},
    {5, "Experimental Mode activated for curve"},
    {6, "Experimental Mode activated due to" + (map_open ? " speed" : " speed being less than " + QString::number(conditionalSpeed) + " mph")},
    {7, "Experimental Mode activated due to" + (map_open ? " speed" : " speed being less than " + QString::number(conditionalSpeedLead) + " mph")},
    {8, "Conditional Experimental Mode ready"}
  };

  // Display the appropriate status
  static QString statusText;
  const QString wheelSuffix = toyotaCar ? ". Double press the \"LKAS\" button to revert" : ". Double tap the screen to revert";
  if (alwaysOnLateral) {
    statusText = QString("Always On Lateral active") + (map_open ? "" : QString(". Press the \"Cruise Control\" button to disable"));
  } else if (conditionalExperimental) {
    statusText = conditionalStatusMap.contains(conditionalStatus) && status != STATUS_DISENGAGED ? conditionalStatusMap[conditionalStatus] : conditionalStatusMap[8];
  }
  // Add the appropriate suffix if always on lateral isn't active and the map isn't being shown
  if ((conditionalStatus == 1 || conditionalStatus == 2) && !alwaysOnLateral && !map_open && status != STATUS_DISENGAGED && !statusText.isEmpty()) {
    statusText += wheelSuffix;
  }

  // Push down the bar below the edges of the screen to give it a cleaner look
  const QRect statusBarRect(rect().left() - 1, rect().bottom() - 50, rect().width() + 2, 100);
  p.setBrush(QColor(0, 0, 0, 150));
  p.setOpacity(1.0);
  p.drawRoundedRect(statusBarRect, 30, 30);

  // Draw the text
  p.setFont(InterFont(40, QFont::Bold));
  p.setPen(Qt::white);
  // Enable Antialiasing
  p.setRenderHint(QPainter::TextAntialiasing);
  // Calculate textRect size
  QRect textRect = p.fontMetrics().boundingRect(statusBarRect, Qt::AlignCenter | Qt::TextWordWrap, statusText);
  // Move text up 50 pixels to hide the bottom rounded edges to give it a cleaner look
  textRect.moveBottom(statusBarRect.bottom() - 50);
  // Draw the text centered in the calculated textRect
  p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, statusText);

  p.restore();
}
