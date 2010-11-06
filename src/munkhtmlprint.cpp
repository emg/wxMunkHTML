/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/htmprint.cpp
// Purpose:     html printing classes
// Author:      Vaclav Slavik
// Created:     25/09/99
// RCS-ID:      $Id: htmprint.cpp 55069 2008-08-12 16:02:31Z VS $
// Copyright:   (c) Vaclav Slavik, 1999
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif


#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/dc.h"
    #include "wx/settings.h"
    #include "wx/msgdlg.h"
    #include "wx/module.h"
#endif

#include "wx/print.h"
#include "wx/printdlg.h"
#include "munkhtmlprint.h"
#include "munkhtml.h"
#include "wx/wfstream.h"
//#include "stringfunc.h"
//#include <wxutil_emdros.h>


// default font size of normal text (HTML font size 0) for printing, in points:
#define DEFAULT_PRINT_FONT_SIZE   12


//--------------------------------------------------------------------------------
// MunkHtmlDCRenderer
//--------------------------------------------------------------------------------


MunkHtmlDCRenderer::MunkHtmlDCRenderer() : wxObject()
{
    m_DC = NULL;
    m_Width = m_Height = 0;
    m_Cells = NULL;
    m_FS = new wxFileSystem();
    m_Parser = new MunkHtmlParsingStructure(NULL);
    m_Parser->SetHTMLBackgroundColour(*wxWHITE);
    m_Parser->SetFS(m_FS);
    SetStandardFonts(DEFAULT_PRINT_FONT_SIZE);
}



MunkHtmlDCRenderer::~MunkHtmlDCRenderer()
{
    if (m_Cells) delete m_Cells;
    if (m_Parser) delete m_Parser;
    if (m_FS) delete m_FS;
}



void MunkHtmlDCRenderer::SetDC(wxDC *dc, double pixel_scale)
{
    m_DC = dc;
    m_Parser->SetDC(m_DC, pixel_scale);
}



void MunkHtmlDCRenderer::SetSize(int width, int height)
{
    m_Width = width;
    m_Height = height;
}


void MunkHtmlDCRenderer::SetHtmlText(const wxString& html, const wxString& basepath, bool isdir)
{
	if (m_DC == NULL) return;
	
	if (m_Cells != NULL) delete m_Cells;
	
	m_FS->ChangePathTo(basepath, isdir);
	
	bool bResult = true;
	std::string error_message = "";
	bResult = m_Parser->Parse(html, error_message);
	if (!bResult) {
		wxMessageBox(wxString(wxT("An error occurred during the parsing of the HTML.\n")) + wxString(error_message.c_str(), wxConvUTF8));
	} else {
		m_Cells = m_Parser->GetInternalRepresentation();
		m_Parser->SetTopCell(0); // Make sure we don't delete the cells in the ps destructor
		m_Cells->SetIndent(0, MunkHTML_INDENT_ALL, MunkHTML_UNITS_PIXELS);
		m_Cells->Layout(m_Width);
	}
}


void MunkHtmlDCRenderer::SetFonts(const wxString& normal_face, const wxString& fixed_face,
                                const int *sizes)
{
	// m_Parser->SetFonts(normal_face, fixed_face, sizes);
	if (m_DC == NULL && m_Cells != NULL)
		m_Cells->Layout(m_Width);
}

void MunkHtmlDCRenderer::SetStandardFonts(int size,
                                        const wxString& normal_face,
                                        const wxString& fixed_face)
{
	// m_Parser->SetStandardFonts(size, normal_face, fixed_face);
	if (m_DC == NULL && m_Cells != NULL)
		m_Cells->Layout(m_Width);
}

