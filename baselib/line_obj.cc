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

#define __line_obj_cc 1

#include "line_obj.h"
#include "app_pkg.h"
#include "act_win.h"

#include "thread.h"

// This is the EPICS specific line right now:
static PV_Factory *pv_factory = new EPICS_PV_Factory();

static void doBlink (
  void *ptr
) {

activeLineClass *alo = (activeLineClass *) ptr;

  if ( !alo->activeMode ) {
    if ( alo->isSelected() ) alo->drawSelectBoxCorners(); // erase via xor
    alo->smartDrawAll();
    if ( alo->isSelected() ) alo->drawSelectBoxCorners();
  }
  else {
    alo->bufInvalidate();
    alo->smartDrawAllActive();
  }

}

static void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id )
{

activeLineClass *alo = (activeLineClass *) client;

  if ( !alo->init ) {
    alo->needToDrawUnconnected = 1;
    alo->needRefresh = 1;
    alo->actWin->addDefExeNode( alo->aglPtr );
  }

  alo->unconnectedTimer = 0;

}

class undoLineOpClass : public undoOpClass {

public:

int n;
int *x;
int *y;

undoLineOpClass::undoLineOpClass ()
{

  printf( "undoLineOpClass::undoLineOpClass\n" );
  n = 0;

}

undoLineOpClass::undoLineOpClass (
  int _n,
  XPoint *_xpoints
) {

int i;

  n = _n;
  x = new int[n];
  y = new int[n];

  for ( i=0; i<n; i++ ) {
    x[i] = _xpoints[i].x;
    y[i] = _xpoints[i].y;
  }

}

undoLineOpClass::~undoLineOpClass ()
{

  delete x;
  x = NULL;
  delete y;
  y = NULL;
  n = 0;

}

};

static void alc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeLineClass *alo = (activeLineClass *) client;

  alo->actWin->setChanged();

  alo->eraseSelectBoxCorners();
  alo->erase();

  alo->lineColorMode = alo->bufLineColorMode;
  if ( alo->lineColorMode == ALC_K_COLORMODE_ALARM )
    alo->lineColor.setAlarmSensitive();
  else
    alo->lineColor.setAlarmInsensitive();
  alo->lineColor.setColorIndex( alo->bufLineColor, alo->actWin->ci );

  alo->fill = alo->bufFill;

  alo->fillColorMode = alo->bufFillColorMode;
  if ( alo->fillColorMode == ALC_K_COLORMODE_ALARM )
    alo->fillColor.setAlarmSensitive();
  else
    alo->fillColor.setAlarmInsensitive();
  alo->fillColor.setColorIndex( alo->bufFillColor, alo->actWin->ci );

  alo->lineWidth = alo->bufLineWidth;

  if ( alo->bufLineStyle == 0 )
    alo->lineStyle = LineSolid;
  else if ( alo->bufLineStyle == 1 )
    alo->lineStyle = LineOnOffDash;

  alo->alarmPvExpStr.setRaw( alo->bufAlarmPvName );

  alo->visPvExpStr.setRaw( alo->bufVisPvName );

  if ( alo->bufVisInverted )
    alo->visInverted = 0;
  else
    alo->visInverted = 1;

  strncpy( alo->minVisString, alo->bufMinVisString, 39 );
  strncpy( alo->maxVisString, alo->bufMaxVisString, 39 );

  alo->x = alo->bufX;
  alo->sboxX = alo->bufX;

  alo->y = alo->bufY;
  alo->sboxY = alo->bufY;

  alo->w = alo->bufW;
  alo->sboxW = alo->bufW;

  alo->h = alo->bufH;
  alo->sboxH = alo->bufH;

  alo->updateDimensions();

}

static void alc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeLineClass *alo = (activeLineClass *) client;

  alc_edit_update( w, client, call );
  alo->refresh( alo );

}

static void alc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

int oneX, oneY, oneW, oneH;
activeLineClass *alo = (activeLineClass *) client;
pointPtr cur;

  alo->actWin->drawGc.saveFg();
  alo->actWin->drawGc.setFG( alo->lineColor.pixelColor() );

  oneW = 3;
  oneH = 3;

  cur = alo->head->flink;
  while ( cur != alo->head ) {

    oneX = cur->x;
    oneY = cur->y;

    alo->actWin->drawGc.setLineStyle( LineSolid );
    alo->actWin->drawGc.setLineWidth( 1 );

    XDrawRectangle( alo->actWin->display(),
     XtWindow(alo->actWin->drawWidgetId()),
     alo->actWin->drawGc.xorGC(), oneX-oneW/2, oneY-oneH/2, oneW, oneH );

    alo->actWin->drawGc.setLineStyle( alo->lineStyle );
    alo->actWin->drawGc.setLineWidth( alo->lineWidth );

    if ( cur->blink != alo->head ) {
      XDrawLine( alo->actWin->display(),
       XtWindow(alo->actWin->drawWidgetId()),
       alo->actWin->drawGc.xorGC(), cur->blink->x, cur->blink->y,
       cur->x, cur->y );
    }

    cur = cur->flink;

  }

  alc_edit_update( w, client, call );
  alo->ef.popdown();

  alo->actWin->drawGc.setFG( alo->lineColor.pixelColor() );

  cur = alo->head->flink;
  while ( cur != alo->head ) {

    oneX = cur->x;
    oneY = cur->y;

    alo->actWin->drawGc.setLineStyle( LineSolid );
    alo->actWin->drawGc.setLineWidth( 1 );

    XDrawRectangle( alo->actWin->display(),
     XtWindow(alo->actWin->drawWidgetId()),
     alo->actWin->drawGc.xorGC(), oneX-oneW/2, oneY-oneH/2, oneW, oneH );

    alo->actWin->drawGc.setLineStyle( alo->lineStyle );
    alo->actWin->drawGc.setLineWidth( alo->lineWidth );

    if ( cur->blink != alo->head ) {
      XDrawLine( alo->actWin->display(),
       XtWindow(alo->actWin->drawWidgetId()),
       alo->actWin->drawGc.xorGC(), cur->blink->x, cur->blink->y,
       cur->x, cur->y );
    }

    cur = cur->flink;

  }

  alo->actWin->drawGc.restoreFg();
  alo->actWin->drawGc.setLineStyle( LineSolid );
  alo->actWin->drawGc.setLineWidth( 1 );

  alo->actWin->setCurrentPointObject( alo );

  alo->lineEditBegin();

}

static void alc_edit_prop_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeLineClass *alo = (activeLineClass *) client;

  alc_edit_update( w, client, call );
  alo->ef.popdown();
  alo->operationComplete();

}

static void alc_edit_prop_cancel (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeLineClass *alo = (activeLineClass *) client;

  alo->ef.popdown();
  alo->operationCancel();

}

void alc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeLineClass *alo = (activeLineClass *) client;

  alo->ef.popdown();
  alo->operationCancel();
  alo->erase();
  alo->deleteRequest = 1;
  alo->drawAll();

}

void activeLineClass::alarmPvConnectStateCallback (
  ProcessVariable *pv,
  void *userarg
) {

activeLineClass *alo = (activeLineClass *) userarg;

  if ( pv->is_valid() ) {

  }
  else { // lost connection

    alo->connection.setPvDisconnected( (void *) alo->alarmPvConnection );
    alo->lineColor.setDisconnected();
    alo->fillColor.setDisconnected();

    alo->actWin->appCtx->proc->lock();
    alo->needRefresh = 1;
    alo->actWin->addDefExeNode( alo->aglPtr );
    alo->actWin->appCtx->proc->unlock();

  }

}

