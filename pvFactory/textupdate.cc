// -*- C++ -*-
// EDM textupdate Widget
//
// kasemir@lanl.gov

#include "textupdate.h"
#include "app_pkg.h"
#include "act_win.h"
#include "epics_pv_factory.h"
#include "cvtFast.h"

inline const char *getRawName(expStringClass &es)
{
    char *s = es.getRaw();
    return s ? s : "";
}

inline const char *getExpandedName(expStringClass &es)
{
    char *s = es.getExpanded();
    return s ? s : "";
}

edmTextupdateClass::edmTextupdateClass()
{
    init(TEXTUPDATE_CLASSNAME);
}

void edmTextupdateClass::init(const char *classname)
{
    name = strdup(classname);
    is_executing = false;
    pv = 0;
    color_pv = 0;
    is_filled = true;
    strcpy(fontTag, "");
    fs = 0;
    alignment = XmALIGNMENT_BEGINNING;
}

edmTextupdateClass::edmTextupdateClass(edmTextupdateClass *rhs)
{
    clone(rhs, TEXTUPDATE_CLASSNAME);
}

void edmTextupdateClass::clone(const edmTextupdateClass *rhs,
                               const char *classname)
{
    // This next line must always be included
    activeGraphicClass *ago = (activeGraphicClass *) this;
    ago->clone((activeGraphicClass *)rhs);

    name = strdup(classname);

    is_executing = false;
    pv = 0;
    color_pv = 0;
    pv_name.setRaw(rhs->pv_name.rawString);
    color_pv_name.setRaw(rhs->color_pv_name.rawString);
    displayMode = rhs->displayMode;
    precision = rhs->precision;
    textColor = rhs->textColor;
    line_width = rhs->line_width;
    fillColor = rhs->fillColor;
    is_filled = rhs->is_filled;
    strncpy(fontTag, rhs->fontTag, 63);
    fontTag[63] = 0;
    fs = actWin->fi->getXFontStruct(fontTag);
    fontAscent = rhs->fontAscent;
    fontDescent = rhs->fontDescent;
    fontHeight = rhs->fontHeight;
    alignment = rhs->alignment;
}

edmTextupdateClass::~edmTextupdateClass()
{
    if (color_pv)
    {
        color_pv->remove_conn_state_callback(pv_conn_state_callback, this);
        color_pv->remove_value_callback(pv_value_callback, this);
        color_pv->release();
        color_pv = 0;
    }
    if (pv)
    {
        pv->remove_conn_state_callback(pv_conn_state_callback, this);
        pv->remove_value_callback(pv_value_callback, this);
        pv->release();
        pv = 0;
    }
    free(name);
}

char *edmTextupdateClass::objName()
{   return name; }

// --------------------------------------------------------
// Load/save
// --------------------------------------------------------
int edmTextupdateClass::save(FILE *f)
{
    // Version, bounding box
    fprintf(f, "%-d %-d %-d\n",
            TEXT_MAJOR, TEXT_MINOR, TEXT_RELEASE);
    fprintf(f, "%-d\n", x);
    fprintf(f, "%-d\n", y);
    fprintf(f, "%-d\n", w);
    fprintf(f, "%-d\n", h);
    // PV Name
    writeStringToFile(f, (char *)getRawName(pv_name));
    // Mode, precision
    fprintf(f, "%-d\n", (int) displayMode);
    fprintf(f, "%-d\n", (int) precision);
    // textcolor, fillcolor
    writeStringToFile(f, actWin->ci->colorName(textColor));
    writeStringToFile(f, actWin->ci->colorName(fillColor));
    writeStringToFile(f, (char *)getRawName(color_pv_name));
    // fill mode, fonts
    fprintf(f, "%-d\n", is_filled);
    writeStringToFile(f, fontTag);
    fprintf(f, "%-d\n", alignment);
    line_width.write(f);
   
    return 1;
}

