#include <time.h>
#include <dirent.h>

/*
static void ui_draw_track(UIState *s, const line_vertices_data &vd) 
{
  // kegman
  if (vd.cnt == 0) return;

  nvgBeginPath(s->vg);
  nvgMoveTo(s->vg, vd.v[0].x, vd.v[0].y);
  for (int i=1; i<vd.cnt; i++) {
    nvgLineTo(s->vg, vd.v[i].x, vd.v[i].y);
  }
  nvgClosePath(s->vg);

  int  steerOverride = s->scene.car_state.getSteeringPressed();
  float  output_scale = s->scene.controls_state.getOutput();

  NVGpaint track_bg;
  if (s->scene.controls_state.getEnabled()) {
    // Draw colored MPC track Kegman's
    if ( steerOverride) {
      track_bg = nvgLinearGradient(s->vg, s->fb_w, s->fb_h, s->fb_w, s->fb_h*.4,
        nvgRGBA(0, 191, 255, 255), nvgRGBA(0, 95, 128, 50));
    } else {
      int torque_scale = (int)fabs(510*(float)output_scale);
      int red_lvl = fmin(255, torque_scale);
      int green_lvl = fmin(255, 510-torque_scale);
      track_bg = nvgLinearGradient(s->vg, s->fb_w, s->fb_h, s->fb_w, s->fb_h*.4,
        nvgRGBA(          red_lvl,            green_lvl,  0, 255),
        nvgRGBA((int)(0.5*red_lvl), (int)(0.5*green_lvl), 0, 50));
    }
  } else {
    // Draw white vision track
    track_bg = nvgLinearGradient(s->vg, s->fb_w, s->fb_h, s->fb_w, s->fb_h * .4,
                                        COLOR_WHITE, COLOR_WHITE_ALPHA(0));
  }

  nvgFillPaint(s->vg, track_bg);
  nvgFill(s->vg); 
}
*/


/*
static void bb_ui_text(const UIState *s, float x, float y, const char *string, float size, NVGcolor color, const char *font_name) {

  if( font_name )
  {
    nvgFontFace(s->vg, font_name);
    nvgFontSize(s->vg, size);
  }

  nvgFillColor(s->vg, color);
  nvgText(s->vg, x, y, string, NULL);
}
*/

static void ui_draw_circle_image_rotation(const UIState *s, int center_x, int center_y, int radius, const char *image, NVGcolor color, float img_alpha, float angleSteers = 0) {
  const int img_size = radius * 1.5;
  float img_rotation =  angleSteers/180*3.141592;
  int ct_pos = -radius * 0.75;

  nvgBeginPath(s->vg);
  nvgCircle(s->vg, center_x, center_y + (bdr_s+7), radius);
  nvgFillColor(s->vg, color);
  nvgFill(s->vg);
  //ui_draw_image(s, {center_x - (img_size / 2), center_y - (img_size / 2), img_size, img_size}, image, img_alpha);

  nvgSave( s->vg );
  nvgTranslate(s->vg, center_x, (center_y + (bdr_s*1.5)));
  nvgRotate(s->vg, -img_rotation);  

  ui_draw_image(s, {ct_pos, ct_pos, img_size, img_size}, image, img_alpha);
  nvgRestore(s->vg); 
}