int MunkHtmlDCRenderer::Render(int x, int y,
                             wxArrayInt& known_pagebreaks,
                             int from, int dont_render, int to)
{
    int pbreak, hght;

    if (m_Cells == NULL || m_DC == NULL) return 0;

    pbreak = (int)(from + m_Height);

    while (m_Cells->AdjustPagebreak(&pbreak, m_Height, known_pagebreaks)) {}

    hght = pbreak - from;
    if(to < hght)
        hght = to;

    if (!dont_render)
    {
	    MunkHtmlRenderingInfo rinfo(wxNullColour);
        MunkDefaultHtmlRenderingStyle rstyle;
        rinfo.SetStyle(&rstyle);
        m_DC->SetBrush(*wxWHITE_BRUSH);
        m_DC->SetClippingRegion(x, y, m_Width, hght);
        m_Cells->Draw(*m_DC,
                      x, (y - from),
                      y, y + hght,
                      rinfo);
        m_DC->DestroyClippingRegion();
    }

    if (pbreak < m_Cells->GetHeight()) return pbreak;
    else return GetTotalHeight();
}


int MunkHtmlDCRenderer::GetTotalHeight()
{
    if (m_Cells) return m_Cells->GetHeight();
    else return 0;
}


//--------------------------------------------------------------------------------
// MunkHtmlPrintout
//--------------------------------------------------------------------------------


wxList MunkHtmlPrintout::m_Filters;

MunkHtmlPrintout::MunkHtmlPrintout(const wxString& title) : wxPrintout(title)
{
    m_Renderer = new MunkHtmlDCRenderer;
    m_RendererHdr = new MunkHtmlDCRenderer;
    m_NumPages = MunkHTML_PRINT_MAX_PAGES;
    m_Document = m_BasePath = wxEmptyString; m_BasePathIsDir = true;
    m_Headers[0] = m_Headers[1] = wxEmptyString;
    m_Footers[0] = m_Footers[1] = wxEmptyString;
    m_HeaderHeight = m_FooterHeight = 0;
    SetMargins(); // to default values
    SetStandardFonts(DEFAULT_PRINT_FONT_SIZE);
}



MunkHtmlPrintout::~MunkHtmlPrintout()
{
    delete m_Renderer;
    delete m_RendererHdr;
}

void MunkHtmlPrintout::CleanUpStatics()
{
    WX_CLEAR_LIST(wxList, m_Filters);
}

// Adds input filter
/*
void MunkHtmlPrintout::AddFilter(MunkHtmlFilter *filter)
{
    m_Filters.Append(filter);
}
*/

void MunkHtmlPrintout::OnPreparePrinting()
{
    int pageWidth, pageHeight, mm_w, mm_h, scr_w, scr_h, dc_w, dc_h;
    float ppmm_h, ppmm_v;

    GetPageSizePixels(&pageWidth, &pageHeight);
    GetPageSizeMM(&mm_w, &mm_h);
    ppmm_h = (float)pageWidth / mm_w;
    ppmm_v = (float)pageHeight / mm_h;

    int ppiPrinterX, ppiPrinterY;
    GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
    int ppiScreenX, ppiScreenY;
    GetPPIScreen(&ppiScreenX, &ppiScreenY);

    wxDisplaySize(&scr_w, &scr_h);
    GetDC()->GetSize(&dc_w, &dc_h);

    GetDC()->SetUserScale((double)dc_w / (double)pageWidth,
                          (double)dc_h / (double)pageHeight);

    /* prepare headers/footers renderer: */

    m_RendererHdr->SetDC(GetDC(), (double)ppiPrinterY / (double)ppiScreenY);
    m_RendererHdr->SetSize((int) (ppmm_h * (mm_w - m_MarginLeft - m_MarginRight)),
                          (int) (ppmm_v * (mm_h - m_MarginTop - m_MarginBottom)));
    if (m_Headers[0] != wxEmptyString)
    {
        m_RendererHdr->SetHtmlText(TranslateHeader(m_Headers[0], 1));
        m_HeaderHeight = m_RendererHdr->GetTotalHeight();
    }
    else if (m_Headers[1] != wxEmptyString)
    {
        m_RendererHdr->SetHtmlText(TranslateHeader(m_Headers[1], 1));
        m_HeaderHeight = m_RendererHdr->GetTotalHeight();
    }
    if (m_Footers[0] != wxEmptyString)
    {
        m_RendererHdr->SetHtmlText(TranslateHeader(m_Footers[0], 1));
        m_FooterHeight = m_RendererHdr->GetTotalHeight();
    }
    else if (m_Footers[1] != wxEmptyString)
    {
        m_RendererHdr->SetHtmlText(TranslateHeader(m_Footers[1], 1));
        m_FooterHeight = m_RendererHdr->GetTotalHeight();
    }

    /* prepare main renderer: */
    m_Renderer->SetDC(GetDC(), (double)ppiPrinterY / (double)ppiScreenY);
    m_Renderer->SetSize((int) (ppmm_h * (mm_w - m_MarginLeft - m_MarginRight)),
                          (int) (ppmm_v * (mm_h - m_MarginTop - m_MarginBottom) -
                          m_FooterHeight - m_HeaderHeight -
                          ((m_HeaderHeight == 0) ? 0 : m_MarginSpace * ppmm_v) -
                          ((m_FooterHeight == 0) ? 0 : m_MarginSpace * ppmm_v)
                          ));
    m_Renderer->SetHtmlText(m_Document, m_BasePath, m_BasePathIsDir);
    CountPages();
}