void activeLineClass::alarmPvValueCallback (
  ProcessVariable *pv,
  void *userarg
) {

activeLineClass *alo = (activeLineClass *) userarg;

  if ( !alo->connection.pvsConnected() ) {

    alo->connection.setPvConnected( (void *) alarmPvConnection );

    if ( alo->connection.pvsConnected() ) {
      alo->actWin->appCtx->proc->lock();
      alo->needConnectInit = 1;
      alo->actWin->addDefExeNode( alo->aglPtr );
      alo->actWin->appCtx->proc->unlock();
    }

  }
  else {

    alo->actWin->appCtx->proc->lock();
    alo->needAlarmUpdate = 1;
    alo->actWin->addDefExeNode( alo->aglPtr );
    alo->actWin->appCtx->proc->unlock();

  }

}

void activeLineClass::visPvConnectStateCallback (
  ProcessVariable *pv,
  void *userarg
) {

activeLineClass *alo = (activeLineClass *) userarg;

  if ( pv->is_valid() ) {

  }
  else { // lost connection

    alo->connection.setPvDisconnected( (void *) alo->visPvConnection );
    alo->lineColor.setDisconnected();
    alo->fillColor.setDisconnected();

    alo->actWin->appCtx->proc->lock();
    alo->needRefresh = 1;
    alo->actWin->addDefExeNode( alo->aglPtr );
    alo->actWin->appCtx->proc->unlock();

  }

}

void activeLineClass::visPvValueCallback (
  ProcessVariable *pv,
  void *userarg
) {

activeLineClass *alo = (activeLineClass *) userarg;

  if ( !alo->connection.pvsConnected() ) {

    alo->connection.setPvConnected( (void *) visPvConnection );

    if ( alo->connection.pvsConnected() ) {
      alo->actWin->appCtx->proc->lock();
      alo->needConnectInit = 1;
      alo->actWin->addDefExeNode( alo->aglPtr );
      alo->actWin->appCtx->proc->unlock();
    }

  }
  else {

    alo->actWin->appCtx->proc->lock();
    alo->needVisUpdate = 1;
    alo->actWin->addDefExeNode( alo->aglPtr );
    alo->actWin->appCtx->proc->unlock();

    }

}

activeLineClass::activeLineClass ( void ) {

  name = new char[strlen("activeLineClass")+1];
  strcpy( name, "activeLineClass" );
  visibility = 0;
  prevVisibility = -1;
  visInverted = 0;
  visPvExists = alarmPvExists = 0;
  activeMode = 0;
  fill = 0;
  lineColorMode = ALC_K_COLORMODE_STATIC;
  fillColorMode = ALC_K_COLORMODE_STATIC;
  lineWidth = 1;
  lineStyle = LineSolid;
  strcpy( minVisString, "" );
  strcpy( maxVisString, "" );
  xpoints = NULL;

  wasSelected = 0;

  head = new pointType;
  head->flink = head;
  head->blink = head;

  connection.setMaxPvs( 2 );

  unconnectedTimer = 0;

  setBlinkFunction( (void *) doBlink );

}

activeLineClass::~activeLineClass ( void ) {

  if ( name ) delete name;
  if ( head ) delete head;
  if ( xpoints ) delete xpoints;

  if ( unconnectedTimer ) {
    XtRemoveTimeOut( unconnectedTimer );
    unconnectedTimer = 0;
  }

}

// copy constructor
activeLineClass::activeLineClass
( const activeLineClass *source ) {

activeGraphicClass *ago = (activeGraphicClass *) this;
int i;

//  printf( "In copy constructor\n" );

  ago->clone( (activeGraphicClass *) source );

  oldX = x;
  oldY = y;
  oldW = w;
  oldH = h;

  name = new char[strlen("activeLineClass")+1];
  strcpy( name, "activeLineClass" );

  lineColor.copy(source->lineColor);
  lineCb = source->lineCb;
  lineColorMode = source->lineColorMode;

  fill = source->fill;

  fillColor.copy(source->fillColor);
  fillCb = source->fillCb;
  fillColorMode = source->fillColorMode;

  visInverted = source->visInverted;

  alarmPvExpStr.setRaw( source->alarmPvExpStr.rawString );
  visPvExpStr.setRaw( source->visPvExpStr.rawString );

  visibility = 0;
  prevVisibility = -1;
  visPvExists = alarmPvExists = 0;
  activeMode = 0;

  strncpy( minVisString, source->minVisString, 39 );
  strncpy( maxVisString, source->maxVisString, 39 );

  head = new pointType;
  head->flink = head;
  head->blink = head;

  numPoints = source->numPoints;
  xpoints = new XPoint[source->numPoints];

  for ( i=0; i<numPoints; i++ ) {
    xpoints[i].x = source->xpoints[i].x;
    xpoints[i].y = source->xpoints[i].y;
  }

  capStyle = source->capStyle;
  joinStyle = source->joinStyle;
  lineStyle = source->lineStyle;
  lineWidth = source->lineWidth;

  wasSelected = 0;

  connection.setMaxPvs( 2 );

  unconnectedTimer = 0;

  setBlinkFunction( (void *) doBlink );

}

int activeLineClass::createInteractive (
  activeWindowClass *aw_obj,
  int _x,
  int _y,
  int _w,
  int _h ) {

//   printf( "In activeLineClass::createInteractive\n" );

  actWin = (activeWindowClass *) aw_obj;
  x = _x;
  y = _y;
  w = _w;
  h = _h;

  oldX = x;
  oldY = y;
  oldW = w;
  oldH = h;

  numPoints = 0;
  xpoints = (XPoint *) NULL;

  lineColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  fillColor.setColorIndex( actWin->defaultBgColor, actWin->ci );

  this->editCreate();

  return 1;

}

int activeLineClass::genericEdit ( void ) {

char title[32], *ptr;

  ptr = actWin->obj.getNameFromClass( "activeLineClass" );
  if ( ptr )
    strncpy( title, ptr, 31 );
  else
    strncpy( title, activeLineClass_str4, 31 );

  strncat( title, activeLineClass_str5, 31 );

  bufX = x;
  bufY = y;
  bufW = w;
  bufH = h;

  bufLineColor = lineColor.pixelIndex();
  bufLineColorMode = lineColorMode;

  bufFill = fill;

  bufFillColor = fillColor.pixelIndex();
  bufFillColorMode = fillColorMode;

  bufLineWidth = lineWidth;

  if ( lineStyle == LineSolid )
    bufLineStyle = 0;
  else if ( lineStyle == LineOnOffDash )
    bufLineStyle = 1;

  if ( alarmPvExpStr.getRaw() )
    strncpy( bufAlarmPvName, alarmPvExpStr.getRaw(), PV_Factory::MAX_PV_NAME );
  else
    strcpy( bufAlarmPvName, "" );

  if ( visPvExpStr.getRaw() )
    strncpy( bufVisPvName, visPvExpStr.getRaw(), PV_Factory::MAX_PV_NAME );
  else
    strcpy( bufVisPvName, "" );

  if ( visInverted )
    bufVisInverted = 0;
  else
    bufVisInverted = 1;

  strncpy( bufMinVisString, minVisString, 39 );
  strncpy( bufMaxVisString, maxVisString, 39 );

  ef.create( actWin->top, actWin->appCtx->ci.getColorMap(),
   &actWin->appCtx->entryFormX,
   &actWin->appCtx->entryFormY, &actWin->appCtx->entryFormW,
   &actWin->appCtx->entryFormH, &actWin->appCtx->largestH,
   title, NULL, NULL, NULL );

  ef.addTextField( activeLineClass_str6, 30, &bufX );
  ef.addTextField( activeLineClass_str7, 30, &bufY );
  ef.addTextField( activeLineClass_str8, 30, &bufW );
  ef.addTextField( activeLineClass_str9, 30, &bufH );
  ef.addOption( activeLineClass_str10, activeLineClass_str11, &bufLineWidth );
  ef.addOption( activeLineClass_str12, activeLineClass_str13, &bufLineStyle );
  ef.addColorButton( activeLineClass_str14, actWin->ci, &lineCb, &bufLineColor );
  ef.addToggle( activeLineClass_str15, &bufLineColorMode );
  ef.addToggle( activeLineClass_str16, &bufFill );
  ef.addColorButton( activeLineClass_str17, actWin->ci, &fillCb, &bufFillColor );
  ef.addToggle( activeLineClass_str18, &bufFillColorMode );
  ef.addTextField( activeLineClass_str19, 30, bufAlarmPvName,
   PV_Factory::MAX_PV_NAME );
  ef.addTextField( activeLineClass_str20, 30, bufVisPvName,
   PV_Factory::MAX_PV_NAME );
  ef.addOption( " ", activeLineClass_str22, &bufVisInverted );
  ef.addTextField( activeLineClass_str23, 30, bufMinVisString, 39 );
  ef.addTextField( activeLineClass_str24, 30, bufMaxVisString, 39 );

  return 1;

}

