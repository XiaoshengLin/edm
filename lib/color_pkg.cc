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

#include <math.h>
#include "color_pkg.h"
#include "color_button.h"
#include "utility.h"
#include "thread.h"

static int showRGB = 0;

void showColorName (
  XtPointer client,
  XtIntervalId *id )
{

showNameBlockPtr block = (showNameBlockPtr) client;
colorInfoClass *cio = (colorInfoClass *) block->ptr;
int x, y, i;
Window root, child;
int rootX, rootY, winX, winY;
unsigned int mask;

  if ( !cio->showNameTimerActive ) return;

  XQueryPointer( cio->display, XtWindow(cio->shell), &root, &child,
   &rootX, &rootY, &winX, &winY, &mask );

  x = rootX + 10;
  y = rootY + 10;
  i = block->i;

  cio->msgDialog.popup( cio->colorName(i), x, y );

}

void doColorBlink (
  XtPointer client,
  XtIntervalId *id )
{

colorInfoClass *cio = (colorInfoClass *) client;
int i;

  if ( !cio->incrementTimerActive ) return;

  cio->incrementTimer = XtAppAddTimeOut( cio->appCtx,
   cio->incrementTimerValue, doColorBlink, client );

  if ( cio->blink ) {
    cio->blink = 0;
    for ( i=0; i<cio->num_blinking_colors; i++ ) {
      XStoreColor( cio->display, cio->cmap, &cio->offBlinkingXColor[i] );
    }
  }
  else {
    cio->blink = 1;
    for ( i=0; i<cio->num_blinking_colors; i++ ) {
      XStoreColor( cio->display, cio->cmap, &cio->blinkingXColor[i] );
    }
  }

  XFlush( cio->display );

}

void colorShellEventHandler (
  Widget w,
  XtPointer client,
  XEvent *e,
  Boolean *continueToDispatch ) {

colorInfoClass *cio;

  cio = (colorInfoClass *) client;

  *continueToDispatch = False;

  if ( e->type == UnmapNotify ) {

    if ( cio->showNameTimerActive ) {
      cio->showNameTimerActive = 0;
      XtRemoveTimeOut( cio->showNameTimer );
    }
    cio->msgDialog.popdown();
    cio->curPaletteRow = -1;
    cio->curPaletteCol = -1;

  }

}

void colorFormEventHandler (
  Widget w,
  XtPointer client,
  XEvent *e,
  Boolean *continueToDispatch ) {

XMotionEvent *me;
XExposeEvent *expe;
XButtonEvent *be;
XCrossingEvent *ce;
colorInfoClass *cio;
int x, y, i, r, c, ncols, nrows, remainder, count;
unsigned int bg;
int *dest;
int red, green, blue;

  cio = (colorInfoClass *) client;

  *continueToDispatch = False;

  if ( ( e->type == Expose ) || ( e->type == ConfigureNotify ) ) {

    if ( e->type == Expose ) {
      expe = (XExposeEvent *) e;
      count = expe->count;
    }
    else if ( e->type == ConfigureNotify ) {
      count = 0;
    }

    if ( !count ) {

      ncols = cio->num_color_cols;
      nrows = (cio->max_colors+cio->num_blinking_colors) / ncols;
      remainder = (cio->max_colors+cio->num_blinking_colors) % ncols;

      i = 0;
      for ( r=0; r<nrows; r++ ) {

        for ( c=0; c<ncols; c++ ) {

          x = c*5 + c*20 + 5;
          y = r*5 + r*20 + 5;

          cio->gc.setFG( cio->colors[i] );
          XFillRectangle( cio->display, XtWindow(cio->form), cio->gc.normGC(),
           x, y, 20, 20 );

          if ( cio->isRule( i ) ) {
            cio->gc.setFG( cio->labelPix(i) );
            XFillArc( cio->display, XtWindow(cio->form),
             cio->gc.normGC(), x+7, y+7, 6, 6, 0, 23040 );
	  }

          if ( i == cio->curIndex ) {
            cio->gc.setFG(
             BlackPixel( cio->display, DefaultScreen(cio->display) ) );
            XDrawRectangle( cio->display, XtWindow(cio->form),
             cio->gc.normGC(), x-2, y-2, 23, 23 );
          }

          i++;

        }

      }

      if ( remainder ) {

        r = nrows;

        for ( c=0; c<remainder; c++ ) {

          x = c*5 + c*20 + 5;
          y = r*5 + r*20 + 5;

          cio->gc.setFG( cio->colors[i] );
          XFillRectangle( cio->display, XtWindow(cio->form), cio->gc.normGC(),
           x, y, 20, 20 );

          if ( i == cio->curIndex ) {
            cio->gc.setFG(
             BlackPixel( cio->display, DefaultScreen(cio->display) ) );
            XDrawRectangle( cio->display, XtWindow(cio->form),
             cio->gc.normGC(), x-2, y-2, 23, 23 );
          }

          i++;

        }

      }

    }

  }
  else if ( e->type == LeaveNotify ) {

    ce = (XCrossingEvent *) e;

    if ( cio->showNameTimerActive ) {
      cio->showNameTimerActive = 0;
      XtRemoveTimeOut( cio->showNameTimer );
    }
    cio->msgDialog.popdown();
    cio->curPaletteRow = -1;
    cio->curPaletteCol = -1;

  }
  else if ( e->type == MotionNotify ) {

    me = (XMotionEvent *) e;

    ncols = cio->num_color_cols;
    nrows = (cio->max_colors+cio->num_blinking_colors) / ncols;
    remainder = (cio->max_colors+cio->num_blinking_colors) % ncols;
    if ( remainder ) nrows++;

    r = me->y / 25;
    if ( r > nrows-1 ) r = nrows-1;
    c = me->x / 25;
    if ( c > ncols-1 ) c = ncols-1;

    i = r * ncols + c;
    if ( i > cio->numColors-1 ) i = cio->numColors-1;

    if ( ( r != cio->curPaletteRow ) || ( c != cio->curPaletteCol ) ) {
      cio->msgDialog.popdown();
      if ( cio->showNameTimerActive ) {
        cio->showNameTimerActive = 0;
        XtRemoveTimeOut( cio->showNameTimer );
      }
      cio->showNameBlock.x = me->x_root;
      cio->showNameBlock.y = me->y_root;
      cio->showNameBlock.i = i;
      cio->showNameBlock.ptr = (void *) cio;
      cio->showNameTimerActive = 1;
      cio->showNameTimer = XtAppAddTimeOut( cio->appCtx, 500, showColorName,
       &cio->showNameBlock );
      //cio->msgDialog.popup( cio->colorName(i), me->x_root, me->y_root+25 );
      cio->curPaletteRow = r;
      cio->curPaletteCol = c;
    }

  }
  else if ( e->type == ButtonPress ) {

    be = (XButtonEvent *) e;

    ncols = cio->num_color_cols;
    nrows = (cio->max_colors+cio->num_blinking_colors) / ncols;
    remainder = (cio->max_colors+cio->num_blinking_colors) % ncols;
    if ( remainder ) nrows++;

    r = be->y / 25;
    if ( r > nrows-1 ) r = nrows-1;
    c = be->x / 25;
    if ( c > ncols-1 ) c = ncols-1;

    i = r * ncols + c;
    if ( i > cio->numColors-1 ) i = cio->numColors-1;

    cio->setCurIndex( i );

    if ( cio->curCb ) cio->curCb->setIndex( i );

    bg = cio->colors[i];

    cio->change = 1;

    if ( cio->menuPosition(i) ) {
      XmListSelectPos( cio->colorList.listWidget(), cio->menuPosition(i),
       FALSE );
      XmListSetBottomPos( cio->colorList.listWidget(), cio->menuPosition(i) );
    }
    else {
      XmListDeselectAllItems( cio->colorList.listWidget() );
    }

    dest = cio->getCurDestination();
    if ( dest ) {
      *dest = i;
    }

    if ( showRGB ) {
      cio->getRGB( bg, &red, &green, &blue );
      printf( colorInfoClass_str8, i, red, green, blue );
    }

  }

}

static int compare_nodes_by_name (
  void *node1,
  void *node2
) {

colorCachePtr p1, p2;

  p1 = (colorCachePtr) node1;
  p2 = (colorCachePtr) node2;

  return strcmp( p1->name, p2->name );

}

static int compare_key_by_name (
  void *key,
  void *node
) {

colorCachePtr p;
char *oneIndex;

  p = (colorCachePtr) node;
  oneIndex = (char *) key;

  return strcmp( oneIndex, p->name );

}

static int compare_nodes_by_index (
  void *node1,
  void *node2
) {

colorCachePtr p1, p2;

  p1 = (colorCachePtr) node1;
  p2 = (colorCachePtr) node2;

  if ( p1->index > p2->index )
      return 1;
  else if ( p1->index < p2->index )
    return -1;

  return 0;

}

static int compare_key_by_index (
  void *key,
  void *node
) {

colorCachePtr p;
int *oneIndex;

  p = (colorCachePtr) node;
  oneIndex = (int *) key;

  if ( *oneIndex > p->index )
      return 1;
  else if ( *oneIndex < p->index )
    return -1;

  return 0;

}

static int compare_nodes_by_pixel (
  void *node1,
  void *node2
) {

colorCachePtr p1, p2;

  p1 = (colorCachePtr) node1;
  p2 = (colorCachePtr) node2;

  if ( p1->pixel > p2->pixel )
      return 1;
  else if ( p1->pixel < p2->pixel )
    return -1;

  return 0;

}

static int compare_key_by_pixel (
  void *key,
  void *node
) {

colorCachePtr p;
unsigned int *onePixel;

  p = (colorCachePtr) node;
  onePixel = (unsigned int *) key;

  if ( *onePixel > p->pixel )
      return 1;
  else if ( *onePixel < p->pixel )
    return -1;

  return 0;

}

static int compare_nodes_by_color (
  void *node1,
  void *node2
) {

int i;
colorCachePtr p1, p2;

  p1 = (colorCachePtr) node1;
  p2 = (colorCachePtr) node2;

  for ( i=0; i<3; i++ ) {
    if ( p1->rgb[i] > p2->rgb[i] )
      return 1;
    else if ( p1->rgb[i] < p2->rgb[i] )
      return -1;
  }

  return 0;

}