int edmTextupdateClass::createFromFile(FILE *f, char *filename,
                                       activeWindowClass *_actWin)
{
    int major, minor, release;
    int index;
    char name[100];

    actWin = _actWin;
    // Version, bounding box
    fscanf(f, "%d %d %d\n", &major, &minor, &release); actWin->incLine();
    fscanf(f, "%d\n", &x); actWin->incLine();
    fscanf(f, "%d\n", &y); actWin->incLine();
    fscanf(f, "%d\n", &w); actWin->incLine();
    fscanf(f, "%d\n", &h); actWin->incLine();
    this->initSelectBox(); // call after getting x,y,w,h
    // PV Name
    readStringFromFile(name, 39, f); actWin->incLine();
    pv_name.setRaw(name);
    // Added in 1.1.0: displayMode & precision
    if (major == 1  && minor == 0)
    {
        displayMode = dm_default;
        precision = 0;
    }
    else
    {
        fscanf(f, "%d\n", &index ); actWin->incLine();
        if (index >=0 && index <= dm_hex)
            displayMode = (DisplayMode)index;
        else
            displayMode = dm_default;
        fscanf(f, "%d\n", &index ); actWin->incLine();
        precision = index;
    }
    // colors
    // Changed for 2.0.0: Use names
    if (major < 2)
    {
        fscanf(f, "%d\n", &index ); actWin->incLine();
        textColor = index;
        // fillcolor index & mode
        fscanf(f, "%d\n", &index ); actWin->incLine();
        fillColor = index;
    }
    else
    {
        readStringFromFile(name, 99, f); actWin->incLine();
        textColor = actWin->ci->colorIndexByName(name);
        readStringFromFile(name, 99, f); actWin->incLine();
        fillColor = actWin->ci->colorIndexByName(name);
    }
    // Since 3.0.0: Use color pv
    if (major < 3)
        color_pv_name.setRaw(0);
    else
    {
        readStringFromFile(name, 39, f); actWin->incLine();
        color_pv_name.setRaw(name);
    }

    fscanf(f, "%d\n", &is_filled); actWin->incLine();
    readStringFromFile(fontTag, 63, f); actWin->incLine();
    fscanf( f, "%d\n", &alignment ); actWin->incLine();
    actWin->fi->loadFontTag(fontTag);
    fs = actWin->fi->getXFontStruct(fontTag);
    updateFont(fontTag, &fs,
               &fontAscent, &fontDescent, &fontHeight);
    if (major >= 1)
    {
        line_width.read(f); actWin->incLine();
    }
    else
    {
        line_width.setNull(1);
    }
    
    return 1;
}

// --------------------------------------------------------
// Edit Mode
// --------------------------------------------------------

// Idea of next two and helper methods:
// createInteractive -> editCreate -> genericEdit (delete on cancel)
// edit -> genericEdit (ignore changes on cancel)
int edmTextupdateClass::createInteractive(activeWindowClass *aw_obj,
                                          int _x, int _y, int _w, int _h)
{   // required
    actWin = (activeWindowClass *) aw_obj;
    x = _x; y = _y; w = _w; h = _h;
    // Honor display scheme
    displayMode = dm_default;
    precision = 0;
    textColor = actWin->defaultFg1Color;
    line_width.setNull(1);
    fillColor = actWin->defaultBgColor;
    strcpy(fontTag, actWin->defaultCtlFontTag);
    alignment = actWin->defaultCtlAlignment;
    fs = actWin->fi->getXFontStruct(fontTag);
    updateFont(fontTag, &fs, &fontAscent, &fontDescent, &fontHeight);

    // initialize and draw some kind of default image for the user
    draw();
    editCreate();
    return 1;
}

int edmTextupdateClass::edit()
{   // Popup property dialog, cancel -> no delete
    genericEdit();
    ef.finished(edit_ok, edit_apply, edit_cancel, this);
    actWin->currentEf = &ef;
    ef.popup();
    return 1;
}

int edmTextupdateClass::editCreate()
{
    // Popup property dialog, cancel -> delete
    genericEdit();
    ef.finished(edit_ok, edit_apply, edit_cancel_delete, this);
    actWin->currentEf = NULL;
    ef.popup();
    return 1;
}

