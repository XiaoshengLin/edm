//  edm - extensible display manager

//  Copyright (C) 1999 John W. Sinclair

//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __menu_button_h
#define __menu_button_h 1

#include "act_grf.h"
#include "entry_form.h"

#ifdef __epics__
#include "cadef.h"
#endif

#ifndef __epics__
#define MAX_ENUM_STATES 4
#define MAX_ENUM_STRING_SIZE 16
#endif

#define MBTC_K_COLORMODE_STATIC 0
#define MBTC_K_COLORMODE_ALARM 1

#define MBTC_MAJOR_VERSION 2
#define MBTC_MINOR_VERSION 3
#define MBTC_RELEASE 0

#define MBTC_K_LITERAL 1
#define MBTC_K_PV_STATE 2

#ifdef __menu_button_cc

#include "menu_button.str"

static void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id );

static void menu_cb (
  Widget w,
  XtPointer client,
  XtPointer call );

static char *dragName[] = {
  activeMenuButtonClass_str1,
  activeMenuButtonClass_str28,
  activeMenuButtonClass_str30
};

static void mbt_infoUpdate (
  struct event_handler_args ast_args );

static void mbt_readInfoUpdate (
  struct event_handler_args ast_args );

static void mbtc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mbtc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mbtc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mbtc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mbtc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mbt_controlUpdate (
  struct event_handler_args ast_args );

static void mbt_alarmUpdate (
  struct event_handler_args ast_args );

static void mbt_monitor_control_connect_state (
  struct connection_handler_args arg );

static void mbt_readUpdate (
  struct event_handler_args ast_args );

static void mbt_readAlarmUpdate (
  struct event_handler_args ast_args );

static void mbt_monitor_read_connect_state (
  struct connection_handler_args arg );

static void mbt_monitor_vis_connect_state (
  struct connection_handler_args arg );

static void mbt_visInfoUpdate (
  struct event_handler_args ast_args );

static void mbt_visUpdate (
  struct event_handler_args ast_args );

#endif

class activeMenuButtonClass : public activeGraphicClass {

private:

friend void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id );