static int compare_key_by_color (
  void *key,
  void *node
) {

int i;
colorCachePtr p;
unsigned int *oneRgb;

  p = (colorCachePtr) node;
  oneRgb = (unsigned int *) key;

  for ( i=0; i<3; i++ ) {
    if ( oneRgb[i] > p->rgb[i] )
      return 1;
    else if ( oneRgb[i] < p->rgb[i] )
      return -1;
  }

  return 0;

}

static int copy_nodes (
  void *node1,
  void *node2
) {

colorCachePtr p1, p2;
ruleConditionPtr cur1, cur2;

  p1 = (colorCachePtr) node1;
  p2 = (colorCachePtr) node2;

  *p1 = *p2;

  // give p1 a copy of the name
  if ( p2->name ) {
    p1->name = new char[ strlen(p2->name) + 1 ];
    strcpy( p1->name, p2->name );
  }

  // give p1 a copy of the rule list
  if ( p2->rule ) {

    p1->rule->ruleHead = new ruleConditionType;
    p1->rule->ruleTail = p1->rule->ruleHead;
    p1->rule->ruleTail->flink = NULL;

    cur2 = p1->rule->ruleHead->flink;
    while ( cur2 ) {

      cur1 = new ruleConditionType;
      *cur1 = *cur2;
      cur1->resultName = new char[strlen(cur2->resultName) + 1];
      strcpy(cur1->resultName,  cur2->resultName );

      p1->rule->ruleTail->flink = cur1;
      p1->rule->ruleTail = cur1;
      p1->rule->ruleTail->flink = NULL;

      cur2 = cur2->flink;

    }

  }

  return 1;

}

colorInfoClass::colorInfoClass ( void ) {

int stat;

  change = 1;
  max_colors = 0;
  num_blinking_colors = 0;
  num_color_cols = 0;
  usingPrivateColorMap = 0;

  fg = 0;
  activeWidget = NULL;
  nameWidget = NULL;
  curDestination = NULL;
  curCb = NULL;
  colorWindowIsOpen = 0;

  stat = avl_init_tree( compare_nodes_by_color,
   compare_key_by_color, copy_nodes, &(this->colorCacheByColorH) );
  if ( !( stat & 1 ) ) this->colorCacheByColorH = (AVL_HANDLE) NULL;

  stat = avl_init_tree( compare_nodes_by_pixel,
   compare_key_by_pixel, copy_nodes, &(this->colorCacheByPixelH) );
  if ( !( stat & 1 ) ) this->colorCacheByPixelH = (AVL_HANDLE) NULL;

  stat = avl_init_tree( compare_nodes_by_index,
   compare_key_by_index, copy_nodes, &(this->colorCacheByIndexH) );
  if ( !( stat & 1 ) ) this->colorCacheByIndexH = (AVL_HANDLE) NULL;

  stat = avl_init_tree( compare_nodes_by_name,
   compare_key_by_name, copy_nodes, &(this->colorCacheByNameH) );
  if ( !( stat & 1 ) ) this->colorCacheByNameH = (AVL_HANDLE) NULL;

  curPaletteRow = -1;
  curPaletteCol = -1;

  showNameTimer = 0;
  showNameTimerActive = 0;

  incrementTimerActive = 0;

  invisibleIndex = -1;

}

colorInfoClass::~colorInfoClass ( void ) {

  XtDestroyWidget( shell );

  if ( showNameTimerActive ) {
    showNameTimerActive = 0;
    XtRemoveTimeOut( showNameTimer );
  }

  if ( incrementTimerActive ) {
    incrementTimerActive = 0;
    XtRemoveTimeOut( incrementTimer );
  }

}

static void file_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

XmPushButtonCallbackStruct *cb;
int num;
Widget p, curP;

  num = (int) client;
  cb = (XmPushButtonCallbackStruct *) call;

  if ( num == 0 ) {   // close window

    /* find topmost widget */
    curP = p = w;
    do {
      p = XtParent(p);
      if ( p ) curP = p;
    } while ( p );

    XtUnmapWidget( curP );

  }
  else if ( num == 1 ) {

    if ( showRGB )
     showRGB = 0;
    else
      showRGB = 1;

  }

}

void colorInfoClass::initParseEngine (
  FILE *f )
{

  readFile = 1;
  tokenState = GET_1ST_NONWS_CHAR;
  parseIndex = -1;
  parseLine = 2;
  parseFile = f;
  colorIndex = 0;

}

void colorInfoClass::parseError (
char *msg )
{

  printf( colorInfoClass_str6, parseLine, msg );

}

int colorInfoClass::getToken (
  char token[MAX_LINE_SIZE+1]
) {

int gotToken, l;
char *ptr;

  gotToken = 0;
  do {

    if ( readFile ) {
      tokenState = GET_1ST_NONWS_CHAR;
      ptr = fgets( parseBuf, MAX_LINE_SIZE, parseFile );
      parseBuf[MAX_LINE_SIZE] = 0;
      if ( !ptr ) {
        strcpy( token, "" );
        return SUCCESS;
      }
      parseLine++;
      readFile = 0;
      parseIndex = -1;
    }

    parseIndex++;

    switch ( tokenState ) {

    case GET_1ST_NONWS_CHAR:

      if ( parseBuf[parseIndex] == 0 ) {
	readFile = 1;
        continue;
      }

      if ( isspace(parseBuf[parseIndex]) || 
           ( parseBuf[parseIndex] == ',' ) ) continue;

      tokenFirst = parseIndex;

      if ( parseBuf[parseIndex] == '"' ) {
        tokenFirst = parseIndex + 1;
        tokenState = GET_TIL_END_OF_QUOTE;
      }
      else if ( ( parseBuf[parseIndex] == '<' ) ||
                ( parseBuf[parseIndex] == '>' ) ||
                ( parseBuf[parseIndex] == '=' ) ||
                ( parseBuf[parseIndex] == '|' ) ||
                ( parseBuf[parseIndex] == '&' ) ||
                ( parseBuf[parseIndex] == '!' ) ) {
        tokenState = GET_TIL_END_OF_SPECIAL;
      }
      else if ( parseBuf[parseIndex] == '#' ) {
        readFile = 1;
        tokenState = GET_1ST_NONWS_CHAR;
      }
      else {
        tokenState = GET_TIL_END_OF_TOKEN;
      }

      break;

    case GET_TIL_END_OF_TOKEN:

      if ( parseBuf[parseIndex] == 0 ) {
	readFile = 1;
      }
      else if ( parseBuf[parseIndex] == '"' ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex + 1;
        gotToken = 1;
        tokenState = GET_TIL_END_OF_QUOTE;
      }
      else if ( ( parseBuf[parseIndex] == '<' ) ||
                ( parseBuf[parseIndex] == '>' ) ||
                ( parseBuf[parseIndex] == '=' ) ||
                ( parseBuf[parseIndex] == '|' ) ||
                ( parseBuf[parseIndex] == '&' ) ||
                ( parseBuf[parseIndex] == '!' ) ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex;
        gotToken = 1;
        tokenState = GET_TIL_END_OF_SPECIAL;
      }
      else if ( parseBuf[parseIndex] == '#' ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex;
        gotToken = 1;
        readFile = 1;
        tokenState = GET_1ST_NONWS_CHAR;
      }
      else if ( isspace(parseBuf[parseIndex]) || 
	   ( parseBuf[parseIndex] == ',' ) ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex;
        gotToken = 1;
        tokenState = GET_1ST_NONWS_CHAR;
      }

      if ( gotToken ) {

        l = tokenLast - tokenFirst + 1;
        strncpy( parseToken, &parseBuf[tokenFirst], l );
        parseToken[l] = 0;
        tokenFirst = tokenNext;

      }

      break;

    case GET_TIL_END_OF_QUOTE:

      if ( parseBuf[parseIndex] == 0 ) {
	return FAIL;
      }
      else if ( parseBuf[parseIndex] == '"' ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex + 1;
        gotToken = 1;
        tokenState = GET_1ST_NONWS_CHAR;
      }

      if ( gotToken ) {

        l = tokenLast - tokenFirst + 1;
        strncpy( parseToken, &parseBuf[tokenFirst], l );
        parseToken[l] = 0;
        tokenFirst = tokenNext;

      }

      break;

    case GET_TIL_END_OF_SPECIAL:

      if ( parseBuf[parseIndex] == 0 ) {
	readFile = 1;
      }
      else if ( parseBuf[parseIndex] == '"' ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex + 1;
        gotToken = 1;
        tokenState = GET_TIL_END_OF_QUOTE;
      }
      else if ( parseBuf[parseIndex] == '#' ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex;
        gotToken = 1;
        readFile = 1;
        tokenState = GET_1ST_NONWS_CHAR;
      }
      else if ( isspace(parseBuf[parseIndex]) || 
	   ( parseBuf[parseIndex] == ',' ) ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex;
        gotToken = 1;
        tokenState = GET_1ST_NONWS_CHAR;
      }
      else if ( ( parseBuf[parseIndex] != '<' ) &&
                ( parseBuf[parseIndex] != '>' ) &&
                ( parseBuf[parseIndex] != '=' ) &&
                ( parseBuf[parseIndex] != '|' ) &&
                ( parseBuf[parseIndex] != '&' ) &&
                ( parseBuf[parseIndex] != '!' ) ) {
        tokenLast = parseIndex - 1;
        tokenNext = parseIndex;
        gotToken = 1;
        tokenState = GET_TIL_END_OF_TOKEN;
      }

      if ( gotToken ) {

        l = tokenLast - tokenFirst + 1;
        strncpy( parseToken, &parseBuf[tokenFirst], l );
        parseToken[l] = 0;
        tokenFirst = tokenNext;

      }

      break;

    }    

  } while ( !gotToken );

  strncpy( token, parseToken, MAX_LINE_SIZE );
  token[MAX_LINE_SIZE] = 0;
  return SUCCESS;

}

static int thisAnd (
  int variable,
  int conditionArg ) {

  return ( variable && conditionArg );

}

static int thisOr (
  int variable,
  int conditionArg ) {

  return ( variable || conditionArg );

}

static int equal (
  double variable,
  double conditionArg ) {

  return ( variable == conditionArg );

}

