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

#define __updownButton_cc 1

#include "updownButton.h"
#include "app_pkg.h"
#include "act_win.h"

#include "thread.h"

static void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  if ( !udbto->init ) {
    udbto->needToDrawUnconnected = 1;
    udbto->needDraw = 1;
    udbto->actWin->addDefExeNode( udbto->aglPtr );
  }

  udbto->unconnectedTimer = 0;

}

static void udbtoCancelKp (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  udbto->keyPadOpen = 0;

}

static void udbtoSetKpDoubleValue (
  Widget w,
  XtPointer client,
  XtPointer call )
{

int stat;
double v;
activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  udbto->keyPadOpen = 0;

  if ( udbto->kpDest == udbto->kpCoarseDest ) {
    udbto->coarse = udbto->kpDouble;
  }
  else if ( udbto->kpDest == udbto->kpFineDest ) {
    udbto->fine = udbto->kpDouble;
  }
  else if ( udbto->kpDest == udbto->kpRateDest ) {
    udbto->rate = udbto->kpDouble;
    udbto->incrementTimerValue = (int) ( 1000.0 * udbto->rate );
    if ( udbto->incrementTimerValue < 50 ) udbto->incrementTimerValue = 50;
  }
  else if ( udbto->kpDest == udbto->kpValueDest ) {
    if ( udbto->destExists ) {
      if ( udbto->kpDouble < udbto->minDv ) {
        v = udbto->minDv;
      }
      else if ( udbto->kpDouble > udbto->maxDv ) {
        v = udbto->maxDv;
      }
      else {
	v = udbto->kpDouble;
      }
#ifdef __epics__
      stat = ca_put( DBR_DOUBLE, udbto->destPvId, &v );
#endif
    }
  }

}

static void menu_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

int stat;
double v;
activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  if ( w == udbto->pbCoarse ) {

    udbto->kpDest = udbto->kpCoarseDest;
    udbto->kp.create( udbto->actWin->top,
     udbto->x+udbto->actWin->x+udbto->w,
     udbto->y+udbto->actWin->y+10, "", &udbto->kpDouble,
     (void *) client,
     (XtCallbackProc) udbtoSetKpDoubleValue,
     (XtCallbackProc) udbtoCancelKp );
    udbto->keyPadOpen = 1;

  }
  else if ( w == udbto->pbFine ) {

    udbto->kpDest = udbto->kpFineDest;
    udbto->kp.create( udbto->actWin->top,
     udbto->x+udbto->actWin->x+udbto->w,
     udbto->y+udbto->actWin->y+10, "", &udbto->kpDouble,
     (void *) client,
     (XtCallbackProc) udbtoSetKpDoubleValue,
     (XtCallbackProc) udbtoCancelKp );
    udbto->keyPadOpen = 1;

  }
  else if ( w == udbto->pbRate ) {

    udbto->kpDest = udbto->kpRateDest;
    udbto->kp.create( udbto->actWin->top,
     udbto->x+udbto->actWin->x+udbto->w,
     udbto->y+udbto->actWin->y+10, "", &udbto->kpDouble,
     (void *) client,
     (XtCallbackProc) udbtoSetKpDoubleValue,
     (XtCallbackProc) udbtoCancelKp );
    udbto->keyPadOpen = 1;

  }
  else if ( w == udbto->pbValue ) {

    udbto->kpDest = udbto->kpValueDest;
    udbto->kp.create( udbto->actWin->top,
     udbto->x+udbto->actWin->x+udbto->w,
     udbto->y+udbto->actWin->y+10, "", &udbto->kpDouble,
     (void *) client,
     (XtCallbackProc) udbtoSetKpDoubleValue,
     (XtCallbackProc) udbtoCancelKp );
    udbto->keyPadOpen = 1;

  }
  else if ( w == udbto->pbSave ) {

    if ( udbto->savePvConnected ) {
      stat = ca_put( DBR_DOUBLE, udbto->savePvId, &udbto->curControlV );
    }
    else {
      XBell( udbto->actWin->d, 50 );
    }

  }
  else if ( w == udbto->pbRestore ) {

    if ( udbto->savePvConnected ) {
      if ( udbto->curSaveV < udbto->minDv ) {
        v = udbto->minDv;
      }
      else if ( udbto->curSaveV > udbto->maxDv ) {
        v = udbto->maxDv;
      }
      else {
	v = udbto->curSaveV;
      }
      stat = ca_put( DBR_DOUBLE, udbto->destPvId, &v );
    }
    else {
      XBell( udbto->actWin->d, 50 );
    }

  }

}

static void udbtc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  udbto->actWin->setChanged();

  udbto->eraseSelectBoxCorners();
  udbto->erase();

  udbto->fgColor.setColorIndex( udbto->bufFgColor, udbto->actWin->ci );

  udbto->bgColor.setColorIndex( udbto->bufBgColor, udbto->actWin->ci );

  udbto->topShadowColor = udbto->bufTopShadowColor;
  udbto->botShadowColor = udbto->bufBotShadowColor;

  udbto->destPvExpString.setRaw( udbto->bufDestPvName );

  udbto->savePvExpString.setRaw( udbto->bufSavePvName );

  udbto->fineExpString.setRaw( udbto->bufFine );

  udbto->coarseExpString.setRaw( udbto->bufCoarse );

  udbto->label.setRaw( udbto->bufLabel );

  strncpy( udbto->fontTag, udbto->fm.currentFontTag(), 63 );
  udbto->actWin->fi->loadFontTag( udbto->fontTag );
  udbto->fs = udbto->actWin->fi->getXFontStruct( udbto->fontTag );

  udbto->_3D = udbto->buf3D;

  udbto->invisible = udbto->bufInvisible;

  udbto->rate = udbto->bufRate;

  udbto->limitsFromDb = udbto->bufLimitsFromDb;

  udbto->efScaleMin = udbto->bufEfScaleMin;
  udbto->efScaleMax = udbto->bufEfScaleMax;

  udbto->minDv = udbto->scaleMin = udbto->efScaleMin.value();
  udbto->maxDv = udbto->scaleMax = udbto->efScaleMax.value();

  udbto->visPvExpString.setRaw( udbto->bufVisPvName );
  strncpy( udbto->minVisString, udbto->bufMinVisString, 39 );
  strncpy( udbto->maxVisString, udbto->bufMaxVisString, 39 );

  if ( udbto->bufVisInverted )
    udbto->visInverted = 0;
  else
    udbto->visInverted = 1;

  udbto->x = udbto->bufX;
  udbto->sboxX = udbto->bufX;

  udbto->y = udbto->bufY;
  udbto->sboxY = udbto->bufY;

  udbto->w = udbto->bufW;
  udbto->sboxW = udbto->bufW;

  udbto->h = udbto->bufH;
  udbto->sboxH = udbto->bufH;

  udbto->updateDimensions();

}

static void udbtc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  udbtc_edit_update ( w, client, call );
  udbto->refresh( udbto );

}

static void udbtc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  udbtc_edit_update ( w, client, call );
  udbto->ef.popdown();
  udbto->operationComplete();

}