bool MunkHtmlPrintout::OnBeginDocument(int startPage, int endPage)
{
    if (!wxPrintout::OnBeginDocument(startPage, endPage)) return false;

    return true;
}


bool MunkHtmlPrintout::OnPrintPage(int page)
{
    wxDC *dc = GetDC();
    if (dc && dc->IsOk())
    {
        if (HasPage(page))
            RenderPage(dc, page);
        return true;
    }
    else return false;
}


void MunkHtmlPrintout::GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo)
{
    *minPage = 1;
    if ( m_NumPages >= (signed)m_PageBreaks.Count()-1)
        *maxPage = m_NumPages;
    else
        *maxPage = (signed)m_PageBreaks.Count()-1;
    *selPageFrom = 1;
    *selPageTo = (signed)m_PageBreaks.Count()-1;
}



bool MunkHtmlPrintout::HasPage(int pageNum)
{
    return pageNum > 0 && (unsigned)pageNum < m_PageBreaks.Count();
}



void MunkHtmlPrintout::SetHtmlText(const wxString& html, const wxString &basepath, bool isdir)
{
    m_Document = html;
    m_BasePath = basepath;
    m_BasePathIsDir = isdir;
}

void MunkHtmlPrintout::SetHtmlFile(const wxString& htmlfile)
{
    wxFileSystem fs;
    wxFSFile *ff;

    ff = fs.OpenFile(htmlfile);

    if (ff == NULL) {
        wxLogError(htmlfile + _(": file does not exist!"));
        return;
    }

    bool done = false;
    //MunkHtmlFilterHTML defaultFilter;

    wxInputStream *s = ff->GetStream();
    wxString doc;
		
    if (s == NULL) {
	    wxLogError(_("Cannot open HTML document: %s"), ff->GetLocation().c_str());
	    return;
    }
    
    ReadString(doc, s, wxConvUTF8); // From munkhtml.cpp.  Assume UTF-8
    
    SetHtmlText(doc, htmlfile, false);

    delete ff;
}




void MunkHtmlPrintout::SetHeader(const wxString& header, int pg)
{
    if (pg == MunkHtmlPAGE_ALL || pg == MunkHtmlPAGE_EVEN)
        m_Headers[0] = header;
    if (pg == MunkHtmlPAGE_ALL || pg == MunkHtmlPAGE_ODD)
        m_Headers[1] = header;
}



void MunkHtmlPrintout::SetFooter(const wxString& footer, int pg)
{
    if (pg == MunkHtmlPAGE_ALL || pg == MunkHtmlPAGE_EVEN)
        m_Footers[0] = footer;
    if (pg == MunkHtmlPAGE_ALL || pg == MunkHtmlPAGE_ODD)
        m_Footers[1] = footer;
}