static int alwaysTrue (
  double variable,
  double conditionArg ) {

  return 1;

}

static int notEqual (
  double variable,
  double conditionArg ) {

  return ( variable != conditionArg );

}

static int lessThan (
  double variable,
  double conditionArg ) {

  return ( variable < conditionArg );

}

static int lessThanOrEqual (
  double variable,
  double conditionArg ) {

  return ( variable <= conditionArg );

}

static int greaterThan (
  double variable,
  double conditionArg ) {

  return ( variable > conditionArg );

}

static int greaterThanOrEqual (
  double variable,
  double conditionArg ) {

  return ( variable >= conditionArg );

}

int colorInfoClass::ver3InitFromFile (
  FILE *f,
  XtAppContext app,
  Display *d,
  Widget top,
  char *fileName )
{

char tk[MAX_LINE_SIZE+1], *endptr;
int i, ii, n, stat, nrows, ncols, remainder, dup,
 parseStatus, state, colorMult, val, index, maxSpecial, firstCond;
XColor color;
Arg arg[20];
XmString str1, str2;
colorCachePtr cur1, cur2, cur[2], curSpecial;
ruleConditionPtr ruleCond;
unsigned long bgColor;
int tmpSize;
int *tmp;
char msg[127+1];

  for ( i=0; i<NUM_SPECIAL_COLORS; i++ ) {
    special[i] = 0;
    specialIndex[i] = 0;
  }

  appCtx = app;
  display = d;
  screen = DefaultScreen( d );
  depth = DefaultDepth( d, screen );
  visual = DefaultVisual( d, screen );
  cmap = DefaultColormap( d, screen );

  num_color_cols = 10;
  maxColor = 0x10000;
  colorMult = 1;
  state = GET_FIRST_TOKEN;
  parseStatus = SUCCESS;
  initParseEngine( f );

  num_blinking_colors = 0;

  max_colors = 0;
  numColors = 0;
  // first, build a list of colors and rules

  while ( state != -1 ) {

    //printf( "[%-d]\n", state );

    switch ( state ) {

    case GET_FIRST_TOKEN:

      stat = getToken( tk );
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
	goto term;
      }

      if ( strcmp( tk, "" ) == 0 ) {

        state = -1; // all done

      }
      else if ( strcmp( tk, "columns" ) == 0 ) {

        state = GET_NUM_COLUMNS;

      }
      else if ( strcmp( tk, "max" ) == 0 ) {

        state = GET_MAX;

      }
      else if ( strcmp( tk, "menumap" ) == 0 ) {

        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        if ( strcmp( tk, "{" ) != 0 ) {
          parseError( colorInfoClass_str11 );
          parseStatus = FAIL;
          goto term;
        }

        maxMenuItems = 0;
        menuMapSize = 128;
        menuIndexMap = new int[menuMapSize];

        state = GET_MENU_MAP;

      }
      else if ( strcmp( tk, "alarm" ) == 0 ) {

        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        if ( strcmp( tk, "{" ) != 0 ) {
          parseError( colorInfoClass_str11 );
          parseStatus = FAIL;
          goto term;
        }

        maxSpecial = -1;

        state = GET_ALARM_PARAMS;

      }
      else if ( strcmp( tk, "static" ) == 0 ) {

        state = GET_COLOR;

      }
      else if ( strcmp( tk, "rule" ) == 0 ) {

        state = GET_RULE;

      }
      else if ( strcmp( tk, "" ) != 0 ) {

        parseError( colorInfoClass_str12 );
        parseStatus = FAIL;
        goto term;

      }

      break;

    case GET_NUM_COLUMNS:

      stat = getToken( tk );
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
	goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }
      else if ( strcmp( tk, "=" ) != 0 ) {
        parseError( colorInfoClass_str13 );
        parseStatus = FAIL;
        goto term;
      }

      stat = getToken( tk );
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
	goto term;
      }
      num_color_cols = strtol( tk, &endptr, 0 );
      if ( strcmp( endptr, "" ) == 0 ) {
        state = GET_FIRST_TOKEN;
      }
      else {
        parseError( colorInfoClass_str14 );
        parseStatus = FAIL;
        goto term;
      }

      break;

    case GET_MAX:

      stat = getToken( tk );
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
	goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }
      else if ( strcmp( tk, "=" ) != 0 ) {
        parseError( colorInfoClass_str13 );
        parseStatus = FAIL;
        goto term;
      }

      stat = getToken( tk );
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
	goto term;
      }
      maxColor = strtol( tk, &endptr, 0 );
      if ( strcmp( endptr, "" ) == 0 ) {
        colorMult = (int) rint( 0x10000 / maxColor );
        state = GET_FIRST_TOKEN;
      }
      else {
        parseError( colorInfoClass_str15 );
        parseStatus = FAIL;
        goto term;
      }

      break;

    case GET_RULE:

      for( n=0; n<2; n++ ) {
        cur[n] = new colorCacheType;
        cur[n]->rule = new ruleType;
        cur[n]->pixel = 0;
        cur[n]->blinkPixel = 0;
        cur[n]->rule->ruleHead = new ruleConditionType; // sentinel
        cur[n]->rule->ruleTail = cur[n]->rule->ruleHead;
        cur[n]->rule->ruleTail->flink = NULL;
      }

      stat = getToken( tk ); // color name
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      for( n=0; n<2; n++ ) {
        cur[n]->name = new char[strlen(tk)+1];
        strcpy( cur[n]->name, tk );
        cur[n]->index = colorIndex; // this is simply an incrementing
	                           // sequence number
      }

      stat = getToken( tk ); // {
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }
      else if ( strcmp( tk, "{" ) != 0 ) {
        parseError( colorInfoClass_str11 );
        parseStatus = FAIL;
        goto term;
      }

      //printf( "rule is [%s]\n", cur[0]->name );

      state = GET_RULE_CONDITION;

      break;

    case GET_RULE_CONDITION:

      //printf( "new condition\n" );
      ruleCond = new ruleConditionType;
      state = GET_FIRST_OP_OR_ARG;
      break;

    case GET_FIRST_OP_OR_ARG:

      stat = getToken( tk ); // operator or number or "default"
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( strcmp( tk, "}" ) == 0 ) {
        //printf( "rule complete\n" );
        delete ruleCond;
        state = INSERT_COLOR;
        break;
      }

      if ( strcmp( tk, "default" ) == 0 ) {

        ruleCond->ruleFunc1 = alwaysTrue;
        ruleCond->value1 = 0;
        ruleCond->connectingFunc = NULL;
        ruleCond->joiningFunc = NULL;
        state = GET_COLON;

      }
      else if ( isLegalFloat( tk ) ) { // implied operator is =

        ruleCond->ruleFunc1 = equal;
        ruleCond->value1 = atof( tk );
        state = GET_CONNECTOR_OR_COLON;

      }
      else { // got an explicit operator

        if ( strcmp( tk, "=" ) == 0 ) {
          ruleCond->ruleFunc1 = equal;
	}
	else if ( strcmp( tk, "!=" ) == 0 ) {
          ruleCond->ruleFunc1 = notEqual;
	}
	else if ( strcmp( tk, ">" ) == 0 ) {
          ruleCond->ruleFunc1 = greaterThan;
	}
	else if ( strcmp( tk, ">=" ) == 0 ) {
          ruleCond->ruleFunc1 = greaterThanOrEqual;
	}
	else if ( strcmp( tk, "<" ) == 0 ) {
          ruleCond->ruleFunc1 = lessThan;
	}
	else if ( strcmp( tk, "<=" ) == 0 ) {
          ruleCond->ruleFunc1 = lessThanOrEqual;
	}
	else {
          parseError( colorInfoClass_str23 );
          parseStatus = FAIL;
          goto term;
        }

        state = GET_FIRST_ARG;

      }

      break;

    case GET_FIRST_ARG:

      stat = getToken( tk ); // number
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( isLegalFloat( tk ) ) {
        ruleCond->value1 = atof( tk );
        state = GET_CONNECTOR_OR_COLON;
      }
      else {
        parseError( colorInfoClass_str25 );
        parseStatus = FAIL;
        goto term;
      }

      break;

    case GET_CONNECTOR_OR_COLON:

      stat = getToken( tk ); // &&, ||, or :
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( strcmp( tk, "&&" ) == 0 ) {
        ruleCond->connectingFunc = thisAnd;
        state = GET_NEXT_OP_OR_ARG;
      }
      else if ( strcmp( tk, "||" ) == 0 ) {
        ruleCond->connectingFunc = thisOr;
        state = GET_NEXT_OP_OR_ARG;
      }
      else if ( strcmp( tk, ":" ) == 0 ) {
        ruleCond->connectingFunc = NULL;
        state = GET_RESULT_NAME_OR_JOININGFUNC;
      }
      else {
        parseError( colorInfoClass_str24 );
        parseStatus = FAIL;
        goto term;
      }

      break;

    case GET_NEXT_OP_OR_ARG:

      stat = getToken( tk ); // operator or number
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( isLegalFloat( tk ) ) { // implied operator is =

        ruleCond->ruleFunc2 = equal;
        ruleCond->value2 = atof( tk );
        state = GET_COLON;

      }
      else { // got an explicit operator

        if ( strcmp( tk, "=" ) == 0 ) {
          ruleCond->ruleFunc2 = equal;
	}
	else if ( strcmp( tk, "!=" ) == 0 ) {
          ruleCond->ruleFunc2 = notEqual;
	}
	else if ( strcmp( tk, ">" ) == 0 ) {
          ruleCond->ruleFunc2 = greaterThan;
	}
	else if ( strcmp( tk, ">=" ) == 0 ) {
          ruleCond->ruleFunc2 = greaterThanOrEqual;
	}
	else if ( strcmp( tk, "<" ) == 0 ) {
          ruleCond->ruleFunc2 = lessThan;
	}
	else if ( strcmp( tk, "<=" ) == 0 ) {
          ruleCond->ruleFunc2 = lessThanOrEqual;
	}
	else {
          parseError( colorInfoClass_str23 );
          parseStatus = FAIL;
          goto term;
        }

        state = GET_NEXT_ARG;

      }

      break;

    case GET_NEXT_ARG:

      stat = getToken( tk ); // number
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( isLegalFloat( tk ) ) {
        ruleCond->value2 = atof( tk );
        state = GET_COLON;
      }
      else {
        parseError( colorInfoClass_str25 );
        parseStatus = FAIL;
        goto term;
      }

      break;

    case GET_COLON:

      stat = getToken( tk ); // :
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( strcmp( tk, ":" ) == 0 ) {
        state = GET_RESULT_NAME_OR_JOININGFUNC;
      }
      else {
        parseError( colorInfoClass_str19 );
        parseStatus = FAIL;
        goto term;
      }

      break;

    case GET_RESULT_NAME_OR_JOININGFUNC:

      stat = getToken( tk ); // color name to use when this condition is true
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( strcmp( tk, "}" ) == 0 ) {
        parseError( colorInfoClass_str26 );
        parseStatus = FAIL;
        goto term;
      }
      else if ( strcmp( tk, "&&" ) == 0 ) {
	printf( "joining func = and\n" );
        ruleCond->resultName = NULL;
        ruleCond->joiningFunc = thisAnd;
        ruleCond->result = 0; // we will map above name to an index
                              // later and store the index here
      }
      else if ( strcmp( tk, "||" ) == 0 ) {
	printf( "joining func = or\n" );
        ruleCond->resultName = NULL;
        ruleCond->joiningFunc = thisOr;
        ruleCond->result = 0; // we will map above name to an index
                              // later and store the index here
      }
      else {
        ruleCond->joiningFunc = NULL;
        ruleCond->resultName = new char[strlen(tk)+1];
        strcpy( ruleCond->resultName, tk );
        ruleCond->result = 0; // we will map above name to an index
                              // later and store the index here
      }

      // link into condition list
      for( n=0; n<2; n++ ) {
        cur[n]->rule->ruleTail->flink = ruleCond;
        cur[n]->rule->ruleTail = ruleCond;
        cur[n]->rule->ruleTail->flink = NULL;
      }
      state = GET_RULE_CONDITION;

      break;

    case GET_COLOR:

      for( n=0; n<2; n++ ) {
        cur[n] = new colorCacheType;
        cur[n]->rule = NULL;
        cur[n]->pixel = 0;
        cur[n]->blinkPixel = 0;
      }

      stat = getToken( tk ); // color name
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      for( n=0; n<2; n++ ) {
        cur[n]->name = new char[strlen(tk)+1];
        strcpy( cur[n]->name, tk );
        cur[n]->index = colorIndex; // this is simply an incrementing
	                            // sequence number
      }

      //printf( "[%s]\n", tk );
      if ( strcmp( tk, "invisible" ) == 0 ) {
        invisibleIndex = colorIndex;
      }

      stat = getToken( tk ); // {
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }
      else if ( strcmp( tk, "{" ) != 0 ) {
        parseError( colorInfoClass_str11 );
        parseStatus = FAIL;
        goto term;
      }

      // get r, g, b
      for ( i=0; i<3; i++ ) {

        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        for( n=0; n<2; n++ ) {
          val = strtol( tk, &endptr, 0 );
          if ( strcmp( endptr, "" ) != 0 ) {
            parseError( colorInfoClass_str16 );
            parseStatus = FAIL;
            goto term;
          }
          cur[n]->rgb[i] = val * colorMult; 
          cur[n]->blinkRgb[i] = cur[n]->rgb[i]; // init blink to the same;
	                                        // may be changed below
	}

      }

      // now we can have } or 3 more r, g, b values

      stat = getToken( tk ); // try }
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }
      else if ( strcmp( tk, "}" ) != 0 ) {

        // cur token must be an rgb value
        for( n=0; n<2; n++ ) {
          val = strtol( tk, &endptr, 0 );
          if ( strcmp( endptr, "" ) != 0 ) {
            parseError( colorInfoClass_str16 );
            parseStatus = FAIL;
            goto term;
          }
          cur[n]->blinkRgb[0] = val * colorMult;
        }

	// now get g, b, }
        for ( i=1; i<3; i++ ) {

          stat = getToken( tk ); // R
          if ( stat == FAIL ) {
            parseError( colorInfoClass_str9 );
            parseStatus = stat;
            goto term;
          }
          if ( strcmp( tk, "" ) == 0 ) {
            parseError( colorInfoClass_str10 );
            parseStatus = FAIL;
            goto term;
          }
          for( n=0; n<2; n++ ) {
            val = strtol( tk, &endptr, 0 );
            if ( strcmp( endptr, "" ) != 0 ) {
              parseError( colorInfoClass_str16 );
              parseStatus = FAIL;
              goto term;
            }
            cur[n]->blinkRgb[i] = val * colorMult;
	  }

        }

        stat = getToken( tk ); // get }
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        else if ( strcmp( tk, "}" ) != 0 ) {
          parseError( colorInfoClass_str17 );
          parseStatus = FAIL;
          goto term;
        }

      }

      state = INSERT_COLOR;

      break;

    case INSERT_COLOR:

      stat = avl_insert_node( this->colorCacheByNameH, (void *) cur[0],
       &dup );
      if ( !( stat & 1 ) ) {
        delete cur[0];
        fclose( f );
        return stat;
      }

      if ( dup ) {
        sprintf( msg, colorInfoClass_str7, cur[0]->name );
        parseError( msg );
        delete cur[0];
      }

      stat = avl_insert_node( this->colorCacheByIndexH, (void *) cur[1],
       &dup );
      if ( !( stat & 1 ) ) {
        delete cur[1];
        fclose( f );
        return stat;
      }

      if ( dup ) {
        parseError( colorInfoClass_str27 );
        delete cur[1];
        parseStatus = FAIL;
        goto term;
      }

      colorIndex++;

      max_colors++;

      state = GET_FIRST_TOKEN;

      break;

    case GET_MENU_MAP:

      stat = getToken( tk );
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( strcmp( tk, "}" ) == 0 ) {

        state = GET_FIRST_TOKEN;
 
      }
      else {

        stat = avl_get_match( this->colorCacheByNameH, (void *) tk,
         (void **) &cur1 );
        if ( !( stat & 1 ) ) {
          parseError( colorInfoClass_str18 );
          parseStatus = FAIL;
          goto term;
        }
        if ( !cur1 ) {
          parseError( colorInfoClass_str18 );
          parseStatus = FAIL;
          goto term;
        }

        if ( ( maxMenuItems + 1 ) >= menuMapSize ) {
          tmpSize = menuMapSize + 128;
          tmp = new int[tmpSize];
          for ( i=0; i<menuMapSize; i++ ) {
            tmp[i] = menuIndexMap[i];
          }
          delete menuIndexMap;
          menuIndexMap = tmp;
        }

        menuIndexMap[maxMenuItems++] = cur1->index;

      }

      break;

    case GET_ALARM_PARAMS:

      stat = getToken( tk );
      if ( stat == FAIL ) {
        parseError( colorInfoClass_str9 );
        parseStatus = stat;
        goto term;
      }
      if ( strcmp( tk, "" ) == 0 ) {
        parseError( colorInfoClass_str10 );
        parseStatus = FAIL;
        goto term;
      }

      if ( strcmp( tk, "disconnected" ) == 0 ) {

        index = COLORINFO_K_DISCONNECTED;

        if ( index > maxSpecial ) maxSpecial = index;

        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        if ( strcmp( tk, ":" ) != 0 ) {
          parseError( colorInfoClass_str19 );
          parseStatus = FAIL;
          goto term;
        }

        // get color name
        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }

        stat = avl_get_match( this->colorCacheByNameH, (void *) tk,
         (void **) &curSpecial );
        if ( !( stat & 1 ) ) {
          parseError( colorInfoClass_str20 );
          parseStatus = FAIL;
          goto term;
        }
        if ( !curSpecial ) {
          parseError( colorInfoClass_str20 );
          parseStatus = FAIL;
          goto term;
        }
        specialIndex[index] = curSpecial->index;

      }
      else if ( strcmp( tk, "invalid" ) == 0 ) {

        index = COLORINFO_K_INVALID;

        if ( index > maxSpecial ) maxSpecial = index;

        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        if ( strcmp( tk, ":" ) != 0 ) {
          parseError( colorInfoClass_str19 );
          parseStatus = FAIL;
          goto term;
        }

        // get color name
        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }

        stat = avl_get_match( this->colorCacheByNameH, (void *) tk,
         (void **) &curSpecial );
        if ( !( stat & 1 ) ) {
          parseError( colorInfoClass_str20 );
          parseStatus = FAIL;
          goto term;
        }
        if ( !curSpecial ) {
          parseError( colorInfoClass_str20 );
          parseStatus = FAIL;
          goto term;
        }
        specialIndex[index] = curSpecial->index;

      }
      else if ( strcmp( tk, "minor" ) == 0 ) {

        index = COLORINFO_K_MINOR;

        if ( index > maxSpecial ) maxSpecial = index;

        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        if ( strcmp( tk, ":" ) != 0 ) {
          parseError( colorInfoClass_str19 );
          parseStatus = FAIL;
          goto term;
        }

        // get color name
        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }

        stat = avl_get_match( this->colorCacheByNameH, (void *) tk,
         (void **) &curSpecial );
        if ( !( stat & 1 ) ) {
          parseError( colorInfoClass_str20 );
          parseStatus = FAIL;
          goto term;
        }
        if ( !curSpecial ) {
          parseError( colorInfoClass_str20 );
          parseStatus = FAIL;
          goto term;
        }
        specialIndex[index] = curSpecial->index;

      }
      else if ( strcmp( tk, "major" ) == 0 ) {

        index = COLORINFO_K_MAJOR;

        if ( index > maxSpecial ) maxSpecial = index;

        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        if ( strcmp( tk, ":" ) != 0 ) {
          parseError( colorInfoClass_str19 );
          parseStatus = FAIL;
          goto term;
        }

        // get color name
        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }

        stat = avl_get_match( this->colorCacheByNameH, (void *) tk,
         (void **) &curSpecial );
        if ( !( stat & 1 ) ) {
          parseError( colorInfoClass_str20 );
          parseStatus = FAIL;
          goto term;
        }
        if ( !curSpecial ) {
          parseError( colorInfoClass_str20 );
          parseStatus = FAIL;
          goto term;
        }
        specialIndex[index] = curSpecial->index;

      }
      else if ( strcmp( tk, "noalarm" ) == 0 ) {

        index = COLORINFO_K_NOALARM;

        if ( index > maxSpecial ) maxSpecial = index;

        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }
        if ( strcmp( tk, ":" ) != 0 ) {
          parseError( colorInfoClass_str19 );
          parseStatus = FAIL;
          goto term;
        }

        // get color name
        stat = getToken( tk );
        if ( stat == FAIL ) {
          parseError( colorInfoClass_str9 );
          parseStatus = stat;
          goto term;
        }
        if ( strcmp( tk, "" ) == 0 ) {
          parseError( colorInfoClass_str10 );
          parseStatus = FAIL;
          goto term;
        }

        if ( strcmp( tk, "*" ) == 0 ) {

          specialIndex[index] = -1;
          
	}
	else {

          stat = avl_get_match( this->colorCacheByNameH, (void *) tk,
           (void **) &curSpecial );
          if ( !( stat & 1 ) ) {
            parseError( colorInfoClass_str20 );
            parseStatus = FAIL;
            goto term;
          }
          if ( !curSpecial ) {
            parseError( colorInfoClass_str20 );
            parseStatus = FAIL;
            goto term;
          }
          specialIndex[index] = curSpecial->index;

	}

      }
      else if ( strcmp( tk, "}" ) == 0 ) {

        if ( maxSpecial < 4 ) {
          parseError( colorInfoClass_str21 );
          parseStatus = FAIL;
          goto term;
        }
 
        state = GET_FIRST_TOKEN;
 
      }
      else {

        parseError( colorInfoClass_str22 );
        parseStatus = FAIL;
        goto term;

      }

      break;

    }

  }

