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

#define __x_text_obj_cc 1

#include "x_text_obj.h"
#include "app_pkg.h"
#include "act_win.h"

#include "thread.h"

#ifdef __epics__

static void axtoMonitorAlarmPvConnectState (
  struct connection_handler_args arg )
{

activeXTextClass *axto = (activeXTextClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    axto->needAlarmConnectInit = 1;

  }
  else { // lost connection

    axto->alarmPvConnected = 0;
    axto->active = 0;
    axto->fgColor.setDisconnected();
    axto->bgColor.setDisconnected();
    axto->bufInvalidate();
    axto->needDraw = 1;

  }

  axto->actWin->appCtx->proc->lock();
  axto->actWin->addDefExeNode( axto->aglPtr );
  axto->actWin->appCtx->proc->unlock();

}

static void xTextAlarmUpdate (
  struct event_handler_args ast_args )
{

class activeXTextClass *axto;
struct dbr_sts_float statusRec;

  axto = (activeXTextClass *) ast_args.usr;

  statusRec = *( (struct dbr_sts_float *) ast_args.dbr );
  axto->fgColor.setStatus( statusRec.status, statusRec.severity );
  axto->bgColor.setStatus( statusRec.status, statusRec.severity );

  if ( axto->active ) {
    axto->bufInvalidate();
    axto->needRefresh = 1;
    axto->actWin->appCtx->proc->lock();
    axto->actWin->addDefExeNode( axto->aglPtr );
    axto->actWin->appCtx->proc->unlock();
  }

}

static void axtoMonitorVisPvConnectState (
  struct connection_handler_args arg )
{

activeXTextClass *axto = (activeXTextClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    axto->needVisConnectInit = 1;

  }
  else { // lost connection

    axto->visPvConnected = 0;
    axto->active = 0;
    axto->fgColor.setDisconnected();
    axto->bgColor.setDisconnected();
    axto->bufInvalidate();
    axto->needDraw = 1;

  }

  axto->actWin->appCtx->proc->lock();
  axto->actWin->addDefExeNode( axto->aglPtr );
  axto->actWin->appCtx->proc->unlock();

}

static void xTextVisUpdate (
  struct event_handler_args ast_args )
{

pvValType pvV;
class activeXTextClass *axto = (activeXTextClass *) ast_args.usr;

  pvV.d = *( (double *) ast_args.dbr );
  if ( ( pvV.d >= axto->minVis.d ) && ( pvV.d < axto->maxVis.d ) )
    axto->visibility = 1 ^ axto->visInverted;
  else
    axto->visibility = 0 ^ axto->visInverted;

  if ( axto->active ) {

    if ( axto->visibility ) {

      axto->needRefresh = 1;

    }
    else {

      axto->needErase = 1;
      axto->needRefresh = 1;

    }

    axto->actWin->appCtx->proc->lock();
    axto->actWin->addDefExeNode( axto->aglPtr );
    axto->actWin->appCtx->proc->unlock();

  }

}

#endif

static void axtc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextClass *axto = (activeXTextClass *) client;

  axto->actWin->setChanged();

  axto->eraseSelectBoxCorners();
  axto->erase();

  strncpy( axto->id, axto->bufId, 31 );

  axto->fgColorMode = axto->bufFgColorMode;
  if ( axto->fgColorMode == AXTC_K_COLORMODE_ALARM )
    axto->fgColor.setAlarmSensitive();
  else
    axto->fgColor.setAlarmInsensitive();
  axto->fgColor.setColor( axto->bufFgColor, axto->actWin->ci );

  axto->bgColorMode = axto->bufBgColorMode;
  if ( axto->bgColorMode == AXTC_K_COLORMODE_ALARM )
    axto->bgColor.setAlarmSensitive();
  else
    axto->bgColor.setAlarmInsensitive();
  axto->bgColor.setColor( axto->bufBgColor, axto->actWin->ci );

  axto->alarmPvExpStr.setRaw( axto->bufAlarmPvName );

  axto->visPvExpStr.setRaw( axto->bufVisPvName );

  if ( axto->bufVisInverted )
    axto->visInverted = 0;
  else
    axto->visInverted = 1;

  strncpy( axto->minVisString, axto->bufMinVisString, 39 );
  strncpy( axto->maxVisString, axto->bufMaxVisString, 39 );

  axto->value.setRaw( axto->bufValue );

  strncpy( axto->fontTag, axto->fm.currentFontTag(), 63 );
  axto->actWin->fi->loadFontTag( axto->fontTag );
  axto->actWin->drawGc.setFontTag( axto->fontTag, axto->actWin->fi );

  axto->stringLength = strlen( axto->value.getRaw() );

  axto->fs = axto->actWin->fi->getXFontStruct( axto->fontTag );

  axto->updateFont( axto->value.getRaw(), axto->fontTag, &axto->fs,
   &axto->fontAscent, &axto->fontDescent, &axto->fontHeight,
   &axto->stringWidth );

  axto->stringY = axto->y + axto->fontAscent;

  axto->alignment = axto->fm.currentFontAlignment();

  if ( axto->alignment == XmALIGNMENT_BEGINNING )
    axto->stringX = axto->x;
  else if ( axto->alignment == XmALIGNMENT_CENTER )
    axto->stringX = axto->x + axto->w/2 - axto->stringWidth/2;
  else if ( axto->alignment == XmALIGNMENT_END )
    axto->stringX = axto->x + axto->w - axto->stringWidth;

  axto->useDisplayBg = axto->bufUseDisplayBg;

  axto->autoSize = axto->bufAutoSize;

  axto->x = axto->bufX;
  axto->sboxX = axto->bufX;

  axto->y = axto->bufY;
  axto->sboxY = axto->bufY;

  axto->w = axto->bufW;
  axto->sboxW = axto->bufW;

  axto->h = axto->bufH;
  axto->sboxH = axto->bufH;

  axto->updateDimensions();

  if ( axto->autoSize && axto->fs ) {
    axto->w = XTextWidth( axto->fs, axto->value.getRaw(), axto->stringLength );
    axto->sboxW = axto->w;
    axto->h = axto->fs->ascent + axto->fs->descent;
    axto->sboxH = axto->h;
  }

}

