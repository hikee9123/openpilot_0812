#include "selfdrive/ui/ui.h"

#include <unistd.h>

#include <cassert>
#include <cmath>
#include <cstdio>

#include "selfdrive/common/util.h"
#include "selfdrive/common/watchdog.h"
#include "selfdrive/hardware/hw.h"

#define BACKLIGHT_DT 0.05
#define BACKLIGHT_TS 10.00
#define BACKLIGHT_OFFROAD 50


// Projects a point in car to space to the corresponding point in full frame
// image space.
static bool calib_frame_to_full_frame(const UIState *s, float in_x, float in_y, float in_z, vertex_data *out) {
  const float margin = 500.0f;
  const QRectF clip_region{-margin, -margin, s->fb_w + 2 * margin, s->fb_h + 2 * margin};

  const vec3 pt = (vec3){{in_x, in_y, in_z}};
  const vec3 Ep = matvecmul3(s->scene.view_from_calib, pt);
  const vec3 KEp = matvecmul3(s->wide_camera ? ecam_intrinsic_matrix : fcam_intrinsic_matrix, Ep);

  // Project.
  QPointF point = s->car_space_transform.map(QPointF{KEp.v[0] / KEp.v[2], KEp.v[1] / KEp.v[2]});
  if (clip_region.contains(point)) {
    out->x = point.x();
    out->y = point.y();
    return true;
  }
  return false;
}

static int get_path_length_idx(const cereal::ModelDataV2::XYZTData::Reader &line, const float path_height) {
  const auto line_x = line.getX();
  int max_idx = 0;
  for (int i = 0; i < TRAJECTORY_SIZE && line_x[i] < path_height; ++i) {
    max_idx = i;
  }
  return max_idx;
}

static void update_leads(UIState *s, const cereal::RadarState::Reader &radar_state, std::optional<cereal::ModelDataV2::XYZTData::Reader> line) {
  for (int i = 0; i < 2; ++i) {
    auto lead_data = (i == 0) ? radar_state.getLeadOne() : radar_state.getLeadTwo();
    if (lead_data.getStatus()) {
      float z = line ? (*line).getZ()[get_path_length_idx(*line, lead_data.getDRel())] : 0.0;
      calib_frame_to_full_frame(s, lead_data.getDRel(), -lead_data.getYRel(), z + 1.22, &s->scene.lead_vertices[i]);
    }
   // s->scene.lead_data[i] = lead_data;
  }
}

static void update_line_data(const UIState *s, const cereal::ModelDataV2::XYZTData::Reader &line,
                             float y_off, float z_off, line_vertices_data *pvd, int max_idx) {
  const auto line_x = line.getX(), line_y = line.getY(), line_z = line.getZ();
  vertex_data *v = &pvd->v[0];
  for (int i = 0; i <= max_idx; i++) {
    v += calib_frame_to_full_frame(s, line_x[i], line_y[i] - y_off, line_z[i] + z_off, v);
  }
  for (int i = max_idx; i >= 0; i--) {
    v += calib_frame_to_full_frame(s, line_x[i], line_y[i] + y_off, line_z[i] + z_off, v);
  }
  pvd->cnt = v - pvd->v;
  assert(pvd->cnt <= std::size(pvd->v));
}

static void update_model(UIState *s, const cereal::ModelDataV2::Reader &model) {
  UIScene &scene = s->scene;
  auto model_position = model.getPosition();
  float max_distance = std::clamp(model_position.getX()[TRAJECTORY_SIZE - 1],
                                  MIN_DRAW_DISTANCE, MAX_DRAW_DISTANCE);

  // update lane lines
  const auto lane_lines = model.getLaneLines();
  const auto lane_line_probs = model.getLaneLineProbs();
  int max_idx = get_path_length_idx(lane_lines[0], max_distance);
  for (int i = 0; i < std::size(scene.lane_line_vertices); i++) {
    scene.lane_line_probs[i] = lane_line_probs[i];
    update_line_data(s, lane_lines[i], 0.025 * scene.lane_line_probs[i], 0, &scene.lane_line_vertices[i], max_idx);
  }

  // update road edges
  const auto road_edges = model.getRoadEdges();
  const auto road_edge_stds = model.getRoadEdgeStds();
  for (int i = 0; i < std::size(scene.road_edge_vertices); i++) {
    scene.road_edge_stds[i] = road_edge_stds[i];
    update_line_data(s, road_edges[i], 0.025, 0, &scene.road_edge_vertices[i], max_idx);
  }

  // update path
  auto lead_one = (*s->sm)["radarState"].getRadarState().getLeadOne();
  if (lead_one.getStatus()) {
    const float lead_d = lead_one.getDRel() * 2.;
    max_distance = std::clamp((float)(lead_d - fmin(lead_d * 0.35, 10.)), 0.0f, max_distance);
  }
  max_idx = get_path_length_idx(model_position, max_distance);
  update_line_data(s, model_position, 0.5, 1.22, &scene.track_vertices, max_idx);
}