static void udbtc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  udbto->ef.popdown();
  udbto->operationCancel();

}

static void udbtc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;

  udbto->ef.popdown();
  udbto->operationCancel();
  udbto->erase();
  udbto->deleteRequest = 1;
  udbto->drawAll();

}

#ifdef __epics__

static void udbtc_monitor_dest_connect_state (
  struct connection_handler_args arg )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    udbto->needConnectInit = 1;

  }
  else {

    udbto->connection.setPvDisconnected( (void *) udbto->destPvConnection );
    udbto->active = 0;
    udbto->bgColor.setDisconnected();
    udbto->needDraw = 1;

  }

  udbto->actWin->appCtx->proc->lock();
  udbto->actWin->addDefExeNode( udbto->aglPtr );
  udbto->actWin->appCtx->proc->unlock();

}

static void udbtc_monitor_save_connect_state (
  struct connection_handler_args arg )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    udbto->needSaveConnectInit = 1;
    udbto->actWin->appCtx->proc->lock();
    udbto->actWin->addDefExeNode( udbto->aglPtr );
    udbto->actWin->appCtx->proc->unlock();

  }
  else {

    udbto->savePvConnected = 0;

  }

}

static void udbt_infoUpdate (
  struct event_handler_args ast_args )
{

activeUpdownButtonClass *udbto =
 (activeUpdownButtonClass *) ca_puser(ast_args.chid);
struct dbr_gr_double controlRec = *( (dbr_gr_double *) ast_args.dbr );

  if ( udbto->limitsFromDb || udbto->efScaleMin.isNull() ) {
    udbto->scaleMin = controlRec.lower_disp_limit;
  }

  if ( udbto->limitsFromDb || udbto->efScaleMax.isNull() ) {
    udbto->scaleMax = controlRec.upper_disp_limit;
  }

  udbto->minDv = udbto->scaleMin;

  udbto->maxDv = udbto->scaleMax;

  udbto->curControlV = controlRec.value;

  udbto->needCtlInfoInit = 1;
  udbto->actWin->appCtx->proc->lock();
  udbto->actWin->addDefExeNode( udbto->aglPtr );
  udbto->actWin->appCtx->proc->unlock();

}

static void udbtc_controlUpdate (
  struct event_handler_args ast_args )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) ast_args.usr;

  udbto->actWin->appCtx->proc->lock();

  udbto->curControlV = *( (double *) ast_args.dbr );

  if ( udbto->savePvConnected ) {
    if ( !udbto->isSaved && ( udbto->curControlV == udbto->curSaveV ) ) {
      udbto->isSaved = 1;
      udbto->needRefresh = 1;
      udbto->actWin->addDefExeNode( udbto->aglPtr );
    }
    else if ( udbto->isSaved && ( udbto->curControlV != udbto->curSaveV ) ) {
      udbto->isSaved = 0;
      udbto->needRefresh = 1;
      udbto->actWin->addDefExeNode( udbto->aglPtr );
    }
  }

  udbto->actWin->appCtx->proc->unlock();

}

static void udbtc_saveUpdate (
  struct event_handler_args ast_args )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) ast_args.usr;

  udbto->actWin->appCtx->proc->lock();

  udbto->curSaveV = *( (double *) ast_args.dbr );

  if ( !udbto->isSaved && ( udbto->curControlV == udbto->curSaveV ) ) {
    udbto->isSaved = 1;
    udbto->needRefresh = 1;
    udbto->actWin->addDefExeNode( udbto->aglPtr );
  }
  else if ( udbto->isSaved && ( udbto->curControlV != udbto->curSaveV ) ) {
    udbto->isSaved = 0;
    udbto->needRefresh = 1;
    udbto->actWin->addDefExeNode( udbto->aglPtr );
  }

  udbto->actWin->appCtx->proc->unlock();

  udbto->savePvConnected = 1;

}

static void udbtc_monitor_vis_connect_state (
  struct connection_handler_args arg )
{

activeUpdownButtonClass *udbto =
 (activeUpdownButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    udbto->needVisConnectInit = 1;

  }
  else {

    udbto->connection.setPvDisconnected( (void *) udbto->visPvConnection );
    udbto->active = 0;
    udbto->bgColor.setDisconnected();
    udbto->needDraw = 1;

  }

  udbto->actWin->appCtx->proc->lock();
  udbto->actWin->addDefExeNode( udbto->aglPtr );
  udbto->actWin->appCtx->proc->unlock();

}

static void udbtc_visInfoUpdate (
  struct event_handler_args ast_args )
{

activeUpdownButtonClass *udbto =
 (activeUpdownButtonClass *) ast_args.usr;

struct dbr_gr_double controlRec = *( (dbr_gr_double *) ast_args.dbr );

  udbto->curVisValue = controlRec.value;

  udbto->actWin->appCtx->proc->lock();
  udbto->needVisInit = 1;
  udbto->actWin->addDefExeNode( udbto->aglPtr );
  udbto->actWin->appCtx->proc->unlock();

}

static void udbtc_visUpdate (
  struct event_handler_args ast_args )
{

activeUpdownButtonClass *udbto =
 (activeUpdownButtonClass *) ast_args.usr;

  udbto->curVisValue = * ( (double *) ast_args.dbr );

  udbto->actWin->appCtx->proc->lock();
  udbto->needVisUpdate = 1;
  udbto->actWin->addDefExeNode( udbto->aglPtr );
  udbto->actWin->appCtx->proc->unlock();

}

#endif

static void udbtc_decrement (
  XtPointer client,
  XtIntervalId *id )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;
int stat;
double dval;
Window root, child;
int rootX, rootY, winX, winY;
unsigned int mask;

  XQueryPointer( udbto->actWin->d, XtWindow(udbto->actWin->top), &root, &child,
   &rootX, &rootY, &winX, &winY, &mask );

  if ( !( mask & Button1Mask ) ) {
    udbto->incrementTimerActive = 0;
  }

  if ( !udbto->incrementTimerActive ) {
    udbto->incrementTimer = 0;
    return;
  }

  udbto->incrementTimer = XtAppAddTimeOut(
   udbto->actWin->appCtx->appContext(),
   udbto->incrementTimerValue, udbtc_decrement, client );

  udbto->actWin->appCtx->proc->lock();
  dval = udbto->curControlV;
  udbto->actWin->appCtx->proc->unlock();

  dval -= udbto->coarse;

  if ( dval < udbto->minDv ) {
    dval = udbto->minDv;
  }
  else if ( dval > udbto->maxDv ) {
    dval = udbto->maxDv;
  }

  if ( udbto->destExists ) {
#ifdef __epics__
  stat = ca_put( DBR_DOUBLE, udbto->destPvId, &dval );
#endif
  }

}