int activeLineClass::editCreate ( void ) {

  this->wasSelected = 0;
  this->genericEdit();
  ef.finished( alc_edit_ok, alc_edit_apply, alc_edit_cancel_delete, this );
  actWin->currentEf = NULL;
  ef.popup();

  return 1;

}

int activeLineClass::edit ( void ) {

  this->genericEdit();
  ef.finished( alc_edit_prop_ok, alc_edit_apply, alc_edit_prop_cancel,
   this );
  actWin->currentEf = &ef;
  ef.popup();

  return 1;

}

int activeLineClass::editLineSegments ( void ) {

int i, oneX, oneY, oneW, oneH;
pointPtr cur;

// The intent of this next block of code is to get rid of the select
// box corners. The function eraseSelectBoxCorners() doesn't work if
// the object is on top of another object because the select box corners
// are merely drawn with the background color. We have to deselect the
// object, refresh the screen, and re-select the object to get rid of the
// select box corners.

  if ( this->isSelected() ) {
    this->wasSelected = 1;
    this->eraseSelectBoxCorners();
    this->deselect();
    this->refresh();
  }
  else {
    this->wasSelected = 0;
  }

  this->erase();
  this->actWin->refreshGrid();

  if ( this->numPoints > 0 ) {

    this->actWin->drawGc.saveFg();
    this->actWin->drawGc.setFG( this->lineColor.pixelColor() );
    oneW = 3;
    oneH = 3;

    for ( i=0; i<this->numPoints; i++ ) {

      cur = new pointType;
      cur->x = this->xpoints[i].x;
      cur->y = this->xpoints[i].y;
      this->head->blink->flink = cur;
      cur->blink = this->head->blink;
      this->head->blink = cur;
      cur->flink = this->head;

      oneX = cur->x;
      oneY = cur->y;

      this->actWin->drawGc.setLineStyle( LineSolid );
      this->actWin->drawGc.setLineWidth( 1 );

      XDrawRectangle( this->actWin->display(),
       XtWindow(this->actWin->drawWidgetId()),
       this->actWin->drawGc.xorGC(), oneX-oneW/2, oneY-oneH/2, oneW, oneH );

      this->actWin->drawGc.setLineStyle( this->lineStyle );
      this->actWin->drawGc.setLineWidth( this->lineWidth );

      if ( cur->blink != this->head ) {
        XDrawLine( this->actWin->display(),
         XtWindow(this->actWin->drawWidgetId()),
         this->actWin->drawGc.xorGC(), cur->blink->x, cur->blink->y, cur->x,
         cur->y );
      }

    }

    if ( this->numPoints > 0 ) {
      this->numPoints = 0;
      delete this->xpoints;
    }

  }

  this->actWin->drawGc.restoreFg();
  this->actWin->drawGc.setLineStyle( LineSolid );
  this->actWin->drawGc.setLineWidth( 1 );

  actWin->setCurrentPointObject( this );

  lineEditBegin();

  return 1;

}

int activeLineClass::addPoint (
  int oneX,
  int oneY )
{

pointPtr cur;
int oneW, oneH;

  cur = new pointType;

  head->blink->flink = cur;
  cur->blink = head->blink;
  head->blink = cur;
  cur->flink = head;

  if ( actWin->orthogonal ) {
    if ( cur->blink != head ) {
      if ( abs( oneX - cur->blink->x ) >= abs( oneY - cur->blink->y ) )
        oneY = cur->blink->y;
      else
        oneX = cur->blink->x;
    }
  }

  cur->x = oneX;
  cur->y = oneY;

  oneW = 3;
  oneH = 3;

  actWin->drawGc.saveFg();
  actWin->drawGc.setFG( lineColor.pixelColor() );

  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.xorGC(), oneX-oneW/2, oneY-oneH/2, oneW, oneH );

  actWin->drawGc.setLineStyle( this->lineStyle );
  actWin->drawGc.setLineWidth( this->lineWidth );

  if ( cur->blink != head ) {

    XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.xorGC(), cur->blink->x, cur->blink->y, cur->x, cur->y );

  }

  actWin->drawGc.restoreFg();
  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  return 1;

}

int activeLineClass::removeLastPoint ( void )
{

pointPtr cur;
int oneX, oneY, oneW, oneH;

//   printf( "In activeLineClass::removeLastPoint\n" );

  actWin->drawGc.saveFg();
  actWin->drawGc.setFG( lineColor.pixelColor() );

  cur = head->blink;
  if ( cur == head ) return 0;

  oneX = cur->x;
  oneY = cur->y;
  oneW = 3;
  oneH = 3;

  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  // erase old via xor gc
  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.xorGC(), oneX-oneW/2, oneY-oneH/2, oneW, oneH );

  actWin->drawGc.setLineStyle( this->lineStyle );
  actWin->drawGc.setLineWidth( this->lineWidth );

  if ( cur->blink != head ) {
    XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.xorGC(), cur->blink->x, cur->blink->y, cur->x, cur->y );
  }

  // unlink
  cur->blink->flink = head;
  head->blink = cur->blink;

  delete cur;

  actWin->drawGc.restoreFg();
  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  this->actWin->refreshGrid();

  return 1;

}

// override base class select for this object

int activeLineClass::select (
  int _x,
  int _y )
{

int effectiveW, effectiveH, small;

  if ( ( w < 5 ) && ( h < 5 ) )
    small = 1;
  else
    small = 0;

  if ( w < 5 )
    effectiveW = 5;
  else
    effectiveW = w;

  if ( h < 5 )
    effectiveH = 5;
  else
    effectiveH = h;

  if ( deleteRequest ) return 0;

  if ( small ) {
    if ( ( _x >= x-effectiveW ) && ( _x <= x+effectiveW ) &&
         ( _y >= y-effectiveH ) && ( _y <= y+effectiveH ) ) {
      selected = 1;
      return 1;
    }
    else {
      return 0;
    }
  }
  else {
    if ( ( _x >= x ) && ( _x <= x+effectiveW ) &&
         ( _y >= y ) && ( _y <= y+effectiveH ) ) {
      selected = 1;
      return 1;
    }
    else {
      return 0;
    }
  }

}

