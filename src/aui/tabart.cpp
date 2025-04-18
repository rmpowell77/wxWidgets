///////////////////////////////////////////////////////////////////////////////
// Name:        src/aui/tabart.cpp
// Purpose:     wxaui: wx advanced user interface - notebook-art
// Author:      Benjamin I. Williams
// Modified by: Jens Lody (moved from auibook.cpp in extra file)
// Created:     2012-03-21
// Copyright:   (C) Copyright 2006, Kirix Corporation, All Rights Reserved
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"


#if wxUSE_AUI

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/dc.h"
    #include "wx/dcclient.h"
    #include "wx/settings.h"
    #include "wx/bitmap.h"
    #include "wx/menu.h"
#endif

#include "wx/renderer.h"
#include "wx/aui/auibook.h"
#include "wx/aui/framemanager.h"
#include "wx/aui/dockart.h"

#ifdef __WXMAC__
#include "wx/osx/private.h"
#endif

#include "wx/private/aui.h"

// -- GUI helper classes and functions --

class wxAuiCommandCapture : public wxEvtHandler
{
public:

    wxAuiCommandCapture() { m_lastId = 0; }
    int GetCommandId() const { return m_lastId; }

    bool ProcessEvent(wxEvent& evt) override
    {
        if (evt.GetEventType() == wxEVT_MENU)
        {
            m_lastId = evt.GetId();
            return true;
        }

        if (GetNextHandler())
            return GetNextHandler()->ProcessEvent(evt);

        return false;
    }

private:
    int m_lastId;
};


// these functions live in dockart.cpp -- they'll eventually
// be moved to a new utility cpp file

float wxAuiGetColourContrast(const wxColour& c1, const wxColour& c2);

wxString wxAuiChopText(wxDC& dc, const wxString& text, int max_size);

// Check if the color has sufficient contrast ratio (4.5 recommended)
// (based on https://www.w3.org/TR/UNDERSTANDING-WCAG20/visual-audio-contrast7.html)
static bool wxAuiHasSufficientContrast(const wxColour& c1, const wxColour& c2)
{
    return wxAuiGetColourContrast(c1, c2) >= 4.5f;
}

// Pick a color that provides better contrast against the background
static wxColour wxAuiGetBetterContrastColour(const wxColour& back_color,
    const wxColour& c1, const wxColour& c2)
{
    return wxAuiGetColourContrast(back_color, c1)
          > wxAuiGetColourContrast(back_color, c2) ? c1 : c2;
}

static void DrawButtons(wxDC& dc,
                        const wxSize& offset,
                        const wxRect& _rect,
                        const wxBitmap& bmp,
                        const wxColour& bkcolour,
                        int button_state)
{
    wxRect rect = _rect;

    if (button_state == wxAUI_BUTTON_STATE_PRESSED)
    {
        rect.x += offset.x;
        rect.y += offset.y;
    }

    if (button_state == wxAUI_BUTTON_STATE_HOVER ||
        button_state == wxAUI_BUTTON_STATE_PRESSED)
    {
        dc.SetBrush(wxBrush(bkcolour.ChangeLightness(120)));
        dc.SetPen(wxPen(bkcolour.ChangeLightness(75)));

        // draw the background behind the button
        dc.DrawRectangle(rect.x, rect.y, bmp.GetLogicalWidth()-offset.x, bmp.GetLogicalHeight()-offset.y);
    }

    // draw the button itself
    dc.DrawBitmap(bmp, rect.x, rect.y, true);
}

static void IndentPressedBitmap(const wxSize& offset, wxRect* rect, int button_state)
{
    if (button_state == wxAUI_BUTTON_STATE_PRESSED)
    {
        rect->x += offset.x;
        rect->y += offset.y;
    }
}

// -- bitmaps --

#ifdef wxHAS_SVG
    static const char* const left_bitmap_data = R"svg(
<svg version="1.0" xmlns="http://www.w3.org/2000/svg" width="16" height="16">
    <polygon points="3, 8 9, 3 9, 13" stroke="currentColor" fill="currentColor" stroke-width="0"/>
</svg>
)svg";

    static const char* const right_bitmap_data = R"svg(
<svg version="1.0" xmlns="http://www.w3.org/2000/svg" width="16" height="16">
    <polygon points="13, 8 7, 3 7, 13" stroke="currentColor" fill="currentColor" stroke-width="0"/>
</svg>
)svg";

    static const char* const list_bitmap_data = R"svg(
<svg version="1.0" xmlns="http://www.w3.org/2000/svg" width="16" height="16">
    <rect x="4.5" y="7" width="7" height="1" stroke="currentColor" fill="currentColor" stroke-width="0"/>
    <polygon points="4.5, 9 11.5 9 8, 13" stroke="currentColor" fill="currentColor" stroke-width="0"/>
</svg>
)svg";

    static const char* const unpin_bitmap_data = R"svg(
<svg version="1.0" xmlns="http://www.w3.org/2000/svg" width="16" height="16">
    <path d="M 7 5 v 6 v -1 h 6 v -1 h -6 h 6 v -3 h -6 v 2 h -4" stroke="currentColor" fill="none" stroke-width="1"/>