term:

  fclose( f );

  if ( parseStatus != SUCCESS ) {
    return 0;
  }

  change = 1;
  blink = 0;
  curIndex = 0;
  curX = 5;
  curY = 5;

  colors = new unsigned int[max_colors];
  blinkingColors = new unsigned int[max_colors];
  colorNames = new (char *)[max_colors];
  colorNodes = new colorCachePtr[max_colors];

  stat = avl_get_first( this->colorCacheByIndexH, (void **) &cur1 );
  if ( !( stat & 1 ) ) {
    return 0;
  }

  i = 0;

  while ( cur1 ) {

    colorNodes[i] = cur1;

    // Allocate X color for static rules only
    if ( !cur1->rule ) { // not a dynamic color rule

      colorNames[i] = cur1->name; // populate color name array

      color.red = cur1->rgb[0];
      color.green = cur1->rgb[1];
      color.blue = cur1->rgb[2];

      stat = XAllocColor( display, cmap, &color );
      if ( stat ) {
        colors[i] = color.pixel;
      }
      else {
        colors[i] = BlackPixel( display, screen );
      }
      cur1->pixel = colors[i];

      if ( ( cur1->rgb[0] != cur1->blinkRgb[0] ) ||
           ( cur1->rgb[1] != cur1->blinkRgb[1] ) ||
           ( cur1->rgb[2] != cur1->blinkRgb[2] ) ) {

        color.red = cur1->rgb[0];
        color.green = cur1->rgb[1];
        color.blue = cur1->rgb[2];

        stat = XAllocColor( display, cmap, &color );
        if ( stat ) {
          blinkingColors[i] = color.pixel;
        }
        else {
          blinkingColors[i] = BlackPixel( display, screen );
        }
        cur1->blinkPixel = blinkingColors[i];

      }
      else {
        blinkingColors[i] = colors[i];
        cur1->blinkPixel = colors[i];
      }

      // --------------------------------------------------------------
      // update tree sorted by name
      stat = avl_get_match( this->colorCacheByNameH, (void *) cur1->name,
       (void **) &cur2 );
      if ( cur2 ) {
        cur2->pixel = cur1->pixel;
        cur2->blinkPixel = cur1->blinkPixel;
        for ( ii=0; ii<3; ii++ ) {
          cur2->rgb[ii] = cur1->rgb[ii];
          cur2->blinkRgb[ii] = cur1->blinkRgb[ii];
        }
      }
      // --------------------------------------------------------------

    }

    stat = avl_get_next( this->colorCacheByIndexH, (void **) &cur1 );
    if ( !( stat & 1 ) ) {
      return 0;
    }

    if ( i < max_colors-1 ) i++;

  }

  //printf( "fixup dynamic rules\n" );

  // map dynamic color rule result name into result index
  for ( i=0; i<max_colors; i++ ) {

    //printf( "i = %-d\n", i );

    if ( colorNodes[i]->rule ) { // dynamic color rules only

      //printf( "rule is %s, index=%-d\n", colorNodes[i]->name,
      // colorNodes[i]->index );

      colorNames[i] = colorNodes[i]->name; // populate color name array

      firstCond = 1;
      ruleCond = colorNodes[i]->rule->ruleHead->flink;

      if ( !ruleCond ) {
        printf( colorInfoClass_str30, colorNodes[i]->name );
        return 0;
      }

      while ( ruleCond ) {

        if ( ruleCond->resultName ) { // this condition might be combined
                                      // with others via ||, &&

          stat = avl_get_match( this->colorCacheByNameH,
           (void *) ruleCond->resultName, (void **) &cur1 );
          if ( cur1 ) {
            if ( isRule( cur1->index ) ) { // rules may not reference
	                                   // other rules
              printf( colorInfoClass_str29, colorNodes[i]->name );
              return 0;
            }
            ruleCond->result = cur1->index;
            //printf( "result name %s --> %-d\n", ruleCond->resultName,
            // ruleCond->result );
          }
          else {
            printf( colorInfoClass_str28, colorNodes[i]->name );
            return 0;
          }

          if ( firstCond ) { // use first rule condition as static
	                     // color for rule

            colors[i] = cur1->pixel;

            firstCond = 0;
            colorNodes[i]->pixel = cur1->pixel;
            colorNodes[i]->blinkPixel = cur1->blinkPixel;
            for ( ii=0; ii<3; ii++ ) {
              colorNodes[i]->rgb[ii] = cur1->rgb[ii];
              colorNodes[i]->blinkRgb[ii] = cur1->blinkRgb[ii];
            }

            // --------------------------------------------------------------
            // update tree sorted by name
            stat = avl_get_match( this->colorCacheByNameH,
             (void *) colorNodes[i]->name,
             (void **) &cur2 );
            if ( cur2 ) {
              cur2->pixel = colorNodes[i]->pixel;
              cur2->blinkPixel = colorNodes[i]->blinkPixel;
              for ( ii=0; ii<3; ii++ ) {
                cur2->rgb[ii] = colorNodes[i]->rgb[ii];
                cur2->blinkRgb[ii] = colorNodes[i]->blinkRgb[ii];
              }
            }
            // --------------------------------------------------------------

	  }

	}

        ruleCond = ruleCond->flink;

      }

    }

  }