//BB START: functions added for the display of various items
static int bb_ui_draw_measure(UIState *s,  const char* bb_value, const char* bb_uom, const char* bb_label,
    int bb_x, int bb_y, int bb_uom_dx,
    NVGcolor bb_valueColor, NVGcolor bb_labelColor, NVGcolor bb_uomColor,
    int bb_valueFontSize, int bb_labelFontSize, int bb_uomFontSize )
    {
  //const UIScene *scene = &s->scene;
  //const UIScene *scene = &s->scene;
  nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
  int dx = 0;
  if (strlen(bb_uom) > 0) {
    dx = (int)(bb_uomFontSize*2.5/2);
   }
  //print value
  nvgFontFace(s->vg, "sans-semibold");
  nvgFontSize(s->vg, bb_valueFontSize*2);
  nvgFillColor(s->vg, bb_valueColor);
  nvgText(s->vg, bb_x-dx/2, bb_y+ (int)(bb_valueFontSize*2.5)+5, bb_value, NULL);
  //print label
  nvgFontFace(s->vg, "sans-regular");
  nvgFontSize(s->vg, bb_labelFontSize*2);
  nvgFillColor(s->vg, bb_labelColor);
  nvgText(s->vg, bb_x, bb_y + (int)(bb_valueFontSize*2.5)+5 + (int)(bb_labelFontSize*2.5)+5, bb_label, NULL);
  //print uom
  if (strlen(bb_uom) > 0) {
      nvgSave(s->vg);
    int rx =bb_x + bb_uom_dx + bb_valueFontSize -3;
    int ry = bb_y + (int)(bb_valueFontSize*2.5/2)+25;
    nvgTranslate(s->vg,rx,ry);
    nvgRotate(s->vg, -1.5708); //-90deg in radians
    nvgFontFace(s->vg, "sans-regular");
    nvgFontSize(s->vg, (int)(bb_uomFontSize*2));
    nvgFillColor(s->vg, bb_uomColor);
    nvgText(s->vg, 0, 0, bb_uom, NULL);
    nvgRestore(s->vg);
  }
  return (int)((bb_valueFontSize + bb_labelFontSize)*2.5) + 5;
}


static void bb_ui_draw_measures_right(UIState *s, int bb_x, int bb_y, int bb_w ) 
{
  const UIScene *scene = &s->scene;
  int bb_rx = bb_x + (int)(bb_w/2);
  int bb_ry = bb_y;
  int bb_h = 5;
  NVGcolor lab_color = nvgRGBA(255, 255, 255, 200);
  NVGcolor uom_color = nvgRGBA(255, 255, 255, 200);
  int value_fontSize=25;
  int label_fontSize=15;
  int uom_fontSize = 15;
  int bb_uom_dx =  (int)(bb_w /2 - uom_fontSize*2.5) ;






  //add CPU temperature
  if( true ) 
  {
    char val_str[16];
    char uom_str[6];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);

    auto  maxCpuTemp = scene->deviceState.getCpuTempC();
   // int   cpuPerc = scene->deviceState.getCpuUsagePercentDEPRECATED();  
    int   cpuPerc = scene->deviceState.getCpuUsagePercent()[0];
    float cpuTemp = maxCpuTemp[0];


      if( cpuTemp > 80) {
        val_color = nvgRGBA(255, 188, 3, 200);
      }
      if(cpuTemp > 92) {
        val_color = nvgRGBA(255, 0, 0, 200);
      }

       // temp is alway in C * 10
      snprintf(val_str, sizeof(val_str), "%.1f", cpuTemp );
      snprintf(uom_str, sizeof(uom_str), "%d", cpuPerc);
      bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "CPU TEMP",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );

    bb_ry = bb_y + bb_h;
  }

  //add GPU temperature
  if( 0 ) 
  {
    char val_str[16];
    char uom_str[6];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
    auto  maxGpuTemp = scene->deviceState.getGpuTempC();
    float  memTemp = scene->deviceState.getMemoryTempC();

    float gpuTemp = maxGpuTemp[0];
    
      if( gpuTemp > 80) {
        val_color = nvgRGBA(255, 188, 3, 200);
      }
      if(gpuTemp > 92) {
        val_color = nvgRGBA(255, 0, 0, 200);
      }

       // temp is alway in C * 10
      snprintf(val_str, sizeof(val_str), "%.1f", gpuTemp );
      snprintf(uom_str, sizeof(uom_str), "%.0f", memTemp);
      bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "GPU TEMP",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );

    bb_ry = bb_y + bb_h;
  } 

   //add battery temperature

  if( true )
  {
    char val_str[16];
    char uom_str[6];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
    float  batteryTemp = scene->deviceState.getBatteryTempCDEPRECATED();

    if(batteryTemp > 40) {
      val_color = nvgRGBA(255, 188, 3, 200);
    }
    if(batteryTemp > 50) {
      val_color = nvgRGBA(255, 0, 0, 200);
    }
    // temp is alway in C * 1000
    snprintf(val_str, sizeof(val_str), "%.1f", batteryTemp);
    snprintf(uom_str, sizeof(uom_str), "");
    bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "BAT TEMP",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }



  //add grey panda GPS accuracy
  if (true) {
    auto gps_ext = scene->gpsLocationExternal;
    float  gpsAccuracyUblox = gps_ext.getAccuracy();
    float  altitudeUblox = gps_ext.getAltitude();
   // float  bearingUblox = data2.getBearingDeg();

    char val_str[16];
    char uom_str[3];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
    //show red/orange if gps accuracy is low
      if(gpsAccuracyUblox > 2.0) {
         val_color = nvgRGBA(255, 188, 3, 200);
      }
      if(gpsAccuracyUblox > 5.0) {
         val_color = nvgRGBA(255, 0, 0, 200);
      }
    // gps accuracy is always in meters
    if(gpsAccuracyUblox > 99 || gpsAccuracyUblox == 0) {
       snprintf(val_str, sizeof(val_str), "None");
    }else if(gpsAccuracyUblox > 9.99) {
      snprintf(val_str, sizeof(val_str), "%.1f", gpsAccuracyUblox);
    }
    else {
      snprintf(val_str, sizeof(val_str), "%.2f", gpsAccuracyUblox);
    }
    snprintf(uom_str, sizeof(uom_str), "%.1f", altitudeUblox);
    bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "GPS PREC",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }

/*
    //add altitude
  if (gpsAccuracyUblox != 0.00) {
    char val_str[16];
    char uom_str[3];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
    snprintf(val_str, sizeof(val_str), "%.1f", (altitudeUblox));
    snprintf(uom_str, sizeof(uom_str), "m");
    bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "ALTITUDE",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }
*/


  //finally draw the frame
  bb_h += 20;
  nvgBeginPath(s->vg);
  nvgRoundedRect(s->vg, bb_x, bb_y, bb_w, bb_h, 20);
  nvgStrokeColor(s->vg, nvgRGBA(255,255,255,80));
  nvgStrokeWidth(s->vg, 6);
  nvgStroke(s->vg);
}


static void bb_ui_draw_measures_left(UIState *s, int bb_x, int bb_y, int bb_w ) 
{
  const UIScene *scene = &s->scene;
  int bb_rx = bb_x + (int)(bb_w/2);
  int bb_ry = bb_y;
  int bb_h = 5;
  NVGcolor lab_color = nvgRGBA(255, 255, 255, 200);
  NVGcolor uom_color = nvgRGBA(255, 255, 255, 200);
  int value_fontSize=25;
  int label_fontSize=15;
  int uom_fontSize = 15;
  int bb_uom_dx =  (int)(bb_w /2 - uom_fontSize*2.5) ;


  auto radar_state = (*s->sm)["radarState"].getRadarState();  // radar
  auto lead_radar = radar_state.getLeadOne();

  //add visual radar relative distance
  if( true )
  {
    char val_str[16];
    char uom_str[6];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
    
    if ( lead_radar.getStatus() ) {
      //show RED if less than 5 meters
      //show orange if less than 15 meters
      float d_rel2 = lead_radar.getDRel();
      if((int)(d_rel2) < 15) {
        val_color = nvgRGBA(255, 188, 3, 200);
      }
      if((int)(d_rel2) < 5) {
        val_color = nvgRGBA(255, 0, 0, 200);
      }
      // lead car relative distance is always in meters
      snprintf(val_str, sizeof(val_str), "%d", (int)d_rel2);
    } else {
       snprintf(val_str, sizeof(val_str), "-");
    }

    auto lead_cam = (*s->sm)["modelV2"].getModelV2().getLeadsV3()[0];  // camera
    if (lead_cam.getProb() > 0.1) {
      float d_rel1 = lead_cam.getX()[0];

      //uom_color = nvgRGBA(255, 255, 255, 200);
      snprintf(uom_str, sizeof(uom_str),  "%d", (int)d_rel1);
    }
    else
    {
      snprintf(uom_str, sizeof(uom_str), "-");
    }

    bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "REL DIST",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }

  //add visual radar relative speed
  if( true )
  {
    char val_str[16];
    char uom_str[6];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
    if ( lead_radar.getStatus() ) {
      float v_rel = lead_radar.getVRel();  
      //show Orange if negative speed (approaching)
      //show Orange if negative speed faster than 5mph (approaching fast)
      if((int)(v_rel) < 0) {
        val_color = nvgRGBA(255, 188, 3, 200);
      }
      if((int)(v_rel) < -5) {
        val_color = nvgRGBA(255, 0, 0, 200);
      }
      // lead car relative speed is always in meters
      if (scene->is_metric) {
         snprintf(val_str, sizeof(val_str), "%d", (int)(v_rel * 3.6 + 0.5));
      } else {
         snprintf(val_str, sizeof(val_str), "%d", (int)(v_rel * 2.2374144 + 0.5));
      }
    } else {
       snprintf(val_str, sizeof(val_str), "-");
    }
    if (scene->is_metric) {
      snprintf(uom_str, sizeof(uom_str), "km/h");;
    } else {
      snprintf(uom_str, sizeof(uom_str), "mph");
    }
    bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "REL SPEED",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }

  //add  steering angle
  if( true )
  {
    float angleSteers = scene->car_state.getSteeringAngleDeg();

    char val_str[16];
    char uom_str[6];
    NVGcolor val_color = nvgRGBA(0, 255, 0, 200);
      //show Orange if more than 30 degrees
      //show red if  more than 50 degrees
      if(((int)(angleSteers) < -30) || ((int)(angleSteers) > 30)) {
        val_color = nvgRGBA(255, 175, 3, 200);
      }
      if(((int)(angleSteers) < -55) || ((int)(angleSteers) > 55)) {
        val_color = nvgRGBA(255, 0, 0, 200);
      }
      // steering is in degrees
      snprintf(val_str, sizeof(val_str), "%.1f",angleSteers);

      snprintf(uom_str, sizeof(uom_str), "");
    bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "REAL STEER",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }

  //add  desired steering angle
  if( true )
  {
    float angleSteersDes = scene->controls_state.getSteeringAngleDesiredDegDEPRECATED();  

    char val_str[50];
    char uom_str[6];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
    if( scene->controls_state.getEnabled() ) {
      //show Orange if more than 6 degrees
      //show red if  more than 12 degrees
      if(((int)(angleSteersDes) < -30) || ((int)(angleSteersDes) > 30)) 
      {
        val_color = nvgRGBA(255, 255, 255, 200);
      }
      if( ((int)(angleSteersDes) < -50) || ((int)(angleSteersDes) > 50) ) 
      {
        val_color = nvgRGBA(255, 255, 255, 200);
      }
      // steering is in degrees
      snprintf(val_str, sizeof(val_str), "%.1f",angleSteersDes);
    } else {
       snprintf(val_str, sizeof(val_str), "-");
    }
    snprintf(uom_str, sizeof(uom_str), "");
    bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "DESIR STEER",
      bb_rx, bb_ry, bb_uom_dx,
      val_color, lab_color, uom_color,
      value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }


  //finally draw the frame
  bb_h += 20;
  nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, bb_x, bb_y, bb_w, bb_h, 20);
    nvgStrokeColor(s->vg, nvgRGBA(255,255,255,80));
    nvgStrokeWidth(s->vg, 6);
    nvgStroke(s->vg);
}