void MunkHtmlPrintout::CountPages()
{
    wxBusyCursor wait;
    int pageWidth, pageHeight, mm_w, mm_h;
    float ppmm_h, ppmm_v;

    GetPageSizePixels(&pageWidth, &pageHeight);
    GetPageSizeMM(&mm_w, &mm_h);
    ppmm_h = (float)pageWidth / mm_w;
    ppmm_v = (float)pageHeight / mm_h;

    int pos = 0;
    m_NumPages = 0;
    // m_PageBreaks[0] = 0;

    m_PageBreaks.Clear();
    m_PageBreaks.Add( 0);
    do
    {
        pos = m_Renderer->Render((int)( ppmm_h * m_MarginLeft),
                                 (int) (ppmm_v * (m_MarginTop + (m_HeaderHeight == 0 ? 0 : m_MarginSpace)) + m_HeaderHeight),
                                 m_PageBreaks,
                                 pos, true, INT_MAX);
        m_PageBreaks.Add( pos);
        if( m_PageBreaks.Count() > MunkHTML_PRINT_MAX_PAGES)
        {
            wxMessageBox( _("HTML pagination algorithm generated more than the allowed maximum number of pages and it can't continue any longer!"),
            _("Warning"), wxCANCEL | wxICON_ERROR );
            break;
        }
    } while (pos < m_Renderer->GetTotalHeight());
}



void MunkHtmlPrintout::RenderPage(wxDC *dc, int page)
{
    wxBusyCursor wait;

    int pageWidth, pageHeight, mm_w, mm_h, scr_w, scr_h, dc_w, dc_h;
    float ppmm_h, ppmm_v;

    GetPageSizePixels(&pageWidth, &pageHeight);
    GetPageSizeMM(&mm_w, &mm_h);
    ppmm_h = (float)pageWidth / mm_w;
    ppmm_v = (float)pageHeight / mm_h;
    wxDisplaySize(&scr_w, &scr_h);
    dc->GetSize(&dc_w, &dc_h);

    int ppiPrinterX, ppiPrinterY;
    GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
    wxUnusedVar(ppiPrinterX);
    int ppiScreenX, ppiScreenY;
    GetPPIScreen(&ppiScreenX, &ppiScreenY);
    wxUnusedVar(ppiScreenX);

    dc->SetUserScale((double)dc_w / (double)pageWidth,
                     (double)dc_h / (double)pageHeight);

    m_Renderer->SetDC(dc, (double)ppiPrinterY / (double)ppiScreenY);

    dc->SetBackgroundMode(wxTRANSPARENT);

    m_Renderer->Render((int) (ppmm_h * m_MarginLeft),
                         (int) (ppmm_v * (m_MarginTop + (m_HeaderHeight == 0 ? 0 : m_MarginSpace)) + m_HeaderHeight), m_PageBreaks,
                         m_PageBreaks[page-1], false, m_PageBreaks[page]-m_PageBreaks[page-1]);


    m_RendererHdr->SetDC(dc, (double)ppiPrinterY / (double)ppiScreenY);
    if (m_Headers[page % 2] != wxEmptyString)
    {
        m_RendererHdr->SetHtmlText(TranslateHeader(m_Headers[page % 2], page));
        m_RendererHdr->Render((int) (ppmm_h * m_MarginLeft), (int) (ppmm_v * m_MarginTop), m_PageBreaks);
    }
    if (m_Footers[page % 2] != wxEmptyString)
    {
        m_RendererHdr->SetHtmlText(TranslateHeader(m_Footers[page % 2], page));
        m_RendererHdr->Render((int) (ppmm_h * m_MarginLeft), (int) (pageHeight - ppmm_v * m_MarginBottom - m_FooterHeight), m_PageBreaks);
    }
}



wxString MunkHtmlPrintout::TranslateHeader(const wxString& instr, int page)
{
    wxString r = instr;
    wxString num;

    num.Printf(wxT("%i"), page);
    r.Replace(wxT("@PAGENUM@"), num);

    num.Printf(wxT("%lu"), (unsigned long)(m_PageBreaks.Count() - 1));
    r.Replace(wxT("@PAGESCNT@"), num);

    const wxDateTime now = wxDateTime::Now();
    r.Replace(wxT("@DATE@"), now.FormatDate());
    r.Replace(wxT("@TIME@"), now.FormatTime());

    r.Replace(wxT("@TITLE@"), GetTitle());

    return r;
}