int edmTextupdateClass::genericEdit() // create Property Dialog
{
    char title[80], *ptr;
    // required
    ptr = actWin->obj.getNameFromClass(name);
    if (ptr)
    {
        strncpy(title, ptr, 80);
        strncat(title, " Properties", 80);
    }
    else
        strncpy(title, "Unknown object Properties", 80);
   
    // Copy data member contents into edit buffers
    bufX = x; bufY = y; bufW = w; bufH = h;
    strncpy(bufPvName,      getRawName(pv_name), 39);
    strncpy(bufColorPvName, getRawName(color_pv_name), 39);
    buf_displayMode = (int)displayMode;
    buf_precision   = precision;
    buf_line_width  = line_width;
    bufTextColor    = textColor;
    bufFillColor    = fillColor;
    bufIsFilled     = is_filled;

    // create entry form dialog box
    ef.create(actWin->top, actWin->appCtx->ci.getColorMap(),
              &actWin->appCtx->entryFormX, &actWin->appCtx->entryFormY,
              &actWin->appCtx->entryFormW, &actWin->appCtx->entryFormH,
              &actWin->appCtx->largestH,
              title, NULL, NULL, NULL);

    // add dialog box entry fields
    ef.addTextField("X", 30, &bufX);
    ef.addTextField("Y", 30, &bufY);
    ef.addTextField("Width", 30, &bufW);
    ef.addTextField("Height", 30, &bufH);
    ef.addTextField("PV", 30, bufPvName, 39);
    ef.addOption("Mode", "default|decimal|hex", &buf_displayMode);
    ef.addTextField("Precision", 30, &buf_precision);
    ef.addTextField("Line Width", 30, &buf_line_width);
    ef.addColorButton("Fg Color", actWin->ci, &textCb, &bufTextColor);
    ef.addToggle("Filled?", &bufIsFilled);
    ef.addColorButton("Bg Color", actWin->ci, &fillCb, &bufFillColor);
    ef.addTextField("Color PV", 30, bufColorPvName, 39);
    ef.addFontMenu("Font", actWin->fi, &fm, fontTag );
    fm.setFontAlignment(alignment);

    return 1;
}

void edmTextupdateClass::redraw_text(Display *dis,
                                     Drawable drw,
                                     gcClass &gcc,
                                     GC gc,
                                     const char *text,
                                     size_t len,
                                     double value)
{
    int fg_pixel;
    
    if (actWin->ci->isRule(textColor))
        fg_pixel = actWin->ci->getPixelByIndex(actWin->ci->evalRule(textColor,
                                                                    value));
    else
        fg_pixel = actWin->ci->getPixelByIndex(textColor);
    
    // Background fill?
    if (is_filled)
    {
        gcc.setFG(actWin->ci->getPixelByIndex(fillColor));
        XFillRectangle(dis, drw, gc, x, y, w, h);
    }
    gcc.setFG(fg_pixel);
    // Border
    if (!line_width.isNull())
    {
        gcc.setLineWidth(line_width.value());
        XDrawRectangle(dis, drw, gc, x, y, w, h);
        gcc.setLineWidth(1);
    }
    if (len > 0)
    {
        // Text
        // Positioning is calculated for each call
        // -> not terrible for critical situation?
        //    a) called during edit -> good enough
        //    b) called for changed value -> have to do it, text is different
        //    c) called for total redraw after window hidden -> hmmm.
        // Right now I care most about b) to assert best update rate.
        XRectangle clip;
        clip.x = x;
        clip.y = y;
        clip.width = w;
        clip.height = h;
        gcc.addNormXClipRectangle(clip);
        gcc.setFontTag(fontTag, actWin->fi);
        int txt_width = XTextWidth(fs, text, len);
        int tx;
        switch (alignment)
        {
            case XmALIGNMENT_BEGINNING:
                tx = x;
                break;
            case XmALIGNMENT_CENTER:
                tx = x + (w - txt_width)/2;
                break;
            default:
                tx = x + w - txt_width;
        }
        int ty = y + (fontAscent + h)/2;
        XDrawString(dis, drw, gc, tx, ty, text, len);
        gcc.removeNormXClipRectangle();
    }
}

void  edmTextupdateClass::remove_text(Display *dis,
                                      Drawable drw,
                                      gcClass &gcc,
                                      GC gc)
{
    XFillRectangle(dis, drw, gc, x, y, w, h);
    if (!line_width.isNull())
    {
        gcc.setLineWidth(line_width.value());
        XDrawRectangle(dis, drw, gc, x, y, w, h);
        gcc.setLineWidth(1);
    }
}

int edmTextupdateClass::draw()  // render the edit-mode image
{
    // required
    if (is_executing || deleteRequest)
        return 1;
    actWin->drawGc.saveFg();
    
    const char *pvname = getRawName(pv_name);
    size_t len = strlen(pvname);
    redraw_text(actWin->d,
                XtWindow(actWin->drawWidget),
                actWin->drawGc,
                actWin->drawGc.normGC(),
                pvname, len, 0.0);
    
    actWin->drawGc.restoreFg();
    return 1;
}