#if 0
  {

    double v;

    printf( "eval rules\n\n\n" );

    i = 0;

    v = 0;
    printf( "rule %-d, value=%-g, result=%-d\n\n", i, v, evalRule( i, v ) );

    v = 1;
    printf( "rule %-d, value=%-g, result=%-d\n\n", i, v, evalRule( i, v ) );

    v = .5;
    printf( "rule %-d, value=%-g, result=%-d\n\n", i, v, evalRule( i, v ) );

    v = 1000;
    printf( "rule %-d, value=%-g, result=%-d\n\n", i, v, evalRule( i, v ) );

    v = 5.5;
    printf( "rule %-d, value=%-g, result=%-d\n\n", i, v, evalRule( i, v ) );

    v = 99.1;
    printf( "rule %-d, value=%-g, result=%-d\n\n", i, v, evalRule( i, v ) );

    v = -1;
    printf( "rule %-d, value=%-g, result=%-d\n\n", i, v, evalRule( i, v ) );

  }
#endif

#if 0

  stat = avl_get_first( this->colorCacheByIndexH, (void **) &cur1 );
  if ( !( stat & 1 ) ) {
    return 0;
  }

  while ( cur1 ) {

    printf( "name: %s, index: %-d, r: %-d, g: %-d, b: %-d, p: %-d, bp: %-d\n",
     cur1->name, cur1->index, cur1->rgb[0], cur1->rgb[1], cur1->rgb[2],
     cur1->pixel, cur1->blinkPixel );

    stat = avl_get_next( this->colorCacheByIndexH, (void **) &cur1 );
    if ( !( stat & 1 ) ) {
      printf( "error 1\n" );
      return 0;
    }

  }

  printf( "\n" );

  stat = avl_get_first( this->colorCacheByNameH, (void **) &cur1 );
  if ( !( stat & 1 ) ) {
    printf( "error 1\n" );
    return 0;
  }

  while ( cur1 ) {

    printf( "name: %s, index: %-d, r: %-d, g: %-d, b: %-d, p: %-d, bp: %-d\n",
     cur1->name, cur1->index, cur1->rgb[0], cur1->rgb[1], cur1->rgb[2],
     cur1->pixel, cur1->blinkPixel );

    stat = avl_get_next( this->colorCacheByNameH, (void **) &cur1 );
    if ( !( stat & 1 ) ) {
      printf( "error 1\n" );
      return 0;
    }

  }