static void udbtc_increment (
  XtPointer client,
  XtIntervalId *id )
{

activeUpdownButtonClass *udbto = (activeUpdownButtonClass *) client;
int stat;
double dval;
Window root, child;
int rootX, rootY, winX, winY;
unsigned int mask;

  XQueryPointer( udbto->actWin->d, XtWindow(udbto->actWin->top), &root, &child,
   &rootX, &rootY, &winX, &winY, &mask );

  if ( !( mask & Button3Mask ) ) {
    udbto->incrementTimerActive = 0;
  }

  if ( !udbto->incrementTimerActive ) {
    udbto->incrementTimer = 0;
    return;
  }

  udbto->incrementTimer = XtAppAddTimeOut(
   udbto->actWin->appCtx->appContext(),
   udbto->incrementTimerValue, udbtc_increment, client );

  udbto->actWin->appCtx->proc->lock();
  dval = udbto->curControlV;
  udbto->actWin->appCtx->proc->unlock();

  dval += udbto->coarse;

  if ( dval < udbto->minDv ) {
    dval = udbto->minDv;
  }
  else if ( dval > udbto->maxDv ) {
    dval = udbto->maxDv;
  }

  if ( udbto->destExists ) {
#ifdef __epics__
  stat = ca_put( DBR_DOUBLE, udbto->destPvId, &dval );
#endif
  }

}

activeUpdownButtonClass::activeUpdownButtonClass ( void ) {

  name = new char[strlen("activeUpdownButtonClass")+1];
  strcpy( name, "activeUpdownButtonClass" );
  buttonPressed = 0;

  _3D = 1;
  invisible = 0;
  rate = 0.1;
  curSaveV = 0.0;
  scaleMin = 0;
  scaleMax = 10;
  limitsFromDb = 1;
  efScaleMin.setNull(1);
  efScaleMax.setNull(1);
  unconnectedTimer = 0;
  visibility = 0;
  prevVisibility = -1;
  visInverted = 0;
  strcpy( minVisString, "" );
  strcpy( maxVisString, "" );
  connection.setMaxPvs( 2 );

}

// copy constructor
activeUpdownButtonClass::activeUpdownButtonClass
 ( const activeUpdownButtonClass *source ) {

activeGraphicClass *udbto = (activeGraphicClass *) this;

  udbto->clone( (activeGraphicClass *) source );

  name = new char[strlen("activeUpdownButtonClass")+1];
  strcpy( name, "activeUpdownButtonClass" );

  buttonPressed = 0;

  fgCb = source->fgCb;
  bgCb = source->bgCb;
  topShadowCb = source->topShadowCb;
  botShadowCb = source->botShadowCb;

  strncpy( fontTag, source->fontTag, 63 );

  fs = actWin->fi->getXFontStruct( fontTag );

  fgColor.copy( source->fgColor );
  bgColor.copy( source->bgColor );

  topShadowColor = source->topShadowColor;
  botShadowColor = source->botShadowColor;

  destPvExpString.copy( source->destPvExpString );

  savePvExpString.copy( source->savePvExpString );

  fineExpString.copy( source->fineExpString );

  coarseExpString.copy( source->coarseExpString );

  label.copy( source->label );

  _3D = source->_3D;
  invisible = source->invisible;
  rate = source->rate;
  limitsFromDb = source->limitsFromDb;
  scaleMin = source->scaleMin;
  scaleMax = source->scaleMax;
  efScaleMin = source->efScaleMin;
  efScaleMax = source->efScaleMax;
  unconnectedTimer = 0;

  visibility = 0;
  prevVisibility = -1;
  visInverted = source->visInverted;
  strncpy( minVisString, source->minVisString, 39 );
  strncpy( maxVisString, source->maxVisString, 39 );

  connection.setMaxPvs( 2 );

  updateDimensions();

}

activeUpdownButtonClass::~activeUpdownButtonClass ( void ) {

  if ( name ) delete name;

  if ( unconnectedTimer ) {
    XtRemoveTimeOut( unconnectedTimer );
    unconnectedTimer = 0;
  }

}

int activeUpdownButtonClass::createInteractive (
  activeWindowClass *aw_obj,
  int _x,
  int _y,
  int _w,
  int _h ) {

  actWin = (activeWindowClass *) aw_obj;
  x = _x;
  y = _y;
  w = _w;
  h = _h;

  fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  bgColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  topShadowColor = actWin->defaultTopShadowColor;
  botShadowColor = actWin->defaultBotShadowColor;

  strcpy( fontTag, actWin->defaultBtnFontTag );

  actWin->fi->loadFontTag( fontTag );
  fs = actWin->fi->getXFontStruct( fontTag );

  updateDimensions();

  this->draw();

  this->editCreate();

  return 1;

}

int activeUpdownButtonClass::save (
  FILE *f )
{

int index;

  fprintf( f, "%-d %-d %-d\n", UDBTC_MAJOR_VERSION, UDBTC_MINOR_VERSION,
   UDBTC_RELEASE );

  fprintf( f, "%-d\n", x );
  fprintf( f, "%-d\n", y );
  fprintf( f, "%-d\n", w );
  fprintf( f, "%-d\n", h );

  index = fgColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );
  //fprintf( f, "%-d\n", index );

  index = bgColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );
  //fprintf( f, "%-d\n", index );

  index = topShadowColor;
  actWin->ci->writeColorIndex( f, index );
  //fprintf( f, "%-d\n", index );

  index = botShadowColor;
  actWin->ci->writeColorIndex( f, index );
  //fprintf( f, "%-d\n", index );

  if ( destPvExpString.getRaw() )
    writeStringToFile( f, destPvExpString.getRaw() );
  else
    writeStringToFile( f, "" );

  if ( fineExpString.getRaw() )
    writeStringToFile( f, fineExpString.getRaw() );
  else
    writeStringToFile( f, "" );

  if ( coarseExpString.getRaw() )
    writeStringToFile( f, coarseExpString.getRaw() );
  else
    writeStringToFile( f, "" );

  if ( label.getRaw() )
    writeStringToFile( f, label.getRaw() );
  else
    writeStringToFile( f, "" );

  fprintf( f, "%-d\n", _3D );

  fprintf( f, "%-d\n", invisible );

  fprintf( f, "%-g\n", rate );

  writeStringToFile( f, fontTag );

  // ver 1.1.0
  if ( savePvExpString.getRaw() )
    writeStringToFile( f, savePvExpString.getRaw() );
  else
    writeStringToFile( f, "" );

  // ver 1.2.0
  fprintf( f, "%-d\n", limitsFromDb );
  efScaleMin.write( f );
  efScaleMax.write( f );

  // ver 1.4.0
  if ( visPvExpString.getRaw() )
    writeStringToFile( f, visPvExpString.getRaw() );
  else
    writeStringToFile( f, "" );
  fprintf( f, "%-d\n", visInverted );
  writeStringToFile( f, minVisString );
  writeStringToFile( f, maxVisString );

  return 1;

}