static void axtc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextClass *axto = (activeXTextClass *) client;

  axtc_edit_update ( w, client, call );
  axto->refresh( axto );

}

static void axtc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextClass *axto = (activeXTextClass *) client;

  axtc_edit_update ( w, client, call );
  axto->ef.popdown();
  axto->operationComplete();

}

static void axtc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextClass *axto = (activeXTextClass *) client;

  axto->ef.popdown();
  axto->operationCancel();

}

static void axtc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextClass *axto = (activeXTextClass *) client;

  axto->erase();
  axto->deleteRequest = 1;
  axto->ef.popdown();
  axto->operationCancel();
  axto->drawAll();

}

activeXTextClass::activeXTextClass ( void ) {

  name = new char[strlen("activeXTextClass")+1];
  strcpy( name, "activeXTextClass" );

  visibility = 0;
  prevVisibility = -1;
  visInverted = 0;
  visPvConnected = alarmPvConnected = 0;
  visPvExists = alarmPvExists = 0;
  active = 0;
  activeMode = 0;
  fgColorMode = AXTC_K_COLORMODE_STATIC;
  bgColorMode = AXTC_K_COLORMODE_STATIC;
  strcpy( minVisString, "" );
  strcpy( maxVisString, "" );
  strcpy( id, "" );

}

// copy constructor
activeXTextClass::activeXTextClass
 ( const activeXTextClass *source ) {

activeGraphicClass *ago = (activeGraphicClass *) this;

  ago->clone( (activeGraphicClass *) source );

  name = new char[strlen("activeXTextClass")+1];
  strcpy( name, "activeXTextClass" );

  fgColor.copy(source->fgColor);
  bgColor.copy(source->bgColor);
  fgCb = source->fgCb;
  bgCb = source->bgCb;
  fgColorMode = source->fgColorMode;
  bgColorMode = source->bgColorMode;
  visInverted = source->visInverted;

  alarmPvExpStr.setRaw( source->alarmPvExpStr.rawString );
  visPvExpStr.setRaw( source->visPvExpStr.rawString );

  visibility = 0;
  prevVisibility = -1;
  visPvConnected = alarmPvConnected = 0;
  visPvExists = alarmPvExists = 0;
  active = 0;
  activeMode = 0;

  strncpy( minVisString, source->minVisString, 39 );
  strncpy( maxVisString, source->maxVisString, 39 );

  strncpy( id, source->id, 31 );

  useDisplayBg = source->useDisplayBg;

  autoSize = source->autoSize;

  strncpy( fontTag, source->fontTag, 63 );
  strncpy( bufFontTag, source->bufFontTag, 63 );

  fs = actWin->fi->getXFontStruct( fontTag );

  value.copy( source->value );

  alignment = source->alignment;

  stringLength = source->stringLength;
  fontAscent = source->fontAscent;
  fontDescent = source->fontDescent;
  fontHeight = source->fontHeight;
  stringWidth = source->stringWidth;
  stringY = source->stringY;
  stringX = source->stringX;

}

int activeXTextClass::createInteractive (
  activeWindowClass *aw_obj,
  int _x,
  int _y,
  int _w,
  int _h ) {

int stat = 1;

  actWin = (activeWindowClass *) aw_obj;
  xOrigin = 0;
  yOrigin = 0;
  x = _x;
  y = _y;
  w = _w;
  h = _h;

  fgColor.setColor( actWin->defaultTextFgColor, actWin->ci );
  bgColor.setColor( actWin->defaultBgColor, actWin->ci );

  useDisplayBg = 1;
  autoSize = 1;

  strcpy( fontTag, actWin->defaultFontTag );

  actWin->fi->loadFontTag( fontTag );

  fs = actWin->fi->getXFontStruct( fontTag );
  if ( fs ) {
    fontAscent = fs->ascent;
    fontDescent = fs->descent;
    fontHeight = fontAscent + fontDescent;
  }
  else {
    fontAscent = 0;
    fontDescent = 0;
    fontHeight = 0;
  }

  updateDimensions();

  alignment = actWin->defaultAlignment;

  this->draw();

  this->editCreate();

  return stat;

}