pointPtr activeLineClass::selectPoint (
  int x,
  int y )
{

pointPtr cur;
int d;

//   printf( "In activeLineClass::selectPoint\n" );
//   printf( "x = %-d, y = %-d\n", x, y );

  cur = head->flink;
  while ( cur != head ) {

    d = ( cur->x - x ) * ( cur->x - x ) + ( cur->y - y ) * ( cur->y - y );
//     printf( "d = %-d\n", d );
    if ( d <= 9 ) return cur;

    cur = cur->flink;

  }

  return (pointPtr) NULL;

}

int activeLineClass::movePoint (
  pointPtr curPoint,
  int _x,
  int _y )
{

int oneX, oneY, oneW, oneH;

//   printf( "In activeLineClass::movePoint\n" );
//   printf( "x = %-d, y = %-d\n", x, y );

  actWin->drawGc.saveFg();
  actWin->drawGc.setFG( lineColor.pixelColor() );

  oneX = curPoint->x;
  oneY = curPoint->y;

  oneW = 3;
  oneH = 3;

  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  // erase old via xor gc
  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.xorGC(), oneX-oneW/2, oneY-oneH/2, oneW, oneH );

  actWin->drawGc.setLineStyle( this->lineStyle );
  actWin->drawGc.setLineWidth( this->lineWidth );

  if ( curPoint->blink != head ) {
    XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.xorGC(), curPoint->blink->x, curPoint->blink->y,
     curPoint->x, curPoint->y );
  }

  if ( curPoint->flink != head ) {
    XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.xorGC(), curPoint->x, curPoint->y,
     curPoint->flink->x, curPoint->flink->y );
  }

  if ( actWin->orthogonal ) {
    if ( curPoint->blink != head ) {
      if ( abs( oneX - curPoint->blink->x ) >=
           abs( oneY - curPoint->blink->y ) )
        _y = curPoint->blink->y;
      else
        _x = curPoint->blink->x;
    }
  }

  curPoint->x = _x;
  curPoint->y = _y;

  oneX = curPoint->x;
  oneY = curPoint->y;

  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  // draw new
  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.xorGC(), oneX-oneW/2, oneY-oneH/2, oneW, oneH );

  actWin->drawGc.setLineStyle( this->lineStyle );
  actWin->drawGc.setLineWidth( this->lineWidth );

  if ( curPoint->blink != head ) {
    XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.xorGC(), curPoint->blink->x, curPoint->blink->y,
     curPoint->x, curPoint->y );
  }

  if ( curPoint->flink != head ) {
    XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.xorGC(), curPoint->x, curPoint->y,
     curPoint->flink->x, curPoint->flink->y );
  }

  actWin->drawGc.restoreFg();
  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  this->actWin->refreshGrid();

  return 1;

}

int activeLineClass::lineEditComplete ( void )
{

int stat;

  stat = lineEditDone();
  this->operationComplete();

  return stat;

}

int activeLineClass::lineEditCancel ( void )
{

int stat;

  stat = lineEditDone();
  this->operationCancel();

  return stat;

}

int activeLineClass::lineEditDone ( void )
{

pointPtr cur, next;
int n, oneX, oneY, oneW, oneH, minX, minY, maxX, maxY;

  oneW = 3;
  oneH = 3;

//   printf( "In activeLineClass::lineEditComplete\n" );

  actWin->drawGc.saveFg();
  actWin->drawGc.setFG( lineColor.pixelColor() );

  // erase all points, count number of points, and find rectangular
  // extent
  minX = minY = 0x7fffffff;
  maxX = maxY = -1;
  cur = head->flink;
  while ( cur != head ) {

    numPoints++;
    oneX = cur->x;
    oneY = cur->y;

    if ( minX > oneX ) minX = oneX;
    if ( minY > oneY ) minY = oneY;
    if ( maxX < oneX ) maxX = oneX;
    if ( maxY < oneY ) maxY = oneY;

    actWin->drawGc.setLineStyle( LineSolid );
    actWin->drawGc.setLineWidth( 1 );

    // erase current rectangle via xor gc
    XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.xorGC(), oneX-oneW/2, oneY-oneH/2, oneW, oneH );

    actWin->drawGc.setLineStyle( this->lineStyle );
    actWin->drawGc.setLineWidth( this->lineWidth );

    if ( cur->blink != head ) {
      XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
       actWin->drawGc.xorGC(), cur->blink->x, cur->blink->y, cur->x, cur->y );
    }

    cur = cur->flink;

  }

  // set select box size
  oneW = maxX - minX;
  oneH = maxY - minY;

  x = minX;
  y = minY;
  w = oneW;
  h = oneH;

  oldX = x;
  oldY = y;
  oldW = w;
  oldH = h;

//   printf( "minX=%-d, minY=%-d, w=%-d, h=%-d\n", x, y, w, h );

  initSelectBox();

  // build XPoint array
  xpoints = new XPoint[numPoints];
  n = 0;
  cur = head->flink;
  while ( cur != head ) {

    next = cur->flink;

    xpoints[n].x = cur->x;
    xpoints[n].y = cur->y;
    n++;

    delete cur;

    cur = next;

  }

  head->flink = head;
  head->blink = head;

  actWin->drawGc.setLineStyle( lineStyle );
  actWin->drawGc.setLineWidth( lineWidth );

  if ( fill ) {

    actWin->drawGc.setFG( fillColor.pixelColor() );

    XFillPolygon( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.normGC(), xpoints, numPoints, Complex, CoordModeOrigin );

  }

  actWin->drawGc.setFG( lineColor.pixelColor() );

  XDrawLines( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), xpoints, numPoints, CoordModeOrigin );

  actWin->drawGc.restoreFg();
  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  if ( this->wasSelected ) {
    this->setSelected();
  }

  this->refresh();

  return 1;

}

int activeLineClass::createFromFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int i, r, g, b, oneX, oneY, index;
int major, minor, release;
unsigned int pixel;
char oneName[PV_Factory::MAX_PV_NAME+1];

  this->actWin = _actWin;

  fscanf( f, "%d %d %d\n", &major, &minor, &release ); actWin->incLine();

  fscanf( f, "%d\n", &x ); actWin->incLine();
  fscanf( f, "%d\n", &y ); actWin->incLine();
  fscanf( f, "%d\n", &w ); actWin->incLine();
  fscanf( f, "%d\n", &h ); actWin->incLine();

  oldX = x;
  oldY = y;
  oldW = w;
  oldH = h;

  this->initSelectBox(); // call after getting x,y,w,h

  fscanf( f, "%d\n", &numPoints ); actWin->incLine();

  xpoints = new XPoint[numPoints];

  for ( i=0; i<numPoints; i++ ) {
    fscanf( f, "%d %d\n", &oneX, &oneY ); actWin->incLine();
    xpoints[i].x = (short) oneX;
    xpoints[i].y = (short) oneY;
  }

  if ( ( major > 2 ) || ( ( major == 2 ) && ( minor > 0 ) ) ) {

    actWin->ci->readColorIndex( f, &index );
    actWin->incLine(); actWin->incLine();
    lineColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &lineColorMode ); actWin->incLine();

    if ( lineColorMode == ALC_K_COLORMODE_ALARM )
      lineColor.setAlarmSensitive();
    else
      lineColor.setAlarmInsensitive();

    fscanf( f, "%d\n", &fill ); actWin->incLine();

    actWin->ci->readColorIndex( f, &index );
    actWin->incLine(); actWin->incLine();
    fillColor.setColorIndex( index, actWin->ci );

  }
  else if ( major > 1 ) {

    fscanf( f, "%d\n", &index ); actWin->incLine();
    lineColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &lineColorMode ); actWin->incLine();

    if ( lineColorMode == ALC_K_COLORMODE_ALARM )
      lineColor.setAlarmSensitive();
    else
      lineColor.setAlarmInsensitive();

    fscanf( f, "%d\n", &fill ); actWin->incLine();

    fscanf( f, "%d\n", &index ); actWin->incLine();
    fillColor.setColorIndex( index, actWin->ci );

  }
  else {

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    index = actWin->ci->pixIndex( pixel );
    lineColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &lineColorMode ); actWin->incLine();

    if ( lineColorMode == ALC_K_COLORMODE_ALARM )
      lineColor.setAlarmSensitive();
    else
      lineColor.setAlarmInsensitive();

    fscanf( f, "%d\n", &fill ); actWin->incLine();

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    index = actWin->ci->pixIndex( pixel );
    fillColor.setColorIndex( index, actWin->ci );

  }

  fscanf( f, "%d\n", &fillColorMode ); actWin->incLine();

  if ( fillColorMode == ALC_K_COLORMODE_ALARM )
    fillColor.setAlarmSensitive();
  else
    fillColor.setAlarmInsensitive();

  fscanf( f, "%d\n", &lineWidth ); actWin->incLine();
  fscanf( f, "%d\n", &lineStyle ); actWin->incLine();

  readStringFromFile( oneName, PV_Factory::MAX_PV_NAME+1, f );
   actWin->incLine();
  alarmPvExpStr.setRaw( oneName );

  readStringFromFile( oneName, PV_Factory::MAX_PV_NAME+1, f );
   actWin->incLine();
  visPvExpStr.setRaw( oneName );

  fscanf( f, "%d\n", &visInverted ); actWin->incLine();

  if ( ( major > 1 ) || ( minor > 0 ) ) {
    readStringFromFile( minVisString, 39+1, f ); actWin->incLine();
    readStringFromFile( maxVisString, 39+1, f ); actWin->incLine();
  }
  else {
    strcpy( minVisString, "1" );
    strcpy( maxVisString, "1" );
  }

  this->wasSelected = 0;

  return 1;

}