int activeUpdownButtonClass::createFromFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int index;
int major, minor, release;
char oneName[activeGraphicClass::MAX_PV_NAME+1];
float fval;

  this->actWin = _actWin;

  fscanf( f, "%d %d %d\n", &major, &minor, &release ); actWin->incLine();

  fscanf( f, "%d\n", &x ); actWin->incLine();
  fscanf( f, "%d\n", &y ); actWin->incLine();
  fscanf( f, "%d\n", &w ); actWin->incLine();
  fscanf( f, "%d\n", &h ); actWin->incLine();

  this->initSelectBox(); // call after getting x,y,w,h

  if ( ( major > 1 ) || ( ( major == 1 ) && ( minor > 2 ) ) ) {

    actWin->ci->readColorIndex( f, &index );
    actWin->incLine(); actWin->incLine();
    fgColor.setColorIndex( index, actWin->ci );

    actWin->ci->readColorIndex( f, &index );
    actWin->incLine(); actWin->incLine();
    bgColor.setColorIndex( index, actWin->ci );

    actWin->ci->readColorIndex( f, &index );
    actWin->incLine(); actWin->incLine();
    topShadowColor = index;

    actWin->ci->readColorIndex( f, &index );
    actWin->incLine(); actWin->incLine();
    botShadowColor = index;

  }
  else {

    fscanf( f, "%d\n", &index ); actWin->incLine();
    fgColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &index ); actWin->incLine();
    bgColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &index ); actWin->incLine();
    topShadowColor = index;

    fscanf( f, "%d\n", &index ); actWin->incLine();
    botShadowColor = index;

  }

  readStringFromFile( oneName, activeGraphicClass::MAX_PV_NAME+1, f );
   actWin->incLine();
  destPvExpString.setRaw( oneName );

  readStringFromFile( oneName, 39+1, f ); actWin->incLine();
  fineExpString.setRaw( oneName );

  readStringFromFile( oneName, 39+1, f ); actWin->incLine();
  coarseExpString.setRaw( oneName );

  readStringFromFile( oneName, 39+1, f ); actWin->incLine();
  label.setRaw( oneName );

  fscanf( f, "%d\n", &_3D ); actWin->incLine();

  fscanf( f, "%d\n", &invisible ); actWin->incLine();

  fscanf( f, "%g\n", &fval ); actWin->incLine();
  rate = (double) fval;

  readStringFromFile( fontTag, 63+1, f ); actWin->incLine();

  if ( ( major > 1 ) || ( ( major == 1 ) && ( minor > 0 ) ) ) {
    readStringFromFile( oneName, activeGraphicClass::MAX_PV_NAME+1, f );
     actWin->incLine();
    savePvExpString.setRaw( oneName );
  }

  if ( ( major > 1 ) || ( ( major == 1 ) && ( minor > 1 ) ) ) {
    fscanf( f, "%d\n", &limitsFromDb ); actWin->incLine();
    efScaleMin.read( f ); actWin->incLine();
    efScaleMax.read( f ); actWin->incLine();
    if ( ( limitsFromDb || efScaleMin.isNull() ) &&
         ( limitsFromDb || efScaleMax.isNull() ) ) {
      minDv = scaleMin = 0;
      maxDv = scaleMax = 10;
    }
    else{
      minDv = scaleMin = efScaleMin.value();
      maxDv = scaleMax = efScaleMax.value();
    }
  }
  else {
    scaleMin = 0;
    scaleMax = 10;
  }

  actWin->fi->loadFontTag( fontTag );
  fs = actWin->fi->getXFontStruct( fontTag );

  if ( ( major > 1 ) || ( ( major == 1 ) && ( minor > 3 ) ) ) {

    readStringFromFile( oneName, activeGraphicClass::MAX_PV_NAME+1, f );
     actWin->incLine();
    visPvExpString.setRaw( oneName );

    fscanf( f, "%d\n", &visInverted ); actWin->incLine();

    readStringFromFile( minVisString, 39+1, f ); actWin->incLine();

    readStringFromFile( maxVisString, 39+1, f ); actWin->incLine();

  }

  updateDimensions();

  return 1;

}