int activeXTextClass::genericEdit ( void ) {

char title[32], *ptr;

  ptr = actWin->obj.getNameFromClass( "activeXTextClass" );
  if ( ptr )
    strncpy( title, ptr, 31 );
  else
    strncpy( title, activeXTextClass_str4, 31 );

  strncat( title, activeXTextClass_str5, 31 );

  strncpy( bufId, id, 31 );

  bufX = x;
  bufY = y;
  bufW = w;
  bufH = h;

  bufFgColor = fgColor.pixelColor();
  bufFgColorMode = fgColorMode;

  bufBgColor = bgColor.pixelColor();
  bufBgColorMode = bgColorMode;

  if ( alarmPvExpStr.getRaw() )
    strncpy( bufAlarmPvName, alarmPvExpStr.getRaw(), 39 );
  else
    strncpy( bufAlarmPvName, "", 39 );

  if ( visPvExpStr.getRaw() )
    strncpy( bufVisPvName, visPvExpStr.getRaw(), 39 );
  else
    strncpy( bufVisPvName, "", 39 );

  if ( visInverted )
    bufVisInverted = 0;
  else
    bufVisInverted = 1;

  strncpy( bufMinVisString, minVisString, 39 );
  strncpy( bufMaxVisString, maxVisString, 39 );

  strncpy( bufFontTag, fontTag, 63 );
  bufUseDisplayBg = useDisplayBg;
  bufAutoSize = autoSize;

  if ( value.getRaw() )
    strncpy( bufValue, value.getRaw(), 255 );
  else
    strncpy( bufValue, "", 255 );

  ef.create( actWin->top, actWin->appCtx->ci.getColorMap(),
   &actWin->appCtx->entryFormX,
   &actWin->appCtx->entryFormY, &actWin->appCtx->entryFormW,
   &actWin->appCtx->entryFormH, &actWin->appCtx->largestH,
   title, NULL, NULL, NULL );

  ef.addTextField( activeXTextClass_str6, 27, bufId, 31 );

  ef.addTextField( activeXTextClass_str7, 27, &bufX );
  ef.addTextField( activeXTextClass_str8, 27, &bufY );
  ef.addTextField( activeXTextClass_str9, 27, &bufW );
  ef.addTextField( activeXTextClass_str10, 27, &bufH );
  ef.addToggle( activeXTextClass_str11, &bufAutoSize );
  ef.addFontMenu( activeXTextClass_str12, actWin->fi, &fm, fontTag );
  fm.setFontAlignment( alignment );
  ef.addColorButton( activeXTextClass_str13, actWin->ci, &fgCb, &bufFgColor );
  ef.addToggle( activeXTextClass_str14, &bufFgColorMode );
  ef.addToggle( activeXTextClass_str15, &bufUseDisplayBg );
  ef.addColorButton( activeXTextClass_str16, actWin->ci, &bgCb, &bufBgColor );
  ef.addToggle( activeXTextClass_str17, &bufBgColorMode );
  ef.addTextField( activeXTextClass_str18, 27, bufAlarmPvName, 39 );
  ef.addTextField( activeXTextClass_str19, 27, bufVisPvName, 39 );
  ef.addOption( " ", activeXTextClass_str20, &bufVisInverted );
  ef.addTextField( activeXTextClass_str21, 27, bufMinVisString, 39 );
  ef.addTextField( activeXTextClass_str22, 27, bufMaxVisString, 39 );
  ef.addTextField( activeXTextClass_str23, 27, bufValue, 255 );

  return 1;

}

int activeXTextClass::editCreate ( void ) {

  this->genericEdit();
  ef.finished( axtc_edit_ok, axtc_edit_apply, axtc_edit_cancel_delete, this );
  actWin->currentEf = NULL;
  ef.popup();

  return 1;

}

int activeXTextClass::edit ( void ) {

  this->genericEdit();
  ef.finished( axtc_edit_ok, axtc_edit_apply, axtc_edit_cancel, this );
  fm.setFontAlignment( alignment );
  actWin->currentEf = &ef;
  ef.popup();

  return 1;

}

