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

#include "cursor.h"

cursorClass::cursorClass ( void )
{

  display = NULL;
  curCursor = (Cursor) NULL;
  crossHair = (Cursor) NULL;
  tinyCrossHair = (Cursor) NULL;
  wait = (Cursor) NULL;

}

cursorClass::~cursorClass ( void )
{

  if ( crossHair ) XFreeCursor( display, crossHair );
  if ( tinyCrossHair ) XFreeCursor( display, tinyCrossHair );
  if ( wait ) XFreeCursor( display, wait );

}

void cursorClass::create (
  Display *dsp,
  Window rootWin,
  Colormap cmap )
{

#define cross_width 16
#define cross_height 16
#define cross_x_hot 7
#define cross_y_hot 8

static char cross_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x9e, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00};

static char cross_mask_bits[] = {
   0x00, 0x00, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
   0xc0, 0x01, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xc0, 0x01, 0xc0, 0x01,
   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01};

#define tinyCross_width 16
#define tinyCross_height 16
#define tinyCross_x_hot 7
#define tinyCross_y_hot 8

static char tinyCross_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x86, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00};

static char tinyCross_mask_bits[] = {
   0x00, 0x00, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0x78, 0x8f, 0x78, 0x0f, 0x78, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01};

#define wait_width 16
#define wait_height 16
#define wait_x_hot 7
#define wait_y_hot 6

static char wait_bits[] = {
   0xc0, 0x01, 0x30, 0x06, 0x0c, 0x18, 0x04, 0x10, 0x02, 0x24, 0x02, 0x23,
   0x81, 0x41, 0xc1, 0x40, 0x81, 0x41, 0x02, 0x23, 0x02, 0x24, 0x04, 0x10,
   0x0c, 0x18, 0x30, 0x06, 0xc0, 0x01, 0x00, 0x00};

//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x20, 0x02, 0x10, 0x04,
//     0x10, 0x04, 0x00, 0x02, 0x80, 0x01, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00,
//     0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static char wait_mask_bits[] = {
   0xf0, 0x07, 0xf8, 0x0f, 0xfc, 0x1f, 0xfe, 0x3f, 0xff, 0x7f, 0xff, 0x7f,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xfe, 0x3f,
   0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0x00, 0x00};

//     0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0xf0, 0x07, 0xf8, 0x0f, 0xf8, 0x0f,
//     0xf8, 0x0f, 0xf8, 0x0f, 0xc0, 0x07, 0xc0, 0x03, 0xc0, 0x01, 0xc0, 0x01,
//     0xc0, 0x01, 0xc0, 0x01, 0x00, 0x00, 0x00, 0x00};

  display = dsp;
  colormap = cmap;
  XParseColor( dsp, cmap, "Black", &shapeColor );
  XParseColor( dsp, cmap, "White", &maskColor );


  crossHairShape = XCreatePixmapFromBitmapData ( dsp, rootWin, cross_bits,
   cross_width, cross_height, 1, 0, 1 );

  crossHairMask = XCreatePixmapFromBitmapData ( dsp, rootWin, cross_mask_bits,
   cross_width, cross_height, 1, 0, 1 );

  crossHair = XCreatePixmapCursor( dsp, crossHairShape, crossHairMask,
   &shapeColor, &maskColor, cross_x_hot, cross_y_hot );


  tinyCrossHairShape = XCreatePixmapFromBitmapData ( dsp, rootWin,
   tinyCross_bits, tinyCross_width, tinyCross_height, 1, 0, 1 );

  tinyCrossHairMask = XCreatePixmapFromBitmapData ( dsp, rootWin,
   tinyCross_mask_bits, tinyCross_width, tinyCross_height, 1, 0, 1 );

  tinyCrossHair = XCreatePixmapCursor( dsp, tinyCrossHairShape,
   tinyCrossHairMask, &shapeColor, &maskColor, tinyCross_x_hot,
   tinyCross_y_hot );


  waitShape = XCreatePixmapFromBitmapData ( dsp, rootWin, wait_bits,
   wait_width, wait_height, 1, 0, 1 );

  waitMask = XCreatePixmapFromBitmapData ( dsp, rootWin, wait_mask_bits,
   wait_width, wait_height, 1, 0, 1 );

  wait = XCreatePixmapCursor( dsp, waitShape, waitMask,
   &shapeColor, &maskColor, wait_x_hot, wait_y_hot );

}

int cursorClass::set (
  Window win,
  int cursorId )
{

  if ( !display ) return 0;

  switch ( cursorId ) {

  case CURSOR_K_DEFAULT:
    XUndefineCursor( display, win );
    curCursor = (Cursor) NULL;
    break;

  case CURSOR_K_CROSSHAIR:
    XDefineCursor( display, win, crossHair );
    curCursor = crossHair;
    break;

  case CURSOR_K_TINYCROSSHAIR:
    XDefineCursor( display, win, tinyCrossHair );
    curCursor = tinyCrossHair;
    break;

  case CURSOR_K_WAIT:
    XDefineCursor( display, win, wait );
    curCursor = wait;
    break;

  default:
    return 0;

  }

  return 1;

}

int cursorClass::setColor (
  unsigned int fg,
  unsigned int bg )
{

int stat;
XColor fgc, bgc;

  if ( !display ) return 0;
  if ( curCursor == ( Cursor ) NULL ) return 1;

  fgc.pixel = fg;
  stat = XQueryColor( display, colormap, &fgc );
  if ( !stat ) return 0;

  bgc.pixel = bg;
  stat = XQueryColor( display, colormap, &bgc );
  if ( !stat ) return 0;

  XRecolorCursor( display, curCursor, &fgc, &bgc );

  return 1;

}