int activeUpdownButtonClass::importFromXchFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin ){

int fgR, fgG, fgB, bgR, bgG, bgB, more, index;
unsigned int pixel;
char *tk, *gotData, *context, buf[255+1];

  fgR = 0xffff;
  fgG = 0xffff;
  fgB = 0xffff;

  bgR = 0xffff;
  bgG = 0xffff;
  bgB = 0xffff;

  this->actWin = _actWin;

  fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  bgColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  topShadowColor = actWin->defaultTopShadowColor;
  botShadowColor = actWin->defaultBotShadowColor;
  strcpy( fontTag, actWin->defaultBtnFontTag );

  label.setRaw( "" );

  // continue until tag is <eod>

  do {

    gotData = getNextDataString( buf, 255, f );
    if ( !gotData ) {
      actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
      return 0;
    }

    context = NULL;

    tk = strtok_r( buf, " \t\n", &context );
    if ( !tk ) {
      actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
      return 0;
    }

    if ( strcmp( tk, "<eod>" ) == 0 ) {

      more = 0;

    }
    else {

      more = 1;

      if ( strcmp( tk, "x" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        x = atol( tk );

      }
      else if ( strcmp( tk, "y" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        y = atol( tk );

      }
      else if ( strcmp( tk, "w" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        w = atol( tk );

      }
      else if ( strcmp( tk, "h" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        h = atol( tk );

      }
            
      else if ( strcmp( tk, "fgred" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        fgR = atol( tk );

      }
            
      else if ( strcmp( tk, "fggreen" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        fgG = atol( tk );

      }
            
      else if ( strcmp( tk, "fgblue" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        fgB = atol( tk );

      }
            
      else if ( strcmp( tk, "bgred" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        bgR = atol( tk );

      }
            
      else if ( strcmp( tk, "bggreen" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        bgG = atol( tk );

      }
            
      else if ( strcmp( tk, "bgblue" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        bgB = atol( tk );

      }
            
      else if ( strcmp( tk, "invisible" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        invisible = atol( tk );

      }
            
      else if ( strcmp( tk, "rate" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        rate = atof( tk );

      }
            
      else if ( strcmp( tk, "font" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeUpdownButtonClass_str1 );
          return 0;
        }

        strncpy( fontTag, tk, 63 );

      }

      else if ( strcmp( tk, "controlpv" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n \t", &context );
        if ( tk ) {
          strncpy( bufDestPvName, tk, 28 );
          bufDestPvName[28] = 0;
          destPvExpString.setRaw( bufDestPvName );
	}

      }

      else if ( strcmp( tk, "fine" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n \t", &context );
        if ( tk ) {
          strncpy( bufFine, tk, 28 );
          bufFine[28] = 0;
          fineExpString.setRaw( bufFine );
	}

      }

      else if ( strcmp( tk, "coarse" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n \t", &context );
        if ( tk ) {
          strncpy( bufCoarse, tk, 28 );
          bufCoarse[28] = 0;
          coarseExpString.setRaw( bufCoarse );
	}

      }

      else if ( strcmp( tk, "label" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n \t", &context );
        if ( tk ) {
          label.setRaw( tk );
	}

      }

    }

  } while ( more );

  this->initSelectBox(); // call after getting x,y,w,h

  actWin->ci->setRGB( fgR, fgG, fgB, &pixel );
  index = actWin->ci->pixIndex( pixel );
  fgColor.setColorIndex( index, actWin->ci );

  actWin->ci->setRGB( bgR, bgG, bgB, &pixel );
  index = actWin->ci->pixIndex( pixel );
  bgColor.setColorIndex( index, actWin->ci );

  actWin->fi->loadFontTag( fontTag );
  actWin->drawGc.setFontTag( fontTag, actWin->fi );
  fs = actWin->fi->getXFontStruct( fontTag );

  updateDimensions();

  return 1;

}

int activeUpdownButtonClass::genericEdit ( void ) {

char title[32], *ptr;

  ptr = actWin->obj.getNameFromClass( "activeUpdownButtonClass" );
  if ( ptr )
    strncpy( title, ptr, 31 );
  else
    strncpy( title, activeUpdownButtonClass_str2, 31 );

  strncat( title, activeUpdownButtonClass_str3, 31 );

  bufX = x;
  bufY = y;
  bufW = w;
  bufH = h;

  bufFgColor = fgColor.pixelIndex();

  bufBgColor = bgColor.pixelIndex();

  bufTopShadowColor = topShadowColor;
  bufBotShadowColor = botShadowColor;
  strncpy( bufFontTag, fontTag, 63 );

  if ( destPvExpString.getRaw() )
    strncpy( bufDestPvName, destPvExpString.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
  else
    strcpy( bufDestPvName, "" );

  if ( savePvExpString.getRaw() )
    strncpy( bufSavePvName, savePvExpString.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
  else
    strcpy( bufSavePvName, "" );

  if ( fineExpString.getRaw() )
    strncpy( bufFine, fineExpString.getRaw(), 39 );
  else
    strncpy( bufFine, "", 39 );

  if ( coarseExpString.getRaw() )
    strncpy( bufCoarse, coarseExpString.getRaw(), 39 );
  else
    strncpy( bufCoarse, "", 39 );

  if ( label.getRaw() )
    strncpy( bufLabel, label.getRaw(), 39 );
  else
    strncpy( bufLabel, "", 39 );

  buf3D = _3D;
  bufInvisible = invisible;
  bufRate = rate;

  bufLimitsFromDb = limitsFromDb;
  bufEfScaleMin = efScaleMin;
  bufEfScaleMax = efScaleMax;

  if ( visPvExpString.getRaw() )
    strncpy( bufVisPvName, visPvExpString.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
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

  ef.addTextField( activeUpdownButtonClass_str4, 35, &bufX );
  ef.addTextField( activeUpdownButtonClass_str5, 35, &bufY );
  ef.addTextField( activeUpdownButtonClass_str6, 35, &bufW );
  ef.addTextField( activeUpdownButtonClass_str7, 35, &bufH );
  ef.addTextField( activeUpdownButtonClass_str8, 35, bufDestPvName,
   activeGraphicClass::MAX_PV_NAME );
  ef.addTextField( activeUpdownButtonClass_str25, 35, bufSavePvName,
   activeGraphicClass::MAX_PV_NAME );

  ef.addToggle( activeUpdownButtonClass_str26, &bufLimitsFromDb );
  ef.addTextField( activeUpdownButtonClass_str27, 35, &bufEfScaleMin );
  ef.addTextField( activeUpdownButtonClass_str28, 35, &bufEfScaleMax );
  ef.addTextField( activeUpdownButtonClass_str9, 35, bufCoarse, 39 );
  ef.addTextField( activeUpdownButtonClass_str10, 35, bufFine, 39 );
  ef.addTextField( activeUpdownButtonClass_str11, 35, &bufRate );
  ef.addToggle( activeUpdownButtonClass_str12, &buf3D );
  ef.addToggle( activeUpdownButtonClass_str13, &bufInvisible );
  ef.addTextField( activeUpdownButtonClass_str14, 35, bufLabel, 39 );
  ef.addColorButton( activeUpdownButtonClass_str16, actWin->ci, &fgCb, &bufFgColor );
  ef.addColorButton( activeUpdownButtonClass_str17, actWin->ci, &bgCb, &bufBgColor );
  ef.addColorButton( activeUpdownButtonClass_str18, actWin->ci, &topShadowCb, &bufTopShadowColor );
  ef.addColorButton( activeUpdownButtonClass_str19, actWin->ci, &botShadowCb, &bufBotShadowColor );

  ef.addFontMenu( activeUpdownButtonClass_str15, actWin->fi, &fm, fontTag );

  XtUnmanageChild( fm.alignWidget() ); // no alignment info

  ef.addTextField( activeUpdownButtonClass_str29, 30, bufVisPvName,
   activeGraphicClass::MAX_PV_NAME );
  ef.addOption( " ", activeUpdownButtonClass_str30, &bufVisInverted );
  ef.addTextField( activeUpdownButtonClass_str31, 30, bufMinVisString, 39 );
  ef.addTextField( activeUpdownButtonClass_str32, 30, bufMaxVisString, 39 );

  return 1;

}

int activeUpdownButtonClass::editCreate ( void ) {

  this->genericEdit();
  ef.finished( udbtc_edit_ok, udbtc_edit_apply, udbtc_edit_cancel_delete, this );
  actWin->currentEf = NULL;
  ef.popup();

  return 1;

}

int activeUpdownButtonClass::edit ( void ) {

  this->genericEdit();
  ef.finished( udbtc_edit_ok, udbtc_edit_apply, udbtc_edit_cancel, this );
  actWin->currentEf = &ef;
  ef.popup();

  return 1;

}

int activeUpdownButtonClass::erase ( void ) {

  if ( deleteRequest ) return 1;

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  return 1;

}

int activeUpdownButtonClass::eraseActive ( void ) {

  if ( !init || !activeMode || invisible ) return 1;

  if ( prevVisibility == 0 ) {
    prevVisibility = visibility;
    return 1;
  }

  prevVisibility = visibility;

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  return 1;

}

int activeUpdownButtonClass::draw ( void ) {

int tX, tY;
XRectangle xR = { x, y, w, h };

  if ( deleteRequest ) return 1;

  actWin->drawGc.saveFg();

  actWin->drawGc.setFG( bgColor.pixelColor() );

  actWin->drawGc.setLineStyle( LineSolid );
  actWin->drawGc.setLineWidth( 1 );

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, w, h );

  if ( !_3D ) {

    actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

  }

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, w, h );

  if ( _3D ) {

  actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, x+w, y );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, x, y+h );

   actWin->drawGc.setFG( actWin->ci->pix(topShadowColor) );

   XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
    actWin->drawGc.normGC(), x, y+h, x+w, y+h );

   XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
    actWin->drawGc.normGC(), x+w, y, x+w, y+h );

  actWin->drawGc.setFG( actWin->ci->pix(topShadowColor) );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+1, y+1, x+w-1, y+1 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+2, y+2, x+w-2, y+2 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+1, y+1, x+1, y+h-1 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+2, y+2, x+2, y+h-2 );

  actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+1, y+h-1, x+w-1, y+h-1 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+2, y+h-2, x+w-2, y+h-2 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+w-1, y+1, x+w-1, y+h-1 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+w-2, y+2, x+w-2, y+h-2 );

  }

  actWin->drawGc.setFG( fgColor.pixelColor() );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+5, y+9, x+w-5, y+9 );

  if ( fs ) {

    actWin->drawGc.addNormXClipRectangle( xR );

    actWin->drawGc.setFontTag( fontTag, actWin->fi );

    tX = x + w/2;
    tY = y + h/2 - fontAscent/2;

    if ( label.getRaw() )
      drawText( actWin->drawWidget, &actWin->drawGc, fs, tX, tY,
       XmALIGNMENT_CENTER, label.getRaw() );
    else
      drawText( actWin->drawWidget, &actWin->drawGc, fs, tX, tY,
       XmALIGNMENT_CENTER, "" );

    actWin->drawGc.removeNormXClipRectangle();

  }

  actWin->drawGc.restoreFg();

  return 1;

}

int activeUpdownButtonClass::drawActive ( void ) {

int tX, tY;
char string[63+1];
XRectangle xR = { x, y, w, h };

  if ( !init ) {
    if ( needToDrawUnconnected ) {
      actWin->executeGc.saveFg();
      actWin->executeGc.setFG( bgColor.getDisconnected() );
      actWin->executeGc.setLineWidth( 1 );
      actWin->executeGc.setLineStyle( LineSolid );
      XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
       actWin->executeGc.normGC(), x, y, w, h );
      actWin->executeGc.restoreFg();
      needToEraseUnconnected = 1;
    }
  }
  else if ( needToEraseUnconnected ) {
    actWin->executeGc.setLineWidth( 1 );
    actWin->executeGc.setLineStyle( LineSolid );
    XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), x, y, w, h );
    needToEraseUnconnected = 0;
    if ( invisible ) {
      eraseActive();
      smartDrawAllActive();
    }
  }

  if ( !init || !activeMode || invisible || !visibility ) return 1;

  prevVisibility = visibility;

  actWin->executeGc.saveFg();

  actWin->executeGc.setFG( bgColor.getColor() );

  actWin->executeGc.setLineStyle( LineSolid );
  actWin->executeGc.setLineWidth( 1 );

  XFillRectangle( actWin->d, XtWindow(actWin->executeWidget),
   actWin->executeGc.normGC(), x, y, w, h );

  if ( !_3D ) {

    actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

  }

  XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
   actWin->executeGc.normGC(), x, y, w, h );

  if ( !buttonPressed ) {

    if ( _3D ) {

    actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y, x+w, y );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y, x, y+h );

    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y+h, x+w, y+h );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+w, y, x+w, y+h );

    // top
    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+1, y+1, x+w-1, y+1 );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+2, y+2, x+w-2, y+2 );

    // left
    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+1, y+1, x+1, y+h-1 );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+2, y+2, x+2, y+h-2 );

    // bottom
    actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+1, y+h-1, x+w-1, y+h-1 );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+2, y+h-2, x+w-2, y+h-2 );

    // right
    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+w-1, y+1, x+w-1, y+h-1 );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+w-2, y+2, x+w-2, y+h-2 );

    }

  }
  else {

    if ( _3D ) {

    actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y, x+w, y );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y, x, y+h );

    // top

    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    // bottom

    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y+h, x+w, y+h );

    //right

    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+w, y, x+w, y+h );

    }

  }

  actWin->executeGc.setFG( fgColor.getColor() );

  XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
   actWin->executeGc.normGC(), x+5, y+9, x+w-5, y+9 );

  if ( fs ) {

    if ( label.getExpanded() )
      strncpy( string, label.getExpanded(), 39 );
    else
      strncpy( string, "", 39 );

    if ( isSaved ) {
      strncat( string, " *", 63 );
    }

    actWin->executeGc.addNormXClipRectangle( xR );

    actWin->executeGc.setFontTag( fontTag, actWin->fi );

    tX = x + w/2;
    tY = y + h/2 - fontAscent/2;

    drawText( actWin->executeWidget, &actWin->executeGc, fs, tX, tY,
     XmALIGNMENT_CENTER, string );

    actWin->executeGc.removeNormXClipRectangle();

  }

  actWin->executeGc.restoreFg();

  return 1;

}