#endif

  // create window

  shell = XtVaAppCreateShell( colorInfoClass_str2, colorInfoClass_str2,
   topLevelShellWidgetClass,
   XtDisplay(top),
   XtNmappedWhenManaged, False,
   NULL );

  rc = XtVaCreateWidget( "", xmRowColumnWidgetClass, shell,
   XmNorientation, XmVERTICAL,
   XmNnumColumns, 1,
   NULL );

  str1 = XmStringCreateLocalized( colorInfoClass_str3 );
  mbar = XmVaCreateSimpleMenuBar( rc, "",
   XmVaCASCADEBUTTON, str1, 'f',
   NULL );
  XmStringFree( str1 );

  str1 = XmStringCreateLocalized( colorInfoClass_str4 );
  str2 = XmStringCreateLocalized( colorInfoClass_str5 );
  mb1 = XmVaCreateSimplePulldownMenu( mbar, "", 0, file_cb,
   XmVaPUSHBUTTON, str1, 'x', NULL, NULL,
   XmVaPUSHBUTTON, str2, 's', NULL, NULL,
   NULL );
  XmStringFree( str1 );
  XmStringFree( str2 );

  ncols = num_color_cols;
  nrows = (max_colors) / ncols;
  remainder = (max_colors) % ncols;
  if ( remainder ) nrows++;

  form = XtVaCreateWidget( "", xmDrawingAreaWidgetClass, rc,
   XmNwidth, ncols*20 + ncols*5 + 5,
   XmNheight, nrows*20 + nrows*5 + 5,
   XmNresizePolicy, XmRESIZE_NONE,
   NULL );

  XtAddEventHandler( form,
   PointerMotionMask|ButtonPressMask|ExposureMask|
   LeaveWindowMask|StructureNotifyMask, False,
   colorFormEventHandler, (XtPointer) this );

  XtAddEventHandler( shell,
   StructureNotifyMask, False,
   colorShellEventHandler, (XtPointer) this );

  Atom wm_delete_window = XmInternAtom( XtDisplay(shell), "WM_DELETE_WINDOW",
   False );

  XmAddWMProtocolCallback( shell, wm_delete_window, file_cb,
    (int) 0 );

  XtVaSetValues( shell, XmNdeleteResponse, XmDO_NOTHING, NULL );

  XtManageChild( mbar );
  XtManageChild( rc );
  XtManageChild( form );
  XtRealizeWidget( shell );
  XSetWindowColormap( display, XtWindow(shell), cmap );

  gc.create( shell );

  n = 0;
  XtSetArg( arg[n], XmNbackground, &bgColor ); n++;
  XtGetValues( form, arg, n );

  gc.setBG( bgColor );
  gc.setBaseBG( bgColor );

  numColors = max_colors;

  // populate special array
  for ( i=0; i<NUM_SPECIAL_COLORS-2; i++ ) {
    if ( specialIndex[i] != -1 ) {
      special[i] = colors[ specialIndex[i] ];
    }
    else {
      special[i] = -1;
    }
  }

  msgDialog.create( shell );

  return 1;

}

int colorInfoClass::initFromFile (
  XtAppContext app,
  Display *d,
  Widget top,
  char *fileName )
{

char line[127+1], *ptr, *tk, *junk;
int i, index, iOn, iOff, n, stat, nrows, ncols, remainder, dup, nSpecial;
FILE *f;
XColor color;
Arg arg[20];
XmString str1, str2;
colorCachePtr cur, curSpecial;
int rgb[3], red, green, blue;
unsigned long plane_masks[1], bgColor;

  if ( !this->colorCacheByIndexH ) return 0;

  appCtx = app;
  display = d;
  screen = DefaultScreen( d );
  depth = DefaultDepth( d, screen );
  visual = DefaultVisual( d, screen );
  cmap = DefaultColormap( d, screen );

  change = 1;
  blink = 0;
  curIndex = 0;
  curX = 5;
  curY = 5;

  maxMenuItems = 0;
  menuIndexMap = NULL;

  f = fopen( fileName, "r" );
  if ( !f ) {
    return COLORINFO_NO_FILE;
  }

  fscanf( f, "%d %d %d\n", &major, &minor, &release );

  if ( major == 3 ) {
    stat = ver3InitFromFile( f, app, d, top, fileName );
    if ( stat & 1 ) colorList.create( max_colors, top, 20, this );
    return stat;
  }

  goto firstTry;

restart:

  // at this point, a private color map is being used

  delete colors;
  delete blinkingColorCells;
  delete blinkingXColor;
  delete offBlinkingXColor;

  stat = avl_init_tree( compare_nodes_by_color,
   compare_key_by_color, copy_nodes, &(this->colorCacheByColorH) );
  if ( !( stat & 1 ) ) this->colorCacheByColorH = (AVL_HANDLE) NULL;

  stat = avl_init_tree( compare_nodes_by_pixel,
   compare_key_by_pixel, copy_nodes, &(this->colorCacheByPixelH) );
  if ( !( stat & 1 ) ) this->colorCacheByPixelH = (AVL_HANDLE) NULL;

  stat = avl_init_tree( compare_nodes_by_index,
   compare_key_by_index, copy_nodes, &(this->colorCacheByIndexH) );
  if ( !( stat & 1 ) ) this->colorCacheByIndexH = (AVL_HANDLE) NULL;

  stat = avl_init_tree( compare_nodes_by_name,
   compare_key_by_name, copy_nodes, &(this->colorCacheByNameH) );
  if ( !( stat & 1 ) ) this->colorCacheByNameH = (AVL_HANDLE) NULL;

  fclose( f );

  f = fopen( fileName, "r" );
  if ( !f ) {
    return COLORINFO_NO_FILE;
  }

  fscanf( f, "%d %d %d\n", &major, &minor, &release );

firstTry:

  if ( major < 2 ) {
    max_colors = 88;
    num_blinking_colors = 8;
    num_color_cols = 11;
  }
  else {
    fscanf( f, "%d %d %d\n", &max_colors, &num_blinking_colors,
     &num_color_cols );
  }

  colors = new unsigned int[max_colors+num_blinking_colors];
  blinkingColorCells = new unsigned long[num_blinking_colors];
  blinkingXColor = new XColor[num_blinking_colors];
  offBlinkingXColor = new XColor[num_blinking_colors];
  colorNames = new (char *)[max_colors+num_blinking_colors+1];

  junk =  new char[strlen("n/a")+1]; // tiny memory leak here
  strcpy( junk, "n/a" );             // for color files < version 3

  numColors = 0;

  index = 0;
  for ( i=0; i<(max_colors); i++ ) {

    colorNames[i] = junk;

    ptr = fgets ( line, 127, f );
    if ( ptr ) {

      numColors++;

      tk = strtok( line, " \t\n" );
      if ( tk )
        red = atol( tk );
      else
        red = 0;

      if ( major < 2 ) red *= 256;
      color.red = red;

      tk = strtok( NULL, " \t\n" );
      if ( tk )
        green = atol( tk );
      else
        green = 0;

      if ( major < 2 ) green *= 256;
      color.green = green;

      tk = strtok( NULL, " \t\n" );
      if ( tk )
        blue = atol( tk );
      else
        blue = 0;

      if ( major < 2 ) blue *= 256;
      color.blue = blue;

      stat = XAllocColor( display, cmap, &color );

      if ( stat ) {
        colors[i] = color.pixel;
      }
      else {

        if ( !usingPrivateColorMap ) {
          usingPrivateColorMap = 1;
	  cmap = XCopyColormapAndFree( display, cmap );
	  XSetWindowColormap( display, XtWindow(top), cmap );
          goto restart;
	}

        colors[i] = BlackPixel( display, screen );

      }

    }
    else {
      if ( i ) {
        colors[i] = BlackPixel( display, screen );
      }
      else {
        colors[i] = WhitePixel( display, screen );
      }
    }

    cur = new colorCacheType;
    if ( !cur ) return 0;

    cur->rgb[0] = (unsigned int) red;
    cur->rgb[1] = (unsigned int) green;
    cur->rgb[2] = (unsigned int) blue;
    cur->pixel = colors[i];
    cur->index = index;

    cur->name = NULL;
    cur->rule = NULL;

    stat = avl_insert_node( this->colorCacheByColorH, (void *) cur,
     &dup );
    if ( !( stat & 1 ) ) {
      delete cur;
      fclose( f );
      return stat;
    }

    if ( dup ) delete cur;

    cur = new colorCacheType;
    if ( !cur ) return 0;

    cur->rgb[0] = (unsigned int) red;
    cur->rgb[1] = (unsigned int) green;
    cur->rgb[2] = (unsigned int) blue;
    cur->pixel = colors[i];
    cur->index = index;

    cur->name = NULL;
    cur->rule = NULL;

    stat = avl_insert_node( this->colorCacheByPixelH, (void *) cur,
     &dup );
    if ( !( stat & 1 ) ) {
      delete cur;
      fclose( f );
      return stat;
    }

    if ( dup ) delete cur;

    cur = new colorCacheType;
    if ( !cur ) return 0;

    cur->rgb[0] = (unsigned int) red;
    cur->rgb[1] = (unsigned int) green;
    cur->rgb[2] = (unsigned int) blue;
    cur->pixel = colors[i];
    cur->index = index;

    cur->name = NULL;
    cur->rule = NULL;

    stat = avl_insert_node( this->colorCacheByIndexH, (void *) cur,
     &dup );
    if ( !( stat & 1 ) ) {
      delete cur;
      fclose( f );
      return stat;
    }

    if ( dup ) delete cur;

    index++;

  }

  stat = XAllocColorCells( display, cmap, False, plane_masks, 0,
   blinkingColorCells, num_blinking_colors );

  if ( stat ) { // success

    for ( i=0; i<num_blinking_colors; i++ ) {
      colorNames[max_colors+i] = junk;
    }

    // blinking colors
    iOn = 0;
    iOff = 0;
    for ( i=0; i<num_blinking_colors*2; i++ ) {

      ptr = fgets ( line, 127, f );
      if ( ptr ) {

        tk = strtok( line, " \t\n" );
        if ( tk )
          red = atol( tk );
        else
          red = 0;

        if ( major < 2 ) red *= 256;
        color.red = red;

        tk = strtok( NULL, " \t\n" );
        if ( tk )
          green = atol( tk );
        else
          green = 0;

        if ( major < 2 ) green *= 256;
        color.green = green;

        tk = strtok( NULL, " \t\n" );
        if ( tk )
          blue = atol( tk );
        else
          blue = 0;

        if ( major < 2 ) blue *= 256;
        color.blue = blue;

        if ( !( i % 2 ) ) {
          color.pixel = blinkingColorCells[iOn];
          color.flags = DoRed | DoGreen | DoBlue;
          colors[numColors] = color.pixel;
          blinkingXColor[iOn] = color;
          iOn++;
          XStoreColor( display, cmap, &color );
        }
        else {
          offBlinkingXColor[iOff] = color;
          iOff++;
        }

      }
      else {
        if ( numColors ) {
          colors[numColors] = BlackPixel( display, screen );
        }
        else {
          colors[numColors] = WhitePixel( display, screen );
        }
      }

      if ( !( i % 2 ) ) {

        cur = new colorCacheType;
        if ( !cur ) return 0;

        cur->rgb[0] = (unsigned int) red;
        cur->rgb[1] = (unsigned int) green;
        cur->rgb[2] = (unsigned int) blue;
        cur->pixel = colors[numColors];
        cur->index = index;

        cur->name = NULL;
        cur->rule = NULL;

        stat = avl_insert_node( this->colorCacheByColorH, (void *) cur,
         &dup );
        if ( !( stat & 1 ) ) {
          delete cur;
          fclose( f );
          return stat;
        }

        if ( dup ) delete cur;

        cur = new colorCacheType;
        if ( !cur ) return 0;

        cur->rgb[0] = (unsigned int) red;
        cur->rgb[1] = (unsigned int) green;
        cur->rgb[2] = (unsigned int) blue;
        cur->pixel = colors[numColors];
        cur->index = index;

        cur->name = NULL;
        cur->rule = NULL;

        stat = avl_insert_node( this->colorCacheByPixelH, (void *) cur,
         &dup );
        if ( !( stat & 1 ) ) {
          delete cur;
          fclose( f );
          return stat;
        }

        if ( dup ) delete cur;

        cur = new colorCacheType;
        if ( !cur ) return 0;

        cur->rgb[0] = (unsigned int) red;
        cur->rgb[1] = (unsigned int) green;
        cur->rgb[2] = (unsigned int) blue;
        cur->pixel = colors[numColors];
        cur->index = index;

        cur->name = NULL;
        cur->rule = NULL;

        stat = avl_insert_node( this->colorCacheByIndexH, (void *) cur,
         &dup );
        if ( !( stat & 1 ) ) {
          delete cur;
          fclose( f );
          return stat;
        }

        if ( dup ) delete cur;

        numColors++;
        index++;

      }

    }

  }
  else {

    if ( !usingPrivateColorMap ) {
      usingPrivateColorMap = 1;
      cmap = XCopyColormapAndFree( display, cmap );
      XSetWindowColormap( display, XtWindow(top), cmap );
      goto restart;
    }

    printf( colorInfoClass_str1 );
    // discard file contents
    for ( i=0; i<num_blinking_colors*2; i++ ) {
      ptr = fgets ( line, 127, f );
    }
    num_blinking_colors = 0;

  }

  // special colors are disconnected, severity=invalid,
  // severity=minor, severity=major, severity=noalarm
  if ( major > 2 ) {
    nSpecial = NUM_SPECIAL_COLORS;
  }
  else {
    nSpecial = NUM_SPECIAL_COLORS - 2; // don't include ack alarm colors
    special[NUM_SPECIAL_COLORS - 2] = (int) BlackPixel( display, screen );
    special[NUM_SPECIAL_COLORS - 1] = (int) BlackPixel( display, screen );
  }

  for ( i=0; i<nSpecial; i++ ) {

    ptr = fgets ( line, 127, f );
    if ( ptr ) {

      tk = strtok( line, " \t\n" );
      if ( tk )
        rgb[0] = atol( tk );
      else
        rgb[0] = 0;

      tk = strtok( NULL, " \t\n" );
      if ( tk )
        rgb[1] = atol( tk );
      else
        rgb[1] = 0;

      tk = strtok( NULL, " \t\n" );
      if ( tk )
        rgb[2] = atol( tk );
      else
        rgb[2] = 0;

      if ( rgb[0] != -1 ) {

        if ( major < 2 ) {
          rgb[0] *= 256;
          rgb[1] *= 256;
          rgb[2] *= 256;
        }

        stat = avl_get_match( this->colorCacheByColorH, (void *) rgb,
         (void **) &curSpecial );

        if ( ( stat & 1 ) && curSpecial ) {
          special[i] = (int) curSpecial->pixel;
        }
        else {
          special[i] = (int) BlackPixel( display, screen );
        }

      }
      else {

        special[i] = -1;

      }

    }
    else {
      special[i] = (int) BlackPixel( display, screen );
    }

  }

  fclose( f );

  // create window

  shell = XtVaAppCreateShell( colorInfoClass_str2, colorInfoClass_str2,
   topLevelShellWidgetClass,
   XtDisplay(top),
   XtNmappedWhenManaged, False,
   NULL );

  rc = XtVaCreateWidget( "", xmRowColumnWidgetClass, shell,
   XmNorientation, XmVERTICAL,
   XmNnumColumns, 1,
   NULL );

  str1 = XmStringCreateLocalized( colorInfoClass_str3 );
  mbar = XmVaCreateSimpleMenuBar( rc, "",
   XmVaCASCADEBUTTON, str1, 'f',
   NULL );
  XmStringFree( str1 );

  str1 = XmStringCreateLocalized( colorInfoClass_str4 );
  str2 = XmStringCreateLocalized( colorInfoClass_str5 );
  mb1 = XmVaCreateSimplePulldownMenu( mbar, "", 0, file_cb,
   XmVaPUSHBUTTON, str1, 'x', NULL, NULL,
   XmVaPUSHBUTTON, str2, 's', NULL, NULL,
   NULL );
  XmStringFree( str1 );
  XmStringFree( str2 );

//   form = XtVaCreateManagedWidget( "", xmFormWidgetClass, rc,
//    NULL );

  ncols = num_color_cols;
  nrows = (max_colors+num_blinking_colors) / ncols;
  remainder = (max_colors+num_blinking_colors) % ncols;
  if ( remainder ) nrows++;

  form = XtVaCreateManagedWidget( "", xmDrawingAreaWidgetClass, rc,
   XmNwidth, ncols*20 + ncols*5 + 5,
   XmNheight, nrows*20 + nrows*5 + 5,
   NULL );

  XtAddEventHandler( form,
   PointerMotionMask|ButtonPressMask|ExposureMask|
   LeaveWindowMask|StructureNotifyMask, False,
   colorFormEventHandler, (XtPointer) this );

  XtAddEventHandler( shell,
   StructureNotifyMask, False,
   colorShellEventHandler, (XtPointer) this );

  Atom wm_delete_window = XmInternAtom( XtDisplay(shell), "WM_DELETE_WINDOW",
   False );

  XmAddWMProtocolCallback( shell, wm_delete_window, file_cb,
    (int) 0 );

  XtVaSetValues( shell, XmNdeleteResponse, XmDO_NOTHING, NULL );

  XtManageChild( mbar );
  XtManageChild( rc );
  XtRealizeWidget( shell );
  XSetWindowColormap( display, XtWindow(shell), cmap );

  gc.create( shell );

   n = 0;
   XtSetArg( arg[n], XmNbackground, &bgColor ); n++;
   XtGetValues( form, arg, n );

  gc.setBG( bgColor );
  gc.setBaseBG( bgColor );

  if ( num_blinking_colors ) {
    incrementTimerValue = 500;
    incrementTimerActive = 1;
    incrementTimer = XtAppAddTimeOut( appCtx, incrementTimerValue,
     doColorBlink, this );
  }

  colorList.create( max_colors+num_blinking_colors, top, 20, this );

  msgDialog.create( shell );

  return 1;

}

