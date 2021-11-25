#include <time.h>
#include <dirent.h>

#define CAPTURE_STATE_NONE 0
#define CAPTURE_STATE_CAPTURING 1
#define CAPTURE_STATE_NOT_CAPTURING 2
#define CAPTURE_STATE_PAUSED 3
#define CLICK_TIME 0.2
#define RECORD_INTERVAL 180 // Time in seconds to rotate recordings; Max for screenrecord is 3 minutes
#define RECORD_FILES 40     // Number of files to create before looping over



long nCurrTimeSec = 0;
int captureState = CAPTURE_STATE_NOT_CAPTURING;
int captureNum = 0;
int start_time = 0;
int stop_time = 0;
int elapsed_time = 0; // Time of current recording
int click_elapsed_time = 0;
int click_time = 0;
char filenames[RECORD_FILES][50]; // Track the filenames so they can be deleted when rotating

bool lock_current_video = false; // If true save the current video before rotating
bool locked_files[RECORD_FILES]; // Track which files are locked
int lock_image;                  // Stores reference to the PNG
int files_created = 0;
int  capture_cnt = 0;
int  program_start = 1;



struct tm get_time_struct()
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  return tm;
}



static int get_time()
{
  int iRet;
  struct timeval tv;
  int seconds = 0;

  iRet = gettimeofday(&tv, NULL);
  if (iRet == 0)
  {
    seconds = (int)tv.tv_sec;
  }
  return seconds;
}


static void remove_file(char *videos_dir, char *filename)
{
  if (filename[0] == '\0')
  {
    // Don't do anything if no filename is passed
    return;
  }

  int status;
  char fullpath[64];
  snprintf(fullpath, sizeof(fullpath), "%s/%s", videos_dir, filename);
  status = remove(fullpath);
  if (status == 0)
  {
    printf("Removed file: %s\n", fullpath);
  }
  else
  {
    printf("Unable to remove file: %s\n", fullpath);
    perror("Error message:");
  }
}

static  void save_file(char *videos_dir, char *filename)
{
  if (!strlen(filename))
  {
    return;
  }

  // Rename file to save it from being overwritten
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "mv %s/%s %s/saved_%s", videos_dir, filename, videos_dir, filename);
  printf("save: %s\n", cmd);
  system(cmd);
}

static  void stop_capture() {
  char videos_dir[50] = "/storage/emulated/0/videos";

  

  if (captureState == CAPTURE_STATE_CAPTURING)
  {
    printf("stop_capture()\n ");
    system("killall -SIGINT screenrecord");
    captureState = CAPTURE_STATE_NOT_CAPTURING;
    elapsed_time = nCurrTimeSec - start_time;
    if (elapsed_time < 3)
    {
      remove_file(videos_dir, filenames[captureNum]);
    }
    else
    {
      //printf("Stop capturing screen\n");
      captureNum++;

      if (captureNum > RECORD_FILES - 1)
      {
        captureNum = 0;
      }
    }
  }
}

static  void start_capture()
{
  captureState = CAPTURE_STATE_CAPTURING;
  char cmd[128] = "";
  char videos_dir[50] = "/storage/emulated/0/videos";

  printf("start_capture()\n ");

  //////////////////////////////////
  // NOTE: make sure videos_dir folder exists on the device!
  //////////////////////////////////
  struct stat st = {0};
  if (stat(videos_dir, &st) == -1)
  {
    mkdir(videos_dir, 0700);
  }

  if (strlen(filenames[captureNum]) && files_created >= RECORD_FILES)
  {
    if (locked_files[captureNum] > 0)
    {
      save_file(videos_dir, filenames[captureNum]);
    }
    else
    {
      // remove the old file
      remove_file(videos_dir, filenames[captureNum]);
    }
    locked_files[captureNum] = 0;
  }

  char filename[64];
  struct tm tm = get_time_struct();
  snprintf(filename, sizeof(filename), "%04d%02d%02d-%02d%02d%02d.mp4", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  
  if(Hardware::TICI()) 
  {
     //snprintf(cmd,sizeof(cmd),"screenrecord --size 2160x1080 --bit-rate 7000000 %s/%s&",videos_dir,filename); 
     snprintf(cmd,sizeof(cmd),"screenrecord --size 1440x720 --bit-rate 5000000 %s/%s&",videos_dir,filename); 
  }
  else 
  {
   // snprintf(cmd,sizeof(cmd),"screenrecord --size 1280x720 --bit-rate 4000000 %s/%s&",videos_dir,filename);
    snprintf(cmd, sizeof(cmd), "screenrecord --size 1280x720 --bit-rate 5000000 %s/%s&", videos_dir, filename);
    //snprintf(cmd,sizeof(cmd),"screenrecord --size 960x540 --bit-rate 5000000 %s/%s&",videos_dir,filename);
  }
  strcpy(filenames[captureNum], filename);

  printf("Capturing to file: %s\n", cmd);
  start_time = nCurrTimeSec;
  system(cmd);

  if (lock_current_video)
  {
    // Lock is still on so mark this file for saving
    locked_files[captureNum] = 1;
  }
  else
  {
    locked_files[captureNum] = 0;
  }

  files_created++;
}


static  bool screen_button_clicked( UIState *s, Rect rect )
{
  static int touch_cnt_old = -1 ;

  int x = s->scene.mouse.touch_x;
  int y = s->scene.mouse.touch_y;
  int touch_cnt = s->scene.mouse.touch_cnt;

  if( touch_cnt_old == -1 )
  {
    touch_cnt_old = touch_cnt;
    return false;
  }

  if( touch_cnt == touch_cnt_old ) return false;
  touch_cnt_old = touch_cnt;

  if( rect.ptInRect( x, y ) )
  {
    return true;
  }

  return false;
}

static void draw_date_time(UIState *s)
{
  // Get local time to display
  char now[50];
  struct tm tm = get_time_struct();
  snprintf(now, sizeof(now), "%04d/%02d/%02d  %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
  nvgFontSize(s->vg, 30);
  nvgFontFace(s->vg, "sans-semibold");
  nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 200));

  const int bb_dmr_x = 0 + s->fb_w - 150;   // 1602
  nvgText(s->vg, bb_dmr_x, 23, now, NULL);
}


