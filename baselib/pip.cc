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

#define __pip_cc 1

#define SMALL_SYM_ARRAY_SIZE 10
#define SMALL_SYM_ARRAY_LEN 31

#include "pip.h"
#include <sys/stat.h>
#include <unistd.h>
#include "app_pkg.h"
#include "act_win.h"

#include "thread.h"

static void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id )
{

activePipClass *pipo = (activePipClass *) client;

  if ( !pipo->init ) {
    pipo->needToDrawUnconnected = 1;
    pipo->needConnectTimeout = 1;
    pipo->actWin->addDefExeNode( pipo->aglPtr );
  }

  pipo->unconnectedTimer = 0;

}

static void menu_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

int i;
activePipClass *pipo = (activePipClass *) client;

  for ( i=0; i<pipo->maxDsps; i++ ) {
    if ( w == pipo->pb[i] ) {
      pipo->readPvId->put( i );
      return;
    }
  }

}

static void pipc_edit_ok1 (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activePipClass *pipo = (activePipClass *) client;

  pipo->ef1->popdownNoDestroy();

}

static void pipc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activePipClass *pipo = (activePipClass *) client;
int i, more;

  pipo->actWin->setChanged();

  pipo->eraseSelectBoxCorners();
  pipo->erase();

  pipo->displayFileName[0].setRaw( pipo->buf->bufDisplayFileName[0] );
  if ( blank( pipo->displayFileName[0].getRaw() ) ) {
    pipo->propagateMacros[0] = 1;
    pipo->label[0].setRaw( "" );
    pipo->symbolsExpStr[0].setRaw( "" );
    pipo->replaceSymbols[0] = 0;
    pipo->numDsps = 0;
  }
  else {
    pipo->propagateMacros[0] = pipo->buf->bufPropagateMacros[0];
    pipo->label[0].setRaw( pipo->buf->bufLabel[0] );
    pipo->symbolsExpStr[0].setRaw( pipo->buf->bufSymbols[0] );
    pipo->replaceSymbols[0] = pipo->buf->bufReplaceSymbols[0];
    pipo->numDsps = 1;
  }

  if ( pipo->numDsps ) {
    more = 1;
    for ( i=1; (i<pipo->maxDsps) && more; i++ ) {
      pipo->displayFileName[i].setRaw( pipo->buf->bufDisplayFileName[i] );
      if ( blank( pipo->displayFileName[i].getRaw() ) ) {
        pipo->propagateMacros[i] = 1;
        pipo->label[i].setRaw( "" );
        pipo->symbolsExpStr[i].setRaw( "" );
        pipo->replaceSymbols[i] = 0;
        more = 0;
      }
      else {
        pipo->propagateMacros[i] = pipo->buf->bufPropagateMacros[i];
        pipo->label[i].setRaw( pipo->buf->bufLabel[i] );
        pipo->symbolsExpStr[i].setRaw( pipo->buf->bufSymbols[i] );
        pipo->replaceSymbols[i] = pipo->buf->bufReplaceSymbols[i];
        (pipo->numDsps)++;
      }
    }
  }

  for ( i=pipo->numDsps; i<pipo->maxDsps; i++ ) {
    pipo->propagateMacros[i] = 1;
    pipo->label[i].setRaw( "" );
    pipo->symbolsExpStr[i].setRaw( "" );
    pipo->replaceSymbols[i] = 0;
  }

  pipo->fgColor.setColorIndex(
   pipo->buf->bufFgColor, pipo->actWin->ci );

  pipo->bgColor.setColorIndex(
   pipo->buf->bufBgColor, pipo->actWin->ci );

  pipo->topShadowColor.setColorIndex(
   pipo->buf->bufTopShadowColor, pipo->actWin->ci );

  pipo->botShadowColor.setColorIndex(
   pipo->buf->bufBotShadowColor, pipo->actWin->ci );

  pipo->readPvExpStr.setRaw( pipo->buf->bufReadPvName );

  pipo->labelPvExpStr.setRaw( pipo->buf->bufLabelPvName );

  pipo->fileNameExpStr.setRaw( pipo->buf->bufFileName );

  pipo->displaySource = pipo->buf->bufDisplaySource;

  pipo->center = pipo->buf->bufCenter;
  pipo->setSize = pipo->buf->bufSetSize;
  pipo->sizeOfs = pipo->buf->bufSizeOfs;
  pipo->noScroll = pipo->buf->bufNoScroll;

  pipo->x = pipo->buf->bufX;
  pipo->sboxX = pipo->buf->bufX;

  pipo->y = pipo->buf->bufY;
  pipo->sboxY = pipo->buf->bufY;

  pipo->w = pipo->buf->bufW;
  pipo->sboxW = pipo->buf->bufW;

  pipo->h = pipo->buf->bufH;
  pipo->sboxH = pipo->buf->bufH;

  if ( pipo->h < pipo->minH ) {
    pipo->h = pipo->minH;
    pipo->sboxH = pipo->minH;
  }

}

static void pipc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activePipClass *pipo = (activePipClass *) client;

  pipc_edit_update ( w, client, call );
  pipo->refresh( pipo );

}

static void pipc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activePipClass *pipo = (activePipClass *) client;

  pipc_edit_update ( w, client, call );
  pipo->ef.popdown();
  pipo->operationComplete();

  delete pipo->buf;
  pipo->buf = NULL;

}

static void pipc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activePipClass *pipo = (activePipClass *) client;

  pipo->ef.popdown();
  pipo->operationCancel();

  delete pipo->buf;
  pipo->buf = NULL;

}

static void pipc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activePipClass *pipo = (activePipClass *) client;

  delete pipo->buf;
  pipo->buf = NULL;

  pipo->ef.popdown();
  pipo->operationCancel();
  pipo->erase();
  pipo->deleteRequest = 1;
  pipo->drawAll();

}

static void pip_monitor_read_connect_state (
  ProcessVariable *pv,
  void *userarg )
{

activePipClass *pipo = (activePipClass *) userarg;

  if ( pv->is_valid() ) {

    pipo->needConnectInit = 1;

  }
  else {

    pipo->readPvConnected = 0;
    pipo->active = 0;
    pipo->fgColor.setDisconnected();
    pipo->needDraw = 1;

  }

  pipo->actWin->appCtx->proc->lock();
  pipo->actWin->addDefExeNode( pipo->aglPtr );
  pipo->actWin->appCtx->proc->unlock();

}

static void pip_readUpdate (
  ProcessVariable *pv,
  void *userarg )
{

activePipClass *pipo = (activePipClass *) userarg;

  if ( pipo->active ) {

    pv->get_string( pipo->curReadV, 39 );
    pipo->curReadV[39] = 0;

    pipo->actWin->appCtx->proc->lock();
    pipo->needUpdate = 1;
    pipo->actWin->addDefExeNode( pipo->aglPtr );
    pipo->actWin->appCtx->proc->unlock();

  }

}

static void pip_monitor_menu_connect_state (
  ProcessVariable *pv,
  void *userarg )
{

activePipClass *pipo = (activePipClass *) userarg;

  if ( pv->is_valid() ) {

    pipo->needMenuConnectInit = 1;

  }
  else {

    pipo->readPvConnected = 0;
    pipo->active = 0;
    pipo->fgColor.setDisconnected();
    pipo->needDraw = 1;

  }

  pipo->actWin->appCtx->proc->lock();
  pipo->actWin->addDefExeNode( pipo->aglPtr );
  pipo->actWin->appCtx->proc->unlock();

}

static void pip_menuUpdate (
  ProcessVariable *pv,
  void *userarg )
{

activePipClass *pipo = (activePipClass *) userarg;

  if ( pipo->active ) {

    pipo->curReadIV = pv->get_int();
    if ( pipo->curReadIV < -1 ) pipo->curReadIV = 0;
    if ( pipo->curReadIV >= pipo->numDsps ) pipo->curReadIV = pipo->numDsps;

    if ( pipo->firstEvent ) { // don't let menu pop up if pv is -1 initially
      pipo->firstEvent = 0;
      if ( pipo->curReadIV == -1 ) {
        pipo->curReadIV = 0;
        pv->put( pipo->curReadIV );
        return;
      }
    }

    pipo->actWin->appCtx->proc->lock();
    pipo->needMenuUpdate = 1;
    pipo->actWin->addDefExeNode( pipo->aglPtr );
    pipo->actWin->appCtx->proc->unlock();

  }

}