int activeLineClass::save (
  FILE *f )
{

int i, index;

  fprintf( f, "%-d %-d %-d\n", ALC_MAJOR_VERSION, ALC_MINOR_VERSION,
   ALC_RELEASE );
  fprintf( f, "%-d\n", x );
  fprintf( f, "%-d\n", y );
  fprintf( f, "%-d\n", w );
  fprintf( f, "%-d\n", h );

  fprintf( f, "%-d\n", numPoints );

  for ( i=0; i<numPoints; i++ ) {
    fprintf( f, "%-d %-d\n", xpoints[i].x, xpoints[i].y );
  }

  index = lineColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );
  //fprintf( f, "%-d\n", index );

  fprintf( f, "%-d\n", lineColorMode );

  fprintf( f, "%-d\n", fill );

  index =  fillColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );
  //fprintf( f, "%-d\n", index );

  fprintf( f, "%-d\n", fillColorMode );

  fprintf( f, "%-d\n", lineWidth );
  fprintf( f, "%-d\n", lineStyle );

  if ( alarmPvExpStr.getRaw() )
    writeStringToFile( f, alarmPvExpStr.getRaw() );
  else
    writeStringToFile( f, "" );

  if ( visPvExpStr.getRaw() )
    writeStringToFile( f, visPvExpStr.getRaw() );
  else
    writeStringToFile( f, "" );

  fprintf( f, "%-d\n", visInverted );
  writeStringToFile( f, minVisString );
  writeStringToFile( f, maxVisString );

  return 1;

}

int activeLineClass::drawActive ( void )
{

int blink = 0;

  if ( !init ) {
    if ( needToDrawUnconnected ) {
      actWin->executeGc.saveFg();
      //actWin->executeGc.setFG( lineColor.getDisconnected() );
      actWin->executeGc.setFG( lineColor.getDisconnectedIndex(), &blink );
      actWin->executeGc.setLineWidth( 1 );
      actWin->executeGc.setLineStyle( LineSolid );
      XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
       actWin->executeGc.normGC(), x, y, w, h );
      actWin->executeGc.restoreFg();
      needToEraseUnconnected = 1;
      updateBlink( blink );
    }
  }
  else if ( needToEraseUnconnected ) {
    actWin->executeGc.setLineWidth( 1 );
    actWin->executeGc.setLineStyle( LineSolid );
    XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), x, y, w, h );
    needToEraseUnconnected = 0;
  }

  if ( !activeMode || !visibility ) return 1;

  prevVisibility = visibility;

  if ( numPoints > 0 ) {

    actWin->executeGc.setLineStyle( lineStyle );
    actWin->executeGc.setLineWidth( lineWidth );

    actWin->executeGc.saveFg();

    if ( fill && fillVisibility ) {

      //actWin->executeGc.setFG( fillColor.getColor() );
      actWin->executeGc.setFG( fillColor.getIndex(), &blink );

      XFillPolygon( actWin->d, XtWindow(actWin->executeWidget),
       actWin->executeGc.normGC(), xpoints, numPoints, Complex,
       CoordModeOrigin );

    }

    if ( lineVisibility ) {
      //actWin->executeGc.setFG( lineColor.getColor() );
      actWin->executeGc.setFG( lineColor.getIndex(), &blink );
      XDrawLines( actWin->d, XtWindow(actWin->executeWidget),
       actWin->executeGc.normGC(), xpoints, numPoints, CoordModeOrigin );
    }

    actWin->executeGc.restoreFg();
    actWin->executeGc.setLineStyle( LineSolid );
    actWin->executeGc.setLineWidth( 1 );

  }

  updateBlink( blink );

  return 1;

}

int activeLineClass::eraseUnconditional ( void )
{

  if ( numPoints > 0 ) {

    actWin->executeGc.setLineStyle( lineStyle );
    actWin->executeGc.setLineWidth( lineWidth );

    if ( fill ) {

      XFillPolygon( actWin->d, XtWindow(actWin->executeWidget),
       actWin->executeGc.eraseGC(), xpoints, numPoints, Complex,
       CoordModeOrigin );

    }

    XDrawLines( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), xpoints, numPoints, CoordModeOrigin );

    actWin->executeGc.setLineStyle( LineSolid );
    actWin->executeGc.setLineWidth( 1 );

  }

  return 1;

}

int activeLineClass::eraseActive ( void )
{

  if ( !activeMode ) return 1;

  if ( prevVisibility == 0 ) {
    prevVisibility = visibility;
    return 1;
  }

  prevVisibility = visibility;

  if ( numPoints > 0 ) {

    actWin->executeGc.setLineStyle( lineStyle );
    actWin->executeGc.setLineWidth( lineWidth );

    if ( fill ) {

      XFillPolygon( actWin->d, XtWindow(actWin->executeWidget),
       actWin->executeGc.eraseGC(), xpoints, numPoints, Complex,
       CoordModeOrigin );

    }

    XDrawLines( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), xpoints, numPoints, CoordModeOrigin );

    actWin->executeGc.setLineStyle( LineSolid );
    actWin->executeGc.setLineWidth( 1 );

  }

  return 1;

}

int activeLineClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat;

  stat = alarmPvExpStr.expand1st( numMacros, macros, expansions );
  stat = visPvExpStr.expand1st( numMacros, macros, expansions );

  return stat;

}

int activeLineClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat;

  stat = alarmPvExpStr.expand2nd( numMacros, macros, expansions );
  stat = visPvExpStr.expand2nd( numMacros, macros, expansions );

  return stat;

}

int activeLineClass::containsMacros ( void ) {

  if ( alarmPvExpStr.containsPrimaryMacros() ) return 1;
  if ( visPvExpStr.containsPrimaryMacros() ) return 1;

  return 0;

}