void MunkHtmlPrintout::SetMargins(float top, float bottom, float left, float right, float spaces)
{
    m_MarginTop = top;
    m_MarginBottom = bottom;
    m_MarginLeft = left;
    m_MarginRight = right;
    m_MarginSpace = spaces;
}




void MunkHtmlPrintout::SetFonts(const wxString& normal_face, const wxString& fixed_face,
                              const int *sizes)
{
    m_Renderer->SetFonts(normal_face, fixed_face, sizes);
    m_RendererHdr->SetFonts(normal_face, fixed_face, sizes);
}

void MunkHtmlPrintout::SetStandardFonts(int size,
                                      const wxString& normal_face,
                                      const wxString& fixed_face)
{
    m_Renderer->SetStandardFonts(size, normal_face, fixed_face);
    m_RendererHdr->SetStandardFonts(size, normal_face, fixed_face);
}



//----------------------------------------------------------------------------
// MunkHtmlEasyPrinting
//----------------------------------------------------------------------------


MunkHtmlEasyPrinting::MunkHtmlEasyPrinting(const wxString& name, wxWindow *parentWindow)
{
    m_ParentWindow = parentWindow;
    m_Name = name;
    m_PrintData = NULL;
    m_PageSetupData = new wxPageSetupDialogData;
    m_Headers[0] = m_Headers[1] = m_Footers[0] = m_Footers[1] = wxEmptyString;

    m_PageSetupData->EnableMargins(true);
    m_PageSetupData->SetMarginTopLeft(wxPoint(25, 25));
    m_PageSetupData->SetMarginBottomRight(wxPoint(25, 25));

    SetStandardFonts(DEFAULT_PRINT_FONT_SIZE);
}



MunkHtmlEasyPrinting::~MunkHtmlEasyPrinting()
{
    delete m_PrintData;
    delete m_PageSetupData;
}


wxPrintData *MunkHtmlEasyPrinting::GetPrintData()
{
    if (m_PrintData == NULL)
        m_PrintData = new wxPrintData();
    return m_PrintData;
}


bool MunkHtmlEasyPrinting::PreviewFile(const wxString &htmlfile)
{
    MunkHtmlPrintout *p1 = CreatePrintout();
    p1->SetHtmlFile(htmlfile);
    MunkHtmlPrintout *p2 = CreatePrintout();
    p2->SetHtmlFile(htmlfile);
    return DoPreview(p1, p2);
}



bool MunkHtmlEasyPrinting::PreviewText(const wxString &htmltext, const wxString &basepath)
{
    MunkHtmlPrintout *p1 = CreatePrintout();
    p1->SetHtmlText(htmltext, basepath, true);
    MunkHtmlPrintout *p2 = CreatePrintout();
    p2->SetHtmlText(htmltext, basepath, true);
    return DoPreview(p1, p2);
}



bool MunkHtmlEasyPrinting::PrintFile(const wxString &htmlfile)
{
    MunkHtmlPrintout *p = CreatePrintout();
    p->SetHtmlFile(htmlfile);
    bool ret = DoPrint(p);
    delete p;
    return ret;
}



bool MunkHtmlEasyPrinting::PrintText(const wxString &htmltext, const wxString &basepath)
{
    MunkHtmlPrintout *p = CreatePrintout();
    p->SetHtmlText(htmltext, basepath, true);
    bool ret = DoPrint(p);
    delete p;
    return ret;
}



bool MunkHtmlEasyPrinting::DoPreview(MunkHtmlPrintout *printout1, MunkHtmlPrintout *printout2)
{
    // Pass two printout objects: for preview, and possible printing.
    wxPrintDialogData printDialogData(*GetPrintData());
    wxPrintPreview *preview = new wxPrintPreview(printout1, printout2, &printDialogData);
    if (!preview->Ok())
    {
        delete preview;
        return false;
    }

    wxPreviewFrame *frame = new wxPreviewFrame(preview, m_ParentWindow,
                                               m_Name + _(" Preview"),
                                               wxPoint(100, 100), wxSize(650, 500));
    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);
    return true;
}