// tpms by neokii
static NVGcolor get_tpms_color(float tpms) {
    if(tpms < 5 || tpms > 60) // N/A
        return nvgRGBA(255, 255, 255, 200);
    if(tpms < 30)
        return nvgRGBA(255, 90, 90, 200);
    return nvgRGBA(255, 255, 255, 200);
}

static std::string get_tpms_text(float tpms) {
    if(tpms < 5 || tpms > 200)
        return "";

    char str[32];
    snprintf(str, sizeof(str), "%.0f", round(tpms));
    return std::string(str);
}

static void bb_draw_tpms(UIState *s, int viz_tpms_x, int viz_tpms_y )
{
    //const UIScene *scene = &s->scene;
    auto car_state = (*s->sm)["carState"].getCarState();
    auto tpms = car_state.getTpms();

    const float fl = tpms.getFl();
    const float fr = tpms.getFr();
    const float rl = tpms.getRl();
    const float rr = tpms.getRr();

    const int w = 58;
    const int h = 126;
    int x = viz_tpms_x;// bdr_s + 80;
    int y = viz_tpms_y - h;// s->fb_h - bdr_s - h - 60;

    const int margin = 10;

    nvgBeginPath(s->vg);
    ui_draw_image(s, {x, y, w, h}, "tire_pressure", 0.8f);

    nvgFontSize(s->vg, 60);
    nvgFontFace(s->vg, "sans-semibold");

    nvgTextAlign(s->vg, NVG_ALIGN_RIGHT);
    nvgFillColor(s->vg, get_tpms_color(fl));
    nvgText(s->vg, x-margin, y+45, get_tpms_text(fl).c_str(), NULL);

    nvgTextAlign(s->vg, NVG_ALIGN_LEFT);
    nvgFillColor(s->vg, get_tpms_color(fr));
    nvgText(s->vg, x+w+margin, y+45, get_tpms_text(fr).c_str(), NULL);

    nvgTextAlign(s->vg, NVG_ALIGN_RIGHT);
    nvgFillColor(s->vg, get_tpms_color(rl));
    nvgText(s->vg, x-margin, y+h-15, get_tpms_text(rl).c_str(), NULL);

    nvgTextAlign(s->vg, NVG_ALIGN_LEFT);
    nvgFillColor(s->vg, get_tpms_color(rr));
    nvgText(s->vg, x+w+margin, y+h-15, get_tpms_text(rr).c_str(), NULL);
}