int edmTextupdateClass::erase()  // erase edit-mode image
{
    // required
    if (is_executing || deleteRequest )
        return 1;
    remove_text(actWin->d,
                XtWindow(actWin->drawWidget),
                actWin->drawGc,
                actWin->drawGc.eraseGC());
    return 1;
}

int edmTextupdateClass::checkResizeSelectBox(int _x, int _y, int _w, int _h)
{   // Assert minimum size
    return checkResizeSelectBoxAbs(_x, _y, w+_w, h+_h);
}

int edmTextupdateClass::checkResizeSelectBoxAbs(int _x, int _y, int _w, int _h)
{   // Similar, but absolute sizes. -1 is also possible
    if (_w != -1  &&  _w < 10)
        return 0;
    if (_h != -1  &&  _h < 10)
        return 0;
    return 1;
}

// Callbacks from property dialog
void edmTextupdateClass::edit_update(Widget w, XtPointer client,XtPointer call)
{
    edmTextupdateClass *me = (edmTextupdateClass *) client;
    // required
    me->actWin->setChanged();
    me->eraseSelectBoxCorners();
    me->erase();

    me->x = me->bufX;
    me->sboxX = me->bufX;
    me->y = me->bufY;
    me->sboxY = me->bufY;
    me->w = me->bufW;
    me->sboxW = me->bufW;
    me->h = me->bufH;
    me->sboxH = me->bufH;

    me->pv_name.setRaw(me->bufPvName);
    me->color_pv_name.setRaw(me->bufColorPvName);
    me->displayMode = (DisplayMode) me->buf_displayMode;
    me->precision   = me->buf_precision;
    me->line_width  = me->buf_line_width;
    me->textColor = me->bufTextColor;
    me->fillColor = me->bufFillColor;
    me->is_filled   = me->bufIsFilled;

    strncpy(me->fontTag, me->fm.currentFontTag(), 63);
    me->fontTag[63] = 0;
    me->actWin->fi->loadFontTag(me->fontTag);
    me->fs          = me->actWin->fi->getXFontStruct(me->fontTag);
    me->alignment   = me->fm.currentFontAlignment();
    me->fs          = me->actWin->fi->getXFontStruct(me->fontTag);
    me->updateFont(me->fontTag, &me->fs,
                   &me->fontAscent, &me->fontDescent, &me->fontHeight);
}

void edmTextupdateClass::edit_ok(Widget w, XtPointer client, XtPointer call)
{
    edmTextupdateClass *me = (edmTextupdateClass *) client;
    edit_update(w, client, call);
    // required
    me->ef.popdown();
    me->operationComplete();
}

void edmTextupdateClass::edit_apply(Widget w, XtPointer client, XtPointer call)
{
    edmTextupdateClass *me = (edmTextupdateClass *) client;
    edit_update(w, client, call);
    // required
    me->refresh(me);
}

void edmTextupdateClass::edit_cancel(Widget w, XtPointer client,XtPointer call)
{
    edmTextupdateClass *me = (edmTextupdateClass *) client;
    // next two lines required
    me->ef.popdown();
    me->operationCancel();
}

void edmTextupdateClass::edit_cancel_delete(Widget w, XtPointer client,
                                            XtPointer cal)
{
    edmTextupdateClass *me = (edmTextupdateClass *) client;
    // all lines required
    me->ef.popdown();
    me->operationCancel();
    me->erase();
    me->deleteRequest = 1;
    me->drawAll();
}

// --------------------------------------------------------
// GroupEdit
// --------------------------------------------------------
// edm-components.doc shows wrong prototype for these!
void edmTextupdateClass::changeDisplayParams(unsigned int flag,
                                             char *_fontTag,
                                             int _alignment,
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
                                             int botShadowColor)
{
    if (flag & ACTGRF_FG1COLOR_MASK)
        textColor = fg1Color;
    if (flag & ACTGRF_BGCOLOR_MASK)
        fillColor = bgColor;
    if (flag & ACTGRF_FONTTAG_MASK)
    {
        strcpy(fontTag, _fontTag);
        alignment = _alignment;
        fs = actWin->fi->getXFontStruct(fontTag);
        updateFont(fontTag, &fs,
                   &fontAscent, &fontDescent, &fontHeight);
    }
}