int activeLineClass::activate (
  int pass,
  void *ptr )
{

  switch ( pass ) {

  case 1: // initialize

    opComplete = 0;

    break;

  case 2: // connect to pv's

    if ( !opComplete ) {

      connection.init();

      curLineColorIndex = -1;
      curFillColorIndex = -1;
      curStatus = -1;
      curSeverity = -1;
      prevVisibility = -1;
      visibility = 0;
      prevLineVisibility = -1;
      lineVisibility = 0;
      prevFillVisibility = -1;
      fillVisibility = 0;

      needConnectInit = needAlarmUpdate = needVisUpdate = needRefresh = 0;

      needToEraseUnconnected = 0;
      needToDrawUnconnected = 0;
      unconnectedTimer = 0;

      if ( !unconnectedTimer ) {
        unconnectedTimer = XtAppAddTimeOut( actWin->appCtx->appContext(),
         2000, unconnectedTimeout, this );
      }

      aglPtr = ptr;

      alarmPvId = visPvId = 0;

      activeMode = 1;
      pvType = -1;

      init = 1; // this stays true if there are no pvs

      if ( !alarmPvExpStr.getExpanded() ||
           ( strcmp( alarmPvExpStr.getExpanded(), "" ) == 0 ) ) {
        alarmPvExists = 0;
        lineVisibility = fillVisibility = 1;
      }
      else {
        connection.addPv();
        alarmPvExists = 1;
        lineColor.setConnectSensitive();
        fillColor.setConnectSensitive();
        init = 0;
      }

      if ( !visPvExpStr.getExpanded() ||
           ( strcmp( visPvExpStr.getExpanded(), "" ) == 0 ) ) {
        visPvExists = 0;
        visibility = 1;
      }
      else {
        connection.addPv();
        visPvExists = 1;
        visibility = 0;
        lineVisibility = fillVisibility = 1;
        lineColor.setConnectSensitive();
        fillColor.setConnectSensitive();
        init = 0;
      }

      if ( alarmPvExists ) {
        alarmPvId = pv_factory->create( alarmPvExpStr.getExpanded() );
        if ( alarmPvId ) {
          if ( alarmPvId->is_valid() ) {
            alarmPvConnectStateCallback( alarmPvId, this );
            alarmPvValueCallback( alarmPvId, this );
	  }
          alarmPvId->add_conn_state_callback( alarmPvConnectStateCallback,
           this );
          alarmPvId->add_value_callback( alarmPvValueCallback, this );
	}
      }

      if ( visPvExists ) {
        visPvId = pv_factory->create( visPvExpStr.getExpanded() );
        if ( visPvId ) {
          if ( visPvId->is_valid() ) {
            visPvConnectStateCallback( visPvId, this );
            visPvValueCallback( visPvId, this );
          }
          visPvId->add_conn_state_callback( visPvConnectStateCallback, this );
          visPvId->add_value_callback( visPvValueCallback, this );
	}
      }

      opComplete = 1;

    }

    break;

  case 3:
  case 4:
  case 5:
  case 6:

    break;

  }

  return 1;

}

int activeLineClass::deactivate (
  int pass )
{

  if ( pass == 1 ) {

    activeMode = 0;

    updateBlink( 0 );

    if ( alarmPvId ) {
      alarmPvId->remove_conn_state_callback( alarmPvConnectStateCallback,
       this );
      alarmPvId->remove_value_callback( alarmPvValueCallback, this );
      alarmPvId->release();
      alarmPvId = 0;
    }

    if ( visPvId ) {
      visPvId->remove_conn_state_callback( visPvConnectStateCallback, this );
      visPvId->remove_value_callback( visPvValueCallback, this );
      visPvId->release();
      visPvId = 0;
    }

  }

  return 1;

}

int activeLineClass::draw ( void ) {

int blink = 0;

  if ( activeMode ) return 1;
  if ( deleteRequest ) return 1;

  actWin->drawGc.saveFg();

  if ( numPoints > 0 ) {

    actWin->drawGc.setLineStyle( lineStyle );
    actWin->drawGc.setLineWidth( lineWidth );

    if ( fill ) {

      //actWin->drawGc.setFG( fillColor.pixelColor() );
      actWin->drawGc.setFG( fillColor.pixelIndex(), &blink );

      XFillPolygon( actWin->d, XtWindow(actWin->drawWidget),
       actWin->drawGc.normGC(), xpoints, numPoints, Complex,
       CoordModeOrigin );

    }

    //actWin->drawGc.setFG( lineColor.pixelColor() );
    actWin->drawGc.setFG( lineColor.pixelIndex(), &blink );

    XDrawLines( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.normGC(), xpoints, numPoints, CoordModeOrigin );

    actWin->drawGc.restoreFg();
    actWin->drawGc.setLineStyle( LineSolid );
    actWin->drawGc.setLineWidth( 1 );

  }

  updateBlink( blink );

  return 1;

}

int activeLineClass::erase ( void )
{

  if ( activeMode ) return 1;
  if ( deleteRequest ) return 1;

  if ( numPoints > 0 ) {

    actWin->drawGc.setLineStyle( lineStyle );
    actWin->drawGc.setLineWidth( lineWidth );

    if ( fill ) {

      XFillPolygon( actWin->d, XtWindow(actWin->drawWidget),
       actWin->drawGc.eraseGC(), xpoints, numPoints, Complex,
       CoordModeOrigin );

    }

    XDrawLines( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.eraseGC(), xpoints, numPoints, CoordModeOrigin );

    actWin->drawGc.setLineStyle( LineSolid );
    actWin->drawGc.setLineWidth( 1 );

  }

  return 1;

}

void activeLineClass::updateDimensions ( void )
{

int i, xTranslate, yTranslate, newX, newY;
float xStretch, yStretch;

  xTranslate = x - oldX;
  yTranslate = y - oldY;
  if ( !oldW ) oldW = 1;
  xStretch = (float) w / (float) oldW;
  if ( !oldH ) oldH = 1;
  yStretch = (float) h / (float) oldH;

  for ( i=0; i<numPoints; i++ ) {

    newX = oldX + xTranslate +
     (int) ( ( (float) xpoints[i].x - (float) oldX ) * xStretch + 0.5 );
    if ( newX < x ) newX = x;
    if ( newX > ( x + w ) ) newX = x + w;

    xpoints[i].x = newX;

    newY = oldY + yTranslate +
     (int) ( ( (float) xpoints[i].y - (float) oldY ) * yStretch + 0.5 );
    if ( newY < y ) newY = y;
    if ( newY > ( y + h ) ) newY = y + h;

    xpoints[i].y = newY;

  }

  oldX = x;
  oldY = y;
  oldW = w;
  oldH = h;

}

int activeLineClass::checkResizeSelectBox (
  int _x,
  int _y,
  int _w,
  int _h )
{

int tmpx, tmpy, tmpw, tmph, ret_stat;

  tmpx = sboxX;
  tmpy = sboxY;
  tmpw = sboxW;
  tmph = sboxH;

  ret_stat = 1;

  tmpx += _x;
  tmpy += _y;

  tmpw += _w;
  if ( tmpw < 0 ) {
    ret_stat = 0;
  }

  tmph += _h;
  if ( tmph < 0 ) {
    ret_stat = 0;
  }

  return ret_stat;

}

int activeLineClass::checkResizeSelectBoxAbs (
  int _x,
  int _y,
  int _w,
  int _h )
{

int tmpx, tmpy, tmpw, tmph, ret_stat;

  tmpx = sboxX;
  tmpy = sboxY;
  tmpw = sboxW;
  tmph = sboxH;

  ret_stat = 1;

  if ( tmpw != -1 ) {
    if ( tmpw < 0 ) {
      ret_stat = 0;
    }
  }

  if ( tmph != -1 ) {
    if ( tmph < 0 ) {
      ret_stat = 0;
    }
  }

  return ret_stat;

}