int colorInfoClass::openColorWindow( void ) {

  XtMapWidget( shell );
  XRaiseWindow( display, XtWindow(shell) );

  colorWindowIsOpen = 1;

  return 1;

}

int colorInfoClass::closeColorWindow( void ) {

  XtUnmapWidget( shell );

  colorWindowIsOpen = 0;

  return 1;

}

unsigned int colorInfoClass::getFg( void ) {

  return fg;

}

void colorInfoClass::setCurDestination( int *ptr ) {

  curDestination = ptr;

}

int *colorInfoClass::getCurDestination( void ) {

  return curDestination;

}

void colorInfoClass::setCurCb( colorButtonClass *cb ) {

  curCb = cb;

}

colorButtonClass *colorInfoClass::getCurCb( void ) {

  return curCb;

}

int colorInfoClass::setActiveWidget( Widget w ) {

  activeWidget = w;

  return 1;

}

Widget colorInfoClass::getActiveWidget( void ) {

  return activeWidget;

}

int colorInfoClass::setNameWidget( Widget w ) {

  nameWidget = w;

  return 1;

}

Widget colorInfoClass::getNameWidget( void ) {

  return nameWidget;

}

int colorInfoClass::getRGB(
  unsigned int pixel,
  int *r,
  int *g,
  int *b )
{

XColor color;
int stat, dup;
colorCachePtr cur;

  stat = avl_get_match( this->colorCacheByPixelH, (void *) &pixel,
   (void **) &cur );
  if ( !(stat & 1) ) return stat;

  if ( cur ) {
    *r = cur->rgb[0];
    *g = cur->rgb[1];
    *b = cur->rgb[2];
    return COLORINFO_SUCCESS;
  }

  *r = 0;
  *g = 0;
  *b = 0;
  color.pixel = pixel;
  stat = XQueryColor( display, cmap, &color );

  if ( !stat ) return COLORINFO_FAIL;

  *r = (int) color.red;
  *g = (int) color.green;
  *b = (int) color.blue;

  cur = new colorCacheType;
  if ( !cur ) return 0;

  cur->rgb[0] = (unsigned int) *r;
  cur->rgb[1] = (unsigned int) *g;
  cur->rgb[2] = (unsigned int) *b;
  cur->pixel = (unsigned int) pixel;
  cur->index = -1;

  cur->name = NULL;
  cur->rule = NULL;

  stat = avl_insert_node( this->colorCacheByPixelH, (void *) cur,
   &dup );
  if ( !( stat & 1 ) ) {
    delete cur;
    return stat;
  }

  if ( dup ) delete cur;

  cur = new colorCacheType;
  if ( !cur ) return 0;

  cur->rgb[0] = (unsigned int) *r;
  cur->rgb[1] = (unsigned int) *g;
  cur->rgb[2] = (unsigned int) *b;
  cur->pixel = (unsigned int) pixel;
  cur->index = -1;

  cur->name = NULL;
  cur->rule = NULL;

  stat = avl_insert_node( this->colorCacheByColorH, (void *) cur,
   &dup );
  if ( !( stat & 1 ) ) {
    delete cur;
    return stat;
  }

  if ( dup ) delete cur;

  return COLORINFO_SUCCESS;

}