void edmTextupdateClass::changePvNames(int flag,
                                       int numCtlPvs,
                                       char *ctlPvs[],
                                       int numReadbackPvs,
                                       char *readbackPvs[],
                                       int numNullPvs,
                                       char *nullPvs[],
                                       int numVisPvs,
                                       char *visPvs[],
                                       int numAlarmPvs,
                                       char *alarmPvs[])
{
    if (flag & ACTGRF_READBACKPVS_MASK)
    {
        if (numReadbackPvs)
            pv_name.setRaw(readbackPvs[0]);
    }
    // Note: There is no "color" PV.
    // We use the "vis" PV for now
    if (flag & ACTGRF_VISPVS_MASK)
    {
        if (numVisPvs)
            color_pv_name.setRaw(visPvs[0]);
    }
}
    
// --------------------------------------------------------
// Macro support
// --------------------------------------------------------
int edmTextupdateClass::containsMacros()
{
    return pv_name.containsPrimaryMacros() ||
        color_pv_name.containsPrimaryMacros();
}

int edmTextupdateClass::expand1st(int numMacros, char *macros[],
                                  char *expansions[])
{
    color_pv_name.expand1st(numMacros, macros, expansions);
    return pv_name.expand1st(numMacros, macros, expansions);
}

int edmTextupdateClass::expand2nd(int numMacros, char *macros[],
                                  char *expansions[])
{
    color_pv_name.expand2nd(numMacros, macros, expansions);
    return pv_name.expand2nd(numMacros, macros, expansions);
}

// --------------------------------------------------------
// Execute
// --------------------------------------------------------
int edmTextupdateClass::activate(int pass, void *ptr)
{
    switch (pass) // ... up to 6
    {
        case 1: // initialize
            aglPtr = ptr;
            is_executing = true;
            is_pv_valid = strcmp(getExpandedName(pv_name), "") != 0;
            is_color_pv_valid =strcmp(getExpandedName(color_pv_name), "") != 0;
            break;
        case 2: // connect to pv
            if (pv)
                printf("textupdate::activate: pv already set!\n");
            if (is_pv_valid)
            {
                pv = the_PV_Factory->create(getExpandedName(pv_name));
                if (pv)
                {
                    pv->add_conn_state_callback(pv_conn_state_callback, this);
                    pv->add_value_callback(pv_value_callback, this);
                }
            }
            if (is_color_pv_valid)
            {
                color_pv=the_PV_Factory->create(getExpandedName(color_pv_name));
                if (color_pv)
                {
                    color_pv->add_conn_state_callback(pv_conn_state_callback,
                                                      this);
                    color_pv->add_value_callback(pv_value_callback, this);
                    printf("Created color PV %s\n", color_pv->get_name());
                }
            }
            if (!pv)
                drawActive();
            break;
    }
    return 1;
}

int edmTextupdateClass::deactivate(int pass)
{
    is_executing = false;
    switch (pass)
    {
        case 1: // disconnect
            if (color_pv)
            {
                color_pv->remove_conn_state_callback(pv_conn_state_callback, this);
                color_pv->remove_value_callback(pv_value_callback, this);
                color_pv->release();
                color_pv = 0;
            }
            if (pv)
            {
                pv->remove_conn_state_callback(pv_conn_state_callback, this);
                pv->remove_value_callback(pv_value_callback, this);
                pv->release();
                pv = 0;
            }
            break;
        case 2: // remove toolkit widgets
            break;
    }
    return 1;
}

// Get text & color value.
// len has to be initialized with the text buffer size.
// Returns 1 if PV is valid
bool edmTextupdateClass::get_current_values(char *text, size_t &len,
                                            double &color_value)
{
    if (pv && pv->is_valid())
    {
        if (color_pv && color_pv->is_valid())
            color_value = color_pv->get_double();
        else
            color_value = pv->get_double();
        switch (displayMode)
        {
            case dm_hex:
                if (pv->get_type().type < ProcessVariable::Type::enumerated)
                {
                    cvtLongToHexString(pv->get_int(), text);
                    len = strlen(text);
                    break;
                }
            case dm_decimal:
                if (pv->get_type().type < ProcessVariable::Type::enumerated)
                {
                    cvtDoubleToString(pv->get_double(), text,
                                      (unsigned short) precision);
                    len = strlen(text);
                    break;
                }
            default:
                len = pv->get_string(text, 80);
        }
        return true;
    }

    color_value = 0.0;
    text[0] = '<';
    strcpy(text+1, getExpandedName(pv_name));
    strcat(text, ">");
    len = strlen(text);

    return false;
}