int activeUpdownButtonClass::activate (
  int pass,
  void *ptr )
{

int stat, opStat, n;
Arg args[5];
XmString str;

  switch ( pass ) {

  case 1:

    opComplete = 0;

    break;

  case 2:

    if ( !opComplete ) {

      connection.init();

      needConnectInit = needSaveConnectInit = needCtlInfoInit = 
       needRefresh = needErase = needDraw = needVisConnectInit =
       needVisInit = needVisUpdate = 0;
      needToEraseUnconnected = 0;
      needToDrawUnconnected = 0;
      unconnectedTimer = 0;
      init = 0;
      aglPtr = ptr;
      widgetsCreated = 0;
      keyPadOpen = 0;
      isSaved = 0;
      incrementTimer = 0;
      incrementTimerActive = 0;

#ifdef __epics__
      destEventId = saveEventId = visEventId = 0;
#endif

      destPvConnected = savePvConnected = active = buttonPressed = 0;
      activeMode = 1;

      incrementTimerValue = (int) ( 1000.0 * rate );
      if ( incrementTimerValue < 50 ) incrementTimerValue = 50;

      if ( !fineExpString.getExpanded() ||
         ( strcmp( fineExpString.getExpanded(), "" ) == 0 ) ) {
        fine = 0;
      }
      else {
        fine = atof( fineExpString.getExpanded() );
      }

      if ( !coarseExpString.getExpanded() ||
         ( strcmp( coarseExpString.getExpanded(), "" ) == 0 ) ) {
        coarse = 0;
      }
      else {
        coarse = atof( coarseExpString.getExpanded() );
      }

      if ( !destPvExpString.getExpanded() ||
         ( strcmp( destPvExpString.getExpanded(), "" ) == 0 ) ) {
        destExists = 0;
      }
      else {
        destExists = 1;
        connection.addPv();
      }

      if ( !visPvExpString.getExpanded() ||
         ( strcmp( visPvExpString.getExpanded(), "" ) == 0 ) ) {
        visExists = 0;
        visibility = 1;
      }
      else {
        visExists = 1;
        connection.addPv();
      }

      if ( !savePvExpString.getExpanded() ||
         ( strcmp( savePvExpString.getExpanded(), "" ) == 0 ) ) {
        saveExists = 0;
      }
      else {
        saveExists = 1;
      }

      if ( !unconnectedTimer ) {
        unconnectedTimer = XtAppAddTimeOut( actWin->appCtx->appContext(),
         2000, unconnectedTimeout, this );
      }

      if ( !widgetsCreated ) {

        n = 0;
        XtSetArg( args[n], XmNmenuPost, (XtArgVal) "<Btn5Down>;" ); n++;
        popUpMenu = XmCreatePopupMenu( actWin->topWidgetId(), "", args, n );

        pullDownMenu = XmCreatePulldownMenu( popUpMenu, "", NULL, 0 );

        str = XmStringCreateLocalized( "Save" );
        pbSave = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
         popUpMenu,
         XmNlabelString, str,
         NULL );
        XmStringFree( str );

        XtAddCallback( pbSave, XmNactivateCallback, menu_cb,
         (XtPointer) this );

        str = XmStringCreateLocalized( "Restore" );
        pbRestore = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
         popUpMenu,
         XmNlabelString, str,
         NULL );
        XmStringFree( str );

        XtAddCallback( pbRestore, XmNactivateCallback, menu_cb,
         (XtPointer) this );

        str = XmStringCreateLocalized( "Set Coarse" );
        pbCoarse = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
         popUpMenu,
         XmNlabelString, str,
         NULL );
        XmStringFree( str );

        XtAddCallback( pbCoarse, XmNactivateCallback, menu_cb,
         (XtPointer) this );

        str = XmStringCreateLocalized( "Set Fine" );
        pbFine = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
         popUpMenu,
         XmNlabelString, str,
         NULL );
        XmStringFree( str );

        XtAddCallback( pbFine, XmNactivateCallback, menu_cb,
         (XtPointer) this );

        str = XmStringCreateLocalized( "Set Rate (sec)" );
        pbRate = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
         popUpMenu,
         XmNlabelString, str,
         NULL );
        XmStringFree( str );

        XtAddCallback( pbRate, XmNactivateCallback, menu_cb,
         (XtPointer) this );

        str = XmStringCreateLocalized( "Set Value" );
        pbValue = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
         popUpMenu,
         XmNlabelString, str,
         NULL );
        XmStringFree( str );

        XtAddCallback( pbValue, XmNactivateCallback, menu_cb,
         (XtPointer) this );

	widgetsCreated = 1;

      }

      opStat = 1;

