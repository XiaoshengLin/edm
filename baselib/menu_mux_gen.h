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

#ifndef __menu_mux_gen_h
#define __menu_mux_gen_h 1

#include "act_grf.h"
#include "entry_form.h"

// #ifdef __epics__
// #include "cadef.h"
// #endif

#include "pv.h"

#define MMUX_MAX_ENTRIES 4
#define MMUX_MAX_STATES 32
#define MMUX_MAX_STRING_SIZE 32

#define MMUXC_K_COLORMODE_STATIC 0
#define MMUXC_K_COLORMODE_ALARM 1

#define MMUXC_MAJOR_VERSION 1
#define MMUXC_MINOR_VERSION 3
#define MMUXC_RELEASE 0

#define MMUXC_K_LITERAL 1
#define MMUXC_K_PV_STATE 2

#ifdef __menu_mux_gen_cc

static void mmux_putValueNoPv (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mmux_putValue (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mmuxc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mmuxc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mmuxc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mmuxc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mmuxc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mmux_controlUpdate (
  pvClass *classPtr,
  void *clientData,
  void *args );

static void mmux_alarmUpdate (
  pvClass *classPtr,
  void *clientData,
  void *args );

static void mmux_monitor_control_connect_state (
  pvClass *classPtr,
  void *clientData,
  void *args );

static void mmux_infoUpdate (
  pvClass *classPtr,
  void *clientData,
  void *args );

static void mmuxSetItem (
  Widget w,
  XtPointer client,
  XtPointer call );

#endif

class menuMuxClass : public activeGraphicClass {

private:

friend void mmux_putValueNoPv (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mmux_putValue (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mmuxc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mmuxc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mmuxc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mmuxc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mmuxc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mmux_controlUpdate (
  pvClass *classPtr,
  void *clientData,
  void *args );

friend void mmux_alarmUpdate (
  pvClass *classPtr,
  void *clientData,
  void *args );

friend void mmux_monitor_control_connect_state (
  pvClass *classPtr,
  void *clientData,
  void *args );

friend void mmux_infoUpdate (
  pvClass *classPtr,
  void *clientData,
  void *args );

friend void mmuxSetItem (
  Widget w,
  XtPointer client,
  XtPointer call );

int opComplete, firstEvent;

int bufX, bufY, bufW, bufH;

int controlV, curControlV;

unsigned int topShadowColor, bufTopShadowColor;
unsigned int botShadowColor, bufBotShadowColor;
unsigned int bufFgColor, bufBgColor;
pvColorClass fgColor, bgColor;
colorButtonClass fgCb, bgCb, topShadowCb, botShadowCb;
int fgColorMode, bgColorMode, bufFgColorMode, bufBgColorMode;

char *stateString[MMUX_MAX_STATES]; // allocated at run-time
int numStates;

entryListBase *elbt, *elbm[MMUX_MAX_ENTRIES], *elbe[MMUX_MAX_ENTRIES];

char tag[MMUX_MAX_STATES][MMUX_MAX_STRING_SIZE+1],
 m[MMUX_MAX_STATES][MMUX_MAX_ENTRIES][MMUX_MAX_STRING_SIZE+1],
 e[MMUX_MAX_STATES][MMUX_MAX_ENTRIES][MMUX_MAX_STRING_SIZE+1];

char bufTag[MMUX_MAX_STATES][MMUX_MAX_STRING_SIZE+1],
 bufM[MMUX_MAX_STATES][MMUX_MAX_ENTRIES][MMUX_MAX_STRING_SIZE+1],
 bufE[MMUX_MAX_STATES][MMUX_MAX_ENTRIES][MMUX_MAX_STRING_SIZE+1];

char *tagPtr[MMUX_MAX_STATES], *mPtr[MMUX_MAX_ENTRIES][MMUX_MAX_STATES],
 *ePtr[MMUX_MAX_ENTRIES][MMUX_MAX_STATES];

char **mac, **exp;
int numItems, numMac;

fontMenuClass fm;
char fontTag[63+1], bufFontTag[63+1];
XmFontList fontList;
XFontStruct *fs;
int fontAscent, fontDescent, fontHeight;

// #ifdef __epics__
// chid controlPvId;
// evid alarmEventId, controlEventId;
// #endif

pvClass *controlPvId;
pvEventClass *controlEventId, *alarmEventId;

char bufControlPvName[39+1];
expStringClass controlPvExpStr;

int controlExists, widgetsCreated, controlPvConnected, active, activeMode;
int connectOnly, bufConnectOnly;

Widget optionMenu, pulldownMenu, curHistoryWidget,
 pb[MMUX_MAX_STATES];

int needConnectInit, needDisconnect, needInfoInit, needUpdate, needDraw;

char pvClassName[15+1], pvUserClassName[15+1], pvOptionList[255+1];
int numPvTypes, pvNameIndex;

public:

menuMuxClass::menuMuxClass ( void );

menuMuxClass::menuMuxClass
 ( const menuMuxClass *source );

menuMuxClass::~menuMuxClass ( void );

char *menuMuxClass::objName ( void ) {

  return name;

}

int menuMuxClass::createInteractive (
  activeWindowClass *aw_obj,
  int x,
  int y,
  int w,
  int h );

int menuMuxClass::save (
  FILE *f );

int menuMuxClass::createFromFile (
  FILE *fptr,
  char *name,
  activeWindowClass *actWin );

int menuMuxClass::genericEdit ( void );

int menuMuxClass::edit ( void );

int menuMuxClass::editCreate ( void );

int menuMuxClass::draw ( void );

int menuMuxClass::erase ( void );

int menuMuxClass::drawActive ( void );

int menuMuxClass::eraseActive ( void );

int menuMuxClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] );

int menuMuxClass::getMacros (
  int *numMacros,
  char ***macro,
  char ***expansion );

int menuMuxClass::createWidgets ( void );

int menuMuxClass::activate ( int pass, void *ptr );

int menuMuxClass::deactivate ( int pass );

void menuMuxClass::updateDimensions ( void );

int isMux ( void ) { return 1; }

void menuMuxClass::executeDeferred ( void );

};

#ifdef __cplusplus
extern "C" {
#endif

void *create_menuMuxClassPtr ( void );
void *clone_menuMuxClassPtr ( void * );

#ifdef __cplusplus
}
#endif

#endif