int activeXTextClass::createFromFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int r, g, b, index;
int major, minor, release;
unsigned int pixel;
char oneValue[255+1];
int stat = 1;

  this->actWin = _actWin;

  fscanf( f, "%d %d %d\n", &major, &minor, &release ); actWin->incLine();

  fscanf( f, "%d\n", &x ); actWin->incLine();
  fscanf( f, "%d\n", &y ); actWin->incLine();
  fscanf( f, "%d\n", &w ); actWin->incLine();
  fscanf( f, "%d\n", &h ); actWin->incLine();

  this->initSelectBox(); // call after getting x,y,w,h

  if ( major > 1 ) {

    fscanf( f, "%d\n", &index ); actWin->incLine();
    actWin->ci->setIndex( index, &pixel );
    fgColor.setColor( pixel, actWin->ci );

    fscanf( f, "%d\n", &fgColorMode ); actWin->incLine();

    if ( fgColorMode == AXTC_K_COLORMODE_ALARM )
      fgColor.setAlarmSensitive();
    else
      fgColor.setAlarmInsensitive();

    fscanf( f, "%d\n", &useDisplayBg ); actWin->incLine();

    fscanf( f, "%d\n", &index ); actWin->incLine();
    actWin->ci->setIndex( index, &pixel );
    bgColor.setColor( pixel, actWin->ci );

  }
  else {

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 3 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    fgColor.setColor( pixel, actWin->ci );

    fscanf( f, "%d\n", &fgColorMode ); actWin->incLine();

    if ( fgColorMode == AXTC_K_COLORMODE_ALARM )
      fgColor.setAlarmSensitive();
    else
      fgColor.setAlarmInsensitive();

    fscanf( f, "%d\n", &useDisplayBg ); actWin->incLine();

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 3 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    bgColor.setColor( pixel, actWin->ci );

  }

  fscanf( f, "%d\n", &bgColorMode ); actWin->incLine();

  if ( bgColorMode == AXTC_K_COLORMODE_ALARM )
    bgColor.setAlarmSensitive();
  else
    bgColor.setAlarmInsensitive();

  readStringFromFile( oneValue, 39, f ); actWin->incLine();
  alarmPvExpStr.setRaw( oneValue );

  readStringFromFile( oneValue, 39, f ); actWin->incLine();
  visPvExpStr.setRaw( oneValue );

  fscanf( f, "%d\n", &visInverted ); actWin->incLine();

  if ( ( major > 1 ) || ( minor > 0 ) ) {
    readStringFromFile( minVisString, 39, f ); actWin->incLine();
    readStringFromFile( maxVisString, 39, f ); actWin->incLine();
  }
  else {
    strcpy( minVisString, "1" );
    strcpy( maxVisString, "1" );
  }

  readStringFromFile( oneValue, 255, f ); actWin->incLine();
  value.setRaw( oneValue );

  readStringFromFile( fontTag, 63, f ); actWin->incLine();

  fscanf( f, "%d\n", &alignment ); actWin->incLine();

  if ( ( major > 1 ) || ( minor > 1 ) ) {
    fscanf( f, "%d\n", &autoSize ); actWin->incLine();
  }
  else {
    autoSize = 0;
  }

  if ( ( major > 1 ) || ( minor > 3 ) ) {
    readStringFromFile( this->id, 31, f ); actWin->incLine();
  }
  else {
    strcpy( this->id, "" );
  }

  actWin->fi->loadFontTag( fontTag );
  actWin->drawGc.setFontTag( fontTag, actWin->fi );

  if ( value.getRaw() )
    stringLength = strlen( value.getRaw() );
  else
    stringLength = 0;

  fs = actWin->fi->getXFontStruct( fontTag );

  if ( value.getRaw() )
    updateFont( value.getRaw(), fontTag, &fs, &fontAscent, &fontDescent,
     &fontHeight, &stringWidth );
  else
    updateFont( " ", fontTag, &fs, &fontAscent, &fontDescent,
     &fontHeight, &stringWidth );

  stringY = y + fontAscent;

  if ( alignment == XmALIGNMENT_BEGINNING )
    stringX = x;
  else if ( alignment == XmALIGNMENT_CENTER )
    stringX = x + w/2 - stringWidth/2;
  else if ( alignment == XmALIGNMENT_END )
    stringX = x + w - stringWidth;

  return stat;

}