static void update_sockets(UIState *s) {
  s->sm->update(0);
}

static void update_state(UIState *s) {
  SubMaster &sm = *(s->sm);
  UIScene &scene = s->scene;
  s->running_time = 1e-9 * (nanos_since_boot() - sm["deviceState"].getDeviceState().getStartedMonoTime());

  // update engageability and DM icons at 2Hz
  if (sm.frame % (UI_FREQ / 2) == 0) {
    auto cs = sm["controlsState"].getControlsState();
    scene.engageable = cs.getEngageable() || cs.getEnabled();
    scene.dm_active = sm["driverMonitoringState"].getDriverMonitoringState().getIsActiveMode();
  }
  if (sm.updated("modelV2") && s->vg) {
    update_model(s, sm["modelV2"].getModelV2());
  }
  if (sm.updated("radarState") && s->vg) {
    std::optional<cereal::ModelDataV2::XYZTData::Reader> line;
    if (sm.rcv_frame("modelV2") > 0) {
      line = sm["modelV2"].getModelV2().getPosition();
    }
    update_leads(s, sm["radarState"].getRadarState(), line);
  }
  if (sm.updated("liveCalibration")) {
    scene.world_objects_visible = true;
    auto rpy_list = sm["liveCalibration"].getLiveCalibration().getRpyCalib();
    Eigen::Vector3d rpy;
    rpy << rpy_list[0], rpy_list[1], rpy_list[2];
    Eigen::Matrix3d device_from_calib = euler2rot(rpy);
    Eigen::Matrix3d view_from_device;
    view_from_device << 0,1,0,
                        0,0,1,
                        1,0,0;
    Eigen::Matrix3d view_from_calib = view_from_device * device_from_calib;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        scene.view_from_calib.v[i*3 + j] = view_from_calib(i,j);
      }
    }
  }
  if (sm.updated("pandaStates")) {
    auto pandaStates = sm["pandaStates"].getPandaStates();
    if (pandaStates.size() > 0) {
      scene.pandaType = pandaStates[0].getPandaType();

      if (scene.pandaType != cereal::PandaState::PandaType::UNKNOWN) {
        scene.ignition = false;
        for (const auto& pandaState : pandaStates) {
          scene.ignition |= pandaState.getIgnitionLine() || pandaState.getIgnitionCan();
        }
      }
    }
  } else if ((s->sm->frame - s->sm->rcv_frame("pandaStates")) > 5*UI_FREQ) {
    scene.pandaType = cereal::PandaState::PandaType::UNKNOWN;
  }
  if (sm.updated("carParams")) {
    scene.longitudinal_control = sm["carParams"].getCarParams().getOpenpilotLongitudinalControl();
  }
  if (!scene.started && sm.updated("sensorEvents")) {
    for (auto sensor : sm["sensorEvents"].getSensorEvents()) {
      if (sensor.which() == cereal::SensorEventData::ACCELERATION) {
        auto accel = sensor.getAcceleration().getV();
        if (accel.totalSize().wordCount) { // TODO: sometimes empty lists are received. Figure out why
          scene.accel_sensor = accel[2];
        }
      } else if (sensor.which() == cereal::SensorEventData::GYRO_UNCALIBRATED) {
        auto gyro = sensor.getGyroUncalibrated().getV();
        if (gyro.totalSize().wordCount) {
          scene.gyro_sensor = gyro[1];
        }
      }
    }
  }
  if (sm.updated("roadCameraState")) {
    auto camera_state = sm["roadCameraState"].getRoadCameraState();

    float max_lines = Hardware::EON() ? 5408 : 1904;
    float max_gain = Hardware::EON() ? 1.0: 10.0;
    float max_ev = max_lines * max_gain;

    if (Hardware::TICI()) {
      max_ev /= 6;
    }

    float ev = camera_state.getGain() * float(camera_state.getIntegLines());

    scene.light_sensor = std::clamp<float>(1.0 - (ev / max_ev), 0.0, 1.0);
  }

  if( scene.IsOpenpilotViewEnabled )
    scene.started = sm["deviceState"].getDeviceState().getStarted();
  else
    scene.started = sm["deviceState"].getDeviceState().getStarted() && scene.ignition;




  // atom 
   if (sm.updated("gpsLocationExternal")) {
    scene.gpsLocationExternal = sm["gpsLocationExternal"].getGpsLocationExternal();
   }

   if (sm.updated("deviceState")) {
    scene.deviceState = sm["deviceState"].getDeviceState();
   }
    
   if (scene.started && sm.updated("controlsState")) {
    scene.controls_state = sm["controlsState"].getControlsState();
// debug Message
    scene.alert.alertTextMsg1 = scene.controls_state.getAlertTextMsg1();
    scene.alert.alertTextMsg2 = scene.controls_state.getAlertTextMsg2();
    scene.alert.alertTextMsg3 = scene.controls_state.getAlertTextMsg3();
   } 
   if (sm.updated("carState")) {
    scene.car_state = sm["carState"].getCarState();

    auto cruiseState = scene.car_state.getCruiseState();
    scene.scr.awake = cruiseState.getCruiseSwState();
   } 
   
   if( sm.updated("liveNaviData"))
   {
     scene.liveNaviData = sm["liveNaviData"].getLiveNaviData();
     scene.scr.map_is_running = scene.liveNaviData.getMapEnable();
   } 

   if( sm.updated("liveParameters") )
   {
      scene.liveParameters = sm["liveParameters"].getLiveParameters();
   }

   if (sm.updated("lateralPlan"))
   {
    scene.lateralPlan = sm["lateralPlan"].getLateralPlan();
   } 
}