int colorInfoClass::setRGB (
  int r,
  int g,
  int b,
  unsigned int *pixel )
{

int stat;
colorCachePtr cur;
int diff, bestPixel, bestR, bestG, bestB, min;

  bestPixel = -1;
  bestR = 0;
  bestG = 0;
  bestB = 0;

  stat = avl_get_first( this->colorCacheByIndexH, (void **) &cur );
  if ( !( stat & 1 ) ) return COLORINFO_FAIL;

  if ( cur ) {

    min = abs( r - cur->rgb[0] ) + abs( g - cur->rgb[1] ) +
          abs( b - cur->rgb[2] );
    bestPixel = cur->pixel;
    bestR = cur->rgb[0];
    bestG = cur->rgb[1];
    bestB = cur->rgb[2];
  }

  while ( cur ) {

    diff = abs( r - cur->rgb[0] ) + abs( g - cur->rgb[1] ) +
          abs( b - cur->rgb[2] );
    if ( diff < min ) {
      min = diff;
      bestPixel = cur->pixel;
      bestR = cur->rgb[0];
      bestG = cur->rgb[1];
      bestB = cur->rgb[2];
    }

    stat = avl_get_next( this->colorCacheByIndexH, (void **) &cur );
    if ( !( stat & 1 ) ) return COLORINFO_FAIL;

  }

  if ( bestPixel == -1 ) return COLORINFO_FAIL;

  *pixel = (unsigned int) bestPixel;

  return COLORINFO_SUCCESS;

}

int colorInfoClass::getIndex(
  unsigned int pixel,
  int *index )
{

int stat;
colorCachePtr cur;

  stat = avl_get_match( this->colorCacheByPixelH, (void *) &pixel,
   (void **) &cur );
  if ( !(stat & 1) ) return stat;

  if ( cur ) {
    *index = cur->index;
    return COLORINFO_SUCCESS;
  }

  return COLORINFO_FAIL;

}

int colorInfoClass::setIndex (
  int index,
  unsigned int *pixel )
{

int stat;
colorCachePtr cur;

//printf( "obsolete colorInfoClass::setIndex\n" );

  stat = avl_get_match( this->colorCacheByIndexH, (void *) &index,
   (void **) &cur );
  if ( !(stat & 1) ) return stat;

  if ( cur ) {
    *pixel = cur->pixel;
    return COLORINFO_SUCCESS;
  }

  return COLORINFO_FAIL;

}

int colorInfoClass::getSpecialColor (
  int index ) {

  if ( index < 0 ) return -1;
  if ( index >= NUM_SPECIAL_COLORS ) return -1;

  return special[index];

}


int colorInfoClass::getSpecialIndex (
  int index ) {

  if ( index < 0 ) return -1;
  if ( index >= NUM_SPECIAL_COLORS ) return -1;

  return specialIndex[index];

  return 0;

}

Colormap colorInfoClass::getColorMap ( void ) {

  return cmap;

}

int colorInfoClass::setCurIndexByPixel (
  unsigned int pixel ) {

int i, curI, stat;

  for ( i=0; i<max_colors+num_blinking_colors; i++ ) {

    if ( colors[i] == pixel ) {

      curI = i;
      break;

    }

  }

  stat = setCurIndex( curI );

  return stat;

}

int colorInfoClass::setCurIndex (
  int index ) {

int x, y, i, r, c, ncols, nrows, remainder;

  if ( index > numColors-1 ) {
    curIndex = numColors-1;
  }
  else if ( index < 0 ) {
    curIndex = 0;
  }
  else {
    curIndex = index;
  }

  XDrawRectangle( display, XtWindow(form), gc.eraseGC(), curX-2, curY-2,
   23, 23 );

  ncols = num_color_cols;
  nrows = (max_colors+num_blinking_colors) / ncols;
  remainder = (max_colors+num_blinking_colors) % ncols;

  i = 0;
  for ( r=0; r<nrows; r++ ) {

    for ( c=0; c<ncols; c++ ) {

      if ( i == curIndex ) {
        x = c*5 + c*20 + 5;
        y = r*5 + r*20 + 5;
        gc.setFG( BlackPixel( display, DefaultScreen(display) ) );
        XDrawRectangle( display, XtWindow(form), gc.normGC(), x-2, y-2,
         23, 23 );
        curX = x;
        curY = y;
      }

      i++;

    }

  }

  if ( remainder ) {

    r = nrows;

    for ( c=0; c<remainder; c++ ) {

      if ( i == curIndex ) {
        x = c*5 + c*20 + 5;
        y = r*5 + r*20 + 5;
        gc.setFG( BlackPixel( display, DefaultScreen(display) ) );
        XDrawRectangle( display, XtWindow(form), gc.normGC(), x-2, y-2,
         23, 23 );
        curX = x;
        curY = y;
      }

      i++;

    }

  }

  return 1;

}

int colorInfoClass::canDiscardPixel (
  unsigned int pixel )
{

int stat;
colorCachePtr cur;

  stat = avl_get_match( this->colorCacheByPixelH, (void *) &pixel,
   (void **) &cur );
  if ( !(stat & 1) ) return 0;

  if ( cur )
    return 0;
  else
    return 1;

}

unsigned int colorInfoClass::getPixelByIndex (
  int index )
{

  if ( index >= max_colors+num_blinking_colors )
    return BlackPixel( display, screen );

  if ( index < 0 )
    return WhitePixel( display, screen );

  return colors[index];

}

unsigned int colorInfoClass::labelPix ( // return reasonable fg for given bg
  int index )
{

int stat;
colorCachePtr cur;
int sum;

  stat = avl_get_match( this->colorCacheByIndexH, (void *) &index,
   (void **) &cur );
  if ( !(stat & 1) ) return BlackPixel( display, screen );

  if ( cur ) {

    sum = cur->rgb[0] + cur->rgb[1] * 3 + cur->rgb[2];

    if ( sum < 180000 )
      return WhitePixel( display, screen );
    else
      return BlackPixel( display, screen );

  }

  return BlackPixel( display, screen );

}

char *colorInfoClass::colorName (
  int index )
{

  if ( index >= max_colors+num_blinking_colors )
    return colorNames[max_colors-1];

  if ( index < 0 )
    return colorNames[0];

  return colorNames[index];

}
int colorInfoClass::colorIndexByName (
  const char *name )
{

int stat;
colorCachePtr cur;

  stat = avl_get_match( this->colorCacheByNameH, (void *) name,
   (void **) &cur );
  if ( !( stat & 1 ) ) {
    return 0;
  }
  if ( !cur ) {
    return 0;
  }

  return cur->index;

}

int colorInfoClass::pixIndex(
  unsigned int pixel )
{

int stat;
colorCachePtr cur;

  stat = avl_get_first( this->colorCacheByIndexH, (void **) &cur );
  if ( !( stat & 1 ) ) {
    return 0;
  }

  while ( cur ) {

    if ( cur->pixel == pixel ) return cur->index;

      stat = avl_get_next( this->colorCacheByIndexH, (void **) &cur );
      if ( !( stat & 1 ) ) {
        return 0;
      }

  }

  return 0;

}

int colorInfoClass::isRule (
  int index )
{

  if ( index >= max_colors+num_blinking_colors )
    return 0;

  if ( index < 0 )
    return 0;

  return ( colorNodes[index]->rule != NULL );

}

char *colorInfoClass::firstColor (
  colorCachePtr node ) {

int stat;

  stat = avl_get_first( this->colorCacheByIndexH, (void **) &node );
  if ( !( stat & 1 ) ) {
    return NULL;
  }

  if ( node ) {
    return node->name;
  }
  else {
    return NULL;
  }

}

char *colorInfoClass::nextColor (
  colorCachePtr node ) {

int stat;

  stat = avl_get_next( this->colorCacheByIndexH, (void **) &node );
  if ( !( stat & 1 ) ) {
    return NULL;
  }

  if ( node ) {
    return node->name;
  }
  else {
    return NULL;
  }

}

int colorInfoClass::majorVersion ( void ) {

  return major;

}

int colorInfoClass::menuIndex (
  int index )
{

  if ( !menuIndexMap ) return index;

  if ( index < 0 ) return 0;
  if ( index >= maxMenuItems ) return maxMenuItems-1;

  return menuIndexMap[index];

}

int colorInfoClass::menuPosition (
  int index )
{

int i;

  if ( !menuIndexMap ) return index + 1;

  if ( index < 0 ) return 0;

  for ( i=0; i<maxMenuItems; i++ ) {
    if ( index == menuIndexMap[i] ) return i+1;
  }

  return 0;

}

int colorInfoClass::menuSize ( void ) {

  if ( menuIndexMap )
    return maxMenuItems;
  else
    return max_colors+num_blinking_colors;

}

int colorInfoClass::evalRule (
  int index,
  double value )
{

ruleConditionPtr ruleCond;
int opResult, opResult1, opResult2;

  if ( !isRule(index) ) {
    return index;
  }

  colorNodes[index]->rule->needJoin = 0;

  ruleCond = colorNodes[index]->rule->ruleHead->flink;
  while ( ruleCond ) {

    opResult1 = ruleCond->ruleFunc1( value, ruleCond->value1 );

    if ( ruleCond->connectingFunc ) {
      opResult2 = ruleCond->ruleFunc2( value, ruleCond->value2 );
      opResult = ruleCond->connectingFunc( opResult1, opResult2 );
    }
    else {
      opResult = opResult1;
    }

    if ( colorNodes[index]->rule->needJoin ) { // set by prev rule
      opResult = colorNodes[index]->rule->curJoinFunc(
       opResult, colorNodes[index]->rule->combinedOpResult );
    }

    if ( ruleCond->joiningFunc ) { // set by this rule
      colorNodes[index]->rule->curJoinFunc = ruleCond->joiningFunc;
      colorNodes[index]->rule->combinedOpResult = opResult;
      colorNodes[index]->rule->needJoin = 1;
    }
    else {
      colorNodes[index]->rule->needJoin = 0;
      if ( opResult ) {
        return ruleCond->result;
      }
    }

    ruleCond = ruleCond->flink;

  }

  return index;

}

int colorInfoClass::isInvisible(
  int index
) {

  return ( index == invisibleIndex );

}