int activeXTextClass::importFromXchFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int r, g, b, more;
unsigned int pixel;
int stat = 1;
char *tk, *gotData, *context, oneValue[255+1], buf[255+1];

  r = 0xffff;
  g = 0xffff;
  b = 0xffff;

  this->actWin = _actWin;

  fgColor.setColor( actWin->defaultTextFgColor, actWin->ci );
  bgColor.setColor( actWin->defaultBgColor, actWin->ci );

  useDisplayBg = 1;
  autoSize = 1;

  strcpy( fontTag, actWin->defaultFontTag );

  alignment = actWin->defaultAlignment;

  // continue until tag is <eod>

  do {

    gotData = getNextDataString( buf, 255, f );
    buf[255] = 0;
    if ( !gotData ) {
      actWin->appCtx->postMessage( activeXTextClass_str24 );
      return 0;
    }

    context = NULL;

    tk = strtok_r( buf, " \t\n", &context );
    if ( !tk ) {
      actWin->appCtx->postMessage( activeXTextClass_str24 );
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
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        x = atol( tk );

      }
      else if ( strcmp( tk, "y" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        y = atol( tk );

      }
      else if ( strcmp( tk, "w" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        w = atol( tk );

      }
      else if ( strcmp( tk, "h" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        h = atol( tk );

      }
            
      else if ( strcmp( tk, "value" ) == 0 ) {

        tk = strtok_r( NULL, "\"", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        strncpy( oneValue, tk, 255 );
        oneValue[255] = 0;

      }
            
      else if ( strcmp( tk, "font" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        strncpy( fontTag, tk, 63 );

      }
            
      else if ( strcmp( tk, "justify" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        alignment = atol( tk );

      }
            
      else if ( strcmp( tk, "red" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        r = atol( tk );

      }
            
      else if ( strcmp( tk, "green" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        g = atol( tk );

      }
            
      else if ( strcmp( tk, "blue" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextClass_str24 );
          return 0;
        }

        b = atol( tk );

      }
            
    }

  } while ( more );

  actWin->ci->setRGB( r, g, b, &pixel );
  fgColor.setColor( pixel, actWin->ci );

  fgColorMode = 0; // alarm insensitive
  fgColor.setAlarmInsensitive();

  bgColorMode = 0; // alarm insensitive
  bgColor.setAlarmInsensitive();

  alarmPvExpStr.setRaw( "" );

  visPvExpStr.setRaw( "" );

  visInverted = 0;

  strcpy( minVisString, "1" );
  strcpy( maxVisString, "1" );

  value.setRaw( oneValue );

  if ( value.getRaw() )
    stringLength = strlen( value.getRaw() );
  else
    stringLength = 0;

  actWin->fi->loadFontTag( fontTag );
  actWin->drawGc.setFontTag( fontTag, actWin->fi );

  fs = actWin->fi->getXFontStruct( fontTag );
  if ( fs ) {
    fontAscent = fs->ascent;
    fontDescent = fs->descent;
    fontHeight = fontAscent + fontDescent;
  }
  else {
    fontAscent = 0;
    fontDescent = 0;
    fontHeight = 0;
  }

  if ( value.getRaw() )
    updateFont( value.getRaw(), fontTag, &fs, &fontAscent, &fontDescent,
     &fontHeight, &stringWidth );
  else
    updateFont( " ", fontTag, &fs, &fontAscent, &fontDescent,
     &fontHeight, &stringWidth );

  y = y + fontDescent;

  this->initSelectBox(); // call after getting x,y,w,h

  if ( alignment == XmALIGNMENT_BEGINNING )
    stringX = x;
  else if ( alignment == XmALIGNMENT_CENTER )
    stringX = x + w/2 - stringWidth/2;
  else if ( alignment == XmALIGNMENT_END )
    stringX = x + w - stringWidth;

  stringY = y + fontAscent;

  return stat;

}

int activeXTextClass::save (
  FILE *f )
{

int index;

  fprintf( f, "%-d %-d %-d\n", AXTC_MAJOR_VERSION, AXTC_MINOR_VERSION,
   AXTC_RELEASE );
  fprintf( f, "%-d\n", x );
  fprintf( f, "%-d\n", y );
  fprintf( f, "%-d\n", w );
  fprintf( f, "%-d\n", h );

  actWin->ci->getIndex( fgColor.pixelColor(), &index );
  fprintf( f, "%-d\n", index );

  fprintf( f, "%-d\n", fgColorMode );

  fprintf( f, "%-d\n", useDisplayBg );

  actWin->ci->getIndex( bgColor.pixelColor(), &index );
  fprintf( f, "%-d\n", index );

  fprintf( f, "%-d\n", bgColorMode );

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

  if ( value.getRaw() )
    writeStringToFile( f, value.getRaw() );
  else
    writeStringToFile( f, "" );

  writeStringToFile( f, fontTag );

  fprintf( f, "%-d\n", alignment );

  fprintf( f, "%-d\n", autoSize );

  // version 1.4.0
  writeStringToFile( f, this->id );

  return 1;

}

int activeXTextClass::drawActive ( void ) {

XRectangle xR = { x, y, w, h };
int clipStat;

  if ( !activeMode || !visibility ) return 1;

  prevVisibility = visibility;

  actWin->executeGc.saveFg();

  actWin->executeGc.setFG( fgColor.getColor() );

  clipStat = actWin->executeGc.addNormXClipRectangle( xR );

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->executeGc.setFontTag( fontTag, actWin->fi );
  }

  if ( useDisplayBg ) {

    XDrawStrings( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), stringX, stringY, fontHeight,
     value.getExpanded(), stringLength );

  }
  else {

    actWin->executeGc.saveBg();
    actWin->executeGc.setBG( bgColor.getColor() );

    XDrawImageStrings( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), stringX, stringY, fontHeight,
     value.getExpanded(), stringLength );

    actWin->executeGc.restoreBg();

  }

  if ( clipStat & 1 ) actWin->executeGc.removeNormXClipRectangle();

  actWin->executeGc.restoreFg();

  return 1;

}

int activeXTextClass::eraseUnconditional ( void ) {

XRectangle xR = { x, y, w, h };

  actWin->executeGc.addEraseXClipRectangle( xR );

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->executeGc.setFontTag( fontTag, actWin->fi );
  }

  if ( useDisplayBg ) {

    XDrawStrings( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), stringX, stringY, fontHeight,
     value.getExpanded(), stringLength );

  }
  else {

    XDrawImageStrings( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), stringX, stringY, fontHeight,
     value.getExpanded(), stringLength );

  }

  actWin->executeGc.removeEraseXClipRectangle();

  return 1;

}