#ifdef __epics__

      if ( destExists ) {
        stat = ca_search_and_connect( destPvExpString.getExpanded(), &destPvId,
         udbtc_monitor_dest_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeUpdownButtonClass_str20 );
          opStat = 0;
        }
      }
      else {
        init = 1;
        smartDrawAllActive();
      }

      if ( visExists ) {

        stat = ca_search_and_connect( visPvExpString.getExpanded(), &visPvId,
         udbtc_monitor_vis_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeUpdownButtonClass_str20 );
          opStat = 0;
        }
      }

      if ( saveExists ) {
        stat = ca_search_and_connect( savePvExpString.getExpanded(), &savePvId,
         udbtc_monitor_save_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeUpdownButtonClass_str20 );
          opStat = 0;
        }
      }

      if ( opStat & 1 ) opComplete = 1;

#endif

      return opStat;

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

int activeUpdownButtonClass::deactivate (
  int pass
) {

int stat;

  if ( pass == 1 ) {

  active = 0;
  activeMode = 0;

  if ( incrementTimerActive ) {
    if ( incrementTimer ) {
      XtRemoveTimeOut( incrementTimer );
      incrementTimer = 0;
    }
    incrementTimerActive = 0;
  }

  if ( 	widgetsCreated ) {
    XtDestroyWidget( popUpMenu );
    widgetsCreated = 0;
  }

  if ( kp.isPoppedUp() ) {
    kp.popdown();
  }

#ifdef __epics__

  if ( destExists ) {
    stat = ca_clear_channel( destPvId );
    if ( stat != ECA_NORMAL )
      printf( activeUpdownButtonClass_str22 );
  }

  if ( visExists ) {
    stat = ca_clear_channel( visPvId );
    if ( stat != ECA_NORMAL ) printf( activeUpdownButtonClass_str22 );
  }

  if ( saveExists ) {
    stat = ca_clear_channel( savePvId );
    if ( stat != ECA_NORMAL )
      printf( activeUpdownButtonClass_str22 );
  }

#endif

  }

  return 1;

}

void activeUpdownButtonClass::updateDimensions ( void )
{

  if ( fs ) {
    fontAscent = fs->ascent;
    fontDescent = fs->descent;
    fontHeight = fontAscent + fontDescent;
  }
  else {
    fontAscent = 10;
    fontDescent = 5;
    fontHeight = fontAscent + fontDescent;
  }

}

void activeUpdownButtonClass::btnUp (
  int _x,
  int _y,
  int buttonState,
  int buttonNumber,
  int *action )
{

XButtonEvent be;

  *action = 0;

  if ( !init || !visibility ) return;

  if ( !ca_write_access( destPvId ) ) return;

  if ( ( _y - y ) < 10 ) {
    memset( (void *) &be, 0, sizeof(XButtonEvent) );
    be.x_root = actWin->x+_x;
    be.y_root = actWin->y+_y;
    XmMenuPosition( popUpMenu, &be );
    XtManageChild( popUpMenu );
    return;
  }

  if ( !buttonPressed ) return;

  if ( keyPadOpen ) return;

  if ( incrementTimerActive ) {
    if ( incrementTimer ) {
      XtRemoveTimeOut( incrementTimer );
      incrementTimer = 0;
    }
    incrementTimerActive = 0;
  }

  buttonPressed = 0;

//    printf( "btn up\n" );

  actWin->appCtx->proc->lock();
  needRefresh = 1;
  actWin->addDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

}

void activeUpdownButtonClass::btnDown (
  int _x,
  int _y,
  int buttonState,
  int buttonNumber,
  int *action )
{

int stat;
double dval;

  *action = 0;

  if ( !init || !visibility ) return;

  if ( !ca_write_access( destPvId ) ) return;

  if ( keyPadOpen ) return;

  if ( ( _y - y ) < 10 ) return;

  buttonPressed = 1;

//    printf( "btn down, x=%-d, y=%-d\n", _x-x, _y-y );

#ifdef __epics__

  actWin->appCtx->proc->lock();
  dval = curControlV;
  needRefresh = 1;
  actWin->addDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

  if ( buttonNumber == 3 ) {
    dval += fine;
  }
  else if ( buttonNumber == 1 ) {
    dval -= fine;
  }

  if ( dval < minDv ) {
    dval = minDv;
  }
  else if ( dval > maxDv ) {
    dval = maxDv;
  }

#ifdef __epics__
  stat = ca_put( DBR_DOUBLE, destPvId, &dval );
#endif

  if ( buttonNumber == 3 ) {
    incrementTimer = XtAppAddTimeOut( actWin->appCtx->appContext(),
     500, udbtc_increment, this );
    incrementTimerActive = 1;
  }
  else if ( buttonNumber == 1 ) {
    incrementTimer = XtAppAddTimeOut( actWin->appCtx->appContext(),
     500, udbtc_decrement, this );
    incrementTimerActive = 1;
  }

#endif

}

void activeUpdownButtonClass::pointerIn (
  int _x,
  int _y,
  int buttonState )
{

  if ( !init || !visibility ) return;

  if ( !ca_write_access( destPvId ) ) {
    actWin->cursor.set( XtWindow(actWin->executeWidget), CURSOR_K_NO );
  }
  else {
    actWin->cursor.set( XtWindow(actWin->executeWidget), CURSOR_K_DEFAULT );
  }

  activeGraphicClass::pointerIn( _x, _y, buttonState );

}