static void pip_monitor_label_connect_state (
  ProcessVariable *pv,
  void *userarg )
{

}

activePipClass::activePipClass ( void ) {

int i;

  name = new char[strlen("activePipClass")+1];
  strcpy( name, "activePipClass" );
  minW = 50;
  minH = 50;
  center = 0;
  setSize = 0;
  sizeOfs = 5;
  noScroll = 0;
  activeMode = 0;
  frameWidget = NULL;
  clipWidget = NULL;
  aw = NULL;
  strcpy( curFileName, "" );
  displaySource = 0;
  readPvId = NULL;
  labelPvId = NULL;
  activateIsComplete = 0;

  for ( i=0; i<maxDsps; i++ ) {
    propagateMacros[i] = 1;
    replaceSymbols[i] = 0;
  }

  numDsps = 0;
  popUpMenu = NULL;
  unconnectedTimer = 0;
  buf = NULL;

}

// copy constructor
activePipClass::activePipClass
 ( const activePipClass *source ) {

activeGraphicClass *pipo = (activeGraphicClass *) this;
int i;

  pipo->clone( (activeGraphicClass *) source );

  name = new char[strlen("activePipClass")+1];
  strcpy( name, "activePipClass" );

  fgCb = source->fgCb;
  bgCb = source->bgCb;
  topCb = source->topCb;
  botCb = source->botCb;

  fgColor.copy( source->fgColor );
  bgColor.copy( source->bgColor );
  topShadowColor.copy( source->topShadowColor );
  botShadowColor.copy( source->botShadowColor );

  readPvExpStr.copy( source->readPvExpStr );
  labelPvExpStr.copy( source->labelPvExpStr );
  fileNameExpStr.copy( source->fileNameExpStr );

  minW = 50;
  minH = 50;
  center = source->center;
  setSize = source->setSize;
  sizeOfs = source->sizeOfs;
  noScroll = source->noScroll;
  frameWidget = NULL;
  clipWidget = NULL;
  aw = NULL;
  strcpy( curFileName, "" );
  displaySource = source->displaySource;
  readPvId = NULL;
  labelPvId = NULL;
  activateIsComplete = 0;

  for ( i=0; i<maxDsps; i++ ) {
    propagateMacros[i] = source->propagateMacros[i];
    replaceSymbols[i] = source->replaceSymbols[i];
    displayFileName[i].copy( source->displayFileName[i] );
    label[i].copy( source->label[i] );
    symbolsExpStr[i].copy( source->symbolsExpStr[i] );
  }

  numDsps = source->numDsps;
  popUpMenu = NULL;
  unconnectedTimer = 0;
  buf = NULL;

}

activePipClass::~activePipClass ( void ) {

  if ( name ) delete[] name;

  if ( buf ) {
    delete buf;
    buf = NULL;
  }

  if ( unconnectedTimer ) {
    XtRemoveTimeOut( unconnectedTimer );
    unconnectedTimer = 0;
  }

}


int activePipClass::createInteractive (
  activeWindowClass *aw_obj,
  int _x,
  int _y,
  int _w,
  int _h ) {

  actWin = (activeWindowClass *) aw_obj;

  if ( _w < minW )
    w = minW;
  else
    w = _w;

  if ( _h < minH )
    h = minH;
  else
    h = _h;

  x = _x;
  y = _y;

  fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  bgColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  topShadowColor.setColorIndex( actWin->defaultTopShadowColor, actWin->ci );
  botShadowColor.setColorIndex( actWin->defaultBotShadowColor, actWin->ci );

  this->draw();

  this->editCreate();

  return 1;

}

int activePipClass::save (
  FILE *f )
{

int major, minor, release, stat;

tagClass tag;

int zero = 0;
int one = 1;
static char *emptyStr = "";

int displaySourceFromPV = 0;
static char *displaySourceEnumStr[3] = {
  "stringPV",
  "file",
  "menu"
};
static int displaySourceEnum[3] = {
  0,
  1,
  2
};

  major = PIPC_MAJOR_VERSION;
  minor = PIPC_MINOR_VERSION;
  release = PIPC_RELEASE;

  tag.init();
  tag.loadW( "beginObjectProperties" );
  tag.loadW( "major", &major );
  tag.loadW( "minor", &minor );
  tag.loadW( "release", &release );
  tag.loadW( "x", &x );
  tag.loadW( "y", &y );
  tag.loadW( "w", &w );
  tag.loadW( "h", &h );
  tag.loadW( "fgColor", actWin->ci, &fgColor );
  tag.loadW( "bgColor", actWin->ci, &bgColor );
  tag.loadW( "topShadowColor", actWin->ci, &topShadowColor );
  tag.loadW( "botShadowColor", actWin->ci, &botShadowColor );
  tag.loadW( "displaySource", 3, displaySourceEnumStr, displaySourceEnum,
   &displaySource, &displaySourceFromPV );
  tag.loadW( "filePv", &readPvExpStr, emptyStr );
  tag.loadW( "labelPv", &labelPvExpStr, emptyStr );
  tag.loadW( "file", &fileNameExpStr, emptyStr );
  tag.loadBoolW( "center", &center, &zero );
  tag.loadBoolW( "setSize", &setSize, &zero );
  tag.loadW( "sizeOfs", &sizeOfs, &zero );
  tag.loadW( "numDsps", &numDsps );
  tag.loadW( "displayFileName", displayFileName, numDsps, emptyStr );
  tag.loadW( "menuLabel", label, numDsps, emptyStr );
  tag.loadW( "symbols", symbolsExpStr, numDsps, emptyStr );
  tag.loadW( "replaceSymbols", replaceSymbols, numDsps, &zero );
  tag.loadW( "propagateMacros", propagateMacros, numDsps, &one );
  tag.loadBoolW( "noScroll", &noScroll, &zero );
  tag.loadW( "endObjectProperties" );
  tag.loadW( "" );

  stat = tag.writeTags( f );

  return stat;

}

int activePipClass::old_save (
  FILE *f )
{

int index;

  fprintf( f, "%-d %-d %-d\n", PIPC_MAJOR_VERSION,
   PIPC_MINOR_VERSION, PIPC_RELEASE );

  fprintf( f, "%-d\n", x );
  fprintf( f, "%-d\n", y );
  fprintf( f, "%-d\n", w );
  fprintf( f, "%-d\n", h );

  index = fgColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );

  index = bgColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );

  index = topShadowColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );

  index = botShadowColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );

  if ( readPvExpStr.getRaw() )
    writeStringToFile( f, readPvExpStr.getRaw() );
  else
    writeStringToFile( f, "" );

  if ( fileNameExpStr.getRaw() )
    writeStringToFile( f, fileNameExpStr.getRaw() );
  else
    writeStringToFile( f, "" );

  return 1;

}

int activePipClass::createFromFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int major, minor, release, stat, n;

tagClass tag;

int zero = 0;
int one = 1;
static char *emptyStr = "";