friend void menu_cb (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mbt_infoUpdate (
  struct event_handler_args ast_args );

friend void mbt_readInfoUpdate (
  struct event_handler_args ast_args );

friend void mbtc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mbtc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mbtc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mbtc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mbtc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mbt_controlUpdate (
  struct event_handler_args ast_args );

friend void mbt_alarmUpdate (
  struct event_handler_args ast_args );

friend void mbt_monitor_control_connect_state (
  struct connection_handler_args arg );

friend void mbt_readUpdate (
  struct event_handler_args ast_args );

friend void mbt_readAlarmUpdate (
  struct event_handler_args ast_args );

friend void mbt_monitor_read_connect_state (
  struct connection_handler_args arg );

friend void mbt_monitor_vis_connect_state (
  struct connection_handler_args arg );

friend void mbt_visInfoUpdate (
  struct event_handler_args ast_args );

friend void mbt_visUpdate (
  struct event_handler_args ast_args );

static const int controlPvConnection = 1;
static const int readPvConnection = 2;
static const int visPvConnection = 3;

pvConnectionClass connection;

int init, opComplete, pvCheckExists, controlValid, readValid;

int bufX, bufY, bufW, bufH;

short curValue, value, curReadValue, readValue;

int topShadowColor, bufTopShadowColor;
int botShadowColor, bufBotShadowColor;
int bufFgColor, bufBgColor, bufInconsistentColor;
pvColorClass fgColor, bgColor, inconsistentColor;
colorButtonClass fgCb, bgCb, inconsistentCb, topShadowCb, botShadowCb;
int fgColorMode, bgColorMode, bufFgColorMode, bufBgColorMode;

char *stateString[MAX_ENUM_STATES]; // allocated at run-time
int numStates;

fontMenuClass fm;
char fontTag[63+1], bufFontTag[63+1];
XmFontList fontList;
XFontStruct *fs;
int fontAscent, fontDescent, fontHeight;

int buttonPressed;

#ifdef __epics__
chid controlPvId, readPvId;
evid alarmEventId, controlEventId, readAlarmEventId, readEventId;
#endif

char bufControlPvName[activeGraphicClass::MAX_PV_NAME+1];
expStringClass controlPvExpStr;

char bufReadPvName[activeGraphicClass::MAX_PV_NAME+1];
expStringClass readPvExpStr;

int controlExists, readExists, widgetsCreated, active, activeMode;

Widget popUpMenu, pullDownMenu, pb[MAX_ENUM_STATES];

int needConnectInit, needReadConnectInit, needInfoInit,
 needReadInfoInit, needDraw, needRefresh, needToDrawUnconnected,
 needToEraseUnconnected;
int unconnectedTimer;

chid visPvId;
evid visEventId;
expStringClass visPvExpString;
char bufVisPvName[activeGraphicClass::MAX_PV_NAME+1];
int visExists;
double visValue, curVisValue, minVis, maxVis;
char minVisString[39+1], bufMinVisString[39+1];
char maxVisString[39+1], bufMaxVisString[39+1];
int prevVisibility, visibility, visInverted, bufVisInverted;
int needVisConnectInit, needVisInit, needVisUpdate;

public:

activeMenuButtonClass::activeMenuButtonClass ( void );

activeMenuButtonClass::~activeMenuButtonClass ( void );

activeMenuButtonClass::activeMenuButtonClass
 ( const activeMenuButtonClass *source );

char *activeMenuButtonClass::objName ( void ) {

  return name;

}

int activeMenuButtonClass::createInteractive (
  activeWindowClass *aw_obj,
  int x,
  int y,
  int w,
  int h );

int activeMenuButtonClass::save (
  FILE *f );

int activeMenuButtonClass::createFromFile (
  FILE *fptr,
  char *name,
  activeWindowClass *actWin );

int activeMenuButtonClass::genericEdit ( void );

int activeMenuButtonClass::edit ( void );

int activeMenuButtonClass::editCreate ( void );

int activeMenuButtonClass::draw ( void );

int activeMenuButtonClass::erase ( void );

int activeMenuButtonClass::drawActive ( void );

int activeMenuButtonClass::eraseActive ( void );

int activeMenuButtonClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] );

int activeMenuButtonClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] );

int activeMenuButtonClass::containsMacros ( void );

int activeMenuButtonClass::createWidgets ( void );

int activeMenuButtonClass::activate ( int pass, void *ptr );

int activeMenuButtonClass::deactivate ( int pass );

void activeMenuButtonClass::updateDimensions ( void );

void activeMenuButtonClass::btnUp (
  int _x,
  int _y,
  int buttonState,
  int buttonNumber,
  int *action );

void activeMenuButtonClass::btnDown (
  int _x,
  int _y,
  int buttonState,
  int buttonNumber,
  int *action );

void activeMenuButtonClass::pointerIn (
  int _x,
  int _y,
  int buttonState );

int activeMenuButtonClass::getButtonActionRequest (
  int *up,
  int *down,
  int *drag,
  int *focus );

void activeMenuButtonClass::executeDeferred ( void );

char *activeMenuButtonClass::firstDragName ( void );

char *activeMenuButtonClass::nextDragName ( void );

char *activeMenuButtonClass::dragValue (
  int i );

void activeMenuButtonClass::changeDisplayParams (
  unsigned int flag,
  char *fontTag,
  int alignment,
  char *ctlFontTag,
  int ctlAlignment,
  char *btnFontTag,
  int btnAlignment,
  int textFgColor,
  int fg1Color,
  int fg2Color,
  int offsetColor,
  int bgColor,
  int topShadowColor,
  int botShadowColor );

void activeMenuButtonClass::changePvNames (
  int flag,
  int numCtlPvs,
  char *ctlPvs[],
  int numReadbackPvs,
  char *readbackPvs[],
  int numNullPvs,
  char *nullPvs[],
  int numVisPvs,
  char *visPvs[],
  int numAlarmPvs,
  char *alarmPvs[] );

};

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeMenuButtonClassPtr ( void );
void *clone_activeMenuButtonClassPtr ( void * );

#ifdef __cplusplus
}
#endif

#endif