int activeXTextClass::eraseActive ( void ) {

XRectangle xR = { x, y, w, h };

  if ( !activeMode ) return 1;

  if ( prevVisibility == 0 ) {
    prevVisibility = visibility;
    return 1;
  }

  prevVisibility = visibility;

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->executeGc.setFontTag( fontTag, actWin->fi );
  }

  if ( useDisplayBg ) {

    actWin->executeGc.addEraseXClipRectangle( xR );

    XDrawStrings( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), stringX, stringY, fontHeight,
     value.getExpanded(), stringLength );

    actWin->executeGc.removeEraseXClipRectangle();

  }
  else {

    actWin->executeGc.addNormXClipRectangle( xR );

    actWin->executeGc.saveFg();
    actWin->executeGc.saveBg();

    actWin->executeGc.setFG( bgColor.getColor() );
    actWin->executeGc.setBG( bgColor.getColor() );

    XDrawImageStrings( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), stringX, stringY, fontHeight,
     value.getExpanded(), stringLength );

    actWin->executeGc.restoreFg();
    actWin->executeGc.restoreBg();

    actWin->executeGc.removeNormXClipRectangle();

  }

  return 1;

}

int activeXTextClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat;

  stat = alarmPvExpStr.expand1st( numMacros, macros, expansions );
  stat = visPvExpStr.expand1st( numMacros, macros, expansions );
  stat = value.expand1st( numMacros, macros, expansions );

  return stat;

}

int activeXTextClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat;

  stat = alarmPvExpStr.expand2nd( numMacros, macros, expansions );
  stat = visPvExpStr.expand2nd( numMacros, macros, expansions );
  stat = value.expand2nd( numMacros, macros, expansions );

  return stat;

}

int activeXTextClass::containsMacros ( void ) {

  if ( alarmPvExpStr.containsPrimaryMacros() ) return 1;
  if ( visPvExpStr.containsPrimaryMacros() ) return 1;
  if ( value.containsPrimaryMacros() ) return 1;

  return 0;

}

int activeXTextClass::activate (
  int pass,
  void *ptr )
{

int stat;

  switch ( pass ) {

  case 1: // initialize

    needVisConnectInit = 0;
    needAlarmConnectInit = 0;
    needErase = needDraw = needRefresh = needPropertyUpdate = 0;
    aglPtr = ptr;
    opComplete = 0;

#ifdef __epics__
    alarmEventId = visEventId = 0;
#endif

    alarmPvConnected = visPvConnected = 0;
    active = 0;
    activeMode = 1;
    prevVisibility = -1;

    stringLength = strlen( value.getExpanded() );

    updateFont( value.getExpanded(), fontTag, &fs, &fontAscent, &fontDescent,
     &fontHeight, &stringWidth );

    stringY = y + fontAscent;

    if ( alignment == XmALIGNMENT_BEGINNING )
      stringX = x;
    else if ( alignment == XmALIGNMENT_CENTER )
      stringX = x + w/2 - stringWidth/2;
    else if ( alignment == XmALIGNMENT_END )
      stringX = x + w - stringWidth;

    init = 1;
    active = 1;

    if ( !alarmPvExpStr.getExpanded() ||
         ( strcmp( alarmPvExpStr.getExpanded(), "" ) == 0 ) ) {
      alarmPvExists = 0;
    }
    else {
      alarmPvExists = 1;
      fgColor.setConnectSensitive();
      bgColor.setConnectSensitive();
      init = 0;
      active = 0;
    }

    if ( !visPvExpStr.getExpanded() ||
         ( strcmp( visPvExpStr.getExpanded(), "" ) == 0 ) ) {
      visPvExists = 0;
      visibility = 1;
    }
    else {
      visPvExists = 1;
      visibility = 0;
      fgColor.setConnectSensitive();
      bgColor.setConnectSensitive();
      init = 0;
      active = 0;
    }

    break;

  case 2: // connect to pv's

    if ( !opComplete ) {

#ifdef __epics__

      if ( alarmPvExists ) {
        stat = ca_search_and_connect( alarmPvExpStr.getExpanded(), &alarmPvId,
         axtoMonitorAlarmPvConnectState, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextClass_str25 );
          return 0;
        }
      }

      if ( visPvExists ) {
        stat = ca_search_and_connect( visPvExpStr.getExpanded(), &visPvId,
         axtoMonitorVisPvConnectState, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextClass_str26 );
          return 0;
        }
      }

      opComplete = 1;
      this->bufInvalidate();

#endif

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

int activeXTextClass::deactivate (
  int pass )
{

int stat;

  if ( pass == 1 ) {

  activeMode = 0;

  if ( value.getRaw() )
    stringLength = strlen( value.getRaw() );
  else
    stringLength = 0;

  if ( value.getRaw() )
    updateFont( value.getRaw(), fontTag, &fs, &fontAscent, &fontDescent,
     &fontHeight, &stringWidth );
  else
    updateFont( " ", fontTag, &fs, &fontAscent, &fontDescent,
     &fontHeight, &stringWidth );

  stringY = y + fontAscent;

  if ( alignment == XmALIGNMENT_BEGINNING )
    stringX = x;
  else if ( alignment == XmALIGNMENT_CENTER )
    stringX = x + w/2 - stringWidth/2;
  else if ( alignment == XmALIGNMENT_END )
    stringX = x + w - stringWidth;

#ifdef __epics__

  if ( alarmEventId ) {
    stat = ca_clear_event( alarmEventId );
    if ( stat != ECA_NORMAL )
      printf( activeXTextClass_str27 );
  }

  if ( visEventId ) {
    stat = ca_clear_event( visEventId );
    if ( stat != ECA_NORMAL )
      printf( activeXTextClass_str28 );
  }

  if ( alarmPvExists ) {
    stat = ca_clear_channel( alarmPvId );
    if ( stat != ECA_NORMAL )
      printf( activeXTextClass_str29 );
  }

  if ( visPvExists ) {
    stat = ca_clear_channel( visPvId );
    if ( stat != ECA_NORMAL )
      printf( activeXTextClass_str30 );
  }

#endif

  }

  return 1;

}

int activeXTextClass::draw ( void ) {

XRectangle xR = { x, y, w, h };
int clipStat;

  if ( activeMode ) return 1;
  if ( deleteRequest ) return 1;

  actWin->drawGc.saveFg();
  actWin->drawGc.saveBg();

  actWin->drawGc.setFG( fgColor.pixelColor() );
  actWin->drawGc.setBG( bgColor.pixelColor() );

  clipStat = actWin->drawGc.addNormXClipRectangle( xR );

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->drawGc.setFontTag( fontTag, actWin->fi );
  }

  if ( useDisplayBg ) {

    if ( value.getRaw() ) {
      XDrawStrings( actWin->d, XtWindow(actWin->drawWidget),
       actWin->drawGc.normGC(), stringX, stringY, fontHeight,
       value.getRaw(), stringLength );
    }

  }
  else {

    if ( value.getRaw() ) {
      XDrawImageStrings( actWin->d, XtWindow(actWin->drawWidget),
       actWin->drawGc.normGC(), stringX, stringY, fontHeight,
       value.getRaw(), stringLength );
    }

  }

  if ( clipStat & 1 )
    actWin->drawGc.removeNormXClipRectangle();
  //else
  //  printf( "clipStat = %-d\n", clipStat );

  actWin->drawGc.restoreFg();
  actWin->drawGc.restoreBg();

  return 1;

}