bool MunkHtmlEasyPrinting::DoPrint(MunkHtmlPrintout *printout)
{
    wxPrintDialogData printDialogData(*GetPrintData());
    wxPrinter printer(&printDialogData);

    if (!printer.Print(m_ParentWindow, printout, true))
    {
        return false;
    }

    (*GetPrintData()) = printer.GetPrintDialogData().GetPrintData();
    return true;
}




void MunkHtmlEasyPrinting::PageSetup()
{
    if (!GetPrintData()->Ok())
    {
        wxLogError(_("There was a problem during page setup: you may need to set a default printer."));
        return;
    }

    m_PageSetupData->SetPrintData(*GetPrintData());
    wxPageSetupDialog pageSetupDialog(m_ParentWindow, m_PageSetupData);

    if (pageSetupDialog.ShowModal() == wxID_OK)
    {
        (*GetPrintData()) = pageSetupDialog.GetPageSetupData().GetPrintData();
        (*m_PageSetupData) = pageSetupDialog.GetPageSetupData();
    }
}



void MunkHtmlEasyPrinting::SetHeader(const wxString& header, int pg)
{
    if (pg == MunkHtmlPAGE_ALL || pg == MunkHtmlPAGE_EVEN)
        m_Headers[0] = header;
    if (pg == MunkHtmlPAGE_ALL || pg == MunkHtmlPAGE_ODD)
        m_Headers[1] = header;
}



void MunkHtmlEasyPrinting::SetFooter(const wxString& footer, int pg)
{
    if (pg == MunkHtmlPAGE_ALL || pg == MunkHtmlPAGE_EVEN)
        m_Footers[0] = footer;
    if (pg == MunkHtmlPAGE_ALL || pg == MunkHtmlPAGE_ODD)
        m_Footers[1] = footer;
}


void MunkHtmlEasyPrinting::SetFonts(const wxString& normal_face, const wxString& fixed_face,
                                  const int *sizes)
{
    m_fontMode = FontMode_Explicit;
    m_FontFaceNormal = normal_face;
    m_FontFaceFixed = fixed_face;

    if (sizes)
    {
        m_FontsSizes = m_FontsSizesArr;
        for (int i = 0; i < 7; i++) m_FontsSizes[i] = sizes[i];
    }
    else
        m_FontsSizes = NULL;
}

void MunkHtmlEasyPrinting::SetStandardFonts(int size,
                                          const wxString& normal_face,
                                          const wxString& fixed_face)
{
    m_fontMode = FontMode_Standard;
    m_FontFaceNormal = normal_face;
    m_FontFaceFixed = fixed_face;
    m_FontsSizesArr[0] = size;
}


MunkHtmlPrintout *MunkHtmlEasyPrinting::CreatePrintout()
{
    MunkHtmlPrintout *p = new MunkHtmlPrintout(m_Name);

    if (m_fontMode == FontMode_Explicit)
    {
        p->SetFonts(m_FontFaceNormal, m_FontFaceFixed, m_FontsSizes);
    }
    else // FontMode_Standard
    {
        p->SetStandardFonts(m_FontsSizesArr[0],
                            m_FontFaceNormal, m_FontFaceFixed);
    }

    p->SetHeader(m_Headers[0], MunkHtmlPAGE_EVEN);
    p->SetHeader(m_Headers[1], MunkHtmlPAGE_ODD);
    p->SetFooter(m_Footers[0], MunkHtmlPAGE_EVEN);
    p->SetFooter(m_Footers[1], MunkHtmlPAGE_ODD);

    p->SetMargins(m_PageSetupData->GetMarginTopLeft().y,
                    m_PageSetupData->GetMarginBottomRight().y,
                    m_PageSetupData->GetMarginTopLeft().x,
                    m_PageSetupData->GetMarginBottomRight().x);

    return p;
}

// A module to allow initialization/cleanup
// without calling these functions from app.cpp or from
// the user's application.

class MunkHtmlPrintingModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(MunkHtmlPrintingModule)
public:
    MunkHtmlPrintingModule() : wxModule() {}
    bool OnInit() { return true; }
    void OnExit() { MunkHtmlPrintout::CleanUpStatics(); }
};

IMPLEMENT_DYNAMIC_CLASS(MunkHtmlPrintingModule, wxModule)