int activeUpdownButtonClass::getButtonActionRequest (
  int *up,
  int *down,
  int *drag,
  int *focus )
{

  *drag = 0;

  if ( destExists )
    *focus = 1;
  else
    *focus = 0;

  if ( !destExists ) {
    *up = 0;
    *down = 0;
    return 1;
  }

  *down = 1;
  *up = 1;

  return 1;

}

int activeUpdownButtonClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat, retStat = 1;

  stat = destPvExpString.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = savePvExpString.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = fineExpString.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = coarseExpString.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = label.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = visPvExpString.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;

  return retStat;

}

int activeUpdownButtonClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat, retStat = 1;

  stat = destPvExpString.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = savePvExpString.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = fineExpString.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = coarseExpString.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = label.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = visPvExpString.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;

  return stat;

}

int activeUpdownButtonClass::containsMacros ( void ) {

  if ( destPvExpString.containsPrimaryMacros() ) return 1;

  if ( savePvExpString.containsPrimaryMacros() ) return 1;

  if ( fineExpString.containsPrimaryMacros() ) return 1;

  if ( coarseExpString.containsPrimaryMacros() ) return 1;

  if ( label.containsPrimaryMacros() ) return 1;

  if ( visPvExpString.containsPrimaryMacros() ) return 1;

  return 0;

}

void activeUpdownButtonClass::executeDeferred ( void ) {

int nc, nsc, nci, nd, ne, nr, nvc, nvi, nvu, stat;

  if ( actWin->isIconified ) return;

  actWin->appCtx->proc->lock();
  visValue = curVisValue;
  nc = needConnectInit; needConnectInit = 0;
  nsc = needSaveConnectInit; needSaveConnectInit = 0;
  nci = needCtlInfoInit; needCtlInfoInit = 0;
  nd = needDraw; needDraw = 0;
  ne = needErase; needErase = 0;
  nr = needRefresh; needRefresh = 0;
  nvc = needVisConnectInit; needVisConnectInit = 0;
  nvi = needVisInit; needVisInit = 0;
  nvu = needVisUpdate; needVisUpdate = 0;
  actWin->remDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

  if ( !activeMode ) return;

//----------------------------------------------------------------------------

#ifdef __epics__

  if ( nc ) {

    connection.setPvConnected( (void *) destPvConnection );
    destType = ca_field_type( destPvId );

    stat = ca_get_callback( DBR_GR_DOUBLE, destPvId,
     udbt_infoUpdate, (void *) this );
    if ( stat != ECA_NORMAL )
      printf( activeUpdownButtonClass_str23 );

  }

  if ( nci ) {

    stat = ca_add_masked_array_event( destType, 1, destPvId,
     udbtc_controlUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
     &destEventId, DBE_VALUE );
    if ( stat != ECA_NORMAL )
      printf( activeUpdownButtonClass_str23 );

    if ( connection.pvsConnected() ) {
      bgColor.setConnected();
      init = 1;
      smartDrawAllActive();
    }

  }

  if ( nsc ) {

    savePvConnected = 1;
    saveType = ca_field_type( savePvId );

    stat = ca_add_masked_array_event( saveType, 1, savePvId,
     udbtc_saveUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
     &saveEventId, DBE_VALUE );
    if ( stat != ECA_NORMAL )
      printf( activeUpdownButtonClass_str23 );

  }

  if ( nvc ) {

    minVis = atof( minVisString );
    maxVis = atof( maxVisString );

    connection.setPvConnected( (void *) visPvConnection );

    stat = ca_get_callback( DBR_GR_DOUBLE, visPvId,
     udbtc_visInfoUpdate, (void *) this );

  }

  if ( nvi ) {

    stat = ca_add_masked_array_event( DBR_DOUBLE, 1, visPvId,
     udbtc_visUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
     &visEventId, DBE_VALUE );
    if ( stat != ECA_NORMAL ) printf( activeUpdownButtonClass_str23 );

    if ( ( visValue >= minVis ) &&
         ( visValue < maxVis ) )
      visibility = 1 ^ visInverted;
    else
      visibility = 0 ^ visInverted;

    if ( prevVisibility != visibility ) {
      if ( !visibility ) eraseActive();
    }

    if ( connection.pvsConnected() ) {
      bgColor.setConnected();
      init = 1;
      smartDrawAllActive();
    }

  }

#endif

//----------------------------------------------------------------------------

  if ( nd ) {

    smartDrawAllActive();

  }

//----------------------------------------------------------------------------

  if ( ne ) {

    eraseActive();

  }

//----------------------------------------------------------------------------

  if ( nr ) {

    eraseActive();
    smartDrawAllActive();

  }

//----------------------------------------------------------------------------

  if ( nvu ) {

    if ( ( visValue >= minVis ) &&
         ( visValue < maxVis ) )
      visibility = 1 ^ visInverted;
    else
      visibility = 0 ^ visInverted;

    if ( prevVisibility != visibility ) {
      if ( !visibility ) eraseActive();
      stat = smartDrawAllActive();
    }

  }

}

char *activeUpdownButtonClass::firstDragName ( void ) {

  dragIndex = 0;
  return dragName[dragIndex];

}

char *activeUpdownButtonClass::nextDragName ( void ) {

  return NULL;

}

char *activeUpdownButtonClass::dragValue (
  int i ) {

  return destPvExpString.getExpanded();

}

void activeUpdownButtonClass::changeDisplayParams (
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

  if ( _flag & ACTGRF_TEXTFGCOLOR_MASK )
    fgColor.setColorIndex( _textFgColor, actWin->ci );

  if ( _flag & ACTGRF_BGCOLOR_MASK )
    bgColor.setColorIndex( _bgColor, actWin->ci );

  if ( _flag & ACTGRF_TOPSHADOWCOLOR_MASK )
    topShadowColor = _topShadowColor;

  if ( _flag & ACTGRF_BOTSHADOWCOLOR_MASK )
    botShadowColor = _botShadowColor;

  if ( _flag & ACTGRF_BTNFONTTAG_MASK ) {
    strncpy( fontTag, _btnFontTag, 63 );
    fontTag[63] = 0;
    actWin->fi->loadFontTag( fontTag );
    fs = actWin->fi->getXFontStruct( fontTag );
    updateDimensions();
  }

}

void activeUpdownButtonClass::changePvNames (
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

  if ( flag & ACTGRF_CTLPVS_MASK ) {
    if ( numCtlPvs ) {
      destPvExpString.setRaw( ctlPvs[0] );
    }
  }

}

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeUpdownButtonClassPtr ( void ) {

activeUpdownButtonClass *ptr;

  ptr = new activeUpdownButtonClass;
  return (void *) ptr;

}

void *clone_activeUpdownButtonClassPtr (
  void *_srcPtr )
{

activeUpdownButtonClass *ptr, *srcPtr;

  srcPtr = (activeUpdownButtonClass *) _srcPtr;

  ptr = new activeUpdownButtonClass( srcPtr );

  return (void *) ptr;

}

#ifdef __cplusplus
}
#endif