int edmTextupdateClass::drawActive()
{
    if (!is_executing)
        return 1;
    actWin->executeGc.saveFg();

    double color_value;
    char text[80];
    size_t len = 80;
    get_current_values(text, len, color_value);
    redraw_text(actWin->d,
                XtWindow(actWin->executeWidget),
                actWin->executeGc,
                actWin->executeGc.normGC(),
                text, len, color_value);
   
    actWin->executeGc.restoreFg();
    return 1;
}

int edmTextupdateClass::eraseActive()
{
    if (!is_executing)
        return 1;
    remove_text(actWin->d,
                XtWindow(actWin->executeWidget),
                actWin->executeGc,
                actWin->executeGc.eraseGC());
    return 1;
}

void edmTextupdateClass::pv_conn_state_callback(ProcessVariable *pv,
                                                void *userarg)
{
    edmTextupdateClass *me = (edmTextupdateClass *)userarg;
    me->actWin->appCtx->proc->lock();
    if (me->is_executing)
    {
        me->bufInvalidate();
        me->actWin->addDefExeNode(me->aglPtr);
    }
    me->actWin->appCtx->proc->unlock();
}

void edmTextupdateClass::pv_value_callback(ProcessVariable *pv,
                                           void *userarg)
{
    edmTextupdateClass *me = (edmTextupdateClass *)userarg;
    me->actWin->appCtx->proc->lock();
    if (me->is_executing)
    {
        me->bufInvalidate();
        me->actWin->addDefExeNode(me->aglPtr);
    }
    me->actWin->appCtx->proc->unlock();
}

void edmTextupdateClass::executeDeferred()
{   // Called as a result of addDefExeNode
    if (actWin->isIconified)
        return;
    actWin->appCtx->proc->lock();
    actWin->remDefExeNode(aglPtr);
    actWin->appCtx->proc->unlock();
    if (is_executing)
        smartDrawAllActive();
}

// Drag & drop support
char *edmTextupdateClass::firstDragName()
{   return "PV"; }

char *edmTextupdateClass::nextDragName()
{   return NULL; }

char *edmTextupdateClass::dragValue(int i)
{   return (char *)getExpandedName(pv_name); }

// --------------------------------------------------------
// Text Entry
// --------------------------------------------------------

edmTextentryClass::edmTextentryClass()
{
    init(TEXTENTRY_CLASSNAME);
    widget = 0;
    editing = false;
}

edmTextentryClass::edmTextentryClass(const edmTextentryClass *rhs)
{
    clone(rhs, TEXTENTRY_CLASSNAME);
    widget = 0;
    editing = false;
}


// callbacks for drag & drop from Motif text widgets

static void drag(Widget w, XEvent *e, String *params, Cardinal numParams)
{
    activeGraphicClass *obj;
    XtVaGetValues(w, XmNuserData, &obj, NULL);
    
    obj->startDrag(w, e);
}

static void selectDrag(Widget w, XEvent *e, String *params, Cardinal numParams)
{
    activeGraphicClass *obj;
    XButtonEvent *be = (XButtonEvent *) e;

    XtVaGetValues(w, XmNuserData, &obj, NULL);
    obj->selectDragValue(obj->getX0() + be->x, obj->getY0() + be->y);
}

static char dragTrans[] =
"#override\n~Shift<Btn2Down>: startDrag()\nShift<Btn2Up>: selectDrag()";

static XtActionsRec dragActions[] =
{
    { "startDrag", (XtActionProc) drag },
    { "selectDrag", (XtActionProc) selectDrag }
};