int activeXTextClass::erase ( void ) {

XRectangle xR = { x, y, w, h };

  if ( activeMode ) return 1;
  if ( deleteRequest ) return 1;

  actWin->drawGc.addEraseXClipRectangle( xR );

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->drawGc.setFontTag( fontTag, actWin->fi );
  }

  if ( useDisplayBg ) {

    if ( value.getRaw() ) {
      XDrawStrings( actWin->d, XtWindow(actWin->drawWidget),
       actWin->drawGc.eraseGC(), stringX, stringY, fontHeight,
       value.getRaw(), stringLength );
    }

  }
  else {

    if ( value.getRaw() ) {
      XDrawImageStrings( actWin->d, XtWindow(actWin->drawWidget),
       actWin->drawGc.eraseGC(), stringX, stringY, fontHeight,
       value.getRaw(), stringLength );
    }

  }

  actWin->drawGc.removeEraseXClipRectangle();

  return 1;

}

void activeXTextClass::updateDimensions ( void )
{

  stringY = y + fontAscent;

  if ( alignment == XmALIGNMENT_BEGINNING )
    stringX = x;
  else if ( alignment == XmALIGNMENT_CENTER )
    stringX = x + w/2 - stringWidth/2;
  else if ( alignment == XmALIGNMENT_END )
    stringX = x + w - stringWidth;

  if ( activeMode ) {
    if ( value.getExpanded() )
      stringLength = strlen( value.getExpanded() );
    else
      stringLength = 0;
  }
  else {
    if ( value.getRaw() )
      stringLength = strlen( value.getRaw() );
    else
      stringLength = 0;
  }

}