static void rotate_video()
{
  // Overwrite the existing video (if needed)
  elapsed_time = 0;
  stop_capture();
  captureState = CAPTURE_STATE_CAPTURING;
  start_capture();
}


static void screen_toggle_record_state()
{
  //if (captureState == CAPTURE_STATE_CAPTURING)
  if( lock_current_video  )
  {
    stop_capture();
    lock_current_video = false;
  }
  else
  {
    // start_capture();
    lock_current_video = true;
  }
}

static void draw_button( UIState *s, const char* string, Rect rect, NVGcolor fillColor, NVGcolor txtColor ) 
{
    int btn_x = rect.x;
    int btn_y = rect.y;
    int btn_w = rect.w;
    int btn_h = rect.h;


    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, btn_x, btn_y, btn_w, btn_h, 100);
    nvgStrokeColor(s->vg, nvgRGBA(0, 0, 0, 80));
    nvgStrokeWidth(s->vg, 6);
    nvgStroke(s->vg);

    //NVGcolor fillColor = nvgRGBA(255,0,0,150);
    nvgFillColor(s->vg, fillColor);
    nvgFill(s->vg);
    nvgFillColor(s->vg, txtColor );   // txtColor = nvgRGBA(255, 255, 255, 200)
    int btn_xc = rect.centerX();
    int btn_yc = rect.centerY();

    nvgFontSize(s->vg, 45);
    nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);    
    nvgText(s->vg, btn_xc, btn_yc, string, NULL);
}

static void screen_draw_button(UIState *s)
{
  draw_date_time(s);
  // Set button to bottom left of screen
  nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
 

  const int bb_dmr_w = 180;
  const int bb_dmr_x = 0 + s->fb_w - bb_dmr_w - bdr_s/2;

   //btn_dashcam_rec
  Rect btn_rec = btn_dashcam_rec;
  NVGcolor fillColor = nvgRGBA(255, 150, 150, 200);
  NVGcolor txtColor = nvgRGBA(255, 255, 255, 100);

  btn_rec.x = bb_dmr_x;

    if ( lock_current_video == false )
    {
       fillColor = nvgRGBA( 50, 50, 100, 200);
    }
    else if (captureState == CAPTURE_STATE_CAPTURING)
    {
      fillColor = nvgRGBA(255, 0, 0, 150);
    }
    else
    {
      fillColor = nvgRGBA(255, 150, 150, 200);
    }
 
   draw_button( s, "REC", btn_rec, fillColor, txtColor ); 
  if (  screen_button_clicked(s, btn_rec) )
  {
    click_elapsed_time = nCurrTimeSec - click_time;

    printf( "screen_button_clicked %d  captureState = %d \n", click_elapsed_time, captureState );
    if (click_elapsed_time > 0)
    {
      click_time = nCurrTimeSec + 1;
      screen_toggle_record_state();
    }
  }  


  if (captureState == CAPTURE_STATE_CAPTURING)
  {

    elapsed_time = nCurrTimeSec - start_time;
    if (elapsed_time >= RECORD_INTERVAL)
    {
      capture_cnt++;
      if( capture_cnt > 10 )
      {
        stop_capture();
        lock_current_video = false;
      }
      else
      {
        rotate_video(); 
      }
    }    
  }
}





static  int get_param( const std::string &key )
{
    auto str = QString::fromStdString(Params().get( key ));
    int value = str.toInt();

    return value;
}



static void update_dashcam(UIState *s)
{
  nCurrTimeSec =  get_time();
  if (!s->awake) return;

  if ( program_start )
  {
    program_start = 0;
    s->scene.scr.autoFocus = get_param("OpkrAutoFocus");
    s->scene.scr.brightness_off = get_param("OpkrUIBrightnessOff");
    s->scene.scr.autoScreenOff = get_param("OpkrAutoScreenOff");
    s->scene.scr.brightness = get_param("OpkrUIBrightness");
    s->scene.scr.nTime = s->scene.scr.autoScreenOff * 60 * UI_FREQ;
  }



  if (!s->scene.started) return;


  screen_draw_button(s);


  if( lock_current_video  )
  {
    float v_ego = s->scene.car_state.getVEgo();
    int engaged = s->scene.controls_state.getEngageable();

    if(  (lock_current_video == 1) && (v_ego < 0.1 || !engaged) )
    {
      elapsed_time = nCurrTimeSec - stop_time;
      if( captureState == CAPTURE_STATE_CAPTURING && elapsed_time > 2 )
      {
        capture_cnt = 0;
        stop_capture();
      }
    }    
    else if( captureState != CAPTURE_STATE_CAPTURING )
    {
      capture_cnt = 0;
      start_capture();
    }
    else
    {
      stop_time = nCurrTimeSec;
    }
    
  }
  else  if( captureState == CAPTURE_STATE_CAPTURING )
  {
    capture_cnt = 0;
    stop_capture();
  }
}