int displaySourceFromPV = 0;
static char *displaySourceEnumStr[3] = {
  "stringPV",
  "file",
  "menu"
};
static int displaySourceEnum[3] = {
  0,
  1,
  2
};

  this->actWin = _actWin;

  // read file and process each "object" tag
  tag.init();
  tag.loadR( "beginObjectProperties" );
  tag.loadR( "major", &major );
  tag.loadR( "minor", &minor );
  tag.loadR( "release", &release );
  tag.loadR( "x", &x );
  tag.loadR( "y", &y );
  tag.loadR( "w", &w );
  tag.loadR( "h", &h );
  tag.loadR( "fgColor", actWin->ci, &fgColor );
  tag.loadR( "bgColor", actWin->ci, &bgColor );
  tag.loadR( "topShadowColor", actWin->ci, &topShadowColor );
  tag.loadR( "botShadowColor", actWin->ci, &botShadowColor );
  tag.loadR( "displaySource", 3, displaySourceEnumStr, displaySourceEnum,
   &displaySource, &displaySourceFromPV );
  tag.loadR( "filePv", &readPvExpStr, emptyStr );
  tag.loadR( "labelPv", &labelPvExpStr, emptyStr );
  tag.loadR( "file", &fileNameExpStr, emptyStr );
  tag.loadR( "center", &center, &zero );
  tag.loadR( "setSize", &setSize, &zero );
  tag.loadR( "sizeOfs", &sizeOfs, &zero );
  tag.loadR( "numDsps", &numDsps, &zero );
  tag.loadR( "displayFileName", maxDsps, displayFileName, &n, emptyStr );
  tag.loadR( "menuLabel", maxDsps, label, &n, emptyStr );
  tag.loadR( "symbols", maxDsps, symbolsExpStr, &n, emptyStr );
  tag.loadR( "replaceSymbols", maxDsps, replaceSymbols, &n, &zero );
  tag.loadR( "propagateMacros", maxDsps, propagateMacros, &n, &one );
  tag.loadR( "noScroll", &noScroll, &zero );
  tag.loadR( "endObjectProperties" );

  stat = tag.readTags( f, "endObjectProperties" );

  if ( !( stat & 1 ) ) {
    actWin->appCtx->postMessage( tag.errMsg() );
  }

  if ( major > PIPC_MAJOR_VERSION ) {
    postIncompatable();
    return 0;
  }

  if ( major < 4 ) {
    postIncompatable();
    return 0;
  }

  this->initSelectBox();

  return stat;

}

int activePipClass::old_createFromFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int index;
int major, minor, release;
char oneName[PV_Factory::MAX_PV_NAME+1];
char oneFileName[127+1];

  this->actWin = _actWin;

  fscanf( f, "%d %d %d\n", &major, &minor, &release );

  if ( major > PIPC_MAJOR_VERSION ) {
    postIncompatable();
    return 0;
  }
  fscanf( f, "%d\n", &x );
  fscanf( f, "%d\n", &y );
  fscanf( f, "%d\n", &w );
  fscanf( f, "%d\n", &h );

  this->initSelectBox();

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  fgColor.setColorIndex( index, actWin->ci );

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  bgColor.setColorIndex( index, actWin->ci );

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  topShadowColor.setColorIndex( index, actWin->ci );

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  botShadowColor.setColorIndex( index, actWin->ci );

  readStringFromFile( oneName, PV_Factory::MAX_PV_NAME+1, f );
  readPvExpStr.setRaw( oneName );

  readStringFromFile( oneFileName, 127, f );
  fileNameExpStr.setRaw( oneFileName );

  return 1;

}

int activePipClass::genericEdit ( void ) {

char title[32], *ptr;
int i;

  buf = new bufType;

  ptr = actWin->obj.getNameFromClass( "activePipClass" );
  if ( ptr )
    strncpy( title, ptr, 31 );
  else
    strncpy( title, activePipClass_str4, 31 );

  Strncat( title, activePipClass_str5, 31 );

  buf->bufX = x;
  buf->bufY = y;
  buf->bufW = w;
  buf->bufH = h;

  buf->bufFgColor = fgColor.pixelIndex();
  buf->bufBgColor = bgColor.pixelIndex();
  buf->bufTopShadowColor = topShadowColor.pixelIndex();
  buf->bufBotShadowColor = botShadowColor.pixelIndex();

  if ( readPvExpStr.getRaw() )
    strncpy( buf->bufReadPvName, readPvExpStr.getRaw(),
     PV_Factory::MAX_PV_NAME );
  else
    strcpy( buf->bufReadPvName, "" );

  if ( labelPvExpStr.getRaw() )
    strncpy( buf->bufLabelPvName, labelPvExpStr.getRaw(),
     PV_Factory::MAX_PV_NAME );
  else
    strcpy( buf->bufLabelPvName, "" );

  if ( fileNameExpStr.getRaw() )
    strncpy( buf->bufFileName, fileNameExpStr.getRaw(), 127 );
  else
    strcpy( buf->bufFileName, "" );

  buf->bufDisplaySource = displaySource;

  buf->bufCenter = center;
  buf->bufSetSize = setSize;
  buf->bufSizeOfs = sizeOfs;
  buf->bufNoScroll = noScroll;

  for ( i=0; i<maxDsps; i++ ) {

    if ( displayFileName[i].getRaw() )
      strncpy( buf->bufDisplayFileName[i], displayFileName[i].getRaw(), 127 );
    else
      strncpy( buf->bufDisplayFileName[i], "", 127 );

    if ( label[i].getRaw() )
      strncpy( buf->bufLabel[i], label[i].getRaw(), 127 );
    else
      strncpy( buf->bufLabel[i], "", 127 );

    if ( symbolsExpStr[i].getRaw() ) {
      strncpy( buf->bufSymbols[i], symbolsExpStr[i].getRaw(), 255 );
    }
    else {
      strncpy( buf->bufSymbols[i], "", 255 );
    }

    buf->bufPropagateMacros[i] = propagateMacros[i];

    buf->bufReplaceSymbols[i] = replaceSymbols[i];

  }

  ef.create( actWin->top, actWin->appCtx->ci.getColorMap(),
   &actWin->appCtx->entryFormX,
   &actWin->appCtx->entryFormY, &actWin->appCtx->entryFormW,
   &actWin->appCtx->entryFormH, &actWin->appCtx->largestH,
   title, NULL, NULL, NULL );

  ef.addTextField( activePipClass_str6, 35, &buf->bufX );
  ef.addTextField( activePipClass_str7, 35, &buf->bufY );
  ef.addTextField( activePipClass_str8, 35, &buf->bufW );
  ef.addTextField( activePipClass_str9, 35, &buf->bufH );
  ef.addOption( "Display Source", "String PV|Form|Menu",
   &buf->bufDisplaySource );
  ef.addTextField( activePipClass_str11, 35, buf->bufReadPvName,
   PV_Factory::MAX_PV_NAME );
  ef.addTextField( "Label PV", 35, buf->bufLabelPvName,
   PV_Factory::MAX_PV_NAME );
  ef.addTextField( activePipClass_str12, 35, buf->bufFileName, 127 );
  ef.addToggle( "Center", &buf->bufCenter );
  ef.addToggle( "Set Size", &buf->bufSetSize );
  ef.addTextField( "Size Ofs", 35, &buf->bufSizeOfs );
  ef.addToggle( "Disable Scroll Bars", &buf->bufNoScroll );

  ef.addEmbeddedEf( "Menu Info", "...", &ef1 );

  ef1->create( actWin->top, actWin->appCtx->ci.getColorMap(),
   &actWin->appCtx->entryFormX,
   &actWin->appCtx->entryFormY, &actWin->appCtx->entryFormW,
   &actWin->appCtx->entryFormH, &actWin->appCtx->largestH,
   title, NULL, NULL, NULL );

  for ( i=0; i<maxDsps; i++ ) {

    ef1->beginSubForm();
    ef1->addTextField( "Label", 35, buf->bufLabel[i], 127 );
    ef1->addLabel( "  File" );
    ef1->addTextField( "", 35, buf->bufDisplayFileName[i], 127 );
    ef1->addLabel( "  Macros" );
    ef1->addTextField( "", 35, buf->bufSymbols[i], 255 );
    ef1->endSubForm();

    ef1->beginLeftSubForm();
    ef1->addLabel( "  Mode" );
    ef1->addOption( "", "Append|Replace",
     &buf->bufReplaceSymbols[i] );
    ef1->addLabel( " " );
    ef1->addToggle( " ", &buf->bufPropagateMacros[i] );
    ef1->addLabel( "Propagate  " );
    ef1->endSubForm();

  }

  ef1->finished( pipc_edit_ok1, this );

  ef.addColorButton( activePipClass_str16, actWin->ci, &fgCb,
   &buf->bufFgColor );
  ef.addColorButton( activePipClass_str18, actWin->ci, &bgCb,
   &buf->bufBgColor );
  ef.addColorButton( activePipClass_str19, actWin->ci, &topCb,
   &buf->bufTopShadowColor );
  ef.addColorButton( activePipClass_str20, actWin->ci, &botCb,
   &buf->bufBotShadowColor );

  return 1;

}

int activePipClass::editCreate ( void ) {

  this->genericEdit();
  ef.finished( pipc_edit_ok, pipc_edit_apply, pipc_edit_cancel_delete, this );
  actWin->currentEf = NULL;
  ef.popup();

  return 1;

}