int activeLineClass::resizeSelectBox (
  int _x,
  int _y,
  int _w,
  int _h )
{

int savex, savey, savew, saveh, ret_stat;

  savex = sboxX;
  savey = sboxY;
  savew = sboxW;
  saveh = sboxH;

  ret_stat = 1;

  sboxX += _x;
  sboxY += _y;

  sboxW += _w;
  if ( sboxW < 0 ) {
    sboxX = savex;
    sboxW = savew;
    ret_stat = 0;
  }

  sboxH += _h;
  if ( sboxH < 0 ) {
    sboxY = savey;
    sboxH = saveh;
    ret_stat = 0;
  }

  return ret_stat;

}

int activeLineClass::resizeSelectBoxAbs (
  int _x,
  int _y,
  int _w,
  int _h )
{

int savex, savey, savew, saveh, ret_stat;

  savex = sboxX;
  savey = sboxY;
  savew = sboxW;
  saveh = sboxH;

  ret_stat = 1;

  if ( _x >= 0 ) sboxX = _x;
  if ( _y >= 0 ) sboxY = _y;

  if ( _w >= 0 ) {
    sboxW = _w;
    if ( sboxW < 0 ) {
      sboxX = savex;
      sboxW = savew;
      ret_stat = 0;
    }
  }

  if ( _h >= 0 ) {
    sboxH = _h;
    if ( sboxH < 0 ) {
      sboxY = savey;
      sboxH = saveh;
      ret_stat = 0;
    }
  }

  return ret_stat;

}

int activeLineClass::rotate (
  int xOrigin,
  int yOrigin,
  char direction ) // '+'=clockwise, '-'=counter clockwise
{

int i;
double dx0, dy0, dxOrig, dyOrig, dxPrime0, dyPrime0;

  // execute base class rotate
  ((activeGraphicClass *)this)->activeGraphicClass::rotate(
   xOrigin, yOrigin, direction );

  oldX = x;
  oldY = y;
  oldW = w;
  oldH = h;

  //printf( "activeLineClass::rotate %c, xO=%-d, yO=%-d\n",
  // direction, xOrigin, yOrigin );

  dxOrig = (double) xOrigin;
  dyOrig = (double) yOrigin;


  for ( i=0; i<numPoints; i++ ) {

    if ( direction == '+' ) { // clockwise

      // translate
      dx0 = (double) ( xpoints[i].x - dxOrig );
      dy0 = (double) ( dyOrig - xpoints[i].y );

      //printf( "1 dx0=%-g, dy0=%-g\n", dx0, dy0 );

      // rotate
      dxPrime0 = dy0;
      dyPrime0 = dx0 * -1.0;

      //printf( "2 dxPrime0=%-g, dyPrime0=%-g\n", dxPrime0, dyPrime0 );

      // translate
      dxPrime0 += dxOrig;
      dyPrime0 = dyOrig - dyPrime0;

      //printf( "3 dxPrime0=%-g, dyPrime0=%-g\n", dxPrime0, dyPrime0 );

      // set final x, y
      xpoints[i].x = (short) dxPrime0;
      xpoints[i].y = (short) dyPrime0;

    }
    else { // counterclockwise

      // translate
      dx0 = (double) ( xpoints[i].x - dxOrig );
      dy0 = (double) ( dyOrig - xpoints[i].y );

      //printf( "1 dx0=%-g, dy0=%-g\n", dx0, dy0 );

      // rotate
      dxPrime0 = dy0 * -1.0;
      dyPrime0 = dx0;

      //printf( "2 dxPrime0=%-g, dyPrime0=%-g\n", dxPrime0, dyPrime0 );

      // translate
      dxPrime0 += dxOrig;
      dyPrime0 = dyOrig - dyPrime0;

      //printf( "3 dxPrime0=%-g, dyPrime0=%-g\n", dxPrime0, dyPrime0 );

      // set final x, y
      xpoints[i].x = (short) dxPrime0;
      xpoints[i].y = (short) dyPrime0;

    }

  }

  return 1;

}

int activeLineClass::flip (
  int xOrigin,
  int yOrigin,
  char direction )
{

int i;
double dx0, dy0, dxOrig, dyOrig, dxPrime0, dyPrime0;

  //printf( "activeLineClass::flip %c, xO=%-d, yO=%-d\n",
  // direction, xOrigin, yOrigin );

  // execute base class flip
  ((activeGraphicClass *)this)->activeGraphicClass::flip(
   xOrigin, yOrigin, direction );

  oldX = x;
  oldY = y;
  oldW = w;
  oldH = h;

  dxOrig = (double) xOrigin;
  dyOrig = (double) yOrigin;


  for ( i=0; i<numPoints; i++ ) {

    //printf( "%-d: x=%-d, y=%-d\n", i, xpoints[i].x, xpoints[i].y );

    if ( direction == 'H' ) { // horizontal

      // translate
      dx0 = (double) ( xpoints[i].x - dxOrig );

      //printf( "1 dx0=%-g\n", dx0 );

      // move
      dxPrime0 = dx0 * -1.0;

      //printf( "2 dxPrime0=%-g\n", dxPrime0 );

      // translate
      dxPrime0 += dxOrig;

      //printf( "3 dxPrime0=%-g\n", dxPrime0 );

      // set final x
      xpoints[i].x = (short) dxPrime0;

    }
    else { // vertical

      // translate
      dy0 = (double) ( dyOrig - xpoints[i].y );

      //printf( "1 dx0=%-g, dy0=%-g\n", dx0, dy0 );

      // move
      dyPrime0 = dy0 * -1.0;

      //printf( "2 dxPrime0=%-g, dyPrime0=%-g\n", dxPrime0, dyPrime0 );

      // translate
      dyPrime0 = dyOrig - dyPrime0;

      //printf( "3 dxPrime0=%-g, dyPrime0=%-g\n", dxPrime0, dyPrime0 );

      // set final y
      xpoints[i].y = (short) dyPrime0;

    }

  }

  return 1;

}

int activeLineClass::isMultiPointObject ( void ) {

  return 1;

}

