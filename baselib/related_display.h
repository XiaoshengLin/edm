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

#ifndef __related_display_h
#define __related_display_h 1

#include "act_grf.h"
#include "entry_form.h"

#include "cadef.h"

#define RDC_MAJOR_VERSION 2
#define RDC_MINOR_VERSION 0
#define RDC_RELEASE 1

typedef struct objAndIndexTag {
  void *obj;
  int index;
} objAndIndexType;

#ifdef __related_display_cc

#include "related_display.str"

static void relDsp_monitor_dest_connect_state (
  struct connection_handler_args arg );

static void rdc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

static void rdc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

static void rdc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

static void rdc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

static void rdc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

#endif

class relatedDisplayClass : public activeGraphicClass {

private:

friend void relDsp_monitor_dest_connect_state (
  struct connection_handler_args arg );

friend void openDisplay (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void rdc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void rdc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void rdc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void rdc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void rdc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

activeWindowClass *aw;
int useFocus, bufUseFocus, needClose;

int bufX, bufY, bufW, bufH;

unsigned int topShadowColor, bufTopShadowColor;
unsigned int botShadowColor, bufBotShadowColor;
unsigned int bufFgColor, bufBgColor;
pvColorClass fgColor, bgColor;
colorButtonClass fgCb, bgCb, topShadowCb, botShadowCb;
int invisible, bufInvisible;
int closeAction, bufCloseAction;
int setPostion, bufSetPostion;
int allowDups, bufAllowDups;
int cascade, bufCascade;
int propagateMacros, bufPropagateMacros;

fontMenuClass fm;
char fontTag[63+1], bufFontTag[63+1];
XmFontList fontList;
XFontStruct *fs;
int fontAscent, fontDescent, fontHeight;

char bufDisplayFileName[127+1];
char displayFileName[127+1];

expStringClass symbolsExpStr;
char bufSymbols[255+1];
char symbols[255+1];

int replaceSymbols, bufReplaceSymbols; // else append

char bufLabel[127+1];
char label[127+1];

#define NUMPVS 4

chid destPvId[NUMPVS];

objAndIndexType objAndIndex[NUMPVS];

int opComplete[NUMPVS], destExists[NUMPVS], destConnected[NUMPVS],
 destType[NUMPVS];

expStringClass destPvExpString[NUMPVS];
char bufDestPvName[NUMPVS][39+1];

expStringClass sourceExpString[NUMPVS];
char bufSource[NUMPVS][39+1];

int activeMode;

public:

relatedDisplayClass::relatedDisplayClass ( void );

relatedDisplayClass::relatedDisplayClass
 ( const relatedDisplayClass *source );

relatedDisplayClass::~relatedDisplayClass ( void );

char *relatedDisplayClass::objName ( void ) {

  return name;

}

int relatedDisplayClass::createInteractive (
  activeWindowClass *aw_obj,
  int x,
  int y,
  int w,
  int h );

int relatedDisplayClass::save (
  FILE *f );

int relatedDisplayClass::createFromFile (
  FILE *fptr,
  char *name,
  activeWindowClass *actWin );

int relatedDisplayClass::importFromXchFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin );

int relatedDisplayClass::genericEdit ( void );

int relatedDisplayClass::edit ( void );

int relatedDisplayClass::editCreate ( void );

int relatedDisplayClass::draw ( void );

int relatedDisplayClass::erase ( void );

int relatedDisplayClass::drawActive ( void );

int relatedDisplayClass::eraseActive ( void );

int relatedDisplayClass::activate (
  int pass,
  void *ptr );

int relatedDisplayClass::deactivate ( int pass );

void relatedDisplayClass::updateDimensions ( void );

int relatedDisplayClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] );

int relatedDisplayClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] );

int relatedDisplayClass::containsMacros ( void );

void relatedDisplayClass::btnUp (
  int x,
  int y,
  int buttonState,
  int buttonNumber,
  int *action );

void relatedDisplayClass::btnDown (
  int x,
  int y,
  int buttonState,
  int buttonNumber,
  int *action );

int relatedDisplayClass::getButtonActionRequest (
  int *up,
  int *down,
  int *drag,
  int *focus );

void relatedDisplayClass::changeDisplayParams (
  unsigned int flag,
  char *fontTag,
  int alignment,
  char *ctlFontTag,
  int ctlAlignment,
  char *btnFontTag,
  int btnAlignment,
  unsigned int textFgColor,
  unsigned int fg1Color,
  unsigned int fg2Color,
  unsigned int offsetColor,
  unsigned int bgColor,
  unsigned int topShadowColor,
  unsigned int botShadowColor );

void relatedDisplayClass::pointerIn (
  int _x,
  int _y,
  int buttonState );

void relatedDisplayClass::pointerOut (
  int _x,
  int _y,
  int buttonState );

 void relatedDisplayClass::executeDeferred ( void );

};

#ifdef __cplusplus
extern "C" {
#endif

void *create_relatedDisplayClassPtr ( void );
void *clone_relatedDisplayClassPtr ( void * );

#ifdef __cplusplus
}
#endif

#endif
