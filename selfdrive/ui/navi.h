#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "selfdrive/logcatd/traffic_sign.h"

/*
this is navigation code by OPKR, and thank you to the OPKR developer.
I love OPKR code.
*/


static float interp( float xv, float xp[], float fp[], int N)
{
	float dResult = 0; 
	int low, hi = 0;

	while ( (hi < N) && (xv > xp[hi]))
	{
		hi += 1;
	}
	low = hi - 1;
	if( low < 0 )
	{
		low = N-1;
		return fp[0];
	}

	if (hi == N && xv > xp[low])
	{
		return fp[N-1];
	}
	else
	{
		if( hi == 0 )
		{
			return fp[0];
		}
		else
		{
			dResult = (xv - xp[low]) * (fp[hi] - fp[low]) / (xp[hi] - xp[low]) + fp[low];
			return dResult;
		}
	}
	return  dResult;
}

static void ui_text(const UIState *s, float x, float y, const char *string, float size, NVGcolor color, const char *font_name) {

  if( font_name )
  {
    nvgFontFace(s->vg, font_name);
    nvgFontSize(s->vg, size);
  }

  nvgFillColor(s->vg, color);
  nvgText(s->vg, x, y, string, NULL);
}

static void ui_print(UIState *s, int x, int y,  const char* fmt, ... )
{
  char* msg_buf = NULL;
  va_list args;
  va_start(args, fmt);
  vasprintf( &msg_buf, fmt, args);
  va_end(args);

  nvgText(s->vg, x, y, msg_buf, NULL);
}


static void ui_draw_traffic_sign(UIState *s, float map_sign, float speedLimit,  float speedLimitAheadDistance ) 
{
    const char *traffic_sign = NULL;
    const char *name_sped[] = {"speed_var","traf_turn", "img_space","speed_30","speed_40","speed_50","speed_60","speed_70","speed_80","speed_90","speed_100","speed_110"};
    const char *name_sign[] = {"speed_bump", "bus_only"};

    const char  *szSign = NULL;

    int  nTrafficSign = int( map_sign );

    if( nTrafficSign == TS_BEND_ROAD ) traffic_sign = name_sped[1];  // 굽은도로
    else if( nTrafficSign == TS_VARIABLE ) traffic_sign = name_sped[0];  // 가변 단속. ( by opkr)
    else if( nTrafficSign == TS_BUS_ONLY ) traffic_sign = name_sign[1];  // 버스전용차로단속
    else if( nTrafficSign == TS_BUMP_ROAD ) traffic_sign = name_sign[0];  // 과속방지턱
    else if( speedLimit <= 10 )  traffic_sign = NULL;
    else if( speedLimit <= 30 )  traffic_sign = name_sped[3];
    else if( speedLimit <= 40 )  traffic_sign = name_sped[4];
    else if( speedLimit <= 50 )  traffic_sign = name_sped[5];
    else if( speedLimit <= 60 )  traffic_sign = name_sped[6];
    else if( speedLimit <= 70 )  traffic_sign = name_sped[7];
    else if( speedLimit <= 80 )  traffic_sign = name_sped[8];
    else if( speedLimit <= 90 )  traffic_sign = name_sped[9];
    else if( speedLimit <= 100 )  traffic_sign = name_sped[10];
    else if( speedLimit <= 110 )  traffic_sign = name_sped[11];

  
    if( nTrafficSign && traffic_sign == NULL )
    {
      traffic_sign = name_sped[2];
    }


    int img_size = 200;   // 472
    int img_xpos = 0 + bdr_s + 184 + 20;
    int img_ypos = 0 + bdr_s - 20;

    nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
    // 1. text  Distance
    if( speedLimitAheadDistance >= 5 )
    {
       char  szSLD[50];
      if( speedLimitAheadDistance >= 1000 )
        sprintf(szSLD,"%.1fkm", speedLimitAheadDistance * 0.001 );
      else
        sprintf(szSLD,"%.0f", speedLimitAheadDistance );

      int txt_size = int(img_size*0.8);
      int txt_xpos = img_xpos + 20;  
      int txt_ypos = img_ypos + img_size - 20;
      const Rect rect = { txt_xpos, txt_ypos, txt_size, 60};
      
      NVGcolor crFill = COLOR_BLACK_ALPHA(100); // nvgRGBA(255, 255, 255,100);
      
      float fpR[] = {255, 255, 0};
      float fpG[] = {0, 255, 100};
      float fpB[] = {0, 100, 255};
      float xp[] = {100, 300, 500};
      int N = sizeof(xp) / sizeof(xp[0]);
      float x = speedLimitAheadDistance;
      int nR = (int)interp( x, xp,  fpR, N);
      int nG = (int)interp( x, xp,  fpG, N);
      int nB = (int)interp( x, xp,  fpB, N);
      
      crFill = nvgRGBA(nR, nG, nB, 200);
      
      ui_fill_rect(s->vg, rect, crFill, 30.);
      ui_draw_rect(s->vg, rect, COLOR_WHITE_ALPHA(100), 5, 20.);        

      nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 255));
      ui_text(s, rect.centerX(), rect.centerY()+15, szSLD, 40, COLOR_WHITE, "sans-bold");
    }

    // 2. image
    if( traffic_sign  )
    {
      float img_alpha = 0.9f;
      ui_draw_image(s, {img_xpos, img_ypos, img_size, img_size}, traffic_sign, img_alpha);
    }


    if( nTrafficSign == TS_VARIABLE ) szSign = "가변구간";
    else if( nTrafficSign == TS_BEND_ROAD ) szSign = "굽은도로";
    else if( nTrafficSign == TS_BUS_ONLY ) szSign = "버스전용차로";
    else if( nTrafficSign == TS_BUMP_ROAD ) szSign = "과속방지턱";
    else if( nTrafficSign == TS_CAMERA1 ) szSign = "신호위반";
    else if( nTrafficSign == TS_CAMERA2_BUS ) szSign = "버스";
    else if( nTrafficSign == TS_CAMERA3 ) szSign = "경찰차";
    else if( nTrafficSign == TS_CAMERA4 ) szSign = "이동식";
    else if( nTrafficSign == TS_CAMERA5 ) szSign = "카메라";  
    else if( nTrafficSign == TS_INTERVAL ) szSign = "구간단속";

    if( szSign )
    {
      ui_text(s, img_xpos + int(img_size*0.5), img_ypos+25, szSign, 26, COLOR_WHITE, "sans-bold"); 
    }
    else
    {
      char  szSignal[50];
      int   nFontSize = 30;

      if( nTrafficSign == TS_CURVE_RIGHT ) szSign = "우측커브";
      else if( nTrafficSign == TS_CURVE_LEFT ) szSign = "좌측커브";
      else if( nTrafficSign == TS_RAIL_ROAD ) szSign = "철길건널목";
      else if( nTrafficSign == TS_PARK_CRACKDOWN ) szSign = "주정차금지";
      else if( nTrafficSign == TS_SCHOOL_ZONE1 ) szSign = "스쿨존#1";
      else if( nTrafficSign == TS_SCHOOL_ZONE2 ) szSign = "스쿨존#2";
      else if( nTrafficSign == TS_NARROW_ROAD ) szSign = "좁아지는도로";
      else if( nTrafficSign == TS_OVERTRAK ) szSign = "추월금지구간";
      else if( nTrafficSign == TS_SHOULDER ) szSign = "갓길단속";
      else if( nTrafficSign == TS_TRAFFIC_INFO ) szSign = "교통정보";      
      else {
        nFontSize = 73;
        szSign = szSignal;
      }
      sprintf(szSignal,"%d", nTrafficSign );
      ui_text(s, img_xpos + int(img_size*0.5), img_ypos + int(img_size*0.5) + int(nFontSize*0.5), szSign, nFontSize, COLOR_BLACK, "sans-bold"); 
    }

}