void activeXTextClass::executeDeferred ( void ) {

int stat, nvc, nac, ne, nd, nr, npu;

  if ( actWin->isIconified ) return;

  actWin->appCtx->proc->lock();
  nvc = needVisConnectInit; needVisConnectInit = 0;
  nac = needAlarmConnectInit; needAlarmConnectInit = 0;
  ne = needErase; needErase = 0;
  nd = needDraw; needDraw = 0;
  nr = needRefresh; needRefresh = 0;
  npu = needPropertyUpdate; needPropertyUpdate = 0;
  actWin->remDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

#ifdef __epics__

  if ( nvc ) {

    if ( ( ca_field_type(visPvId) == DBR_ENUM ) ||
         ( ca_field_type(visPvId) == DBR_INT ) ||
         ( ca_field_type(visPvId) == DBR_LONG ) ||
         ( ca_field_type(visPvId) == DBR_FLOAT ) ||
         ( ca_field_type(visPvId) == DBR_DOUBLE ) ) {

      visPvConnected = 1;

      pvType = ca_field_type( visPvId );

      minVis.d = (double) atof( minVisString );
      maxVis.d = (double) atof( maxVisString );

      if ( ( visPvConnected || !visPvExists ) &&
           ( alarmPvConnected || !alarmPvExists ) ) {

        active = 1;
        fgColor.setConnected();
        bgColor.setConnected();
        bufInvalidate();

        if ( init ) {
          eraseUnconditional();
	}

        init = 1;

        actWin->requestActiveRefresh();

      }

      if ( !visEventId ) {
        stat = ca_add_masked_array_event( DBR_DOUBLE, 1, visPvId,
         xTextVisUpdate, (void *) this, (float) 0.0, (float) 0.0,
         (float) 0.0, &visEventId, DBE_VALUE );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextClass_str31 );
        }
      }

    }
    else { // force a draw in the non-active state

      active = 0;
      fgColor.setDisconnected();
      bgColor.setDisconnected();
      bufInvalidate();
      drawActive();

    }

  }

  if ( nac ) {

    alarmPvConnected = 1;

    if ( ( visPvConnected || !visPvExists ) &&
         ( alarmPvConnected || !alarmPvExists ) ) {

      active = 1;
      fgColor.setConnected();
      bgColor.setConnected();
      bufInvalidate();

      if ( init ) {
        eraseUnconditional();
      }

      init = 1;

      actWin->requestActiveRefresh();

    }

    if ( !alarmEventId ) {
      stat = ca_add_masked_array_event( DBR_STS_FLOAT, 1, alarmPvId,
       xTextAlarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
       (float) 0.0, &alarmEventId, DBE_ALARM );
      if ( stat != ECA_NORMAL ) {
        printf( activeXTextClass_str32 );
      }
    }

  }

#endif

  if ( ne ) {
    eraseActive();
  }

  if ( nd ) {
//      drawActive();
    stat = smartDrawAllActive();
  }

  if ( nr ) {
//      actWin->requestActiveRefresh();
    stat = smartDrawAllActive();
  }

  if ( npu ) {

    eraseActive();

    value.setRaw( bufValue );

    stringLength = strlen( bufValue );

    updateFont( value.getRaw(), fontTag, &fs, &fontAscent, &fontDescent,
     &fontHeight, &stringWidth );

    if ( autoSize && fs ) {
      w = XTextWidth( fs, value.getRaw(), stringLength );
      sboxW = w;
      h = fs->ascent + fs->descent;
      sboxH = h;
    }

    updateDimensions();

    stat = smartDrawAllActive();

  }

}

int activeXTextClass::setProperty (
  char *prop,
  char *_value )
{

  if ( strcmp( prop, activeXTextClass_str33 ) == 0 ) {

    strncpy( bufValue, _value, 255 );

    actWin->appCtx->proc->lock();
    actWin->addDefExeNode( aglPtr );
    actWin->appCtx->proc->unlock();

    needPropertyUpdate = 1;

  }

  return 1;

}

char *activeXTextClass::firstDragName ( void ) {

  dragIndex = 0;
  return dragName[dragIndex];

}

char *activeXTextClass::nextDragName ( void ) {

  if ( dragIndex < (int) ( sizeof(dragName) / sizeof(char *) ) - 1 ) {
    dragIndex++;
    return dragName[dragIndex];
  }
  else {
    return NULL;
  }

}

char *activeXTextClass::dragValue (
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

void activeXTextClass::changeDisplayParams (
  unsigned int _flag,
  char *_fontTag,
  int _alignment,
  char *_ctlFontTag,
  int _ctlAlignment,
  char *_btnFontTag,
  int _btnAlignment,
  unsigned int _textFgColor,
  unsigned int _fg1Color,
  unsigned int _fg2Color,
  unsigned int _offsetColor,
  unsigned int _bgColor,
  unsigned int _topShadowColor,
  unsigned int _botShadowColor )
{

  if ( _flag & ACTGRF_TEXTFGCOLOR_MASK )
    fgColor.setColor( _textFgColor, actWin->ci );

  if ( _flag & ACTGRF_BGCOLOR_MASK )
    bgColor.setColor( _bgColor, actWin->ci );

  if ( _flag & ACTGRF_ALIGNMENT_MASK )
    alignment = _alignment;

  if ( _flag & ACTGRF_FONTTAG_MASK ) {

    strcpy( fontTag, _fontTag );
    actWin->fi->loadFontTag( fontTag );
    fs = actWin->fi->getXFontStruct( fontTag );

    if ( fs ) {
      fontAscent = fs->ascent;
      fontDescent = fs->descent;
      fontHeight = fontAscent + fontDescent;
    }
    else {
      fontAscent = 0;
      fontDescent = 0;
      fontHeight = 0;
    }

    updateDimensions();

  }

}

void activeXTextClass::changePvNames (
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

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeXTextClassPtr ( void ) {

activeXTextClass *ptr;

  ptr = new activeXTextClass;
  return (void *) ptr;

}

void *clone_activeXTextClassPtr (
  void *_srcPtr )
{

activeXTextClass *ptr, *srcPtr;

  srcPtr = (activeXTextClass *) _srcPtr;

  ptr = new activeXTextClass( srcPtr );

  return (void *) ptr;

}

#ifdef __cplusplus
}
#endif