void ui_update_params(UIState *s) {
  s->scene.is_metric = Params().getBool("IsMetric");
  s->scene.IsOpenpilotViewEnabled = Params().getBool("IsOpenpilotViewEnabled");
}

static void update_status(UIState *s) {
  if (s->scene.started && s->sm->updated("controlsState")) {
    auto controls_state = (*s->sm)["controlsState"].getControlsState();
    auto alert_status = controls_state.getAlertStatus();
    if (alert_status == cereal::ControlsState::AlertStatus::USER_PROMPT) {
      s->status = STATUS_WARNING;
    } else if (alert_status == cereal::ControlsState::AlertStatus::CRITICAL) {
      s->status = STATUS_ALERT;
    } else {
      s->status = controls_state.getEnabled() ? STATUS_ENGAGED : STATUS_DISENGAGED;
    }
  }

  // Handle onroad/offroad transition
  static bool started_prev = false;
  if (s->scene.started != started_prev) {
    if (s->scene.started) {
      s->status = STATUS_DISENGAGED;
      s->scene.started_frame = s->sm->frame;
      s->scene.end_to_end = Params().getBool("EndToEndToggle");
      s->wide_camera = Hardware::TICI() ? Params().getBool("EnableWideCamera") : false;
    }
    // Invisible until we receive a calibration message.
    s->scene.world_objects_visible = false;
  }
  started_prev = s->scene.started;
}


QUIState::QUIState(QObject *parent) : QObject(parent) {
  ui_state.sm = std::make_unique<SubMaster, const std::initializer_list<const char *>>({
    "modelV2", "controlsState", "liveCalibration", "radarState", "deviceState", "roadCameraState",
    "pandaStates", "carParams", "driverMonitoringState", "sensorEvents", "carState", "liveLocationKalman",
    "liveNaviData", "gpsLocationExternal", "lateralPlan", "liveParameters",
  });

  Params params;
  ui_state.wide_camera = Hardware::TICI() ? params.getBool("EnableWideCamera") : false;
  ui_state.has_prime = params.getBool("HasPrime");

  // update timer
  timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, &QUIState::update);
  timer->start(1000 / UI_FREQ);
}

void QUIState::update() {
  update_sockets(&ui_state);
  update_state(&ui_state);
  update_status(&ui_state);

  if (ui_state.scene.started != started_prev || ui_state.sm->frame == 1) {
    started_prev = ui_state.scene.started;
    emit offroadTransition(!ui_state.scene.started);
  }

  if (ui_state.sm->frame % UI_FREQ == 0) {
    watchdog_kick();
  }
  emit uiUpdate(ui_state);
}

Device::Device(QObject *parent) : brightness_filter(BACKLIGHT_OFFROAD, BACKLIGHT_TS, BACKLIGHT_DT), QObject(parent) {
}