static void ui_draw_navi(UIState *s) 
{
  UIScene &scene = s->scene;


 
  float speedLimit =  scene.liveNaviData.getSpeedLimit();  
  float speedLimitAheadDistance =  scene.liveNaviData.getArrivalDistance(); // getSpeedLimitDistance();  
  float map_sign = scene.liveNaviData.getSafetySign();
  int   mapValid = scene.liveNaviData.getMapValid();


   //float dSec = scene.liveNaviData.getArrivalSec();
  // float dDistance = scene.liveNaviData.getArrivalDistance();

  //  printf("ui_draw_navi %d  %.1f  %d \n", mapValid, speedLimit, opkrturninfo);
  if( mapValid )
  {
    ui_draw_traffic_sign( s, map_sign, speedLimit, speedLimitAheadDistance );

    //nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
    //nvgFontSize(s->vg, 48);
    //int xpos = 250;
    //int ypos = 300;
    //nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 255));
    //ui_print(s, xpos, ypos, "AT:%.1f", dSec  );
    //ui_print(s, xpos, ypos + 50, "AD:%.1f", dDistance  );
  }
    
  
}

static void ui_draw_debug1(UIState *s) 
{
  UIScene &scene = s->scene;
 
  nvgFontSize(s->vg, 38);
  nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  
  const Rect rect = {bdr_s, 930, 1600, 1010};
  ui_fill_rect(s->vg, rect, COLOR_BLACK_ALPHA(100), 30.);  
  nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 255));

  ui_print(s, 0, 30, scene.alert.alertTextMsg1.c_str()  );
  ui_print(s, 0, 970, scene.alert.alertTextMsg2.c_str() );
  ui_print(s, 0, 1010, scene.alert.alertTextMsg3.c_str() );
}



void ui_main_navi(UIState *s) 
{
  ui_draw_navi( s );

  ui_draw_debug1( s );
}