int activePipClass::edit ( void ) {

  this->genericEdit();
  ef.finished( pipc_edit_ok, pipc_edit_apply, pipc_edit_cancel, this );
  actWin->currentEf = &ef;
  ef.popup();

  return 1;

}

int activePipClass::erase ( void ) {

  if ( deleteRequest ) return 1;

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  return 1;

}

int activePipClass::eraseActive ( void ) {

  return 1;

}

int activePipClass::draw ( void ) {

  if ( deleteRequest ) return 1;

  actWin->drawGc.saveFg();

  actWin->drawGc.setFG( bgColor.pixelColor() );
  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, w, h );

  actWin->drawGc.setFG( fgColor.pixelColor() );
  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, w, h );

  actWin->drawGc.restoreFg();

  return 1;

}

int activePipClass::drawActive ( void ) {

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
  }

  if ( !enabled || !activeMode || !init ) return 1;

  if ( aw ) {
    if ( aw->loadFailure ) {
      aw = NULL;
      frameWidget = NULL;
    }
    else {
      if ( frameWidget ) {
        if ( *frameWidget ) XtMapWidget( *frameWidget );
      }
    }
  }

  return 1;

}

int activePipClass::activate (
  int pass,
  void *ptr )
{

int i, n;
Arg args[5];
XmString str;

  switch ( pass ) {

  case 1:

    opComplete = 0;

    break;

  case 2:

    if ( !opComplete ) {

      opComplete = 1;

      initEnable();

      aglPtr = ptr;
      needConnectInit = needUpdate = needMenuConnectInit = needMenuUpdate =
       needDraw = needFileOpen = needInitMenuFileOpen = needUnmap =
       needMap = needToEraseUnconnected = needToDrawUnconnected =
       needConnectTimeout = 0;
      unconnectedTimer = 0;
      activateIsComplete = 0;
      curReadIV = 0;
      strcpy( curReadV, "" );
      firstEvent = 1;
      initialReadConnection = initialMenuConnection =
       initialLabelConnection = 1;

      readPvId = labelPvId = NULL;

      readPvConnected = active = init = 0;
      activeMode = 1;

      if ( !readPvExpStr.getExpanded() ||
            blankOrComment( readPvExpStr.getExpanded() ) ) {
        readExists = 0;
      }
      else {
        readExists = 1;
        fgColor.setConnectSensitive();
      }

      if ( !labelPvExpStr.getExpanded() ||
            blankOrComment( labelPvExpStr.getExpanded() ) ) {
        labelExists = 0;
      }
      else {
        labelExists = 1;
      }

      if ( !fileNameExpStr.getExpanded() ||
            blank( fileNameExpStr.getExpanded() ) ) {
        fileExists = 0;
      }
      else {
        fileExists = 1;
      }

      switch ( displaySource ) {

      case displayFromPV:

        if ( readExists ) {

          if ( !unconnectedTimer ) {
            unconnectedTimer = appAddTimeOut( actWin->appCtx->appContext(),
             10000, unconnectedTimeout, this );
          }

	  readPvId = the_PV_Factory->create( readPvExpStr.getExpanded() );
	  if ( readPvId ) {
	    readPvId->add_conn_state_callback( pip_monitor_read_connect_state,
             this );
	  }
	  else {
            printf( activePipClass_str22 );
          }

        }

        activateIsComplete = 1;

      break;

      case displayFromForm:

        if ( fileExists ) {
          needFileOpen = 1;
          actWin->addDefExeNode( aglPtr );
        }
        else {
          activateIsComplete = 1;
        }

        break;

      case displayFromMenu:

        if ( readExists && ( numDsps > 0 ) ) {

          if ( !unconnectedTimer ) {
            unconnectedTimer = appAddTimeOut( actWin->appCtx->appContext(),
             5000, unconnectedTimeout, this );
          }

	  readPvId = the_PV_Factory->create( readPvExpStr.getExpanded() );
	  if ( readPvId ) {
	    readPvId->add_conn_state_callback( pip_monitor_menu_connect_state,
             this );
	  }
	  else {
            printf( activePipClass_str22 );
          }

          if ( labelExists ) {
	    labelPvId = the_PV_Factory->create( labelPvExpStr.getExpanded() );
	    if ( labelPvId ) {
	      labelPvId->add_conn_state_callback(
               pip_monitor_label_connect_state, this );
	    }
	    else {
              printf( activePipClass_str22 );
            }
	  }

          if ( !popUpMenu ) {

            n = 0;
            XtSetArg( args[n], XmNpopupEnabled, (XtArgVal) False ); n++;
            popUpMenu = XmCreatePopupMenu( actWin->topWidgetId(), "", args,
             n );

            pullDownMenu = XmCreatePulldownMenu( popUpMenu, "", NULL, 0 );

            for ( i=0; i<numDsps; i++ ) {

              if ( label[i].getExpanded() ) {
                str = XmStringCreateLocalized( label[i].getExpanded() );
              }
              else {
                str = XmStringCreateLocalized( " " );
              }
              pb[i] = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
               popUpMenu,
               XmNlabelString, str,
               NULL );
              XmStringFree( str );

              XtAddCallback( pb[i], XmNactivateCallback, menu_cb,
               (XtPointer) this );

            }

          }

#if 0
          if ( !blank( displayFileName[0].getExpanded() ) ) {
            needInitMenuFileOpen = 1;
            actWin->addDefExeNode( aglPtr );
          }
          else {
            activateIsComplete = 1;
          }
#endif

	}
	else {

          activateIsComplete = 1;

        }

        break;

      default:

        activateIsComplete = 1;
        break;

      }

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

int activePipClass::deactivate (
  int pass
) {

int okToClose;
activeWindowListPtr cur;

  if ( pass == 1 ) {

    active = 0;
    activeMode = 0;

    if ( unconnectedTimer ) {
      XtRemoveTimeOut( unconnectedTimer );
      unconnectedTimer = 0;
    }

    if ( aw ) {
      if ( aw->loadFailure ) {
        aw = NULL;
        frameWidget = NULL;
      }
    }

    if ( frameWidget ) {
      if ( *frameWidget ) XtUnmapWidget( *frameWidget );
    }

    if ( aw ) {

      okToClose = 0;
      // make sure the window was successfully opened
      cur = actWin->appCtx->head->flink;
      while ( cur != actWin->appCtx->head ) {
        if ( &cur->node == aw ) {
          okToClose = 1;
          break;
        }
        cur = cur->flink;
      }

      if ( okToClose ) {
        aw->returnToEdit( 1 );
      }

      aw = NULL;

    }

    if ( frameWidget ) {
      frameWidget = NULL;
    }

    if ( readPvId ) {
      readPvId->remove_conn_state_callback( pip_monitor_read_connect_state,
       this );
      if ( !initialReadConnection ) {
        readPvId->remove_value_callback( pip_readUpdate, this );
      }
      if ( !initialMenuConnection ) {
        readPvId->remove_value_callback( pip_menuUpdate, this );
      }
      readPvId->release();
      readPvId = NULL;
    }

    if ( labelPvId ) {
      labelPvId->remove_conn_state_callback( pip_monitor_label_connect_state,
       this );
      labelPvId->release();
      labelPvId = NULL;
    }

    if ( popUpMenu ) {
      XtDestroyWidget( popUpMenu );
      popUpMenu = NULL;
    }

  }

  return 1;

}

int activePipClass::preReactivate (
  int pass ) {

int okToClose;
activeWindowListPtr cur;

  if ( pass == 1 ) {

    active = 0;
    activeMode = 0;

    if ( aw ) {
      if ( aw->loadFailure ) {
        aw = NULL;
        frameWidget = NULL;
      }
    }

    if ( frameWidget ) {
      if ( *frameWidget ) XtUnmapWidget( *frameWidget );
    }

    if ( aw ) {

      okToClose = 0;
      // make sure the window was successfully opened
      cur = actWin->appCtx->head->flink;
      while ( cur != actWin->appCtx->head ) {
        if ( &cur->node == aw ) {
          okToClose = 1;
          break;
        }
        cur = cur->flink;
      }

      if ( okToClose ) {
        aw->returnToEdit( 1 );
      }

      aw = NULL;

    }

    if ( frameWidget ) {
      frameWidget = NULL;
    }

    if ( readPvId ) {
      readPvId->remove_conn_state_callback( pip_monitor_read_connect_state,
       this );
      if ( !initialReadConnection ) {
        readPvId->remove_value_callback( pip_readUpdate, this );
      }
      if ( !initialMenuConnection ) {
        readPvId->remove_value_callback( pip_menuUpdate, this );
      }
      readPvId->release();
      readPvId = NULL;
    }

    if ( labelPvId ) {
      labelPvId->remove_conn_state_callback( pip_monitor_label_connect_state,
       this );
      labelPvId->release();
      labelPvId = NULL;
    }

    if ( popUpMenu ) {
      XtDestroyWidget( popUpMenu );
      popUpMenu = NULL;
    }

  }

  return 1;

}

int activePipClass::reactivate (
  int pass,
  void *ptr ) {

int i, n;
Arg args[5];
XmString str;

  switch ( pass ) {

  case 1:

    opComplete = 0;

    break;

  case 2:

    if ( !opComplete ) {

      opComplete = 1;

      //initEnable();

      aglPtr = ptr;
      needConnectInit = needUpdate = needMenuConnectInit = needMenuUpdate =
       needDraw = needFileOpen = needInitMenuFileOpen = needUnmap =
       needMap = 0;
      activateIsComplete = 0;
      curReadIV = 0;
      strcpy( curReadV, "" );

      readPvId = labelPvId = NULL;

      readPvConnected = active = init = 0;
      activeMode = 1;

      if ( !readPvExpStr.getExpanded() ||
            blankOrComment( readPvExpStr.getExpanded() ) ) {
        readExists = 0;
      }
      else {
        readExists = 1;
        fgColor.setConnectSensitive();
      }

      if ( !labelPvExpStr.getExpanded() ||
            blankOrComment( labelPvExpStr.getExpanded() ) ) {
        labelExists = 0;
      }
      else {
        labelExists = 1;
      }

      if ( !fileNameExpStr.getExpanded() ||
            blank( fileNameExpStr.getExpanded() ) ) {
        fileExists = 0;
      }
      else {
        fileExists = 1;
      }

      switch ( displaySource ) {

      case displayFromPV:

        if ( readExists ) {

          if ( !unconnectedTimer ) {
            unconnectedTimer = appAddTimeOut( actWin->appCtx->appContext(),
             5000, unconnectedTimeout, this );
          }

 	  readPvId = the_PV_Factory->create( readPvExpStr.getExpanded() );
	  if ( readPvId ) {
	    readPvId->add_conn_state_callback( pip_monitor_read_connect_state,
             this );
	  }
	  else {
            printf( activePipClass_str22 );
          }

        }

        activateIsComplete = 1;

        break;

      case displayFromForm:

        if ( fileExists ) {
          needFileOpen = 1;
          actWin->addDefExeNode( aglPtr );
        }
        else {
          activateIsComplete = 1;
        }

        break;

      case displayFromMenu:

        if ( readExists && ( numDsps > 0 ) ) {

          if ( !unconnectedTimer ) {
            unconnectedTimer = appAddTimeOut( actWin->appCtx->appContext(),
             5000, unconnectedTimeout, this );
          }

	  readPvId = the_PV_Factory->create( readPvExpStr.getExpanded() );
	  if ( readPvId ) {
	    readPvId->add_conn_state_callback( pip_monitor_menu_connect_state,
             this );
	  }
	  else {
            printf( activePipClass_str22 );
          }

          if ( labelExists ) {
	    labelPvId = the_PV_Factory->create( labelPvExpStr.getExpanded() );
	    if ( labelPvId ) {
	      labelPvId->add_conn_state_callback(
               pip_monitor_label_connect_state, this );
	    }
	    else {
              printf( activePipClass_str22 );
            }
	  }

          if ( !popUpMenu ) {

            n = 0;
            XtSetArg( args[n], XmNpopupEnabled, (XtArgVal) False ); n++;
            popUpMenu = XmCreatePopupMenu( actWin->topWidgetId(), "", args,
             n );

            pullDownMenu = XmCreatePulldownMenu( popUpMenu, "", NULL, 0 );

            for ( i=0; i<numDsps; i++ ) {

              if ( label[i].getExpanded() ) {
                str = XmStringCreateLocalized( label[i].getExpanded() );
              }
              else {
                str = XmStringCreateLocalized( " " );
              }
              pb[i] = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
               popUpMenu,
               XmNlabelString, str,
               NULL );
              XmStringFree( str );

              XtAddCallback( pb[i], XmNactivateCallback, menu_cb,
               (XtPointer) this );

            }

          }

	}
	else {

          activateIsComplete = 1;

        }

        break;

      default:

        activateIsComplete = 1;
        break;

      }

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

int activePipClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat, retStat, i;

  retStat = readPvExpStr.expand1st( numMacros, macros, expansions );

  stat = labelPvExpStr.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;

  stat = fileNameExpStr.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;

  for ( i=0; i<numDsps; i++ ) {
    stat = symbolsExpStr[i].expand1st( numMacros, macros, expansions );
    if ( !( stat & 1 ) ) retStat = stat;
    stat = label[i].expand1st( numMacros, macros, expansions );
    if ( !( stat & 1 ) ) retStat = stat;
    stat = displayFileName[i].expand1st( numMacros, macros, expansions );
    if ( !( stat & 1 ) ) retStat = stat;
  }

  return retStat;

}

int activePipClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat, retStat, i;

  retStat = readPvExpStr.expand2nd( numMacros, macros, expansions );

  stat = labelPvExpStr.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;

  stat = fileNameExpStr.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;

  for ( i=0; i<numDsps; i++ ) {
    stat = symbolsExpStr[i].expand2nd( numMacros, macros, expansions );
    if ( !( stat & 1 ) ) retStat = stat;
    stat = label[i].expand2nd( numMacros, macros, expansions );
    if ( !( stat & 1 ) ) retStat = stat;
    stat = displayFileName[i].expand2nd( numMacros, macros, expansions );
    if ( !( stat & 1 ) ) retStat = stat;
  }

  return retStat;

}

int activePipClass::containsMacros ( void ) {

int i;

  if ( readPvExpStr.containsPrimaryMacros() ) return 1;

  if ( labelPvExpStr.containsPrimaryMacros() ) return 1;

  if ( fileExists && fileNameExpStr.containsPrimaryMacros() ) return 1;

  for ( i=0; i<numDsps; i++ ) {
    if ( symbolsExpStr[i].containsPrimaryMacros() ) return 1;
    if ( label[i].containsPrimaryMacros() ) return 1;
    if ( displayFileName[i].containsPrimaryMacros() ) return 1;
  }

  return 0;

}

int activePipClass::createPipWidgets ( void ) {

  frameWidget = new Widget;
  *frameWidget = NULL;

  if ( noScroll ) {

    *frameWidget = XtVaCreateWidget( "", xmBulletinBoardWidgetClass,
     actWin->executeWidgetId(),
     XmNx, x,
     XmNy, y,
     XmNwidth, w,
     XmNheight, h,
     XmNnoResize, True,
     XmNresizePolicy, XmRESIZE_NONE,
     XmNmarginWidth, 0,
     XmNmarginHeight, 0,
     XmNtopShadowColor, topShadowColor.pixelColor(),
     XmNbottomShadowColor, botShadowColor.pixelColor(),
     XmNborderColor, bgColor.pixelColor(),
     XmNhighlightColor, bgColor.pixelColor(),
     XmNforeground, bgColor.pixelColor(),
     XmNbackground, bgColor.pixelColor(),
     NULL );

    if ( !(*frameWidget) ) {
      printf( activePipClass_str24 );
      frameWidget = NULL;
      return 0;
    }

  }
  else {

    *frameWidget = XtVaCreateWidget( "", xmScrolledWindowWidgetClass,
     actWin->executeWidgetId(),
     XmNx, x,
     XmNy, y,
     XmNwidth, w,
     XmNheight, h,
     XmNscrollBarDisplayPolicy, XmAS_NEEDED,
     XmNscrollingPolicy, XmAUTOMATIC,
     XmNvisualPolicy, XmCONSTANT,
     XmNmarginWidth, 0,
     XmNmarginHeight, 0,
     XmNtopShadowColor, topShadowColor.pixelColor(),
     XmNbottomShadowColor, botShadowColor.pixelColor(),
     XmNborderColor, bgColor.pixelColor(),
     XmNhighlightColor, bgColor.pixelColor(),
     XmNforeground, bgColor.pixelColor(),
     XmNbackground, bgColor.pixelColor(),
     NULL );

    if ( !(*frameWidget) ) {
      printf( activePipClass_str24 );
      frameWidget = NULL;
      return 0;
    }

    XtVaGetValues( *frameWidget,
     XmNclipWindow, &clipWidget,
     XmNhorizontalScrollBar, &hsbWidget,
     XmNverticalScrollBar, &vsbWidget,
     NULL );

    if ( clipWidget ) {
      XtVaSetValues( clipWidget,
        XmNtopShadowColor, topShadowColor.pixelColor(),
        XmNbottomShadowColor, botShadowColor.pixelColor(),
        XmNborderColor, bgColor.pixelColor(),
        XmNhighlightColor, bgColor.pixelColor(),
        XmNforeground, bgColor.pixelColor(),
        XmNbackground, bgColor.pixelColor(),
       NULL );
    }

    if ( hsbWidget ) {
      XtVaSetValues( hsbWidget,
        XmNtopShadowColor, topShadowColor.pixelColor(),
        XmNbottomShadowColor, botShadowColor.pixelColor(),
        XmNborderColor, bgColor.pixelColor(),
        XmNhighlightColor, bgColor.pixelColor(),
        XmNforeground, bgColor.pixelColor(),
        XmNbackground, bgColor.pixelColor(),
        XmNtroughColor, bgColor.pixelColor(),
        NULL );
    }

    if ( vsbWidget ) {
      XtVaSetValues( vsbWidget,
        XmNtopShadowColor, topShadowColor.pixelColor(),
        XmNbottomShadowColor, botShadowColor.pixelColor(),
        XmNborderColor, bgColor.pixelColor(),
        XmNhighlightColor, bgColor.pixelColor(),
        XmNforeground, bgColor.pixelColor(),
        XmNbackground, bgColor.pixelColor(),
        XmNtroughColor, bgColor.pixelColor(),
        NULL );
    }

  }

  return 1;

}

int activePipClass::checkResizeSelectBox (
  int _x,
  int _y,
  int _w,
  int _h ) {

int tmpw, tmph, ret_stat;

  tmpw = sboxW;
  tmph = sboxH;

  ret_stat = 1;

  tmpw += _w;
  if ( tmpw < minW ) {
    ret_stat = 0;
  }

  tmph += _h;
  if ( tmph < minH ) {
    ret_stat = 0;
  }

  return ret_stat;

}

int activePipClass::checkResizeSelectBoxAbs (
  int _x,
  int _y,
  int _w,
  int _h ) {

int tmpw, tmph, ret_stat;

  ret_stat = 1;

  if ( _w != -1 ) {
    tmpw = _w;
    if ( tmpw < minW ) {
      ret_stat = 0;
    }
  }

  if ( _h != -1 ) {
    tmph = _h;
    if ( tmph < minH ) {
      ret_stat = 0;
    }
  }

  return ret_stat;

}

void activePipClass::openEmbeddedByIndex (
  int index )
{

activeWindowListPtr cur;
int i, l, stat;
char symbolsWithSubs[255+1];
int useSmallArrays, symbolCount, maxSymbolLength;
char smallNewMacros[SMALL_SYM_ARRAY_SIZE+1][SMALL_SYM_ARRAY_LEN+1+1];
char smallNewValues[SMALL_SYM_ARRAY_SIZE+1][SMALL_SYM_ARRAY_LEN+1+1];
char *newMacros[100];
char *newValues[100];
int numNewMacros, max, numFound;

char *formTk, *formContext, formBuf[255+1], *fileTk, *fileContext, fileBuf[255+1],
 *result, msg[79+1], macDefFileName[127+1];
FILE *f;
expStringClass symbolsFromFile;
int gotSymbolsFromFile;

  // allow the syntax: @filename s1=v1,s2=v2,...
  // which means read symbols from file and append list
  gotSymbolsFromFile = 0;
  strncpy( formBuf, symbolsExpStr[index].getExpanded(), 255 );
  formBuf[255] = 0;
  formContext = NULL;
  formTk = strtok_r( formBuf, " \t\n", &formContext );
  if ( formTk ) {
    if ( formTk[0] == '@' ) {
      if ( formTk[1] ) {
        f = actWin->openAnyGenericFile( &formTk[1], "r", macDefFileName, 127 );
	if ( !f ) {
          snprintf( msg, 79, activePipClass_str27, &formTk[1] );
	  msg[79] = 0;
          actWin->appCtx->postMessage( msg );
          symbolsFromFile.setRaw( "" );
	}
	else {
	  result = fgets( fileBuf, 255, f );
	  if ( result ) {
            fileContext = NULL;
            fileTk = strtok_r( fileBuf, "\n", &fileContext );
            if ( fileTk ) {
              symbolsFromFile.setRaw( fileTk );
	    }
	    else {
              snprintf( msg, 79, activePipClass_str28, macDefFileName );
              msg[79] = 0;
              actWin->appCtx->postMessage( msg );
              symbolsFromFile.setRaw( "" );
	    }
	  }
	  else {
            if ( errno ) {
              snprintf( msg, 79, activePipClass_str29, macDefFileName );
	    }
	    else {
              snprintf( msg, 79, activePipClass_str28, macDefFileName );
	    }
            msg[79] = 0;
            actWin->appCtx->postMessage( msg );
            symbolsFromFile.setRaw( "" );
	  }
	  fclose( f );
	}
      }
      // append inline list to file contents
      formTk = strtok_r( NULL, "\n", &formContext );
      if ( formTk ) {
        strncpy( fileBuf, symbolsFromFile.getRaw(), 255 );
        fileBuf[255] = 0;
        if ( blank(fileBuf) ) {
          strcpy( fileBuf, "" );
	}
        else {
          Strncat( fileBuf, ",", 255 );
	}
	Strncat( fileBuf, formTk, 255 );
        symbolsFromFile.setRaw( fileBuf );
      }
      // do special substitutions
      actWin->substituteSpecial( 255, symbolsFromFile.getExpanded(),
       symbolsWithSubs );
      gotSymbolsFromFile = 1;
    }
  }

  if ( !gotSymbolsFromFile ) {
    // do special substitutions
    actWin->substituteSpecial( 255, symbolsExpStr[index].getExpanded(),
     symbolsWithSubs );
  }

  numNewMacros = 0;

  // get info on whether to use the small local array for symbols
  stat = countSymbolsAndValues( symbolsWithSubs, &symbolCount,
   &maxSymbolLength );

  if ( !replaceSymbols[index] ) {

    if ( propagateMacros[index] ) {

      for ( i=0; i<actWin->numMacros; i++ ) {

        l = strlen(actWin->macros[i]);
        if ( l > maxSymbolLength ) maxSymbolLength = l;

        l = strlen(actWin->expansions[i]);
        if ( l > maxSymbolLength ) maxSymbolLength = l;

      }

      symbolCount += actWin->numMacros;

    }
    else {

      for ( i=0; i<actWin->appCtx->numMacros; i++ ) {

        l = strlen(actWin->appCtx->macros[i]);
        if ( l > maxSymbolLength ) maxSymbolLength = l;

        l = strlen(actWin->appCtx->expansions[i]);
        if ( l > maxSymbolLength ) maxSymbolLength = l;

      }

      symbolCount += actWin->appCtx->numMacros;

    }

  }

  useSmallArrays = 1;
  if ( symbolCount > SMALL_SYM_ARRAY_SIZE ) useSmallArrays = 0;
  if ( maxSymbolLength > SMALL_SYM_ARRAY_LEN ) useSmallArrays = 0;

  if ( useSmallArrays ) {

    for ( i=0; i<SMALL_SYM_ARRAY_SIZE; i++ ) {
      newMacros[i] = &smallNewMacros[i][0];
      newValues[i] = &smallNewValues[i][0];
    }

    if ( !replaceSymbols[index] ) {

      if ( propagateMacros[index] ) {

        for ( i=0; i<actWin->numMacros; i++ ) {

          strcpy( newMacros[i], actWin->macros[i] );

          strcpy( newValues[i], actWin->expansions[i] );

          numNewMacros++;

        }

      }
      else {

        for ( i=0; i<actWin->appCtx->numMacros; i++ ) {

          strcpy( newMacros[i], actWin->appCtx->macros[i] );

          strcpy( newValues[i], actWin->appCtx->expansions[i] );

          numNewMacros++;

        }

      }

    }

    max = SMALL_SYM_ARRAY_SIZE - numNewMacros;
    stat = parseLocalSymbolsAndValues( symbolsWithSubs, max,
     SMALL_SYM_ARRAY_LEN, &newMacros[numNewMacros], &newValues[numNewMacros],
     &numFound );
    numNewMacros += numFound;

  }
  else {

    if ( !replaceSymbols[index] ) {

      if ( propagateMacros[index] ) {

        for ( i=0; i<actWin->numMacros; i++ ) {

          l = strlen(actWin->macros[i]) + 1;
          newMacros[i] = (char *) new char[l];
          strcpy( newMacros[i], actWin->macros[i] );

          l = strlen(actWin->expansions[i]) + 1;
          newValues[i] = (char *) new char[l];
          strcpy( newValues[i], actWin->expansions[i] );

          numNewMacros++;

        }

      }
      else {

        for ( i=0; i<actWin->appCtx->numMacros; i++ ) {

          l = strlen(actWin->appCtx->macros[i]) + 1;
          newMacros[i] = (char *) new char[l];
          strcpy( newMacros[i], actWin->appCtx->macros[i] );

          l = strlen(actWin->appCtx->expansions[i]) + 1;
          newValues[i] = (char *) new char[l];
          strcpy( newValues[i], actWin->appCtx->expansions[i] );

          numNewMacros++;

        }

      }

    }

    max = 100 - numNewMacros;
    stat = parseSymbolsAndValues( symbolsWithSubs, max,
     &newMacros[numNewMacros], &newValues[numNewMacros], &numFound );
    numNewMacros += numFound;

  }

  cur = new activeWindowListType;
  actWin->appCtx->addActiveWindow( cur );

  cur->node.createEmbedded( actWin->appCtx, frameWidget, 0, 0, w, h,
   x, y, center, setSize, sizeOfs, noScroll, numNewMacros, newMacros,
   newValues );

  cur->node.realize();

  cur->node.setGraphicEnvironment( &cur->node.appCtx->ci,
   &cur->node.appCtx->fi );

  if ( index < 0 ) index = 0;
  if ( index >= numDsps ) index = numDsps;
  cur->node.storeFileName( displayFileName[index].getExpanded() );

  actWin->appCtx->openActivateActiveWindow( &cur->node, 0, 0 );

  aw = &cur->node;

  aw->parent = actWin;
  (actWin->numChildren)++;

  activateIsComplete = 1;

  if ( !useSmallArrays ) {

    for ( i=0; i<numNewMacros; i++ ) {
      delete[] newMacros[i];
      delete[] newValues[i];
    }

  }

}

void activePipClass::executeDeferred ( void ) {

int iv;
char v[39+1];
int i, nc, nu, nmc, nmu, nd, nfo, nimfo, ncto, nmap, nunmap, okToClose;
activeWindowListPtr cur;
Window root, child;
int rootX, rootY, winX, winY;
unsigned int mask;
XButtonEvent be;

//----------------------------------------------------------------------------

  actWin->appCtx->proc->lock();
  nc = needConnectInit; needConnectInit = 0;
  nu = needUpdate; needUpdate = 0;
  nmc = needMenuConnectInit; needMenuConnectInit = 0;
  nmu = needMenuUpdate; needMenuUpdate = 0;
  nd = needDraw; needDraw = 0;
  nfo = needFileOpen; needFileOpen = 0;
  nimfo = needInitMenuFileOpen; needInitMenuFileOpen = 0;
  ncto = needConnectTimeout; needConnectTimeout = 0;
  nmap = needMap; needMap = 0;
  nunmap = needUnmap; needUnmap = 0;
  strncpy( v, curReadV, 39 );
  iv = curReadIV;
  actWin->remDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

  if ( !activeMode ) return;

//----------------------------------------------------------------------------

  if ( nc ) {

    readPvConnected = 1;
    active = 1;
    init = 1;

    if ( initialReadConnection ) {

      initialReadConnection = 0;

      readPvId->add_value_callback( pip_readUpdate, this );

    }

    fgColor.setConnected();
    drawActive();

  }

  if ( nmc ) {

    readPvConnected = 1;
    active = 1;
    init = 1;

    if ( initialMenuConnection ) {

      initialMenuConnection = 0;

      readPvId->add_value_callback( pip_menuUpdate, this );

    }

    fgColor.setConnected();
    drawActive();

  }

  if ( nu ) {

    strncpy( readV, v, 39 );
    //printf( "readV = [%s]\n", readV );

    if ( enabled && !blank( readV ) ) {

      // close old

      if ( frameWidget ) {
        if ( *frameWidget ) XtUnmapWidget( *frameWidget );
      }

      if ( aw ) {

        okToClose = 0;
        // make sure the window was successfully opened
        cur = actWin->appCtx->head->flink;
        while ( cur != actWin->appCtx->head ) {
          if ( &cur->node == aw ) {
            okToClose = 1;
            break;
          }
          cur = cur->flink;
        }

        if ( okToClose ) {
          aw->returnToEdit( 1 ); // this frees frameWidget
        }

        aw = NULL;

      }

      if ( frameWidget ) {
        frameWidget = NULL;
      }

      // prevent possible mutual recursion
      if ( actWin->sameAncestorName( readV ) ) {

        actWin->appCtx->postMessage( activePipClass_str26 );
        activateIsComplete = 1;

      }
      else {

        // open new

        if ( !frameWidget ) {
          createPipWidgets();
        }

        if ( !aw ) {

          //printf( "Open file %s\n", readV );

          strncpy( curFileName, readV, 127 );
          curFileName[127] = 0;

          cur = new activeWindowListType;
          actWin->appCtx->addActiveWindow( cur );

          cur->node.createEmbedded( actWin->appCtx, frameWidget, 0, 0, w, h,
           x, y, center, setSize, sizeOfs, noScroll, actWin->numMacros,
           actWin->macros, actWin->expansions );

          cur->node.realize();

          cur->node.setGraphicEnvironment( &cur->node.appCtx->ci,
           &cur->node.appCtx->fi );

          cur->node.storeFileName( readV );

          actWin->appCtx->openActivateActiveWindow( &cur->node, 0, 0 );

          aw = &cur->node;

          aw->parent = actWin;
          (actWin->numChildren)++;

          activateIsComplete = 1;

          drawActive();

        }

      }

    }
    else {

      activateIsComplete = 1;

    }

    if ( !enabled ) { // copy filename to be used when enabled becomes true
      strncpy( curFileName, readV, 127 );
      curFileName[127] = 0;
    }

  }

//----------------------------------------------------------------------------

  if ( nmu ) {

    i = iv;

    if ( enabled ) {

      if ( i == -1 ) {

        XQueryPointer( actWin->d, XtWindow(actWin->top), &root, &child,
         &rootX, &rootY, &winX, &winY, &mask );
        be.x_root = rootX;
        be.y_root = rootY;
        be.x = 0;
        be.y = 0;
        XmMenuPosition( popUpMenu, &be );
        XtManageChild( popUpMenu );

      }
      else {

        if ( i < numDsps ) {

          if ( !blank( displayFileName[i].getExpanded() ) ) {

            // close old

            if ( frameWidget ) {
              if ( *frameWidget ) XtUnmapWidget( *frameWidget );
            }

            if ( aw ) {

              okToClose = 0;
              // make sure the window was successfully opened
              cur = actWin->appCtx->head->flink;
              while ( cur != actWin->appCtx->head ) {
                if ( &cur->node == aw ) {
                  okToClose = 1;
                  break;
                }
                cur = cur->flink;
              }

              if ( okToClose ) {
                aw->returnToEdit( 1 ); // this frees frameWidget
              }

              aw = NULL;

            }

            if ( frameWidget ) {
              frameWidget = NULL;
            }

            // prevent possible mutual recursion
            if (actWin->sameAncestorName( displayFileName[i].getExpanded() )) {

              actWin->appCtx->postMessage( activePipClass_str26 );
              activateIsComplete = 1;

            }
            else {

              // open new

              if ( !frameWidget ) {
                createPipWidgets();
              }

              if ( !aw ) {

                //printf( "Open file %s\n", readV );

                strncpy( curFileName, displayFileName[i].getExpanded(), 127 );
                curFileName[127] = 0;

                openEmbeddedByIndex( i );

                if ( labelPvId ) {
		  labelPvId->putText( label[i].getExpanded() );
		}

                drawActive();

              }

            }

          }
	  else {

	    activateIsComplete = 1;

	  }

        }
	else {

	  activateIsComplete = 1;

	}

      }

    }
    else {

      activateIsComplete = 1;

    }

    if ( !enabled ) { // copy filename to be used when enabled becomes true
      strncpy( curFileName, displayFileName[i].getExpanded(), 127 );
      curFileName[127] = 0;
    }

  }

//----------------------------------------------------------------------------

  if ( nd ) {
    drawActive();
  }

//----------------------------------------------------------------------------

  if ( nfo ) {

    if ( enabled && fileExists ) {

      //printf( "Open file %s\n", fileNameExpStr.getExpanded() );

      strncpy( curFileName, fileNameExpStr.getExpanded(), 127 );
      curFileName[127] = 0;

      // prevent possible mutual recursion
      if ( actWin->sameAncestorName( curFileName ) ) {

        actWin->appCtx->postMessage( activePipClass_str26 );
        activateIsComplete = 1;

      }
      else {

        if ( !frameWidget ) {
          createPipWidgets();
        }

        if ( !aw ) {

          cur = new activeWindowListType;
          actWin->appCtx->addActiveWindow( cur );

          cur->node.createEmbedded( actWin->appCtx, frameWidget, 0, 0, w, h,
           x, y, center, setSize, sizeOfs, noScroll, actWin->numMacros,
           actWin->macros, actWin->expansions );

          cur->node.realize();

          cur->node.setGraphicEnvironment( &cur->node.appCtx->ci,
           &cur->node.appCtx->fi );

          cur->node.storeFileName( fileNameExpStr.getExpanded() );

          actWin->appCtx->openActivateActiveWindow( &cur->node, 0, 0 );

          aw = &cur->node;

          aw->parent = actWin;
          (actWin->numChildren)++;

          activateIsComplete = 1;

        }

      }

    }

    if ( !enabled ) { // copy filename to be used when enabled becomes true
      strncpy( curFileName, fileNameExpStr.getExpanded(), 127 );
      curFileName[127] = 0;
    }

  }

//----------------------------------------------------------------------------

  if ( nimfo ) {

    if ( enabled ) {

      strncpy( curFileName, displayFileName[0].getExpanded(), 127 );
      curFileName[127] = 0;

      // prevent possible mutual recursion
      if ( actWin->sameAncestorName( curFileName ) ) {

        actWin->appCtx->postMessage( activePipClass_str26 );
        activateIsComplete = 1;

      }
      else {

        if ( !frameWidget ) {
          createPipWidgets();
        }

        if ( !aw ) {

	  openEmbeddedByIndex( 0 );

          if ( labelPvId ) {
            labelPvId->putText( label[0].getExpanded() );
	  }
        }

      }

    }

    if ( !enabled ) { // copy filename to be used when enabled becomes true
      strncpy( curFileName, displayFileName[0].getExpanded(), 127 );
      curFileName[127] = 0;
    }

  }

//----------------------------------------------------------------------------

  if ( ncto ) {

    activateIsComplete = 1;

    drawActive();

  }

//----------------------------------------------------------------------------

  if ( nunmap ) {

    if ( frameWidget ) {
      if ( *frameWidget ) XtUnmapWidget( *frameWidget );
    }

    if ( aw ) {

      okToClose = 0;
      // make sure the window was successfully opened
      cur = actWin->appCtx->head->flink;
      while ( cur != actWin->appCtx->head ) {
        if ( &cur->node == aw ) {
          okToClose = 1;
          break;
        }
        cur = cur->flink;
      }

      if ( okToClose ) {
        aw->returnToEdit( 1 ); // this frees frameWidget
      }

      aw = NULL;

    }

    if ( frameWidget ) {
      frameWidget = NULL;
    }

  }

//----------------------------------------------------------------------------

  if ( nmap ) {

    // curFileName should contain the file to open

    if ( !blank( curFileName ) ) {

      // close old ( however, one should not be open )

      if ( frameWidget ) {
        if ( *frameWidget ) XtUnmapWidget( *frameWidget );
      }

      if ( aw ) {

        okToClose = 0;
        // make sure the window was successfully opened
        cur = actWin->appCtx->head->flink;
        while ( cur != actWin->appCtx->head ) {
          if ( &cur->node == aw ) {
            okToClose = 1;
            break;
          }
          cur = cur->flink;
        }

        if ( okToClose ) {
          aw->returnToEdit( 1 ); // this frees frameWidget
        }

        aw = NULL;

      }

      if ( frameWidget ) {
        frameWidget = NULL;
      }

      // prevent possible mutual recursion
      if ( actWin->sameAncestorName( curFileName ) ) {

        actWin->appCtx->postMessage( activePipClass_str26 );
        activateIsComplete = 1;

      }
      else {

        // open new

        if ( !frameWidget ) {
          createPipWidgets();
        }

        if ( !aw ) {

          cur = new activeWindowListType;
          actWin->appCtx->addActiveWindow( cur );

          cur->node.createEmbedded( actWin->appCtx, frameWidget, 0, 0, w, h,
           x, y, center, setSize, sizeOfs, noScroll, actWin->numMacros,
           actWin->macros, actWin->expansions );

          cur->node.realize();

          cur->node.setGraphicEnvironment( &cur->node.appCtx->ci,
           &cur->node.appCtx->fi );

          cur->node.storeFileName( curFileName );

          actWin->appCtx->openActivateActiveWindow( &cur->node, 0, 0 );

          aw = &cur->node;

          aw->parent = actWin;
          (actWin->numChildren)++;

          drawActive();

        }

      }

    }

  }

//----------------------------------------------------------------------------

}

void activePipClass::changeDisplayParams (
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
    fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );

  if ( _flag & ACTGRF_BGCOLOR_MASK )
    bgColor.setColorIndex( actWin->defaultBgColor, actWin->ci );

  if ( _flag & ACTGRF_TOPSHADOWCOLOR_MASK )
    topShadowColor.setColorIndex( actWin->defaultTopShadowColor, actWin->ci );

  if ( _flag & ACTGRF_BOTSHADOWCOLOR_MASK )
    botShadowColor.setColorIndex( actWin->defaultBotShadowColor, actWin->ci );

}

void activePipClass::changePvNames (
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

  if ( flag & ACTGRF_READBACKPVS_MASK ) {
    if ( numReadbackPvs ) {
      readPvExpStr.setRaw( readbackPvs[0] );
    }
  }

}

int activePipClass::isWindowContainer ( void ) {

  return 1;

}

int activePipClass::activateComplete ( void ) {

int flag;

  if ( aw ) {
    if ( aw->loadFailure ) {
      activateIsComplete = 1;
    }
  }

  if ( !activateIsComplete ) return 0;

  if ( aw ) {
    if ( aw->isExecuteMode() || aw->loadFailure ) {
      flag = aw->okToDeactivate();
    }
    else {
      flag = 0;
    }
  }
  else {
    flag = 1;
  }

  return flag;

}

void activePipClass::map ( void ) {

  needMap = 1;
  actWin->addDefExeNode( aglPtr );

}

void activePipClass::unmap ( void ) {

  needUnmap = 1;
  actWin->addDefExeNode( aglPtr );

}

#ifdef __cplusplus
extern "C" {
#endif

void *create_activePipClassPtr ( void ) {

activePipClass *ptr;

  ptr = new activePipClass;
  return (void *) ptr;

}

void *clone_activePipClassPtr (
  void *_srcPtr )
{

activePipClass *ptr, *srcPtr;

  srcPtr = (activePipClass *) _srcPtr;

  ptr = new activePipClass( srcPtr );

  return (void *) ptr;

}

#ifdef __cplusplus
}
#endif