void Device::update(const UIState &s) {
  updateBrightness(s);
  updateWakefulness(s);

  // TODO: remove from UIState and use signals
  QUIState::ui_state.awake = awake;
}

void Device::setAwake(bool on, bool reset) {
  UIScene  &scene = QUIState::ui_state.scene;
  if (on != awake) {
    awake = on;

    // atom
    if( scene.ignition || !scene.scr.autoScreenOff )
    {    
      Hardware::set_display_power(awake);
      LOGD("setting display power %d", awake);
      emit displayPowerChanged(awake);
    }
  }

  if (reset) {
    awake_timeout = 30 * UI_FREQ;
    scene.scr.nTime = scene.scr.autoScreenOff * 60 * UI_FREQ;
  }
}

void Device::updateBrightness(const UIState &s) {
  float clipped_brightness = BACKLIGHT_OFFROAD;
  if (s.scene.started) {
    // Scale to 0% to 100%
    clipped_brightness = 100.0 * s.scene.light_sensor;

    // CIE 1931 - https://www.photonstophotos.net/GeneralTopics/Exposure/Psychometric_Lightness_and_Gamma.htm
    if (clipped_brightness <= 8) {
      clipped_brightness = (clipped_brightness / 903.3);
    } else {
      clipped_brightness = std::pow((clipped_brightness + 16.0) / 116.0, 3.0);
    }

    // Scale back to 10% to 100%
    clipped_brightness = std::clamp(100.0f * clipped_brightness, 10.0f, 100.0f);

    // Limit brightness if running for too long
    if (Hardware::TICI()) {
      const float MAX_BRIGHTNESS_HOURS = 4;
      const float HOURLY_BRIGHTNESS_DECREASE = 5;
      float ui_running_hours = s.running_time / (60*60);
      float anti_burnin_max_percent = std::clamp(100.0f - HOURLY_BRIGHTNESS_DECREASE * (ui_running_hours - MAX_BRIGHTNESS_HOURS),
                                                 30.0f, 100.0f);
      clipped_brightness = std::min(clipped_brightness, anti_burnin_max_percent);
    }
  }

  int brightness = brightness_filter.update(clipped_brightness);
  if (!awake) {
    brightness = 0;
  } else if (s.scene.started && s.scene.scr.nTime <= 0 && s.scene.scr.autoScreenOff != 0) {
    brightness = s.scene.scr.brightness_off * 0.01 * brightness;
  }

  if (brightness != last_brightness) {
    std::thread{Hardware::set_brightness, brightness}.detach();
  }
  last_brightness = brightness;
}

void Device::updateWakefulness(const UIState &s) {
  awake_timeout = std::max(awake_timeout - 1, 0);

  bool should_wake = false;
  if( !s.scene.scr.autoScreenOff || !s.scene.ignition )
  {
    should_wake = s.scene.started || s.scene.ignition;
    if (!should_wake) {
      // tap detection while display is off
      bool accel_trigger = abs(s.scene.accel_sensor - accel_prev) > 0.2;
      bool gyro_trigger = abs(s.scene.gyro_sensor - gyro_prev) > 0.15;
      should_wake = accel_trigger && gyro_trigger;
      gyro_prev = s.scene.gyro_sensor;
      accel_prev = (accel_prev * (accel_samples - 1) + s.scene.accel_sensor) / accel_samples;
    }
  }

  ScreenAwake();
  setAwake(awake_timeout, should_wake);
}


//  atom
void Device::ScreenAwake() 
{
  UIScene  &scene = QUIState::ui_state.scene;
  const bool draw_alerts = scene.started;
  const float speed = scene.car_state.getVEgo();

  if( scene.scr.nTime > 0 )
  {
    awake_timeout = 30 * UI_FREQ;
    scene.scr.nTime--;
  }
  else if( scene.scr.brightness_off )
  {
    awake_timeout = 30 * UI_FREQ;
  }
  else if( scene.ignition && (speed < 1))
  {
    awake_timeout = 30 * UI_FREQ;
  }
  else if( scene.scr.autoScreenOff && scene.scr.nTime == 0)
  {
   // awake = false;
  }

  int  cur_key = scene.scr.awake;
  if (draw_alerts && scene.controls_state.getAlertSize() != cereal::ControlsState::AlertSize::NONE) 
  {
      cur_key += 1;
  }

  static int old_key;
  if( cur_key != old_key )
  {
    old_key = cur_key;
    if(cur_key)
        setAwake(true, true);
  } 
}