</svg>
)svg";
#else // !wxHAS_SVG
static const unsigned char left_bitmap_data[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xfe, 0x3f, 0xfe,
   0x1f, 0xfe, 0x0f, 0xfe, 0x1f, 0xfe, 0x3f, 0xfe, 0x7f, 0xfe, 0xff, 0xfe,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static const unsigned char right_bitmap_data[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0x9f, 0xff, 0x1f, 0xff,
   0x1f, 0xfe, 0x1f, 0xfc, 0x1f, 0xfe, 0x1f, 0xff, 0x9f, 0xff, 0xdf, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static const unsigned char list_bitmap_data[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xf8, 0xff, 0xff, 0x0f, 0xf8, 0x1f, 0xfc, 0x3f, 0xfe, 0x7f, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static const unsigned char unpin_bitmap_data[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0x3f, 0xe0,
   0xbf, 0xef, 0x87, 0xef, 0x3f, 0xe0, 0x3f, 0xe0, 0xbf, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
#endif // wxHAS_SVG/!wxHAS_SVG

// wxAuiTabArt implementation

int wxAuiTabArt::GetButtonRect(
                         wxReadOnlyDC& WXUNUSED(dc),
                         wxWindow* WXUNUSED(wnd),
                         const wxRect& WXUNUSED(inRect),
                         int WXUNUSED(bitmapId),
                         int WXUNUSED(buttonState),
                         int WXUNUSED(orientation),
                         wxRect* WXUNUSED(outRect))
{
    wxFAIL_MSG("Should be implemented if wxAUI_NB_MULTILINE is used");

    return 0;
}

void wxAuiTabArt::DrawTab(
                         wxDC& WXUNUSED(dc),
                         wxWindow* WXUNUSED(wnd),
                         const wxAuiNotebookPage& WXUNUSED(pane),
                         const wxRect& WXUNUSED(inRect),
                         int WXUNUSED(closeButtonState),
                         wxRect* WXUNUSED(outTabRect),
                         wxRect* WXUNUSED(outButtonRect),
                         int* WXUNUSED(xExtent))
{
    // This can only be called from the default implementation of
    // DrawPageTab(), which means that that function is not overridden
    // in the derived class -- but then this one must be.
    wxFAIL_MSG("Did you forget to override DrawPageTab()?");
}

int wxAuiTabArt::DrawPageTab(
                         wxDC& dc,
                         wxWindow* wnd,
                         wxAuiNotebookPage& page,
                         const wxRect& rect)
{
    wxRect* closeRect = nullptr;
    int closeState = wxAUI_BUTTON_STATE_HIDDEN;

    for ( auto& button : page.buttons )
    {
        if ( button.id == wxAUI_BUTTON_CLOSE )
        {
            closeRect = &button.rect;
            closeState = button.curState;
        }
        else
        {
            wxFAIL_MSG("Must be overridden if using buttons other than close");
        }
    }

    int xExtent = 0;

    DrawTab(dc, wnd, page, rect, closeState, &page.rect, closeRect, &xExtent);

    return xExtent;
}

wxSize wxAuiTabArt::GetTabSize(
                         wxReadOnlyDC& WXUNUSED(dc),
                         wxWindow* WXUNUSED(wnd),
                         const wxString& WXUNUSED(caption),
                         const wxBitmapBundle& WXUNUSED(bitmap),
                         bool WXUNUSED(active),
                         int WXUNUSED(closeButtonState),
                         int* WXUNUSED(xExtent))
{
    wxFAIL_MSG("Did you forget to override GetPageTabSize()?");

    return wxSize{};
}

wxSize wxAuiTabArt::GetPageTabSize(
                         wxReadOnlyDC& dc,
                         wxWindow* wnd,
                         const wxAuiNotebookPage& page,
                         int* xExtent)
{
    int closeState = wxAUI_BUTTON_STATE_HIDDEN;

    for ( const auto& button : page.buttons )
    {
        if ( button.id == wxAUI_BUTTON_CLOSE )
        {
            closeState = button.curState;
        }
        else
        {
            wxFAIL_MSG("Must be overridden if using buttons other than close");
        }
    }

    // This function allows passing null pointer for the extent, but
    // GetTabSize() doesn't, so give it something if necessary.
    int dummyExtent;

    return GetTabSize(dc, wnd, page.caption, page.bitmap, page.active,
                      closeState, xExtent ? xExtent : &dummyExtent);
}

// -- wxAuiGenericTabArt class implementation --

wxAuiGenericTabArt::wxAuiGenericTabArt()
    : m_normalFont(*wxNORMAL_FONT)
    , m_selectedFont(m_normalFont)
{
    m_selectedFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_measuringFont = m_selectedFont;

    m_fixedTabWidth = wxWindow::FromDIP(100, nullptr);
    m_tabCtrlHeight = 0;
    m_flags = 0;

    UpdateColoursFromSystem();
}

wxAuiGenericTabArt::~wxAuiGenericTabArt()
{
}

void wxAuiGenericTabArt::UpdateColoursFromSystem()
{
    wxColor baseColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);

    // the baseColour is too pale to use as our base colour,
    // so darken it a bit --
    if ((255-baseColour.Red()) +
        (255-baseColour.Green()) +
        (255-baseColour.Blue()) < 60)
    {
        baseColour = baseColour.ChangeLightness(92);
    }

    m_activeColour = baseColour;
    m_baseColour = baseColour;
    wxColor borderColour = baseColour.ChangeLightness(75);

    m_borderPen = wxPen(borderColour);
    m_baseColourPen = wxPen(m_baseColour);
    m_baseColourBrush = wxBrush(m_baseColour);

    const int disabledLightness = wxSystemSettings::GetAppearance().IsUsingDarkBackground() ? 130 : 70;

    m_activeCloseBmp = wxAuiCreateCloseButtonBitmap(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_disabledCloseBmp = wxAuiCreateCloseButtonBitmap(wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT).ChangeLightness(disabledLightness));

    m_activeLeftBmp = wxAuiCreateBitmap(left_bitmap_data, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_disabledLeftBmp = wxAuiCreateBitmap(left_bitmap_data, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    m_activeRightBmp = wxAuiCreateBitmap(right_bitmap_data, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_disabledRightBmp = wxAuiCreateBitmap(right_bitmap_data, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    m_activeWindowListBmp = wxAuiCreateBitmap(list_bitmap_data, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_disabledWindowListBmp = wxAuiCreateBitmap(list_bitmap_data, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));

    // This is a bit confusing, but we use "pin" bitmap to indicate that the
    // tab is currently pinned, i.e. for the "unpin" button, and vice versa.
    m_activePinBmp = wxAuiCreateBitmap(unpin_bitmap_data, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_disabledPinBmp = wxAuiCreateBitmap(unpin_bitmap_data, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));

    m_activeUnpinBmp = wxAuiCreatePinButtonBitmap(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_disabledUnpinBmp = wxAuiCreatePinButtonBitmap(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
}

wxAuiTabArt* wxAuiGenericTabArt::Clone()
{
    return new wxAuiGenericTabArt(*this);
}

void wxAuiGenericTabArt::SetFlags(unsigned int flags)
{
    m_flags = flags;
}

void wxAuiGenericTabArt::SetSizingInfo(const wxSize& tab_ctrl_size,
                                       size_t tab_count,
                                       wxWindow* wnd)
{
    if ( !wnd )
    {
        // This is only allowed for backwards compatibility, we should be
        // really passed a valid window.
        wnd = wxTheApp->GetTopWindow();
        wxCHECK_RET( wnd, "must have some window" );
    }

    m_fixedTabWidth = wnd->FromDIP(100);

    int tot_width = (int)tab_ctrl_size.x - GetIndentSize() - wnd->FromDIP(4);

    if (m_flags & wxAUI_NB_CLOSE_BUTTON)
        tot_width -= m_activeCloseBmp.GetPreferredLogicalSizeFor(wnd).x;
    if (m_flags & wxAUI_NB_WINDOWLIST_BUTTON)
        tot_width -= m_activeWindowListBmp.GetPreferredLogicalSizeFor(wnd).x;

    if (tab_count > 0)
    {
        m_fixedTabWidth = tot_width/(int)tab_count;
    }


    m_fixedTabWidth = wxMax(m_fixedTabWidth, wnd->FromDIP(100));

    if (m_fixedTabWidth > tot_width/2)
        m_fixedTabWidth = tot_width/2;

    m_fixedTabWidth = wxMin(m_fixedTabWidth, wnd->FromDIP(220));

    // We don't use this member variable ourselves any longer but keep it for
    // compatibility with the existing code, deriving from this class and using
    // it for its own purposes.
    m_tabCtrlHeight = tab_ctrl_size.y;
}


void wxAuiGenericTabArt::DrawBorder(wxDC& dc, wxWindow* wnd, const wxRect& rect)
{
    int i, border_width = GetBorderWidth(wnd);

    wxRect theRect(rect);
    for (i = 0; i < border_width; ++i)
    {
        dc.DrawRectangle(theRect.x, theRect.y, theRect.width, theRect.height);
        theRect.Deflate(1);
    }
}

void wxAuiGenericTabArt::DrawBackground(wxDC& dc,
                                        wxWindow* WXUNUSED(wnd),
                                        const wxRect& rect)
{
    // draw background using arbitrary hard-coded, but at least adapted to dark
    // mode, gradient
    int topLightness, bottomLightness;
    if (wxSystemSettings::GetAppearance().IsUsingDarkBackground())
    {
        topLightness = 110;
        bottomLightness = 90;
    }
    else
    {
        topLightness = 90;
        bottomLightness = 170;
    }

    wxColor top_color    = m_baseColour.ChangeLightness(topLightness);
    wxColor bottom_color = m_baseColour.ChangeLightness(bottomLightness);
    wxRect r;

   if (m_flags &wxAUI_NB_BOTTOM)
       r = wxRect(rect.x, rect.y, rect.width+2, rect.height);
   // TODO: else if (m_flags &wxAUI_NB_LEFT) {}
   // TODO: else if (m_flags &wxAUI_NB_RIGHT) {}
   else //for wxAUI_NB_TOP
       r = wxRect(rect.x, rect.y, rect.width+2, rect.height-3);

    dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);


   // draw base lines

   dc.SetPen(m_borderPen);
   int y = rect.GetHeight();
   int w = rect.GetWidth();

   if (m_flags &wxAUI_NB_BOTTOM)
   {
       dc.SetBrush(wxBrush(bottom_color));
       dc.DrawRectangle(-1, 0, w+2, 4);
   }
   // TODO: else if (m_flags &wxAUI_NB_LEFT) {}
   // TODO: else if (m_flags &wxAUI_NB_RIGHT) {}
   else //for wxAUI_NB_TOP
   {
       dc.SetBrush(m_baseColourBrush);
       dc.DrawRectangle(-1, y-4, w+2, 4);
   }
}


const wxBitmapBundle*
wxAuiGenericTabArt::GetButtonBitmapBundle(const wxAuiTabContainerButton& button) const
{
    if (button.curState & wxAUI_BUTTON_STATE_HIDDEN)
        return nullptr;

    const auto active = button.curState & (wxAUI_BUTTON_STATE_HOVER |
                                           wxAUI_BUTTON_STATE_PRESSED);

    switch (button.id)
    {
        case wxAUI_BUTTON_CLOSE:
            return active ? &m_activeCloseBmp : &m_disabledCloseBmp;

        case wxAUI_BUTTON_PIN:
            return button.curState & wxAUI_BUTTON_STATE_CHECKED
                    ? (active ? &m_activeUnpinBmp : &m_disabledUnpinBmp)
                    : (active ? &m_activePinBmp : &m_disabledPinBmp);
    }

    return nullptr;
}

// DrawPageTab() draws the tab for the given page.
//
// It updates the page rectangle and returns the amount to advance by to the
// start of the next tab.

int wxAuiGenericTabArt::DrawPageTab(
                                 wxDC& dc,
                                 wxWindow* wnd,
                                 wxAuiNotebookPage& page,
                                 const wxRect& in_rect)
{
    wxCoord normal_textx, normal_texty;
    wxCoord selected_textx, selected_texty;
    wxCoord texty;

    // if the caption is empty, measure some temporary text
    wxString caption = page.caption;
    if (caption.empty())
        caption = wxT("Xj");

    dc.SetFont(m_selectedFont);
    dc.GetTextExtent(caption, &selected_textx, &selected_texty);

    dc.SetFont(m_normalFont);
    dc.GetTextExtent(caption, &normal_textx, &normal_texty);

    // figure out the size of the tab
    int xExtent = 0;
    wxSize tab_size = GetPageTabSize(dc, wnd, page, &xExtent);

    wxCoord tab_height = in_rect.height - 3;
    wxCoord tab_width = tab_size.x;
    wxCoord tab_x = in_rect.x;
    wxCoord tab_y = in_rect.y + in_rect.height - tab_height;


    caption = page.caption;


    // select pen, brush and font for the tab to be drawn

    if (page.active)
    {
        dc.SetFont(m_selectedFont);
        texty = selected_texty;
    }
    else
    {
        dc.SetFont(m_normalFont);
        texty = normal_texty;
    }


    // create points that will make the tab outline

    int clip_width = tab_width;
    if (tab_x + clip_width > in_rect.x + in_rect.width)
        clip_width = (in_rect.x + in_rect.width) - tab_x;

    wxDCClipper clip(dc, tab_x, tab_y, clip_width+1, tab_height-3);


    wxPoint border_points[6];
    if (m_flags &wxAUI_NB_BOTTOM)
    {
        border_points[0] = wxPoint(tab_x,             tab_y);
        border_points[1] = wxPoint(tab_x,             tab_y+tab_height-6);
        border_points[2] = wxPoint(tab_x+2,           tab_y+tab_height-4);
        border_points[3] = wxPoint(tab_x+tab_width-2, tab_y+tab_height-4);
        border_points[4] = wxPoint(tab_x+tab_width,   tab_y+tab_height-6);
        border_points[5] = wxPoint(tab_x+tab_width,   tab_y);
    }
    else //if (m_flags & wxAUI_NB_TOP) {}
    {
        border_points[0] = wxPoint(tab_x,             tab_y+tab_height-4);
        border_points[1] = wxPoint(tab_x,             tab_y+2);
        border_points[2] = wxPoint(tab_x+2,           tab_y);
        border_points[3] = wxPoint(tab_x+tab_width-2, tab_y);
        border_points[4] = wxPoint(tab_x+tab_width,   tab_y+2);
        border_points[5] = wxPoint(tab_x+tab_width,   tab_y+tab_height-4);
    }
    // TODO: else if (m_flags &wxAUI_NB_LEFT) {}
    // TODO: else if (m_flags &wxAUI_NB_RIGHT) {}

    int drawn_tab_yoff = border_points[1].y;
    int drawn_tab_height = border_points[0].y - border_points[1].y;

    bool isdark = wxSystemSettings::GetAppearance().IsUsingDarkBackground();

    wxColor back_color = m_baseColour;
    if (page.active)
    {
        // draw active tab

        // draw base background color
        wxRect r(tab_x, tab_y, tab_width, tab_height);
        dc.SetPen(wxPen(m_activeColour));
        dc.SetBrush(wxBrush(m_activeColour));
        dc.DrawRectangle(r.x+1, r.y+1, r.width-1, r.height-4);

        // this white helps fill out the gradient at the top of the tab
        wxColor gradient = *wxWHITE;
        if (isdark)
        {
            //dark mode, we go darker
            gradient = m_activeColour.ChangeLightness(70);
        }
        back_color = gradient;

        dc.SetPen(wxPen(gradient));
        dc.SetBrush(wxBrush(gradient));
        dc.DrawRectangle(r.x+2, r.y+1, r.width-3, r.height-4);

        // these two points help the rounded corners appear more antialiased
        dc.SetPen(wxPen(m_activeColour));
        dc.DrawPoint(r.x+2, r.y+1);
        dc.DrawPoint(r.x+r.width-2, r.y+1);

        // set rectangle down a bit for gradient drawing
        r.SetHeight(r.GetHeight()/2);
        r.x += 2;
        r.width -= 3;
        r.y += r.height;
        r.y -= 2;

        // draw gradient background
        wxColor top_color = gradient;
        wxColor bottom_color = m_activeColour;
        dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);
    }
    else
    {
        // draw inactive tab

        wxRect r(tab_x, tab_y+1, tab_width, tab_height-3);

        // start the gradient up a bit and leave the inside border inset
        // by a pixel for a 3D look.  Only the top half of the inactive
        // tab will have a slight gradient
        r.x += 3;
        r.y++;
        r.width -= 4;
        r.height /= 2;
        r.height--;

        // -- draw top gradient fill for glossy look
        wxColor top_color = m_baseColour;
        wxColor bottom_color = top_color.ChangeLightness(160);
        if (isdark)
        {
            //dark mode, we go darker
            top_color = m_activeColour.ChangeLightness(70);
            bottom_color = m_baseColour;
        }

        dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);

        r.y += r.height;
        r.y--;

        // -- draw bottom fill for glossy look
        top_color = m_baseColour;
        bottom_color = m_baseColour;
        dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);
    }

    // draw tab outline
    dc.SetPen(m_borderPen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawPolygon(WXSIZEOF(border_points), border_points);

    // there are two horizontal grey lines at the bottom of the tab control,
    // this gets rid of the top one of those lines in the tab control
    if (page.active)
    {
        if (m_flags &wxAUI_NB_BOTTOM)
            dc.SetPen(wxPen(m_baseColour.ChangeLightness(170)));
        // TODO: else if (m_flags &wxAUI_NB_LEFT) {}
        // TODO: else if (m_flags &wxAUI_NB_RIGHT) {}
        else //for wxAUI_NB_TOP
            dc.SetPen(m_baseColourPen);
        dc.DrawLine(border_points[0].x+1,
                    border_points[0].y,
                    border_points[5].x,
                    border_points[5].y);
    }


    int text_offset;
    int bitmap_offset = 0;
    if (page.bitmap.IsOk())
    {
        bitmap_offset = tab_x + wnd->FromDIP(8);

        const wxBitmap bitmap = page.bitmap.GetBitmapFor(wnd);

        // draw bitmap
        dc.DrawBitmap(bitmap,
                      bitmap_offset,
                      drawn_tab_yoff + (drawn_tab_height/2) - (bitmap.GetLogicalHeight()/2),
                      true);

        text_offset = bitmap_offset + bitmap.GetLogicalWidth();
        text_offset += wnd->FromDIP(3); // bitmap padding
    }
    else
    {
        text_offset = tab_x + wnd->FromDIP(8);
    }

    // draw buttons: start by computing their total width (note that we don't
    // use any padding between them currently because the current bitmaps don't
    // need it)
    int buttonsWidth = 0;
    for (auto& button : page.buttons)
    {
        const wxBitmapBundle* const bb = GetButtonBitmapBundle(button);
        if (!bb)
            continue;

        const wxBitmap bmp = bb->GetBitmapFor(wnd);
        buttonsWidth += bmp.GetLogicalWidth();
    }

    // now draw them
    int buttonX = tab_x + tab_width - buttonsWidth - wnd->FromDIP(1);
    for (auto& button : page.buttons)
    {
        const wxBitmapBundle* const bb = GetButtonBitmapBundle(button);
        if (!bb)
            continue;

        const wxBitmap bmp = bb->GetBitmapFor(wnd);

        int offsetY = tab_y-1;
        if (m_flags & wxAUI_NB_BOTTOM)
            offsetY = 1;

        wxRect rect(buttonX,
                    offsetY + (tab_height/2) - (bmp.GetLogicalHeight()/2),
                    bmp.GetLogicalWidth(),
                    tab_height);

        IndentPressedBitmap(wnd->FromDIP(wxSize(1, 1)), &rect, button.curState);
        dc.DrawBitmap(bmp, rect.x, rect.y, true);

        button.rect = rect;
        buttonX += bmp.GetLogicalWidth();
    }

    wxString draw_text = wxAuiChopText(dc,
                          caption,
                          tab_width - (text_offset-tab_x) - buttonsWidth);

    // draw tab text
    wxColor sys_color = wxSystemSettings::GetColour(
        page.active ? wxSYS_COLOUR_CAPTIONTEXT : wxSYS_COLOUR_INACTIVECAPTIONTEXT);
    wxColor font_color = wxAuiHasSufficientContrast(back_color, sys_color) ? sys_color
        : wxAuiGetBetterContrastColour(back_color, *wxWHITE, *wxBLACK);
    dc.SetTextForeground(font_color);
    dc.DrawText(draw_text,
                text_offset,
                drawn_tab_yoff + (drawn_tab_height)/2 - (texty/2) - 1);

    // draw focus rectangle except under macOS where it looks out of place
#ifndef __WXOSX__
    if (page.active && (wnd->FindFocus() == wnd))
    {
        wxRect focusRectText(text_offset, (drawn_tab_yoff + (drawn_tab_height)/2 - (texty/2) - 1),
            selected_textx, selected_texty);

        wxRect focusRect;
        wxRect focusRectBitmap;

        if (page.bitmap.IsOk())
        {
            const wxBitmap bitmap = page.bitmap.GetBitmapFor(wnd);

            focusRectBitmap = wxRect(bitmap_offset, drawn_tab_yoff + (drawn_tab_height/2) - (bitmap.GetLogicalHeight()/2),
                                     bitmap.GetLogicalWidth(), bitmap.GetLogicalHeight());
        }

        if (page.bitmap.IsOk() && draw_text.IsEmpty())
            focusRect = focusRectBitmap;
        else if (!page.bitmap.IsOk() && !draw_text.IsEmpty())
            focusRect = focusRectText;
        else if (page.bitmap.IsOk() && !draw_text.IsEmpty())
            focusRect = focusRectText.Union(focusRectBitmap);

        focusRect.Inflate(2, 2);

        wxRendererNative::Get().DrawFocusRect(wnd, dc, focusRect, 0);
    }
#endif // !__WXOSX__

    page.rect = wxRect(tab_x, tab_y, tab_width, tab_height);

    return xExtent;
}

int wxAuiGenericTabArt::GetIndentSize()
{
    return wxWindow::FromDIP(5, nullptr);
}

int wxAuiGenericTabArt::GetBorderWidth(wxWindow* wnd)
{
    wxAuiManager* mgr = wxAuiManager::GetManager(wnd);
    if (mgr)
    {
        wxAuiDockArt* art = mgr->GetArtProvider();
        if (art)
            return art->GetMetricForWindow(wxAUI_DOCKART_PANE_BORDER_SIZE, wnd);
    }
    return 1;
}

int wxAuiGenericTabArt::GetAdditionalBorderSpace(wxWindow* WXUNUSED(wnd))
{
    return 0;
}

wxSize wxAuiGenericTabArt::GetPageTabSize(
                                      wxReadOnlyDC& dc,
                                      wxWindow* wnd,
                                      const wxAuiNotebookPage& page,
                                      int* x_extent)
{
    wxCoord measured_textx, measured_texty, tmp;

    dc.SetFont(m_measuringFont);
    dc.GetTextExtent(page.caption, &measured_textx, &measured_texty);

    dc.GetTextExtent(wxT("ABCDEFXj"), &tmp, &measured_texty);

    // add padding around the text
    wxCoord tab_width = measured_textx;
    wxCoord tab_height = measured_texty;

    // add space for the buttons, if any
    for (const auto& button : page.buttons)
    {
        if (const wxBitmapBundle* bmp = GetButtonBitmapBundle(button))
        {
            tab_width += bmp->GetBitmapFor(wnd).GetLogicalWidth() + wnd->FromDIP(3);
        }
    }

    // if there's a bitmap, add space for it
    if (page.bitmap.IsOk())
    {
        // we need the correct size of the bitmap to be used on this window in
        // logical dimensions for drawing
        const wxSize bitmapSize = page.bitmap.GetPreferredLogicalSizeFor(wnd);

        // increase by bitmap plus right side bitmap padding
        tab_width += bitmapSize.x + wnd->FromDIP(3);
        tab_height = wxMax(tab_height, bitmapSize.y);
    }

    // add padding
    wxSize padding = wnd->FromDIP(wxSize(16, 10));
    tab_width  += padding.x;
    tab_height += padding.y;

    if (m_flags & wxAUI_NB_TAB_FIXED_WIDTH)
    {
        tab_width = m_fixedTabWidth;
    }

    if (x_extent)
        *x_extent = tab_width;

    return wxSize(tab_width, tab_height);
}


bool wxAuiGenericTabArt::DoGetButtonRectAndBitmap(wxWindow* wnd,
                                                  const wxRect& in_rect,
                                                  int bitmap_id,
                                                  int button_state,
                                                  int orientation,
                                                  wxRect* outRect,
                                                  wxBitmap* outBitmap)
{
    wxBitmapBundle bb;
    wxRect rect;

    switch (bitmap_id)
    {
        case wxAUI_BUTTON_CLOSE:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bb = m_disabledCloseBmp;
            else
                bb = m_activeCloseBmp;
            break;
        case wxAUI_BUTTON_LEFT:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bb = m_disabledLeftBmp;
            else
                bb = m_activeLeftBmp;
            break;
        case wxAUI_BUTTON_RIGHT:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bb = m_disabledRightBmp;
            else
                bb = m_activeRightBmp;
            break;
        case wxAUI_BUTTON_WINDOWLIST:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bb = m_disabledWindowListBmp;
            else
                bb = m_activeWindowListBmp;
            break;
    }


    if (!bb.IsOk())
        return false;

    const wxBitmap bmp = bb.GetBitmapFor(wnd);

    rect = in_rect;

    if (orientation == wxLEFT)
    {
        rect.SetX(in_rect.x);
        rect.SetY(((in_rect.y + in_rect.height)/2) - (bmp.GetLogicalHeight()/2));
        rect.SetSize(bmp.GetLogicalSize());
    }
    else
    {
        rect = wxRect(in_rect.x + in_rect.width - bmp.GetLogicalWidth(),
                      ((in_rect.y + in_rect.height)/2) - (bmp.GetLogicalHeight()/2),
                      bmp.GetLogicalWidth(), bmp.GetLogicalHeight());
    }

    IndentPressedBitmap(wnd->FromDIP(wxSize(1, 1)), &rect, button_state);

    if ( outRect )
        *outRect = rect;
    if ( outBitmap )
        *outBitmap = bmp;

    return true;
}

void wxAuiGenericTabArt::DrawButton(wxDC& dc,
                                    wxWindow* wnd,
                                    const wxRect& in_rect,
                                    int bitmap_id,
                                    int button_state,
                                    int orientation,
                                    wxRect* out_rect)
{
    wxRect rect;
    wxBitmap bmp;

    if ( !DoGetButtonRectAndBitmap(wnd, in_rect,
                                   bitmap_id, button_state, orientation,
                                   &rect, &bmp) )
        return;

    dc.DrawBitmap(bmp, rect.x, rect.y, true);

    *out_rect = rect;
}

int wxAuiGenericTabArt::GetButtonRect(wxReadOnlyDC& WXUNUSED(dc),
                                      wxWindow* wnd,
                                      const wxRect& inRect,
                                      int bitmapId,
                                      int buttonState,
                                      int orientation,
                                      wxRect* outRect)
{
    wxRect rect;
    DoGetButtonRectAndBitmap(wnd, inRect,
                             bitmapId, buttonState, orientation,
                             &rect);
    if ( outRect )
        *outRect = rect;

    return rect.width;
}

int wxAuiGenericTabArt::ShowDropDown(wxWindow* wnd,
                                     const wxAuiNotebookPageArray& pages,
                                     int /*active_idx*/)
{
    wxMenu menuPopup;

    size_t i, count = pages.GetCount();
    for (i = 0; i < count; ++i)
    {
        const wxAuiNotebookPage& page = pages.Item(i);

        // Preserve ampersands possibly present in the caption string by
        // escaping them before passing the caption to wxMenuItem.
        wxString caption = wxControl::EscapeMnemonics(page.caption);

        // if there is no caption, make it a space.  This will prevent
        // an assert in the menu code.
        if (caption.IsEmpty())
            caption = wxT(" ");

        wxMenuItem* item = new wxMenuItem(nullptr, 1000+i, caption);
        if (page.bitmap.IsOk())
            item->SetBitmap(page.bitmap.GetBitmapFor(wnd));
        menuPopup.Append(item);
    }

    // find out where to put the popup menu of window items
    wxPoint pt = ::wxGetMousePosition();
    pt = wnd->ScreenToClient(pt);

    // find out the screen coordinate at the bottom of the tab ctrl
    wxRect cli_rect = wnd->GetClientRect();
    pt.y = cli_rect.y + cli_rect.height;

    wxAuiCommandCapture* cc = new wxAuiCommandCapture;
    wnd->PushEventHandler(cc);
    wnd->PopupMenu(&menuPopup, pt);
    int command = cc->GetCommandId();
    wnd->PopEventHandler(true);

    if (command >= 1000)
        return command-1000;

    return -1;
}

int wxAuiGenericTabArt::GetBestTabCtrlSize(wxWindow* wnd,
                                           const wxAuiNotebookPageArray& pages,
                                           const wxSize& requiredBmp_size)
{
    wxInfoDC dc(wnd);
    dc.SetFont(m_measuringFont);

    // sometimes a standard bitmap size needs to be enforced, especially
    // if some tabs have bitmaps and others don't.  This is important because
    // it prevents the tab control from resizing when tabs are added.
    wxBitmap measureBmp;
    if (requiredBmp_size.IsFullySpecified())
    {
        measureBmp.Create(requiredBmp_size.x,
                           requiredBmp_size.y);
    }

    // we don't use the caption text because we don't
    // want tab heights to be different in the case
    // of a very short piece of text on one tab and a very
    // tall piece of text on another tab
    const wxString measureText(wxT("ABCDEFGHIj"));

    int max_y = 0;
    size_t i, page_count = pages.GetCount();
    for (i = 0; i < page_count; ++i)
    {
        // Make a copy of the page as we modify it below.
        wxAuiNotebookPage page = pages.Item(i);

        wxBitmapBundle bmp;
        if (measureBmp.IsOk())
            page.bitmap = measureBmp;

        page.caption = measureText;
        page.active = true;

        wxSize s = GetPageTabSize(dc, wnd, page);

        max_y = wxMax(max_y, s.y);
    }

    return max_y+2;
}

void wxAuiGenericTabArt::SetNormalFont(const wxFont& font)
{
    m_normalFont = font;
}

void wxAuiGenericTabArt::SetSelectedFont(const wxFont& font)
{
    m_selectedFont = font;
}

void wxAuiGenericTabArt::SetMeasuringFont(const wxFont& font)
{
    m_measuringFont = font;
}

void wxAuiGenericTabArt::SetColour(const wxColour& colour)
{
    m_baseColour = colour;
    m_borderPen = wxPen(m_baseColour.ChangeLightness(75));
    m_baseColourPen = wxPen(m_baseColour);
    m_baseColourBrush = wxBrush(m_baseColour);
}

void wxAuiGenericTabArt::SetActiveColour(const wxColour& colour)
{
    m_activeColour = colour;
}

wxFont wxAuiGenericTabArt::GetNormalFont() const
{
    return m_normalFont;
}

wxFont wxAuiGenericTabArt::GetSelectedFont() const
{
    return m_selectedFont;
}

// -- wxAuiSimpleTabArt class implementation --

wxAuiSimpleTabArt::wxAuiSimpleTabArt()
    : m_normalFont(*wxNORMAL_FONT)
    , m_selectedFont(m_normalFont)
    , m_activeCloseBmp(wxAuiCreateCloseButtonBitmap(*wxBLACK))
    , m_disabledCloseBmp(wxAuiCreateCloseButtonBitmap(wxColour(128,128,128)))
    , m_activeLeftBmp(wxAuiCreateBitmap(left_bitmap_data, 16, 16, *wxBLACK))
    , m_disabledLeftBmp(wxAuiCreateBitmap(left_bitmap_data, 16, 16, wxColour(128,128,128)))
    , m_activeRightBmp(wxAuiCreateBitmap(right_bitmap_data, 16, 16, *wxBLACK))
    , m_disabledRightBmp(wxAuiCreateBitmap(right_bitmap_data, 16, 16, wxColour(128,128,128)))
    , m_activeWindowListBmp(wxAuiCreateBitmap(list_bitmap_data, 16, 16, *wxBLACK))
    , m_disabledWindowListBmp(wxAuiCreateBitmap(list_bitmap_data, 16, 16, wxColour(128,128,128)))
{
    m_selectedFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_measuringFont = m_selectedFont;

    m_flags = 0;
    m_fixedTabWidth = wxWindow::FromDIP(100, nullptr);

    wxColour baseColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);

    wxColour backgroundColour = baseColour;
    wxColour normaltabColour = baseColour;
    wxColour selectedtabColour = *wxWHITE;

    m_bkBrush = wxBrush(backgroundColour);
    m_normalBkBrush = wxBrush(normaltabColour);
    m_normalBkPen = wxPen(normaltabColour);
    m_selectedBkBrush = wxBrush(selectedtabColour);
    m_selectedBkPen = wxPen(selectedtabColour);

}

wxAuiSimpleTabArt::~wxAuiSimpleTabArt()
{
}

wxAuiTabArt* wxAuiSimpleTabArt::Clone()
{
    return new wxAuiSimpleTabArt(*this);
}

void wxAuiSimpleTabArt::SetFlags(unsigned int flags)
{
    m_flags = flags;
}

void wxAuiSimpleTabArt::SetSizingInfo(const wxSize& tab_ctrl_size,
                                      size_t tab_count,
                                      wxWindow* wnd)
{
    if ( !wnd )
    {
        wnd = wxTheApp->GetTopWindow();
        wxCHECK_RET( wnd, "must have some window" );
    }

    m_fixedTabWidth = wnd->FromDIP(100);

    int tot_width = (int)tab_ctrl_size.x - GetIndentSize() - wnd->FromDIP(4);

    if (m_flags & wxAUI_NB_CLOSE_BUTTON)
        tot_width -= m_activeCloseBmp.GetBitmapFor(wnd).GetLogicalWidth();
    if (m_flags & wxAUI_NB_WINDOWLIST_BUTTON)
        tot_width -= m_activeWindowListBmp.GetBitmapFor(wnd).GetLogicalWidth();

    if (tab_count > 0)
    {
        m_fixedTabWidth = tot_width/(int)tab_count;
    }


    m_fixedTabWidth = wxMax(m_fixedTabWidth, wnd->FromDIP(100));

    if (m_fixedTabWidth > tot_width/2)
        m_fixedTabWidth = tot_width/2;

    m_fixedTabWidth = wxMin(m_fixedTabWidth, wnd->FromDIP(220));
}

void wxAuiSimpleTabArt::SetColour(const wxColour& colour)
{
    m_bkBrush = wxBrush(colour);
    m_normalBkBrush = wxBrush(colour);
    m_normalBkPen = wxPen(colour);
}

void wxAuiSimpleTabArt::SetActiveColour(const wxColour& colour)
{
    m_selectedBkBrush = wxBrush(colour);
    m_selectedBkPen = wxPen(colour);
}

void wxAuiSimpleTabArt::DrawBorder(wxDC& dc, wxWindow* wnd, const wxRect& rect)
{
    int i, border_width = GetBorderWidth(wnd);

    wxRect theRect(rect);
    for (i = 0; i < border_width; ++i)
    {
        dc.DrawRectangle(theRect.x, theRect.y, theRect.width, theRect.height);
        theRect.Deflate(1);
    }
}

void wxAuiSimpleTabArt::DrawBackground(wxDC& dc,
                                       wxWindow* WXUNUSED(wnd),
                                       const wxRect& rect)
{
    // draw background
    dc.SetBrush(m_bkBrush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(-1, -1, rect.GetWidth()+2, rect.GetHeight()+2);

    // draw base line
    dc.SetPen(*wxGREY_PEN);
    dc.DrawLine(0, rect.GetHeight()-1, rect.GetWidth(), rect.GetHeight()-1);
}


// DrawTab() draws an individual tab.
//
// dc       - output dc
// in_rect  - rectangle the tab should be confined to
// caption  - tab's caption
// active   - whether or not the tab is active
// out_rect - actual output rectangle
// x_extent - the advance x; where the next tab should start

void wxAuiSimpleTabArt::DrawTab(wxDC& dc,
                                wxWindow* wnd,
                                const wxAuiNotebookPage& page,
                                const wxRect& in_rect,
                                int close_button_state,
                                wxRect* out_tab_rect,
                                wxRect* out_button_rect,
                                int* x_extent)
{
    wxCoord normal_textx, normal_texty;
    wxCoord selected_textx, selected_texty;
    wxCoord textx, texty;

    // if the caption is empty, measure some temporary text
    wxString caption = page.caption;
    if (caption.empty())
        caption = wxT("Xj");

    dc.SetFont(m_selectedFont);
    dc.GetTextExtent(caption, &selected_textx, &selected_texty);

    dc.SetFont(m_normalFont);
    dc.GetTextExtent(caption, &normal_textx, &normal_texty);

    // figure out the size of the tab
    wxSize tab_size = GetTabSize(dc,
                                 wnd,
                                 page.caption,
                                 page.bitmap,
                                 page.active,
                                 close_button_state,
                                 x_extent);

    wxCoord tab_height = tab_size.y;
    wxCoord tab_width = tab_size.x;
    wxCoord tab_x = in_rect.x;
    wxCoord tab_y = in_rect.y + in_rect.height - tab_height;

    caption = page.caption;

    // select pen, brush and font for the tab to be drawn

    if (page.active)
    {
        dc.SetPen(m_selectedBkPen);
        dc.SetBrush(m_selectedBkBrush);
        dc.SetFont(m_selectedFont);
        textx = selected_textx;
        texty = selected_texty;
    }
    else
    {
        dc.SetPen(m_normalBkPen);
        dc.SetBrush(m_normalBkBrush);
        dc.SetFont(m_normalFont);
        textx = normal_textx;
        texty = normal_texty;
    }


    // -- draw line --

    wxPoint points[7];
    points[0].x = tab_x;
    points[0].y = tab_y + tab_height - 1;
    points[1].x = tab_x + tab_height - 3;
    points[1].y = tab_y + 2;
    points[2].x = tab_x + tab_height + 3;
    points[2].y = tab_y;
    points[3].x = tab_x + tab_width - 2;
    points[3].y = tab_y;
    points[4].x = tab_x + tab_width;
    points[4].y = tab_y + 2;
    points[5].x = tab_x + tab_width;
    points[5].y = tab_y + tab_height - 1;
    points[6] = points[0];

    wxDCClipper clip(dc, in_rect);

    dc.DrawPolygon(WXSIZEOF(points) - 1, points);

    dc.SetPen(*wxGREY_PEN);

    //dc.DrawLines(active ? WXSIZEOF(points) - 1 : WXSIZEOF(points), points);
    dc.DrawLines(WXSIZEOF(points), points);


    int text_offset;

    int close_button_width = 0;

    // draw close button if necessary
    if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
    {
        const wxBitmapBundle bb(page.active ? m_activeCloseBmp
                                            : m_disabledCloseBmp);

        const wxBitmap bmp = bb.GetBitmapFor(wnd);

        wxRect rect(tab_x + tab_width - bmp.GetLogicalWidth() - 1,
                    tab_y + (tab_height/2) - (bmp.GetLogicalHeight()/2) + 1,
                    bmp.GetLogicalWidth(),
                    tab_height - 1);
        DrawButtons(dc, wnd->FromDIP(wxSize(1, 1)), rect, bmp, *wxWHITE, close_button_state);

        *out_button_rect = rect;
        close_button_width = bmp.GetLogicalWidth();
    }

    text_offset = tab_x + (tab_height/2) + ((tab_width-close_button_width)/2) - (textx/2);

    // set minimum text offset
    if (text_offset < tab_x + tab_height)
        text_offset = tab_x + tab_height;

    // chop text if necessary
    wxString draw_text = wxAuiChopText(dc,
                          caption,
                          tab_width - (text_offset-tab_x) - close_button_width);

    // draw tab text
    wxColor back_color = dc.GetBrush().GetColour();
    wxColor sys_color = wxSystemSettings::GetColour(
        page.active ? wxSYS_COLOUR_CAPTIONTEXT : wxSYS_COLOUR_INACTIVECAPTIONTEXT);
    wxColor font_color = wxAuiHasSufficientContrast(back_color, sys_color) ? sys_color
        : wxAuiGetBetterContrastColour(back_color, *wxWHITE, *wxBLACK);
    dc.SetTextForeground(font_color);

    const auto textY = tab_y + (tab_height - texty) / 2 + 1;
    dc.DrawText(draw_text, text_offset, textY);


    // draw focus rectangle except under macOS where it looks out of place
#ifndef __WXOSX__
    if (page.active && (wnd->FindFocus() == wnd))
    {
        wxRect focusRect(text_offset, textY, selected_textx, selected_texty);

        focusRect.Inflate(2, 2);

        wxRendererNative::Get().DrawFocusRect(wnd, dc, focusRect, 0);
    }
#endif // !__WXOSX__

    *out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);
}

int wxAuiSimpleTabArt::GetIndentSize()
{
    return 0;
}

int wxAuiSimpleTabArt::GetBorderWidth(wxWindow* wnd)
{
    wxAuiManager* mgr = wxAuiManager::GetManager(wnd);
    if (mgr)
    {
       wxAuiDockArt*  art = mgr->GetArtProvider();
        if (art)
            return art->GetMetricForWindow(wxAUI_DOCKART_PANE_BORDER_SIZE, wnd);
    }
    return 1;
}

int wxAuiSimpleTabArt::GetAdditionalBorderSpace(wxWindow* WXUNUSED(wnd))
{
    return 0;
}

wxSize wxAuiSimpleTabArt::GetTabSize(wxReadOnlyDC& dc,
                                     wxWindow* wnd,
                                     const wxString& caption,
                                     const wxBitmapBundle& WXUNUSED(bitmap),
                                     bool WXUNUSED(active),
                                     int close_button_state,
                                     int* x_extent)
{
    wxCoord measured_textx, measured_texty;

    dc.SetFont(m_measuringFont);
    dc.GetTextExtent(caption, &measured_textx, &measured_texty);

    wxCoord tab_height = measured_texty + wnd->FromDIP(4);
    wxCoord tab_width = measured_textx + tab_height + wnd->FromDIP(5);

    if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
    {
        // increase by button size plus the padding
        tab_width += m_activeCloseBmp.GetBitmapFor(wnd).GetLogicalWidth() + wnd->FromDIP(3);
    }

    if (m_flags & wxAUI_NB_TAB_FIXED_WIDTH)
    {
        tab_width = m_fixedTabWidth;
    }

    *x_extent = tab_width - (tab_height/2) - 1;

    return wxSize(tab_width, tab_height);
}


bool wxAuiSimpleTabArt::DoGetButtonRectAndBitmap(wxWindow* wnd,
                                                 const wxRect& in_rect,
                                                 int bitmap_id,
                                                 int button_state,
                                                 int orientation,
                                                 wxRect* outRect,
                                                 wxBitmap* outBitmap)
{
    wxBitmapBundle bb;
    wxRect rect;

    switch (bitmap_id)
    {
        case wxAUI_BUTTON_CLOSE:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bb = m_disabledCloseBmp;
            else
                bb = m_activeCloseBmp;
            break;
        case wxAUI_BUTTON_LEFT:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bb = m_disabledLeftBmp;
            else
                bb = m_activeLeftBmp;
            break;
        case wxAUI_BUTTON_RIGHT:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bb = m_disabledRightBmp;
            else
                bb = m_activeRightBmp;
            break;
        case wxAUI_BUTTON_WINDOWLIST:
            if (button_state & wxAUI_BUTTON_STATE_DISABLED)
                bb = m_disabledWindowListBmp;
            else
                bb = m_activeWindowListBmp;
            break;
    }

    if (!bb.IsOk())
        return false;

    const wxBitmap bmp = bb.GetBitmapFor(wnd);

    rect = in_rect;

    if (orientation == wxLEFT)
    {
        rect.SetX(in_rect.x);
        rect.SetY(((in_rect.y + in_rect.height)/2) - (bmp.GetLogicalHeight()/2));
        rect.SetSize(bmp.GetLogicalSize());
    }
    else
    {
        rect = wxRect(in_rect.x + in_rect.width - bmp.GetLogicalWidth(),
                      ((in_rect.y + in_rect.height)/2) - (bmp.GetLogicalHeight()/2),
                      bmp.GetLogicalWidth(), bmp.GetLogicalHeight());
    }

    if ( outRect )
        *outRect = rect;
    if ( outBitmap )
        *outBitmap = bmp;

    return true;
}

void wxAuiSimpleTabArt::DrawButton(wxDC& dc,
                                   wxWindow* wnd,
                                   const wxRect& in_rect,
                                   int bitmap_id,
                                   int button_state,
                                   int orientation,
                                   wxRect* out_rect)
{
    wxRect rect;
    wxBitmap bmp;

    if ( !DoGetButtonRectAndBitmap(wnd, in_rect,
                                   bitmap_id, button_state, orientation,
                                   &rect, &bmp) )
        return;

    DrawButtons(dc, wnd->FromDIP(wxSize(1, 1)), rect, bmp, *wxWHITE, button_state);

    *out_rect = rect;
}

int wxAuiSimpleTabArt::GetButtonRect(wxReadOnlyDC& WXUNUSED(dc),
                                     wxWindow* wnd,
                                     const wxRect& inRect,
                                     int bitmapId,
                                     int buttonState,
                                     int orientation,
                                     wxRect* outRect)
{
    wxRect rect;
    DoGetButtonRectAndBitmap(wnd, inRect,
                             bitmapId, buttonState, orientation,
                             &rect);
    if ( outRect )
        *outRect = rect;

    return rect.width;
}

int wxAuiSimpleTabArt::ShowDropDown(wxWindow* wnd,
                                    const wxAuiNotebookPageArray& pages,
                                    int active_idx)
{
    wxMenu menuPopup;

    size_t i, count = pages.GetCount();
    for (i = 0; i < count; ++i)
    {
        const wxAuiNotebookPage& page = pages.Item(i);
        menuPopup.AppendCheckItem(1000+i, page.caption);
    }

    if (active_idx != -1)
    {
        menuPopup.Check(1000+active_idx, true);
    }

    // find out where to put the popup menu of window
    // items.  Subtract 100 for now to center the menu
    // a bit, until a better mechanism can be implemented
    int offset = wnd->FromDIP(100);
    wxPoint pt = ::wxGetMousePosition();
    pt = wnd->ScreenToClient(pt);
    if (pt.x < offset)
        pt.x = 0;
    else
        pt.x -= offset;

    // find out the screen coordinate at the bottom of the tab ctrl
    wxRect cli_rect = wnd->GetClientRect();
    pt.y = cli_rect.y + cli_rect.height;

    wxAuiCommandCapture* cc = new wxAuiCommandCapture;
    wnd->PushEventHandler(cc);
    wnd->PopupMenu(&menuPopup, pt);
    int command = cc->GetCommandId();
    wnd->PopEventHandler(true);

    if (command >= 1000)
        return command-1000;

    return -1;
}

int wxAuiSimpleTabArt::GetBestTabCtrlSize(wxWindow* wnd,
                                          const wxAuiNotebookPageArray& WXUNUSED(pages),
                                          const wxSize& WXUNUSED(requiredBmp_size))
{
    wxInfoDC dc(wnd);
    dc.SetFont(m_measuringFont);
    int x_ext = 0;
    wxSize s = GetTabSize(dc,
                          wnd,
                          wxT("ABCDEFGHIj"),
                          wxNullBitmap,
                          true,
                          wxAUI_BUTTON_STATE_HIDDEN,
                          &x_ext);
    return s.y+3;
}

void wxAuiSimpleTabArt::SetNormalFont(const wxFont& font)
{
    m_normalFont = font;
}

void wxAuiSimpleTabArt::SetSelectedFont(const wxFont& font)
{
    m_selectedFont = font;
}

void wxAuiSimpleTabArt::SetMeasuringFont(const wxFont& font)
{
    m_measuringFont = font;
}

wxFont wxAuiSimpleTabArt::GetNormalFont() const
{
    return m_normalFont;
}

wxFont wxAuiSimpleTabArt::GetSelectedFont() const
{
    return m_selectedFont;
}

#endif // wxUSE_AUI
