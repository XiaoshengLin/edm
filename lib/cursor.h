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

#ifndef __cursor_h
#define __cursor_h 1

#include <X11/Xlib.h>

#define CURSOR_K_DEFAULT 1
#define CURSOR_K_CROSSHAIR 2
#define CURSOR_K_TINYCROSSHAIR 3
#define CURSOR_K_WAIT 4

class cursorClass {

private:

Display *display;
Colormap colormap;

Pixmap crossHairShape, crossHairMask;
XColor shapeColor, maskColor;

Pixmap tinyCrossHairShape, tinyCrossHairMask;
XColor tinyCrossHairShapeColor, tinyCrossHairMaskColor;

Pixmap waitShape, waitMask;
XColor waitShapeColor, waitMaskColor;

Cursor curCursor, crossHair, tinyCrossHair, wait;

public:

cursorClass::cursorClass ( void );

cursorClass::~cursorClass ( void );

void cursorClass::create (
  Display *dsp,
  Window rootWin,
  Colormap cmap );

int cursorClass::set (
  Window win,
  int cursorId );

int cursorClass::setColor (
  unsigned int fg,
  unsigned int bg );

};

#endif