//draw compass by opkr and re-designed by hoya
static void bb_draw_compass(UIState *s, int compass_x, int compass_y )
{
  UIScene &scene = s->scene;

  auto   gps_ext = scene.gpsLocationExternal;
  //float  gpsAccuracyUblox = gps_ext.getAccuracy();
  //float  altitudeUblox = gps_ext.getAltitude();
  float  bearingUblox = gps_ext.getBearingDeg();
  //float  fAlpha = 1.0f;

  //if ( gpsAccuracyUblox == 0.00 )
  //    fAlpha = 0.2f

  const int radius = 130;// 85 + 40;
  ui_draw_circle_image_rotation(s, compass_x, compass_y, radius, "direction", nvgRGBA(0, 0, 0, 0), 0.7f, -bearingUblox);
  ui_draw_circle_image_rotation(s, compass_x, compass_y, radius, "compass", nvgRGBA(0, 0, 0, 0), 0.8f);

}


static void bb_ui_draw_UI(UIState *s)
{
  UIScene &scene = s->scene;
  const int bb_dml_w = 180;
  const int bb_dml_x = (0 + bdr_s);
  const int bb_dml_y = (0 + bdr_s) + 220;

  const int bb_dmr_w = 180;
  const int bb_dmr_x = 0 + s->fb_w - bb_dmr_w - bdr_s;
  const int bb_dmr_y = (0 + bdr_s) + 220;

  // 1. kegman ui
  bb_ui_draw_measures_left(s, bb_dml_x, bb_dml_y, bb_dml_w);
  bb_ui_draw_measures_right(s, bb_dmr_x, bb_dmr_y, bb_dmr_w);

  // 2. tpms
  if( true )
  {
    int viz_tpms_x = s->fb_w - bb_dmr_w / 2 - 60;
    int viz_tpms_y = s->fb_h - bdr_s - 220;  
    bb_draw_tpms( s, viz_tpms_x, viz_tpms_y);
  }

  // 3. compass
  if( true )
  {
    const int compass_x = s->fb_w / 2 - 20;
    const int compass_y = s->fb_h - 40;
    bb_draw_compass( s, compass_x, compass_y );
  }
    

  // 4. debug
  int xpos = 250;
  int ypos = 400;
  nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  nvgFontSize(s->vg, 40);
  nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 255));    


  if( false )
  {
    ui_print(s, xpos, ypos+50, "sR:%.3f", scene.liveParameters.getSteerRatio() );
    ui_print(s, xpos, ypos+100, "mS:%.1f", scene.lateralPlan.getModelSpeed() * 3.6 );

  }

}
//BB END: functions added for the display of various items