void activeLineClass::executeDeferred ( void ) {


int stat, nc, nau, nvu, nr, index, change;
pvValType pvV;

  if ( actWin->isIconified ) return;

  actWin->appCtx->proc->lock();
  nc = needConnectInit; needConnectInit = 0;
  nau = needAlarmUpdate; needAlarmUpdate = 0;
  nvu = needVisUpdate; needVisUpdate = 0;
  nr = needRefresh; needRefresh = 0;
  actWin->remDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

  if ( !activeMode ) return;

  if ( nc ) {

    minVis.d = (double) atof( minVisString );
    maxVis.d = (double) atof( maxVisString );

    lineColor.setConnected();
    fillColor.setConnected();

    if ( alarmPvExists ) {

      curStatus = alarmPvId->get_status();
      curSeverity = alarmPvId->get_severity();

      lineColor.setStatus( curStatus, curSeverity );
      fillColor.setStatus( curStatus, curSeverity );

      curLineColorIndex = actWin->ci->evalRule( lineColor.pixelIndex(),
       alarmPvId->get_double() );
      lineColor.changeIndex( curLineColorIndex, actWin->ci );

      curFillColorIndex = actWin->ci->evalRule( fillColor.pixelIndex(),
       alarmPvId->get_double() );
      fillColor.changeIndex( curFillColorIndex, actWin->ci );

      if ( !visPvExists ) {

        if ( actWin->ci->isInvisible( curLineColorIndex ) ) {
          prevLineVisibility = lineVisibility = 0;
        }
        else {
          prevLineVisibility = lineVisibility = 1;
        }

        if ( actWin->ci->isInvisible( curFillColorIndex ) ) {
          prevFillVisibility = fillVisibility = 0;
        }
        else {
          prevFillVisibility = fillVisibility = 1;
        }

      }

    }

    if ( visPvExists ) {

      pvV.d = visPvId->get_double();
      if ( ( pvV.d >= minVis.d ) && ( pvV.d < maxVis.d ) )
        visibility = 1 ^ visInverted;
      else
        visibility = 0 ^ visInverted;

      prevVisibility = visibility;

    }

    init = 1;

    eraseUnconditional();
    stat = smartDrawAllActive();

  }

  if ( nau ) {

    change = 0;

    if ( curStatus != alarmPvId->get_status() ) {
      curStatus = alarmPvId->get_status();
      change = 1;
    }

    if ( curSeverity != alarmPvId->get_severity() ) {
      curSeverity = alarmPvId->get_severity();
      change = 1;
    }

    if ( change ) {
      lineColor.setStatus( curStatus, curSeverity );
      fillColor.setStatus( curStatus, curSeverity );
    }

    index = actWin->ci->evalRule( lineColor.pixelIndex(),
    alarmPvId->get_double() );

    if ( curLineColorIndex != index ) {
      curLineColorIndex = index;
      change = 1;
    }

    index = actWin->ci->evalRule( fillColor.pixelIndex(),
    alarmPvId->get_double() );

    if ( curFillColorIndex != index ) {
      curFillColorIndex = index;
      change = 1;
    }

    if ( change ) {

      if ( !visPvExists ) {

        if ( actWin->ci->isInvisible( curLineColorIndex ) ) {
          lineVisibility = 0;
        }
        else {
          lineVisibility = 1;
        }

        if ( actWin->ci->isInvisible( curFillColorIndex ) ) {
          fillVisibility = 0;
        }
        else {
          fillVisibility = 1;
        }

      }

      lineColor.changeIndex( curLineColorIndex, actWin->ci );
      fillColor.changeIndex( curFillColorIndex, actWin->ci );
      if ( ( prevLineVisibility != lineVisibility ) ||
	   ( prevFillVisibility != fillVisibility ) ) {
	prevLineVisibility = lineVisibility;
	prevFillVisibility = fillVisibility;
        eraseActive();
      }
      smartDrawAllActive();

    }

  }

  if ( nvu ) {

    pvV.d = visPvId->get_double();
    if ( ( pvV.d >= minVis.d ) && ( pvV.d < maxVis.d ) )
      visibility = 1 ^ visInverted;
    else
      visibility = 0 ^ visInverted;

    if ( prevVisibility != visibility ) {
      if ( !visibility ) eraseActive();
      stat = smartDrawAllActive();
    }

  }

  if ( nr ) {
    stat = smartDrawAllActive();
  }

}

char *activeLineClass::firstDragName ( void ) {

  dragIndex = 0;
  return dragName[dragIndex];

}

char *activeLineClass::nextDragName ( void ) {

  if ( dragIndex < (int) ( sizeof(dragName) / sizeof(char *) ) - 1 ) {
    dragIndex++;
    return dragName[dragIndex];
  }
  else {
    return NULL;
  }

}

char *activeLineClass::dragValue (
  int i ) {

  switch ( i ) {

  case 1:
    return alarmPvExpStr.getExpanded();
    break;

  case 2:
    return visPvExpStr.getExpanded();
    break;

  }

  // else, disabled

  return (char *) NULL;

}

void activeLineClass::changeDisplayParams (
  unsigned int _flag,
  char *_fontTag,
  int _alignment,
  char *_ctlFontTag,
  int _ctlAlignment,
  char *_btnFontTag,
  int _btnAlignment,
  int _textFgColor,
  int _fg1Color,
  int _fg2Color,
  int _offsetColor,
  int _bgColor,
  int _topShadowColor,
  int _botShadowColor )
{

  if ( _flag & ACTGRF_FG1COLOR_MASK )
    lineColor.setColorIndex( _fg1Color, actWin->ci );

  if ( _flag & ACTGRF_BGCOLOR_MASK )
    fillColor.setColorIndex( _bgColor, actWin->ci );

}

void activeLineClass::changePvNames (
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
  char *alarmPvs[] )
{

  if ( flag & ACTGRF_VISPVS_MASK ) {
    if ( numVisPvs ) {
      visPvExpStr.setRaw( visPvs[0] );
    }
  }

  if ( flag & ACTGRF_ALARMPVS_MASK ) {
    if ( numAlarmPvs ) {
      alarmPvExpStr.setRaw( alarmPvs[0] );
    }
  }

}

void activeLineClass::updateColors (
  double colorValue )
{

int index, change;

  index = actWin->ci->evalRule( lineColor.pixelIndex(), colorValue );

  if ( curLineColorIndex != index ) {
    curLineColorIndex = index;
    change = 1;
  }

  index = actWin->ci->evalRule( fillColor.pixelIndex(), colorValue );

  if ( curFillColorIndex != index ) {
    curFillColorIndex = index;
    change = 1;
  }

  if ( change ) {

    if ( actWin->ci->isInvisible( curLineColorIndex ) ) {
      lineVisibility = 0;
    }
    else {
      lineVisibility = 1;
    }

    if ( actWin->ci->isInvisible( curFillColorIndex ) ) {
      fillVisibility = 0;
    }
    else {
      fillVisibility = 1;
    }

    lineColor.changeIndex( curLineColorIndex, actWin->ci );
    fillColor.changeIndex( curFillColorIndex, actWin->ci );
    if ( ( prevLineVisibility != lineVisibility ) ||
         ( prevFillVisibility != fillVisibility ) ) {
      prevLineVisibility = lineVisibility;
      prevFillVisibility = fillVisibility;
    }

  }

}

int activeLineClass::addUndoRotateNode ( 
  undoClass *undoObj
) {

int stat;
undoLineOpClass *ptr;

  ptr = new undoLineOpClass( numPoints, xpoints );

  stat = undoObj->addRotateNode( this, ptr, x, y, w, h );
  return stat;

}

int activeLineClass::addUndoFlipNode (
  undoClass *undoObj
) {

int stat;

  stat = addUndoRotateNode( undoObj );
  return stat;

}

int activeLineClass::undoRotate (
  undoOpClass *_opPtr,
  int _x,
  int _y,
  int _w,
  int _h )
{

undoLineOpClass *opPtr = (undoLineOpClass *) _opPtr;
int i;

  for ( i=0; i<opPtr->n; i++ ) {
    xpoints[i].x = opPtr->x[i];
    xpoints[i].y = opPtr->y[i];
  }

  oldX = _x;
  oldY = _y;
  oldW = _w;
  oldH = _h;
  resizeAbs( _x, _y, _w, _h );
  resizeSelectBoxAbs( _x, _y, _w, _h );

  return 1;

}

int activeLineClass::undoFlip (
  undoOpClass *_opPtr,
  int x,
  int y,
  int w,
  int h )
{

int stat;

  stat = undoRotate( _opPtr, x, y, w, h );

  return stat;

}

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeLineClassPtr ( void ) {

activeLineClass *ptr;

  ptr = new activeLineClass;
  return (void *) ptr;

}

void *clone_activeLineClassPtr (
  void *_srcPtr )
{

activeLineClass *ptr, *srcPtr;

  srcPtr = (activeLineClass *) _srcPtr;

  ptr = new activeLineClass( srcPtr );

  return (void *) ptr;

}

#ifdef __cplusplus
}
#endif