int edmTextentryClass::activate(int pass, void *ptr)
{
    XmFontList fonts;
    if (! edmTextupdateClass::activate(pass, ptr))
        return 0;
    
    switch (pass) // ... up to 6
    {
        case 1: // initialize
            // from man XmTextField
            fonts = XmFontListCreate(fs, XmSTRING_DEFAULT_CHARSET);
            XtTranslations parsedTrans;
            parsedTrans = XtParseTranslationTable(dragTrans);
            XtAppAddActions(actWin->appCtx->appContext(), dragActions,
                            XtNumber(dragActions));
            
            widget = XtVaCreateManagedWidget("TextEntry",
                                             xmTextFieldWidgetClass,
                                             actWin->executeWidgetId(),
                                             XtNx, (XtArgVal)x,
                                             XtNy, (XtArgVal)y,
                                             XtNheight,(XtArgVal)h,
                                             XtNwidth, (XtArgVal)w,
                                             XmNforeground,
                                             (XtArgVal)
                                             actWin->ci->getPixelByIndex(textColor),
                                             XmNbackground, (XtArgVal)
                                             actWin->ci->getPixelByIndex(fillColor),
                                             XmNfontList, (XtArgVal)fonts,
                                             // next 2 seem to have no effect:
                                             XmNentryAlignment,
                                                 (XtArgVal)alignment,
                                             XmNalignment,
                                                 (XtArgVal)alignment,
                                             XmNtranslations, parsedTrans,
                                             XmNuserData,
                                                 this,// obj accessible to d&d
                                             NULL);
            // callback: text entered, sent it to PV
            XtAddCallback(widget,XmNactivateCallback,
                          (XtCallbackProc)text_entered_callback,
                          (XtPointer)this);

            // callback: go into edit mode
            // Used to try XmNmodifyVerifyCallback,
            // but this one is better (John found it)
            XtAddCallback(widget,XmNmotionVerifyCallback,
                          (XtCallbackProc)text_edit_callback,
                          (XtPointer)this);
            break;
    }
    return 1;
}

int edmTextentryClass::deactivate(int pass)
{
    is_executing = false;
    switch (pass)
    {
        case 2: // remove toolkit widgets
            if (widget)
            {
                XtUnmapWidget(widget);
                XtDestroyWidget(widget);
                widget = 0;
            }
            break;
    }
    return edmTextupdateClass::deactivate(pass);
}

int edmTextentryClass::drawActive()
{
    if (!is_executing)
        return 1;
    if (editing)
        return 1;

    double color_value;
    char text[80];
    size_t len = 80;
    if (get_current_values(text, len, color_value))
    {
        XtVaSetValues(widget, XmNeditable, True, NULL);
        if (pv->have_write_access())
            actWin->cursor.set(XtWindow(widget), CURSOR_K_DEFAULT);
        else
            actWin->cursor.set(XtWindow(widget), CURSOR_K_NO);
    }
    else
    {
        XtVaSetValues(widget, XmNeditable, False, NULL);
        actWin->cursor.set(XtWindow(widget), CURSOR_K_WAIT);
    }
    XmTextFieldSetString(widget, text);
   
    return 1;
}

int edmTextentryClass::eraseActive()
{
    return 1;
}

void edmTextentryClass::text_entered_callback(Widget w,
                                              XtPointer clientData,
                                              XtPointer)
{
    edmTextentryClass *me = (edmTextentryClass *) clientData;
    char *text = XmTextFieldGetString(w);
    double num;
    int hexnum;
    
    if (me->pv && me->pv->is_valid())
    {
        switch (me->displayMode)
        {
            case dm_default:
                if (me->pv->get_type().type <
                    ProcessVariable::Type::enumerated)
                {
                    num = strtod(text, 0);
                    me->pv->put(num);
                }
                else
                {
                    me->pv->put(text);
                }
                break;
            case dm_hex:
                hexnum = strtol(text, 0, 16);
                me->pv->put(hexnum);
                break;
            default:
                me->pv->put(text);
        }
    }
    XtFree(text);
}

void edmTextentryClass::text_edit_callback(Widget w,
                                             XtPointer clientData,
                                             XtPointer pCallbackData)
{
    edmTextentryClass *me = (edmTextentryClass *) clientData;
    XmTextVerifyCallbackStruct *pcbs =
        (XmTextVerifyCallbackStruct *) pCallbackData;
    /* NULL event means value changed programmatically; hence don't process */
    if (pcbs->event != NULL)
    {
        switch (XtHasCallbacks(w,XmNlosingFocusCallback))
        {
            case XtCallbackNoList:
            case XtCallbackHasNone:
                XtAddCallback(w, XmNlosingFocusCallback,
                              (XtCallbackProc)text_noedit_callback, me);
                me->editing = true;
                break;
            case XtCallbackHasSome:
                break;
        }
        pcbs->doit = True;
    }
}

void edmTextentryClass::text_noedit_callback(Widget w,
                                             XtPointer clientData,
                                             XtPointer pCallbackData)
{
    edmTextentryClass *me = (edmTextentryClass *) clientData;

    XtRemoveCallback(w, XmNlosingFocusCallback,
                     (XtCallbackProc)text_noedit_callback, me);
    me->editing= false;
    pv_value_callback(me->pv, me);
}

