/////////////////////////////////////////////////////////////////////////////
// Name:        htmldefs.h
// Purpose:     constants for wxhtml library
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmldefs.h 40823 2006-08-25 16:52:58Z VZ $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#ifndef _MUNK_HTML_H_
#define _MUNK_HTML_H_

#include <iostream>


#include <istream>
#include <string>
#include <map>
#include <list>
#include <stack>
#include <sstream>

/*
#include <exception_emdros.h>
#include <string_func.h>
#include <llist.h>
*/

///////////////////////////////////////////////////////////////
//
// MunkException
//
///////////////////////////////////////////////////////////////



class MunkException {
private:
  std::string m_message; /**< The exception message. */
public:
  /** Constructor: Give a default exception message. 
   */
  MunkException() : m_message("An exception occurred.") {};

  /** Constructor: Set the exception message.
   *
   * @param message the message to use.
   */
  MunkException(const std::string& message) : m_message(message) {};

  /** Destructor. */
  ~MunkException() {};

  /** Return the message.
   *
   * @return the message in the exception.
   */
  std::string what() { return m_message; };
};

#define MUNK_ASSERT_THROW(A, B) { if (!(A)) { throw MunkException(B); } }


///////////////////////////////////////////////////////////////
//
// Munk QD XML Handler
//
///////////////////////////////////////////////////////////////




class MunkQDException : public MunkException {
public:
	MunkQDException(const std::string& msg) : MunkException(msg) {};
	~MunkQDException() {};
};


typedef std::map<std::string, std::string> MunkAttributeMap;


extern std::string getMunkAttribute(const MunkAttributeMap& attrs, const std::string& key) throw(MunkQDException);



class MunkQDDocHandler {
 public:
	MunkQDDocHandler() {};
	virtual ~MunkQDDocHandler() {};
	virtual void startElement(const std::string& tag, const MunkAttributeMap& attrs) throw(MunkQDException) {};
	virtual void endElement(const std::string& tag) throw(MunkQDException) {};
	virtual void startDocument(void) throw(MunkQDException) {};
	virtual void endDocument(void) throw(MunkQDException) {};
	virtual void text(const std::string& str) throw(MunkQDException) {};
	virtual void comment(const std::string& str) throw(MunkQDException) {};
};


enum eMunkCharsets {
  kMCSASCII,       /**< 7-bit ASCII.  */
  kMCSISO_8859_1,  /**< ISO-8859-1. */
  kMCSISO_8859_8,  /**< ISO-8869-8. */
  kMCSUTF8,        /**< UTF-8. */
  kMCSISO_8859_2,  /**< ISO-8859-2.  */
  kMCSISO_8859_3,  /**< ISO-8859-3.  */
  kMCSISO_8859_4,  /**< ISO-8859-4.  */
  kMCSISO_8859_5,  /**< ISO-8859-5.  */
  kMCSISO_8859_6,  /**< ISO-8859-6.  */
  kMCSISO_8859_7,  /**< ISO-8859-7.  */
  kMCSISO_8859_9,  /**< ISO-8859-9.  */
  kMCSISO_8859_10,  /**< ISO-8859-10.  */
  kMCSISO_8859_13,  /**< ISO-8859-13.  */
  kMCSISO_8859_14,  /**< ISO-8859-14.  */
  kMCSISO_8859_15,  /**< ISO-8859-15.  */
  kMCSISO_8859_16   /**< ISO-8859-16. */
};



class MunkQDParser {
 protected:
	const static std::string NOTAG;
	eMunkCharsets m_encoding;
	int m_line;
	int m_column;
	int m_tag_depth;
	MunkAttributeMap m_attributes;
	std::istream *m_pInStream;
	MunkQDDocHandler *m_pDH;
	std::string m_tag_name;
	std::string m_lvalue;
	std::string m_rvalue;
	bool m_end_of_line;
	char m_quote_char;
	std::string m_entity;
	std::string m_text;

	

	typedef enum {
		TEXT,
		ENTITY,
		OPEN_TAG,
		CLOSE_TAG,
		START_TAG,
		ATTRIBUTE_LVALUE,
		ATTRIBUTE_EQUAL,
		ATTRIBUTE_RVALUE,
		QUOTE,
		IN_TAG,
		SINGLE_TAG,
		COMMENT,
		DONE,
		DOCTYPE,
		IN_XMLDECLARATION,
		BEFORE_XMLDECLARATION,
		OPEN_XMLDECLARATION,
		CDATA
	} eMunkQDStates;

	typedef std::stack<eMunkQDStates> StateStack;
	StateStack m_stack;
 public:
	MunkQDParser();
	~MunkQDParser();
	void parse(MunkQDDocHandler *pDH, std::istream *pStream) throw(MunkQDException);

 protected:
	void cleanUp() 
	{
		if (bot) {
			delete[] bot;
			bot = 0;
		}
	}
	void eraseAttributes()
	{
		m_attributes.clear();
	}

	bool myisspace(char c)
	{
		return c == ' ' 
			|| c == '\n' 
			|| c == '\r' 
			|| c == '\t' 
			|| c == '\x0b' // Vertical space
			|| c == '\f';
	}

	void pushState(eMunkQDStates state) {
		m_stack.push(state);
	}
		

	eMunkQDStates popState(void) throw(MunkQDException)
	{
		if (!m_stack.empty()) {
			eMunkQDStates result = m_stack.top();
			m_stack.pop();
			return result;
		} else {
			except("MunkQDParser::popState: Attempted to pop off of an empty StateStack.");
			return TEXT; // Just to fool the compiler into giving now warning!
		}
	}

	void handle_entity(eMunkQDStates mode) throw(MunkQDException);

	std::string state2string(eMunkQDStates e);

	void except(const std::string& s) throw(MunkQDException);

	char getNextChar(void) throw(MunkQDException);

	void fillBuffer(void);
 protected:
	bool hasMoreInput() { return eof == 0 || cur != eof; };

 private:
	char *bot, *tok, *ptr, *cur, *pos, *lim, *top, *eof;
};



#include "wx/defs.h"
#include "wx/brush.h"
#include "wx/radiobox.h"
#include "wx/button.h"
#include "wx/combobox.h"
#include "wx/panel.h"
#include "wx/regex.h"




//--------------------------------------------------------------------------------
// ALIGNMENTS
//                  Describes alignment of text etc. in containers
//--------------------------------------------------------------------------------

#define MunkHTML_ALIGN_LEFT            0x0000
#define MunkHTML_ALIGN_RIGHT           0x0002
#define MunkHTML_ALIGN_JUSTIFY         0x0010

#define MunkHTML_ALIGN_TOP             0x0004
#define MunkHTML_ALIGN_BOTTOM          0x0008

#define MunkHTML_ALIGN_CENTER          0x0001



//--------------------------------------------------------------------------------
// COLOR MODES
//                  Used by MunkHtmlColourCell to determine clr of what is changing
//--------------------------------------------------------------------------------

#define MunkHTML_CLR_FOREGROUND        0x0001
#define MunkHTML_CLR_BACKGROUND        0x0002



//--------------------------------------------------------------------------------
// UNITS
//                  Used to specify units
//--------------------------------------------------------------------------------

#define MunkHTML_UNITS_NONE            0x0000
#define MunkHTML_UNITS_PIXELS          0x0001
#define MunkHTML_UNITS_PERCENT         0x0002
#define MunkHTML_UNITS_INCHES          0x0004



//--------------------------------------------------------------------------------
// INDENTS
//                  Used to specify indetation relatives
//--------------------------------------------------------------------------------

#define MunkHTML_INDENT_LEFT           0x0010
#define MunkHTML_INDENT_RIGHT          0x0020
#define MunkHTML_INDENT_TOP            0x0040
#define MunkHTML_INDENT_BOTTOM         0x0080

#define MunkHTML_INDENT_HORIZONTAL     (MunkHTML_INDENT_LEFT | MunkHTML_INDENT_RIGHT)
#define MunkHTML_INDENT_VERTICAL       (MunkHTML_INDENT_TOP | MunkHTML_INDENT_BOTTOM)
#define MunkHTML_INDENT_ALL            (MunkHTML_INDENT_VERTICAL | MunkHTML_INDENT_HORIZONTAL)




//--------------------------------------------------------------------------------
// FIND CONDITIONS
//                  Identifiers of MunkHtmlCell's Find() conditions
//--------------------------------------------------------------------------------

#define MunkHTML_COND_ISANCHOR              1
        // Finds the anchor of 'param' name (pointer to wxString).

#define MunkHTML_COND_ISIMAGEMAP            2
        // Finds imagemap of 'param' name (pointer to wxString).
    // (used exclusively by m_image.cpp)

#define MunkHTML_COND_USER              10000
        // User-defined conditions should start from this number


//--------------------------------------------------------------------------------
// INTERNALS
//                  MunkHTML internal constants
//--------------------------------------------------------------------------------

    /* size of one scroll step of MunkHtmlWindow in pixels */
#define MunkHTML_SCROLL_STEP               16

    /* size of temporary buffer used during parsing */
#define MunkHTML_BUFLEN                  1024

    /* maximum number of pages printable via html printing */
#define MunkHTML_PRINT_MAX_PAGES          999


/////////////////////////////////////////////////////////////////////////////
// Name:        htmltag.h
// Purpose:     MunkHtmlTag class (represents single tag)
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmltag.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/object.h"
#include "wx/arrstr.h"


class wxColour; // Forward declaration
class MunkQDHTMLHandler; // Forward declaration

//--------------------------------------------------------------------------------
// MunkHtmlTag
//                  This represents single tag. It is used as internal structure
//                  by MunkHtmlParser.
//--------------------------------------------------------------------------------

class MunkHtmlTag : public wxObject
{
    DECLARE_CLASS(MunkHtmlTag)

protected:
    // constructs MunkHtmlTag object based on HTML tag.
    // The tag begins (with '<' character) at position pos in source
    // end_pos is position where parsing ends (usually end of document)
	MunkHtmlTag(const wxString& tagname, const MunkAttributeMap& attrs);
	friend class MunkQDHTMLHandler;
public:
    virtual ~MunkHtmlTag();

    // Returns tag's name in uppercase.
    inline wxString GetName() const {return m_Name;}

    // Returns true if the tag has given parameter. Parameter
    // should always be in uppercase.
    // Example : <IMG SRC="test.jpg"> HasParam("SRC") returns true
    bool HasParam(const wxString& par) const;

    // Returns value of the param. Value is in uppercase unless it is
    // enclosed with "
    // Example : <P align=right> GetParam("ALIGN") returns (RIGHT)
    //           <P IMG SRC="WhaT.jpg"> GetParam("SRC") returns (WhaT.jpg)
    //                           (or ("WhaT.jpg") if with_commas == true)
    wxString GetParam(const wxString& par, bool with_commas = false) const;

    // Convenience functions:
    bool GetParamAsColour(const wxString& par, wxColour *clr) const;
    bool GetParamAsInt(const wxString& par, int *clr) const;
    bool GetParamAsLengthInInches(const wxString& par, double *inches) const;
    bool GetParamAsLength(const wxString& par, int *pixels, double inches_to_pixels_factor) const;


    // Scans param like scanf() functions family does.
    // Example : ScanParam("COLOR", "\"#%X\"", &clr);
    // This is always with with_commas=false
    // Returns number of scanned values
    // (like sscanf() does)
    // NOTE: unlike scanf family, this function only accepts
    //       *one* parameter !
    int ScanParam(const wxString& par, const wxChar *format, void *param) const;

    // Returns string containing all params.
    wxString GetAllParams() const;
private:
    wxString m_Name;
    wxArrayString m_ParamNames, m_ParamValues;

    DECLARE_NO_COPY_CLASS(MunkHtmlTag)
};






/////////////////////////////////////////////////////////////////////////////
// Name:        htmlcell.h
// Purpose:     MunkHtmlCell class is used by MunkHtmlWindow/MunkHtmlWinParser
//              as a basic visual element of HTML page
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmlcell.h 41020 2006-09-05 20:47:48Z VZ $
// Copyright:   (c) 1999-2003 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/window.h"


class MunkHtmlWindowInterface;
class MunkHtmlWindow;
class MunkHtmlLinkInfo;
class MunkHtmlCell;
class MunkHtmlContainerCell;


// MunkHtmlSelection is data holder with information about text selection.
// Selection is defined by two positions (beginning and end of the selection)
// and two leaf(!) cells at these positions.
class MunkHtmlSelection
{
public:
    MunkHtmlSelection()
        : m_fromPos(wxDefaultPosition), m_toPos(wxDefaultPosition),
          m_fromPrivPos(wxDefaultPosition), m_toPrivPos(wxDefaultPosition),
          m_fromCell(NULL), m_toCell(NULL) {}

    void Set(const wxPoint& fromPos, const MunkHtmlCell *fromCell,
             const wxPoint& toPos, const MunkHtmlCell *toCell);
    void Set(const MunkHtmlCell *fromCell, const MunkHtmlCell *toCell);

    const MunkHtmlCell *GetFromCell() const { return m_fromCell; }
    const MunkHtmlCell *GetToCell() const { return m_toCell; }

    // these values are in absolute coordinates:
    const wxPoint& GetFromPos() const { return m_fromPos; }
    const wxPoint& GetToPos() const { return m_toPos; }

    // these are From/ToCell's private data
    const wxPoint& GetFromPrivPos() const { return m_fromPrivPos; }
    const wxPoint& GetToPrivPos() const { return m_toPrivPos; }
    void SetFromPrivPos(const wxPoint& pos) { m_fromPrivPos = pos; }
    void SetToPrivPos(const wxPoint& pos) { m_toPrivPos = pos; }
    void ClearPrivPos() { m_toPrivPos = m_fromPrivPos = wxDefaultPosition; }

    bool IsEmpty() const
        { return m_fromPos == wxDefaultPosition &&
                 m_toPos == wxDefaultPosition; }

private:
    wxPoint m_fromPos, m_toPos;
    wxPoint m_fromPrivPos, m_toPrivPos;
    const MunkHtmlCell *m_fromCell, *m_toCell;
};



enum MunkHtmlSelectionState
{
    MunkHTML_SEL_OUT,     // currently rendered cell is outside the selection
    MunkHTML_SEL_IN,      // ... is inside selection
    MunkHTML_SEL_CHANGING // ... is the cell on which selection state changes
};

// Selection state is passed to MunkHtmlCell::Draw so that it can render itself
// differently e.g. when inside text selection or outside it.
class MunkHtmlRenderingState
{
public:
 MunkHtmlRenderingState() : m_selState(MunkHTML_SEL_OUT), m_fgColour(wxNullColour), m_bgColour(wxNullColour) {}

    void SetSelectionState(MunkHtmlSelectionState s) { m_selState = s; }
    MunkHtmlSelectionState GetSelectionState() const { return m_selState; }

    void SetFgColour(const wxColour& c) { m_fgColour = c; }
    const wxColour& GetFgColour() const { return m_fgColour; }
    void SetBgColour(const wxColour& c) { m_bgColour = c; }
    const wxColour& GetBgColour() const { return m_bgColour; }

private:
    MunkHtmlSelectionState  m_selState;
    wxColour              m_fgColour, m_bgColour;
};


// HTML rendering customization. This class is used when rendering MunkHtmlCells
// as a callback:
class MunkHtmlRenderingStyle
{
public:
    virtual ~MunkHtmlRenderingStyle() {}
    virtual wxColour GetSelectedTextColour(const wxColour& clr) = 0;
    virtual wxColour GetSelectedTextBgColour(const wxColour& clr) = 0;
};

// Standard style:
class MunkDefaultHtmlRenderingStyle : public MunkHtmlRenderingStyle
{
public:
    virtual wxColour GetSelectedTextColour(const wxColour& clr);
    virtual wxColour GetSelectedTextBgColour(const wxColour& clr);
};


// Information given to cells when drawing them. Contains rendering state,
// selection information and rendering style object that can be used to
// customize the output.
class MunkHtmlRenderingInfo
{
public:
 MunkHtmlRenderingInfo(const wxColour& WindowBackgroundColour) : m_selection(NULL), m_style(NULL), m_bUnderline(false), m_WindowBackgroundColour(WindowBackgroundColour) {}

    void SetSelection(MunkHtmlSelection *s) { m_selection = s; }
    MunkHtmlSelection *GetSelection() const { return m_selection; }

    void SetStyle(MunkHtmlRenderingStyle *style) { m_style = style; }

    const wxColour& GetWindowBackgroundColour(void) const { return m_WindowBackgroundColour; };

    MunkHtmlRenderingStyle& GetStyle() { return *m_style; }

    MunkHtmlRenderingState& GetState() { return m_state; }

    void SetUnderline(bool bUnderline) { m_bUnderline = bUnderline; };
    bool GetUnderline(void) const { return m_bUnderline; };

protected:
    MunkHtmlSelection      *m_selection;
    MunkHtmlRenderingStyle *m_style;
    MunkHtmlRenderingState m_state;
    bool m_bUnderline;
    wxColour m_WindowBackgroundColour;
};


enum {
	MunkHTML_BACKGROUND_REPEAT_NO_REPEAT = 0,
	MunkHTML_BACKGROUND_REPEAT_REPEAT_X = 1,
	MunkHTML_BACKGROUND_REPEAT_REPEAT_Y = 2,
	MunkHTML_BACKGROUND_REPEAT_REPEAT = 3,
};

// Flags for MunkHtmlCell::FindCellByPos
enum
{
    MunkHTML_FIND_EXACT             = 1,
    MunkHTML_FIND_NEAREST_BEFORE    = 2,
    MunkHTML_FIND_NEAREST_AFTER     = 4
};


// Superscript/subscript/normal script mode of a cell
enum MunkHtmlScriptMode
{
    MunkHTML_SCRIPT_NORMAL,
    MunkHTML_SCRIPT_SUB,
    MunkHTML_SCRIPT_SUP
};

class MunkHTMLFontAttributes {
public:
	bool m_bBold;
	bool m_bItalic;
	bool m_bUnderline;
	bool m_bSmallCaps;
	long m_scriptBaseLine;
	MunkHtmlScriptMode m_scriptMode;
	unsigned int m_sizeFactor; // In percent
	wxColour m_color;
	wxString m_face;
	MunkHTMLFontAttributes(bool bBold, bool bItalic, bool bUnderline, long scriptBaseline, MunkHtmlScriptMode scriptMode, unsigned int sizeFactor = 100, const wxColour& color = *wxBLACK, const wxString& face = wxT("Arial"));
	MunkHTMLFontAttributes();
	MunkHTMLFontAttributes(const MunkHTMLFontAttributes& other);
	~MunkHTMLFontAttributes();
	std::string toString() const;
	void setFace(const wxString& face);
	wxString getFace(void) const;
	long toLong() const;
	static MunkHTMLFontAttributes fromString(const std::string& s);
	MunkHTMLFontAttributes& operator=(const MunkHTMLFontAttributes& other);
protected:
	void copy_to_self(const MunkHTMLFontAttributes& other);
};

typedef std::stack<MunkHTMLFontAttributes>  MunkFontAttributeStack;




// ---------------------------------------------------------------------------
// MunkHtmlCell
//                  Internal data structure. It represents fragments of parsed
//                  HTML page - a word, picture, table, horizontal line and so
//                  on.  It is used by MunkHtmlWindow to represent HTML page in
//                  memory.
// ---------------------------------------------------------------------------


class MunkHtmlCell : public wxObject
{
public:
    MunkHtmlCell();
    virtual ~MunkHtmlCell();

    void SetParent(MunkHtmlContainerCell *p) {m_Parent = p;}
    MunkHtmlContainerCell *GetParent() const {return m_Parent;}

    int GetPosX() const {return m_PosX;}
    int GetPosY() const {return m_PosY;}
    int GetWidth() const {return m_Width;}

    // Returns the maximum possible length of the cell.
    // Call Layout at least once before using GetMaxTotalWidth()
    virtual int GetMaxTotalWidth() const { return m_Width; }

    int GetHeight() const {return m_Height;}
    int GetDescent() const {return m_Descent;}

    void SetScriptMode(MunkHtmlScriptMode mode, long previousBase);
    MunkHtmlScriptMode GetScriptMode() const { return m_ScriptMode; }
    long GetScriptBaseline() { return m_ScriptBaseline; }

    // Formatting cells are not visible on the screen, they only alter
    // renderer's state.
    bool IsFormattingCell() const { return m_Width == 0 && m_Height == 0; }
    virtual bool ForceLineBreak(void) { return false; };
    virtual bool IsInlineBlock(void) { return false; };

    const wxString& GetId() const { return m_id; }
    void SetId(const wxString& id) { m_id = id; }

    virtual bool IsInlineBlock(void) const { return false; };

    // returns the link associated with this cell. The position is position
    // within the cell so it varies from 0 to m_Width, from 0 to m_Height
    virtual MunkHtmlLinkInfo* GetLink(int WXUNUSED(x) = 0,
                                    int WXUNUSED(y) = 0) const
        { return m_Link; }

    // Returns cursor to be used when mouse is over the cell:
    virtual wxCursor GetMouseCursor(MunkHtmlWindowInterface *window) const;

#if WXWIN_COMPATIBILITY_2_6
    // this was replaced by GetMouseCursor, don't use in new code!
    virtual wxCursor GetCursor() const;
#endif

    // return next cell among parent's cells
    MunkHtmlCell *GetNext() const {return m_Next;}
    // returns first child cell (if there are any, i.e. if this is container):
    virtual MunkHtmlCell* GetFirstChild() const { return NULL; }

    // members writing methods
    virtual void SetPos(int x, int y) {m_PosX = x, m_PosY = y;}
    void SetLink(const MunkHtmlLinkInfo& link);
    void SetNext(MunkHtmlCell *cell) {m_Next = cell;}

    // 1. adjust cell's width according to the fact that maximal possible width
    //    is w.  (this has sense when working with horizontal lines, tables
    //    etc.)
    // 2. prepare layout (=fill-in m_PosX, m_PosY (and sometime m_Height)
    //    members) = place items to fit window, according to the width w
    virtual void Layout(int w);

    // renders the cell
    virtual void Draw(wxDC& WXUNUSED(dc),
                      int WXUNUSED(x), int WXUNUSED(y),
                      int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                      MunkHtmlRenderingInfo& WXUNUSED(info)) {}

    // proceed drawing actions in case the cell is not visible (scrolled out of
    // screen).  This is needed to change fonts, colors and so on.
    virtual void DrawInvisible(wxDC& WXUNUSED(dc),
                               int WXUNUSED(x), int WXUNUSED(y),
                               MunkHtmlRenderingInfo& WXUNUSED(info)) {}

    // This method returns pointer to the FIRST cell for that
    // the condition
    // is true. It first checks if the condition is true for this
    // cell and then calls m_Next->Find(). (Note: it checks
    // all subcells if the cell is container)
    // Condition is unique condition identifier (see htmldefs.h)
    // (user-defined condition IDs should start from 10000)
    // and param is optional parameter
    // Example : m_Cell->Find(MunkHTML_COND_ISANCHOR, "news");
    //   returns pointer to anchor news
    virtual const MunkHtmlCell* Find(int condition, const void* param) const;


    // This function is called when mouse button is clicked over the cell.
    // Returns true if a link is clicked, false otherwise.
    //
    // 'window' is pointer to MunkHtmlWindowInterface of the window which
    // generated the event.
    // HINT: if this handling is not enough for you you should use
    //       MunkHtmlWidgetCell
    virtual bool ProcessMouseClick(MunkHtmlWindowInterface *window,
                                   const wxPoint& pos,
                                   const wxMouseEvent& event);

#if WXWIN_COMPATIBILITY_2_6
    // this was replaced by ProcessMouseClick, don't use in new code!
    virtual void OnMouseClick(wxWindow *window,
                              int x, int y, const wxMouseEvent& event);
#endif

    // This method used to adjust pagebreak position. The parameter is variable
    // that contains y-coordinate of page break (= horizontal line that should
    // not be crossed by words, images etc.). If this cell cannot be divided
    // into two pieces (each one on another page) then it moves the pagebreak
    // few pixels up.
    //
    // Returned value : true if pagebreak was modified, false otherwise
    // Usage : while (container->AdjustPagebreak(&p)) {}
    virtual bool AdjustPagebreak(int *pagebreak, int render_height,
                                 wxArrayInt& known_pagebreaks);

    // Sets cell's behaviour on pagebreaks (see AdjustPagebreak). Default
    // is true - the cell can be split on two pages
    void SetCanLiveOnPagebreak(bool can) { m_CanLiveOnPagebreak = can; }

    // Can the line be broken before this cell?
    virtual bool IsLinebreakAllowed() const
        { return !IsFormattingCell(); }

    virtual wxString toString() const { return wxT(""); };
 
    // Returns true for simple == terminal cells, i.e. not composite ones.
    // This if for internal usage only and may disappear in future versions!
    virtual bool IsTerminalCell() const { return true; }

    // Find a cell inside this cell positioned at the given coordinates
    // (relative to this's positions). Returns NULL if no such cell exists.
    // The flag can be used to specify whether to look for terminal or
    // nonterminal cells or both. In either case, returned cell is deepest
    // cell in cells tree that contains [x,y].
    virtual MunkHtmlCell *FindCellByPos(wxCoord x, wxCoord y,
                                  unsigned flags = MunkHTML_FIND_EXACT) const;

    // Returns absolute position of the cell on HTML canvas.
    // If rootCell is provided, then it's considered to be the root of the
    // hierarchy and the returned value is relative to it.
    wxPoint GetAbsPos(MunkHtmlCell *rootCell = NULL) const;

    // Returns root cell of the hierarchy (i.e. grand-grand-...-parent that
    // doesn't have a parent itself)
    MunkHtmlCell *GetRootCell() const;

    // Returns first (last) terminal cell inside this cell. It may return NULL,
    // but it is rare -- only if there are no terminals in the tree.
    virtual MunkHtmlCell *GetFirstTerminal() const
        { return wxConstCast(this, MunkHtmlCell); }
    virtual MunkHtmlCell *GetLastTerminal() const
        { return wxConstCast(this, MunkHtmlCell); }

    // Returns cell's depth, i.e. how far under the root cell it is
    // (if it is the root, depth is 0)
    unsigned GetDepth() const;

    // Returns true if the cell appears before 'cell' in natural order of
    // cells (= as they are read). If cell A is (grand)parent of cell B,
    // then both A.IsBefore(B) and B.IsBefore(A) always return true.
    bool IsBefore(MunkHtmlCell *cell) const;

    // Converts the cell into text representation. If sel != NULL then
    // only part of the cell inside the selection is converted.
    virtual wxString ConvertToText(MunkHtmlSelection *WXUNUSED(sel)) const
        { return wxEmptyString; }

protected:
    // pointer to the next cell
    MunkHtmlCell *m_Next;
    // pointer to parent cell
    MunkHtmlContainerCell *m_Parent;

    // dimensions of fragment (m_Descent is used to position text & images)
    long m_Width, m_Height, m_Descent;
    // position where the fragment is drawn:
    long m_PosX, m_PosY;

    // superscript/subscript/normal:
    MunkHtmlScriptMode m_ScriptMode;
    long m_ScriptBaseline;

    // destination address if this fragment is hypertext link, NULL otherwise
    MunkHtmlLinkInfo *m_Link;
    // true if this cell can be placed on pagebreak, false otherwise
    bool m_CanLiveOnPagebreak;
    // unique identifier of the cell, generated from "id" property of tags
    wxString m_id;

    // background image, may be invalid
    wxBitmap m_bmpBg;

    // background image repeat (see enum with
    // MunkHTML_BACKGROUND_REPEAT_... enum constants)
    int m_nBackgroundRepeat;

    DECLARE_ABSTRACT_CLASS(MunkHtmlCell)
    DECLARE_NO_COPY_CLASS(MunkHtmlCell)
};




// ----------------------------------------------------------------------------
// Inherited cells:
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// MunkHtmlWordCell
//                  Single word in input stream.
// ----------------------------------------------------------------------------

class MunkHtmlWordCell : public MunkHtmlCell
{
public:
    MunkHtmlWordCell(const wxString& word, const wxDC& dc);
    MunkHtmlWordCell(long m_SpaceWidth, long m_SpaceHeight, long m_SpaceDescent);
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
              MunkHtmlRenderingInfo& info);
    virtual wxCursor GetMouseCursor(MunkHtmlWindowInterface *window) const;
    wxString ConvertToText(MunkHtmlSelection *sel) const;
    bool IsLinebreakAllowed() const { return m_allowLinebreak; }

    void SetPreviousWord(MunkHtmlWordCell *cell);

    virtual wxString toString() const { return m_Word; };

protected:
    void SetSelectionPrivPos(const wxDC& dc, MunkHtmlSelection *s) const;
    void Split(const wxDC& dc,
               const wxPoint& selFrom, const wxPoint& selTo,
               unsigned& pos1, unsigned& pos2) const;

    wxString m_Word;
    bool     m_allowLinebreak;

    DECLARE_ABSTRACT_CLASS(MunkHtmlWordCell)
    DECLARE_NO_COPY_CLASS(MunkHtmlWordCell)
};

enum eMunkMiniDOMTagKind {
	kSingleTag,
	kStartTag,
	kEndTag
};

class MunkMiniDOMTag {
protected:	
	std::string m_tag;
	eMunkMiniDOMTagKind m_kind;
	MunkAttributeMap m_attributes;
public:
	MunkMiniDOMTag(const std::string& tag, eMunkMiniDOMTagKind kind);
	MunkMiniDOMTag(const std::string& tag, eMunkMiniDOMTagKind kind, const MunkAttributeMap& attrs);
	virtual ~MunkMiniDOMTag();
	void setAttr(const std::string& name, const std::string& value);
	const MunkAttributeMap& getAttrs() const { return m_attributes; };
	eMunkMiniDOMTagKind getKind() const { return m_kind; };
	std::string getTag() const { return m_tag; };
	virtual wxString toString() const;
 private:
	DECLARE_NO_COPY_CLASS(MunkMiniDOMTag)
};

class MunkHtmlTagCell : public MunkHtmlCell
{
public:
	MunkHtmlTagCell(MunkMiniDOMTag *pTag);
	virtual ~MunkHtmlTagCell();
	virtual wxString toString() const;
 protected:
	MunkMiniDOMTag *m_pTag;
	DECLARE_NO_COPY_CLASS(MunkHtmlTagCell)
};




// ----------------------------------------------------------------------------
// MunkHtmlNegativeSpaceCell
//                  Single word in input stream.
// ----------------------------------------------------------------------------

class MunkHtmlNegativeSpaceCell : public MunkHtmlWordCell
{
 protected:
	int m_pixels;
 public:
	MunkHtmlNegativeSpaceCell(int pixels, const wxDC& dc);
	virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
			  MunkHtmlRenderingInfo& info);

 protected:
    DECLARE_ABSTRACT_CLASS(MunkHtmlNegativeSpaceCell)
    DECLARE_NO_COPY_CLASS(MunkHtmlNegativeSpaceCell)
};


// ----------------------------------------------------------------------------
// MunkHtmlPagebreakCell
//                  Page break when printing (otherwise, invisible)
// ----------------------------------------------------------------------------

class MunkHtmlPagebreakCell : public MunkHtmlCell
{
public:
	MunkHtmlPagebreakCell();
	virtual ~MunkHtmlPagebreakCell();
	virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
		  MunkHtmlRenderingInfo& info);
	virtual bool AdjustPagebreak(int *pagebreak, int render_height,
				     wxArrayInt& WXUNUSED(known_pagebreaks));
	// 1. adjust cell's width according to the fact that maximal possible width
	//    is w.  (this has sense when working with horizontal lines, tables
	//    etc.)
	// 2. prepare layout (=fill-in m_PosX, m_PosY (and sometime m_Height)
	//    members) = place items to fit window, according to the width w
	virtual void Layout(int w);
 protected:
	bool m_bHasSeenPagebreak;
};


enum MunkHtmlDirection
{
    MunkHTML_LTR,     // Left to right (the default)
    MunkHTML_RTL      // Right to left
};

enum MunkHtmlBorderDirection {
	MunkHTML_BORDER_TOP,
	MunkHTML_BORDER_RIGHT,
	MunkHTML_BORDER_BOTTOM,
	MunkHTML_BORDER_LEFT
};

enum MunkHtmlBorderStyle {
	MunkHTML_BORDER_STYLE_NONE,
	MunkHTML_BORDER_STYLE_SOLID,
	MunkHTML_BORDER_STYLE_OUTSET
};



// Container contains other cells, thus forming tree structure of rendering
// elements. Basic code of layout algorithm is contained in this class.
class MunkHtmlContainerCell : public MunkHtmlCell
{
public:
    MunkHtmlContainerCell(MunkHtmlContainerCell *parent);
    virtual ~MunkHtmlContainerCell();

    virtual void Layout(int w);
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                      MunkHtmlRenderingInfo& info);
    virtual void DrawInvisible(wxDC& dc, int x, int y,
                               MunkHtmlRenderingInfo& info);
/*    virtual bool AdjustPagebreak(int *pagebreak, int *known_pagebreaks = NULL, int number_of_pages = 0);*/
    virtual bool AdjustPagebreak(int *pagebreak, int render_height, wxArrayInt& known_pagebreaks);

    // insert cell at the end of m_Cells list
    void InsertCell(MunkHtmlCell *cell);

    // sets horizontal/vertical alignment
    void SetAlignHor(int al) {m_AlignHor = al; m_LastLayout = -1;}
    int GetAlignHor() const {return m_AlignHor;}
    void SetAlignVer(int al) {m_AlignVer = al; m_LastLayout = -1;}
    int GetAlignVer() const {return m_AlignVer;}

    void SetFirstLineIndent(int i) { m_IndentFirstLine = i; };
    void SetDeclaredHeight(int h) { m_DeclaredHeight = h; };
    void SetHeight(int h) { m_Height = h; };

    int GetDeclaredHeight() const { return m_DeclaredHeight; };

    // sets left-border indentation. units is one of MunkHTML_UNITS_* constants
    // what is combination of MunkHTML_INDENT_*
    void SetIndent(int i, int what, int units = MunkHTML_UNITS_PIXELS);
    // returns the indentation. ind is one of MunkHTML_INDENT_* constants
    int GetIndent(int ind) const;
    // returns type of value returned by GetIndent(ind)
    int GetIndentUnits(int ind) const;

    void SetFirstLineIndent(const MunkHtmlTag& tag);
    void SetLeftMargin(const MunkHtmlTag& tag);
    void SetRightMargin(const MunkHtmlTag& tag);

    bool m_bIsInlineBlock;
    void SetIsInlineBlock(bool bIsInlineBlock) { m_bIsInlineBlock = bIsInlineBlock; };
    virtual bool IsInlineBlock(void) { return m_bIsInlineBlock; };


    // sets alignment info based on given tag's params
    void SetAlign(const MunkHtmlTag& tag);
    void SetVAlign(const MunkHtmlTag& tag);

    // sets floating width adjustment
    // (examples : 32 percent of parent container,
    // -15 pixels percent (this means 100 % - 15 pixels)
    void SetWidthFloat(int w, int units) {m_WidthFloat = w; m_WidthFloatUnits = units; m_LastLayout = -1;}
    void SetWidthFloat(const MunkHtmlTag& tag, double pixel_scale = 1.0);
    // Tage HEIGHT atttribute and set m_DeclaredHeight.
    void SetHeight(const MunkHtmlTag& tag, double pixel_scale = 1.0);
    void SetBorder(const wxColour& clr1, const wxColour& clr2);
    void SetBorder(MunkHtmlBorderDirection direction, MunkHtmlBorderStyle style, int border_width, const wxColour& set_clr1, const wxColour& set_clr2 = wxNullColour);

    // sets minimal height of this container.
    void SetMinHeight(int h, int align = MunkHTML_ALIGN_TOP) {m_MinHeight = h; m_MinHeightAlign = align; m_LastLayout = -1; }

    // Gets minimal height of this container
    int GetMinHeight() const { return m_MinHeight; };


    void SetBackgroundColour(const wxColour& clr);
    void SetBackgroundImage(const wxBitmap& bmpBg) { m_bmpBg = bmpBg; }
    void SetBackgroundRepeat(int background_repeat) { m_nBackgroundRepeat = background_repeat; };

    // returns background colour (of wxNullColour if none set), so that widgets can
    // adapt to it:
    wxColour GetBackgroundColour();
    virtual MunkHtmlLinkInfo* GetLink(int x = 0, int y = 0) const;
    virtual const MunkHtmlCell* Find(int condition, const void* param) const;

    void SetDirection(const MunkHtmlTag& tag);
    void SetMarginsAndPaddingAndTextIndent(const std::string& tag, const MunkHtmlTag& munkTag, wxString& css_style, int CurrentCharHeight);


#if WXWIN_COMPATIBILITY_2_6
    // this was replaced by ProcessMouseClick, don't use in new code!
    virtual void OnMouseClick(wxWindow *window,
                              int x, int y, const wxMouseEvent& event);
#endif
    virtual bool ProcessMouseClick(MunkHtmlWindowInterface *window,
                                   const wxPoint& pos,
                                   const wxMouseEvent& event);

    virtual MunkHtmlCell* GetFirstChild() const { return m_Cells; }
#if WXWIN_COMPATIBILITY_2_4
    wxDEPRECATED( MunkHtmlCell* GetFirstCell() const );
#endif
    // returns last child cell:
    MunkHtmlCell* GetLastChild() const { return m_LastCell; }

    // see comment in MunkHtmlCell about this method
    virtual bool IsTerminalCell() const { return false; }

    virtual MunkHtmlCell *FindCellByPos(wxCoord x, wxCoord y,
                                  unsigned flags = MunkHTML_FIND_EXACT) const;

    virtual MunkHtmlCell *GetFirstTerminal() const;
    virtual MunkHtmlCell *GetLastTerminal() const;


    // Removes indentation on top or bottom of the container (i.e. above or
    // below first/last terminal cell). For internal use only.
    virtual void RemoveExtraSpacing(bool top, bool bottom);

    // Returns the maximum possible length of the container.
    // Call Layout at least once before using GetMaxTotalWidth()
    virtual int GetMaxTotalWidth() const { return m_MaxTotalWidth; }

    

protected:
    void UpdateRenderingStatePre(MunkHtmlRenderingInfo& info,
                                 MunkHtmlCell *cell) const;
    void UpdateRenderingStatePost(MunkHtmlRenderingInfo& info,
                                  MunkHtmlCell *cell) const;

protected:
    int m_IndentLeft, m_IndentRight, m_IndentTop, m_IndentBottom;

    // Borders
    bool m_bUseBorder;
    int m_BorderWidthTop, m_BorderWidthRight, m_BorderWidthBottom, m_BorderWidthLeft;
    wxColour m_BorderColour1Top, m_BorderColour2Top;
    wxColour m_BorderColour1Right, m_BorderColour2Right;
    wxColour m_BorderColour1Bottom, m_BorderColour2Bottom;
    wxColour m_BorderColour1Left, m_BorderColour2Left;

    MunkHtmlBorderStyle m_BorderStyleTop, m_BorderStyleRight, m_BorderStyleBottom, m_BorderStyleLeft;
    // indentation of subcells. There is always m_Indent pixels
    // big space between given border of the container and the subcells
    // it m_Indent < 0 it is in PERCENTS, otherwise it is in pixels

    // Indentation of first line.  There is always m_IndentFirstLine
    // pixels between the m_IndentLeft position and the start of the
    // first line.  This is always in pixels.  If it is negative, it
    // means that the distance is to the left of the m_IndentLeft
    // position.
    int m_IndentFirstLine;
    int m_MinHeight, m_MinHeightAlign;
        // minimal height.
    MunkHtmlCell *m_Cells, *m_LastCell;
            // internal cells, m_Cells points to the first of them, m_LastCell to the last one.
            // (LastCell is needed only to speed-up InsertCell)
    int m_AlignHor, m_AlignVer;
            // alignment horizontal and vertical (left, center, right)
    int m_WidthFloat, m_WidthFloatUnits;
            // width float is used in adjustWidth
    bool m_UseBkColour;
    wxColour m_BkColour;
            // background color of this container
            // borders color of this container
    int m_LastLayout;
            // if != -1 then call to Layout may be no-op
            // if previous call to Layout has same argument
    int m_MaxTotalWidth;
            // Maximum possible length if ignoring line wrap
    MunkHtmlDirection m_direction;

    // width and height which are declared with CSS-like
    // attributes
    int m_DeclaredHeight;

    wxBitmap *m_pBgImg; // Background image

    DECLARE_ABSTRACT_CLASS(MunkHtmlContainerCell)
    DECLARE_NO_COPY_CLASS(MunkHtmlContainerCell)
};

#if WXWIN_COMPATIBILITY_2_4
inline MunkHtmlCell* MunkHtmlContainerCell::GetFirstCell() const
    { return GetFirstChild(); }
#endif


//-----------------------------------------------------------------------------
// MunkHtmlListCell
//-----------------------------------------------------------------------------

struct MunkHtmlListItemStruct
{
    MunkHtmlContainerCell *mark;
    MunkHtmlContainerCell *cont;
    int minWidth;
    int maxWidth;
};

class MunkHtmlListCell : public MunkHtmlContainerCell
{
    private:
        wxBrush m_Brush;

        int m_NumRows;
        MunkHtmlListItemStruct *m_RowInfo;
        void ReallocRows(int rows);
        void ComputeMinMaxWidths();
        int ComputeMaxBase(MunkHtmlCell *cell);
        int m_ListmarkWidth;

    public:
        MunkHtmlListCell(MunkHtmlContainerCell *parent);
        virtual ~MunkHtmlListCell();
        void AddRow(MunkHtmlContainerCell *mark, MunkHtmlContainerCell *cont);
        virtual void Layout(int w);

    DECLARE_NO_COPY_CLASS(MunkHtmlListCell)
};



class MunkHtmlLineBreakCell : public MunkHtmlCell
{
 private:
	long m_CharHeight;
    public:
        MunkHtmlLineBreakCell(long CurrentCharHeight, MunkHtmlCell *pPreviousCell);
        virtual ~MunkHtmlLineBreakCell();
        virtual void Layout(int w);
	virtual bool ForceLineBreak(void) { return true; };

    DECLARE_NO_COPY_CLASS(MunkHtmlLineBreakCell)
};



// ---------------------------------------------------------------------------
// MunkHtmlColourCell
//                  Color changer.
// ---------------------------------------------------------------------------

class MunkHtmlColourCell : public MunkHtmlCell
{
public:
 MunkHtmlColourCell(const wxColour& clr, int flags = MunkHTML_CLR_FOREGROUND) : MunkHtmlCell() {m_Colour = clr; m_Flags = flags; m_Brush = wxBrush(m_Colour, wxSOLID); }
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                      MunkHtmlRenderingInfo& info);
    virtual void DrawInvisible(wxDC& dc, int x, int y,
                               MunkHtmlRenderingInfo& info);

protected:
    wxColour m_Colour;
    unsigned m_Flags;
    wxBrush m_Brush;

    DECLARE_ABSTRACT_CLASS(MunkHtmlColourCell)
    DECLARE_NO_COPY_CLASS(MunkHtmlColourCell)
};




//--------------------------------------------------------------------------------
// MunkHtmlFontCell
//                  Sets actual font used for text rendering
//--------------------------------------------------------------------------------

class MunkHtmlFontCell : public MunkHtmlCell
{
public:
 MunkHtmlFontCell(wxFont *font, bool bUnderline) : MunkHtmlCell() { m_Font = (*font); m_bUnderline = bUnderline; }
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                      MunkHtmlRenderingInfo& info);
    virtual void DrawInvisible(wxDC& dc, int x, int y,
                               MunkHtmlRenderingInfo& info);

protected:
    wxFont m_Font;
    bool m_bUnderline;

    DECLARE_ABSTRACT_CLASS(MunkHtmlFontCell)
    DECLARE_NO_COPY_CLASS(MunkHtmlFontCell)
};






//--------------------------------------------------------------------------------
// MunkHtmlwidgetCell
//                  This cell is connected with wxWindow object
//                  You can use it to insert windows into HTML page
//                  (buttons, input boxes etc.)
//--------------------------------------------------------------------------------

class MunkHtmlWidgetCell : public MunkHtmlCell
{
public:
    // !!! wnd must have correct parent!
    // if w != 0 then the m_Wnd has 'floating' width - it adjust
    // it's width according to parent container's width
    // (w is percent of parent's width)
    MunkHtmlWidgetCell(wxWindow *wnd, int w = 0);
    virtual ~MunkHtmlWidgetCell() { m_Wnd->Destroy(); }
    virtual void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                      MunkHtmlRenderingInfo& info);
    virtual void DrawInvisible(wxDC& dc, int x, int y,
                               MunkHtmlRenderingInfo& info);
    virtual void Layout(int w);
protected:
    wxWindow* m_Wnd;
    int m_WidthFloat;
            // width float is used in adjustWidth (it is in percents)

    DECLARE_ABSTRACT_CLASS(MunkHtmlWidgetCell)
    DECLARE_NO_COPY_CLASS(MunkHtmlWidgetCell)
};




//--------------------------------------------------------------------------------
// MunkHtmlLinkInfo
//                  Internal data structure. It represents hypertext link
//--------------------------------------------------------------------------------

class MunkHtmlLinkInfo : public wxObject
{
public:
    MunkHtmlLinkInfo() : wxObject()
          { m_Href = m_Target = wxEmptyString; m_Event = NULL, m_Cell = NULL; }
    MunkHtmlLinkInfo(const wxString& href, const wxString& target = wxEmptyString) : wxObject()
          { m_Href = href; m_Target = target; m_Event = NULL, m_Cell = NULL; }
    MunkHtmlLinkInfo(const MunkHtmlLinkInfo& l) : wxObject()
          { m_Href = l.m_Href, m_Target = l.m_Target, m_Event = l.m_Event;
            m_Cell = l.m_Cell; }
    MunkHtmlLinkInfo& operator=(const MunkHtmlLinkInfo& l)
          { m_Href = l.m_Href, m_Target = l.m_Target, m_Event = l.m_Event;
            m_Cell = l.m_Cell; return *this; }

    void SetEvent(const wxMouseEvent *e) { m_Event = e; }
    void SetHtmlCell(const MunkHtmlCell *e) { m_Cell = e; }

    void SetFirstLineIndent(const MunkHtmlTag& tag);

    wxString GetHref() const { return m_Href; }
    wxString GetTarget() const { return m_Target; }
    const wxMouseEvent* GetEvent() const { return m_Event; }
    const MunkHtmlCell* GetHtmlCell() const { return m_Cell; }

private:
    wxString m_Href, m_Target;
    const wxMouseEvent *m_Event;
    const MunkHtmlCell *m_Cell;
};



// ----------------------------------------------------------------------------
// MunkHtmlTerminalCellsIterator
// ----------------------------------------------------------------------------

class MunkHtmlTerminalCellsIterator
{
public:
    MunkHtmlTerminalCellsIterator(const MunkHtmlCell *from, const MunkHtmlCell *to)
        : m_to(to), m_pos(from) {}

    operator bool() const { return m_pos != NULL; }
    const MunkHtmlCell* operator++();
    const MunkHtmlCell* operator->() const { return m_pos; }
    const MunkHtmlCell* operator*() const { return m_pos; }

private:
    const MunkHtmlCell *m_to, *m_pos;
};

class MunkHtmlCellsIterator {
 public:
 MunkHtmlCellsIterator(const MunkHtmlCell *from, const MunkHtmlCell *to)
	 : m_to(to), m_pos(from) {}
	
	operator bool() const { return m_pos != NULL; }
	const MunkHtmlCell* operator++();
	const MunkHtmlCell* operator->() const { return m_pos; }
	const MunkHtmlCell* operator*() const { return m_pos; }
	
 private:
	const MunkHtmlCell *m_to, *m_pos;
};


/////////////////////////////////////////////////////////////////////////////
// Name:        htmlwin.h
// Purpose:     MunkHtmlWindow class for parsing & displaying HTML
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmlwin.h 43854 2006-12-07 08:56:57Z PC $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#include "wx/scrolwin.h"
#include "wx/config.h"
#include "wx/stopwatch.h"
#include "wx/filesys.h"
#include "wx/filename.h"
#include "wx/bitmap.h"

class MunkHtmlWinModule;
class MunkHtmlHistoryArray;
class MunkHtmlWinAutoScrollTimer;
class MunkHtmlCellEvent;
class MunkHtmlLinkEvent;


// MunkHtmlWindow flags:
#define MunkHW_SCROLLBAR_NEVER    0x0002
#define MunkHW_SCROLLBAR_AUTO     0x0004
#define MunkHW_NO_SELECTION       0x0008

#define MunkHW_DEFAULT_STYLE      MunkHW_SCROLLBAR_AUTO

enum MunkHtmlURLType
{
    MunkHTML_URL_PAGE,
    MunkHTML_URL_IMAGE,
    MunkHTML_URL_OTHER
};



/// Enum for MunkHtmlWindow::OnOpeningURL and MunkHtmlWindowInterface::OnOpeningURL
enum MunkHtmlOpeningStatus
{
    /// Open the requested URL
    MunkHTML_OPEN,
    /// Do not open the URL
    MunkHTML_BLOCK,
    /// Redirect to another URL (returned from OnOpeningURL)
    MunkHTML_REDIRECT
};

/**
    Abstract interface to a HTML rendering window (such as MunkHtmlWindow or
    MunkHtmlListBox) that is passed to MunkHtmlWinParser. It encapsulates all
    communication from the parser to the window.
 */
class MunkHtmlWindowInterface
{
public:
    /// Ctor
    MunkHtmlWindowInterface() {}
    virtual ~MunkHtmlWindowInterface() {}

    /**
        Called by the parser to set window's title to given text.
     */
    virtual void SetHTMLWindowTitle(const wxString& title) = 0;

    /**
        Called when a link is clicked.

        @param link information about the clicked link
     */
    virtual void OnHTMLLinkClicked(const MunkHtmlLinkInfo& link) = 0;

    /**
        Called when the parser needs to open another URL (e.g. an image).

        @param type     Type of the URL request (e.g. image)
        @param url      URL the parser wants to open
        @param redirect If the return value is MunkHTML_REDIRECT, then the
                        URL to redirect to will be stored in this variable
                        (the pointer must never be NULL)

        @return indicator of how to treat the request
     */
    virtual MunkHtmlOpeningStatus OnHTMLOpeningURL(MunkHtmlURLType type,
						   const wxString& url,
						   wxString *redirect) const = 0;

    /**
        Converts coordinates @a pos relative to given @a cell to
        physical coordinates in the window.
     */
    virtual wxPoint HTMLCoordsToWindow(MunkHtmlCell *cell,
                                       const wxPoint& pos) const = 0;

    /// Returns the window used for rendering (may be NULL).
    virtual wxWindow* GetHTMLWindow() = 0;

    /// Returns background colour to use by default.
    virtual wxColour GetHTMLBackgroundColour() const = 0;

    /// Sets window's background to colour @a clr.
    virtual void SetHTMLBackgroundColour(const wxColour& clr) = 0;

    /// Sets window's background to given bitmap.
    virtual void SetHTMLBackgroundImage(const wxBitmap& bmpBg, int background_repeat) = 0;

    /// Sets status bar text.
    virtual void SetHTMLStatusText(const wxString& text) = 0;

    /// Type of mouse cursor
    enum HTMLCursor
    {
        /// Standard mouse cursor (typically an arrow)
        HTMLCursor_Default,
        /// Cursor shown over links
        HTMLCursor_Link,
        /// Cursor shown over selectable text
        HTMLCursor_Text
    };

    /**
        Returns mouse cursor of given @a type.
     */
    virtual wxCursor GetHTMLCursor(HTMLCursor type) const = 0;
};

/**
    Helper class that implements part of mouse handling for MunkHtmlWindow and
    MunkHtmlListBox. Cursor changes and clicking on links are handled, text
    selection is not.
 */
class MunkHtmlWindowMouseHelper
{
protected:
    /**
        Ctor.

        @param iface Interface to the owner window.
     */
    MunkHtmlWindowMouseHelper(MunkHtmlWindowInterface *iface);

    /**
        Virtual dtor.

        It is not really needed in this case but at leats it prevents gcc from
        complaining about its absence.
     */
    virtual ~MunkHtmlWindowMouseHelper() { }

    /// Returns true if the mouse moved since the last call to HandleIdle
    bool DidMouseMove() const { return m_tmpMouseMoved; }

    /// Call this from EVT_MOTION event handler
    void HandleMouseMoved();

    /**
        Call this from EVT_LEFT_UP handler (or, alternatively, EVT_LEFT_DOWN).

        @param rootCell HTML cell inside which the click occured. This doesn't
                        have to be the leaf cell, it can be e.g. toplevel
                        container, but the mouse must be inside the container's
                        area, otherwise the event would be ignored.
        @param pos      Mouse position in coordinates relative to @a cell
        @param event    The event that triggered the call
     */
    bool HandleMouseClick(MunkHtmlCell *rootCell,
                          const wxPoint& pos, const wxMouseEvent& event);

    /**
        Call this from OnInternalIdle of the HTML displaying window. Handles
        mouse movements and must be used together with HandleMouseMoved.

        @param rootCell HTML cell inside which the click occured. This doesn't
                        have to be the leaf cell, it can be e.g. toplevel
                        container, but the mouse must be inside the container's
                        area, otherwise the event would be ignored.
        @param pos      Current mouse position in coordinates relative to
                        @a cell
     */
    void HandleIdle(MunkHtmlCell *rootCell, const wxPoint& pos);

    /**
        Called by HandleIdle when the mouse hovers over a cell. Default
        behaviour is to do nothing.

        @param cell   the cell the mouse is over
        @param x, y   coordinates of mouse relative to the cell
     */
    virtual void OnCellMouseHover(MunkHtmlCell *cell, wxCoord x, wxCoord y);

    /**
        Called by HandleMouseClick when the user clicks on a cell.
        Default behavior is to call MunkHtmlWindowInterface::OnLinkClicked()
        if this cell corresponds to a hypertext link.

        @param cell   the cell the mouse is over
        @param x, y   coordinates of mouse relative to the cell
        @param event    The event that triggered the call


        @return true if a link was clicked, false otherwise.
     */
    virtual bool OnCellClicked(MunkHtmlCell *cell,
                               wxCoord x, wxCoord y,
                               const wxMouseEvent& event);

protected:
    // this flag indicates if the mouse moved (used by HandleIdle)
    bool m_tmpMouseMoved;
    // contains last link name
    MunkHtmlLinkInfo *m_tmpLastLink;
    // contains the last (terminal) cell which contained the mouse
    MunkHtmlCell *m_tmpLastCell;

private:
    MunkHtmlWindowInterface *m_interface;
};

typedef int form_id_t;
typedef std::string form_element_name;


enum eMunkHtmlFormElementKind {
	kFESubmit,
	kFEHidden,
	kFERadioBox,
	kFESelect,
	kFEText,
};

class MunkHtmlButtonPanel : public wxPanel {
	DECLARE_NO_COPY_CLASS(MunkHtmlButtonPanel)
	DECLARE_EVENT_TABLE()
	form_id_t m_form_id;
	wxString m_strLabel;
	MunkHtmlWindow *m_pParent;
	wxButton *m_pButton;
	std::string m_name;
public:
	MunkHtmlButtonPanel(MunkHtmlWindow *pParent,
			    int id,
			    const wxString& strLabel,
			    form_id_t form_id,
			    wxSize size,
			    const std::string& name);
	virtual ~MunkHtmlButtonPanel();
	void OnButtonClicked(wxCommandEvent& event);
};

class MunkHtmlTextInputPanel : public wxPanel {
	DECLARE_EVENT_TABLE()
 public:
	MunkHtmlTextInputPanel(bool bEnable, int size_in_chars, int maxlength, form_id_t form_id, const wxString& value, MunkHtmlWindow *pParent, wxWindowID id, const wxPoint& point , const wxSize& size, long style = 0, bool bSubmitOnEnter = false, const std::string& name = "text_input");
	virtual ~MunkHtmlTextInputPanel();

	void OnEnter(wxCommandEvent& event);

	void setSubmitOnEnter(bool bSubmitOnEnter) { m_bSubmitOnEnter = bSubmitOnEnter; };

	wxString GetValue(void) { return m_pTextCtrl->GetValue(); };
 protected:
	wxTextCtrl *m_pTextCtrl;
	MunkHtmlWindow *m_pParent;
	bool m_bSubmitOnEnter;
	form_id_t m_form_id;
	std::string m_name;
};

class MunkHtmlRadioBoxPanel : public wxPanel {
 public:
	MunkHtmlRadioBoxPanel(bool bEnable, int selection, wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& point , const wxSize& size, const wxArrayString& choices, int majorDimension = 0, long style = wxRA_SPECIFY_ROWS, const std::string& name = "");
	virtual ~MunkHtmlRadioBoxPanel();

	int GetSelection(void) { return m_pRadioBox->GetSelection(); };
 protected:
	wxRadioBox *m_pRadioBox;
	std::string m_name;
};

class MunkHtmlComboBoxPanel : public wxPanel {
	DECLARE_EVENT_TABLE()
	form_id_t m_form_id;
	MunkHtmlWindow *m_pParent;
	bool m_bSubmitOnSelect;
	std::string m_name;
 public:
	MunkHtmlComboBoxPanel(int selection, 
			      MunkHtmlWindow* parent, 
			      wxWindowID id, 
			      const wxString& label, 
			      const wxPoint& point, 
			      const wxSize& size, 
			      const wxArrayString& choices, 
			      long style,
			      form_id_t form_id,
			      bool bSubmitOnSelect,
			      const std::string& name);
	virtual ~MunkHtmlComboBoxPanel();

	void OnSelect(wxCommandEvent& event);

	void setSubmitOnSelect(bool bSubmitOnSelect) { m_bSubmitOnSelect = bSubmitOnSelect; };

	int GetSelection(void) { return m_pComboBox->GetSelection(); };
 protected:
	wxComboBox *m_pComboBox;
};


class MunkHtmlFormElement {
 private:
	static int m_next_id;
	form_id_t m_form_id;
	eMunkHtmlFormElementKind m_kind;
	std::list<std::pair<std::string, std::string> > m_value_label_pair_list;
	int m_selected_index;
	MunkHtmlWidgetCell *m_pWidgetCell;
	MunkHtmlButtonPanel *m_pButtonPanel;
	MunkHtmlTextInputPanel *m_pTextInputPanel;
	MunkHtmlRadioBoxPanel *m_pRadioBoxPanel;
	MunkHtmlComboBoxPanel *m_pComboBoxPanel;
	bool m_bDisabled;
	int m_xSize;
	int m_xMaxLength;
	bool m_bSubmitOnSelect;
	std::string m_name;
 public:
	MunkHtmlFormElement(form_id_t form_id, eMunkHtmlFormElementKind kind, int xSize, int xMaxLength, const std::string& name);
	~MunkHtmlFormElement();
	std::string getValue(); // Get selected value
	void addValueLabelPair(const std::string& value, const std::string& label, bool bSelected = false);
	MunkHtmlWidgetCell *realizeCell(MunkHtmlWindow *pParent);
	void setDisabled(bool bDisabled) { m_bDisabled = bDisabled; };

	void setSubmitOnSelect(bool bSubmitOnSelect) { m_bSubmitOnSelect = bSubmitOnSelect; };

	static int GetNextID() { return m_next_id++; };
	static void ResetNextID() { m_next_id = 20000; };
};

class MunkHtmlForm {
 private:
	form_id_t m_form_id;
	std::map<form_element_name, MunkHtmlFormElement*> m_form_elements;
	std::string m_method;
	std::string m_action;
 public:
	MunkHtmlForm(form_id_t form_id, const std::string& method, const std::string& action);
	~MunkHtmlForm();

	std::string getMethod() const { return m_method; };
	std::string getAction() const { return m_action; };
	int getFormNumber(void) const { return m_form_id; };

	void addFormElement(const std::string& name, eMunkHtmlFormElementKind kind, int xSize, int xMaxLength); // Silently does not add if already there
	MunkHtmlFormElement *getFormElement(const std::string& name);
	std::map<std::string, std::string> getREQUEST(void);
	std::list<MunkHtmlFormElement*> getFormElementList();
};

class MunkHtmlFormContainer {
 private:
	std::map<form_id_t, MunkHtmlForm*> m_forms;
 public:
	MunkHtmlFormContainer();
	~MunkHtmlFormContainer();
	void addForm(form_id_t new_form_id, const std::string& method, const std::string& action);
	MunkHtmlForm *getForm(form_id_t form_id);
	std::list<MunkHtmlForm*> getFormList();
};






// ----------------------------------------------------------------------------
// MunkHtmlWindow
//                  (This is probably the only class you will directly use.)
//                  Purpose of this class is to display HTML page (either local
//                  file or downloaded via HTTP protocol) in a window. Width of
//                  window is constant - given in constructor - virtual height
//                  is changed dynamicly depending on page size.  Once the
//                  window is created you can set it's content by calling
//                  SetPage(text) or LoadPage(filename).
// ----------------------------------------------------------------------------

class MunkHtmlWindow : public wxScrolledWindow,
	                              public MunkHtmlWindowInterface,
                                      public MunkHtmlWindowMouseHelper
{
    DECLARE_DYNAMIC_CLASS(MunkHtmlWindow)
    friend class MunkHtmlWinModule;

public:
    MunkHtmlWindow() : MunkHtmlWindowMouseHelper(this) { Init(); }
    MunkHtmlWindow(wxWindow *parent, wxWindowID id = wxID_ANY,
		   const wxPoint& pos = wxDefaultPosition,
		   const wxSize& size = wxDefaultSize,
		   long style = MunkHW_DEFAULT_STYLE,
		   const wxString& name = wxT("htmlWindow"))
        : MunkHtmlWindowMouseHelper(this)
    {
        Init();
        Create(parent, id, pos, size, style, name);
    }
    virtual ~MunkHtmlWindow();

    void SetForms(MunkHtmlFormContainer *pForms);
    MunkHtmlFormContainer *m_pForms;

    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = MunkHW_SCROLLBAR_AUTO,
                const wxString& name = wxT("htmlWindow"));

    // Set HTML page and display it. !! source is HTML document itself,
    // it is NOT address/filename of HTML document. If you want to
    // specify document location, use LoadPage() istead
    // Return value : false if an error occurred, true otherwise
    virtual bool SetPage(const wxString& source, std::string& error_message);

    // Load HTML page from given location. Location can be either
    // a) /usr/wxGTK2/docs/html/wx.htm
    // b) http://www.somewhere.uk/document.htm
    // c) ftp://ftp.somesite.cz/pub/something.htm
    // In case there is no prefix (http:,ftp:), the method
    // will try to find it itself (1. local file, then http or ftp)
    // After the page is loaded, the method calls SetPage() to display it.
    // Note : you can also use path relative to previously loaded page
    // Return value : same as SetPage
    virtual bool LoadPage(const wxString& location, std::string& error_message);

    // Loads HTML page from file
    bool LoadFile(const wxFileName& filename, std::string& error_message);

    // Returns full location of opened page
    wxString GetOpenedPage() const {return m_OpenedPage;}
    // Returns anchor within opened page
    wxString GetOpenedAnchor() const {return m_OpenedAnchor;}
    // Returns <TITLE> of opened page or empty string otherwise
    wxString GetOpenedPageTitle() const {return m_OpenedPageTitle;}

    // Sets frame in which page title will  be displayed. Format is format of
    // frame title, e.g. "HtmlHelp : %s". It must contain exactly one %s
    void SetRelatedFrame(wxFrame* frame, const wxString& format);
    wxFrame* GetRelatedFrame() const {return m_RelatedFrame;}

    void OnFormSubmitClicked(form_id_t form_id, const wxString& strLabel);
    virtual void OnFormAction(const std::map<std::string, std::string>& REQUEST, const std::string& method, const std::string& action);

#if wxUSE_STATUSBAR
    // After(!) calling SetRelatedFrame, this sets statusbar slot where messages
    // will be displayed. Default is -1 = no messages.
    void SetRelatedStatusBar(int bar);
#endif // wxUSE_STATUSBAR

    // Sets space between text and window borders.
    void SetBorders(int b) {m_Borders = b;}

    // Sets the bitmap to use for background (currnetly it will be tiled,
    // when/if we have CSS support we could add other possibilities...)
    void SetBackgroundImage(const wxBitmap& bmpBg) { m_bmpBg = bmpBg; }
    void SetBackgroundRepeat(int background_repeat) { m_nBackgroundRepeat = background_repeat; };

    // Goes to previous/next page (in browsing history)
    // Returns true if successful, false otherwise
    bool HistoryBack();
    bool HistoryForward();
    bool HistoryCanBack();
    bool HistoryCanForward();
    // Resets history
    void HistoryClear();

    // Returns a pointer to the parser.
    // MunkHtmlWinParser *GetParser() const { return m_Parser; }

    // -- Callbacks --

    // Sets the title of the window
    // (depending on the information passed to SetRelatedFrame() method)
    virtual void OnSetTitle(const wxString& title);

    // Called when user clicked on hypertext link. Default behavior is to
    // call LoadPage(loc)
    virtual void OnLinkClicked(const MunkHtmlLinkInfo& link);

    // Called when MunkHtmlWindow wants to fetch data from an URL (e.g. when
    // loading a page or loading an image). The data are downloaded if and only if
    // OnOpeningURL returns true. If OnOpeningURL returns MunkHTML_REDIRECT,
    // it must set *redirect to the new URL
    virtual MunkHtmlOpeningStatus OnOpeningURL(MunkHtmlURLType WXUNUSED(type),
                                             const wxString& WXUNUSED(url),
                                             wxString *WXUNUSED(redirect)) const
        { return MunkHTML_OPEN; }

#if wxUSE_CLIPBOARD
    // Returns iff IsSelectionEnabled() returns true && there is a selection.
    bool CanCopy() const;
    
    // Helper functions to select parts of page:
    void SelectWord(const wxPoint& pos);
    void SelectLine(const wxPoint& pos);
    void SelectAll();

    // Convert selection to text:
    wxString SelectionToText() { return DoSelectionToText(m_selection); }
    wxString SelectionToHtml() { return DoSelectionToHtml(m_selection); }


    // Converts current page to text:
    wxString ToText();
#endif // wxUSE_CLIPBOARD

    virtual void OnInternalIdle();

    /// Returns standard HTML cursor as used by MunkHtmlWindow
    static wxCursor GetDefaultHTMLCursor(HTMLCursor type);

protected:
    void Init();

 public:
    // Scrolls to anchor of this name. (Anchor is #news
    // or #features etc. it is part of address sometimes:
    // http://www.ms.mff.cuni.cz/~vsla8348/wxhtml/index.html#news)
    // Return value : true if anchor exists, false otherwise
    bool ScrollToAnchor(const wxString& anchor);
 protected:

    // Prepares layout (= fill m_PosX, m_PosY for fragments) based on
    // actual size of window. This method also setup scrollbars
    void CreateLayout();

    void PaintBackground(wxDC& dc);
    void OnEraseBackground(wxEraseEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);

 public:
 
    // Override this in descendant in order to do something.
    // It is called right before deleting the old selection and making
    // the start of the selection, in OnMouseDown.
    virtual void HandleMouseStartMakingSelection() {};

    void OnMouseMove(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnFormSubmitted(wxCommandEvent& event);
#if wxUSE_CLIPBOARD
    void OnKeyUp(wxKeyEvent& event);
    void OnDoubleClick(wxMouseEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnClipboardEvent(wxClipboardTextEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
#endif // wxUSE_CLIPBOARD

    // cleans static variables
    static void CleanUpStatics();

    // Returns true if text selection is enabled (wxClipboard must be available
    // and MunkHW_NO_SELECTION not used)
 public:
    bool IsSelectionEnabled() const;
    void ClearSelection();

 protected:

    enum ClipboardOutputType {
 	    kCOTHTML,
 	    kCOTText
    };
    

    enum ClipboardType
    {
        Primary,
        Secondary
    };

    // Copies selection to clipboard if the clipboard support is available
    //
    // returns true if anything was copied to clipboard, false otherwise
    bool CopySelection(ClipboardType t = Secondary, ClipboardOutputType cot = kCOTText);

#if wxUSE_CLIPBOARD
    // Automatic scrolling during selection:
    void StopAutoScrolling();

    wxString DoSelectionToText(MunkHtmlSelection *sel);
    wxString DoSelectionToHtml(MunkHtmlSelection *sel);
 
#endif // wxUSE_CLIPBOARD


public:
    virtual wxFileSystem *GetFS() { return m_FS; };
    virtual void SetFS(wxFileSystem *pFS) { m_FS = pFS; };

    // Returns pointer to conteiners/cells structure.
    // It should be used ONLY when printing
    MunkHtmlContainerCell* GetInternalRepresentation() const { return m_Cell; }
    void SetTopCell(MunkHtmlContainerCell *pCell) { m_Cell = pCell; };
    
    // MunkHtmlWindowInterface methods:
    virtual void SetHTMLWindowTitle(const wxString& title);
    virtual void OnHTMLLinkClicked(const MunkHtmlLinkInfo& link);
    virtual MunkHtmlOpeningStatus OnHTMLOpeningURL(MunkHtmlURLType type,
                                                 const wxString& url,
                                                 wxString *redirect) const;
    virtual wxPoint HTMLCoordsToWindow(MunkHtmlCell *cell,
                                       const wxPoint& pos) const;
    virtual wxWindow* GetHTMLWindow();
    virtual wxColour GetHTMLBackgroundColour() const;
    virtual void SetHTMLBackgroundColour(const wxColour& clr);
    virtual void SetHTMLBackgroundImage(const wxBitmap& bmpBg, int background_repeat);
    virtual void SetHTMLStatusText(const wxString& text);
    virtual wxCursor GetHTMLCursor(HTMLCursor type) const;

    virtual bool ChangeMagnification(int nNewMagnification, std::string& error_message);

    // implementation of SetPage()
    bool DoSetPage(const wxString& source, std::string& error_message);

    wxString GetPageSource(void) const { return m_strPageSource; };

 protected:
    wxString m_strPageSource; // The source of the current page.

    // This is pointer to the first cell in parsed data.  (Note: the first cell
    // is usually top one = all other cells are sub-cells of this one)
    MunkHtmlContainerCell *m_Cell;
    
    // class for opening files (file system)
    wxFileSystem* m_FS;
    wxDC *m_pDC;
    
    // parser which is used to parse HTML input.
    // Each MunkHtmlWindow has it's own parser because sharing one global
    // parser would be problematic (because of reentrancy)
    // MunkHtmlWinParser *m_Parser;
    // contains name of actualy opened page or empty string if no page opened
    wxString m_OpenedPage;
    // contains name of current anchor within m_OpenedPage
    wxString m_OpenedAnchor;
    // contains title of actualy opened page or empty string if no <TITLE> tag
    wxString m_OpenedPageTitle;

    wxFrame *m_RelatedFrame;
    wxString m_TitleFormat;
    wxColour m_HTML_background_colour;
    int m_nMagnification;
#if wxUSE_STATUSBAR
    // frame in which page title should be displayed & number of it's statusbar
    // reserved for usage with this html window
    int m_RelatedStatusBar;
#endif // wxUSE_STATUSBAR

    // borders (free space between text and window borders)
    // defaults to 10 pixels.
    int m_Borders;

    // current text selection or NULL
    MunkHtmlSelection *m_selection;

    // true if the user is dragging mouse to select text
    bool m_makingSelection;

#if wxUSE_CLIPBOARD
    // time of the last doubleclick event, used to detect tripleclicks
    // (tripleclicks are used to select whole line):
    wxMilliClock_t m_lastDoubleClick;

    // helper class to automatically scroll the window if the user is selecting
    // text and the mouse leaves MunkHtmlWindow:
    MunkHtmlWinAutoScrollTimer *m_timerAutoScroll;
#endif // wxUSE_CLIPBOARD

private:
    // window content for double buffered rendering:
    wxBitmap *m_backBuffer;

    // background image, may be invalid
    wxBitmap m_bmpBg;

    // background image repeat (see enum with
    // MunkHTML_BACKGROUND_REPEAT_... enum constants)
    int m_nBackgroundRepeat;

    // variables used when user is selecting text
    wxPoint     m_tmpSelFromPos;
    MunkHtmlCell *m_tmpSelFromCell;

    // if >0 contents of the window is not redrawn
    // (in order to avoid ugly blinking)
    int m_tmpCanDrawLocks;

    // browser history
    MunkHtmlHistoryArray *m_History;
    int m_HistoryPos;
    // if this FLAG is false, items are not added to history
    bool m_HistoryOn;

    // a flag set if we need to erase background in OnPaint() (otherwise this
    // is supposed to have been done in OnEraseBackground())
    bool m_eraseBgInOnPaint;

    // standard mouse cursors
    static wxCursor *ms_cursorLink;
    static wxCursor *ms_cursorText;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(MunkHtmlWindow)
};



BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(MunkEVT_COMMAND_HTML_CELL_CLICKED, 1000)
    DECLARE_EVENT_TYPE(MunkEVT_COMMAND_HTML_CELL_HOVER, 1001)
    DECLARE_EVENT_TYPE(MunkEVT_COMMAND_HTML_LINK_CLICKED, 1002)
    DECLARE_EVENT_TYPE(MunkEVT_COMMAND_HTML_FORM_SUBMITTED, 1003)
END_DECLARE_EVENT_TYPES()





/*!
 * Html cell window event
 */

class MunkHtmlCellEvent : public wxCommandEvent
{
public:
    MunkHtmlCellEvent() {}
    MunkHtmlCellEvent(wxEventType commandType, int id,
                    MunkHtmlCell *cell, const wxPoint &pt,
                    const wxMouseEvent &ev)
        : wxCommandEvent(commandType, id)
    {
        m_cell = cell;
        m_pt = pt;
        m_mouseEvent = ev;
        m_bLinkWasClicked = false;
    }

    MunkHtmlCell* GetCell() const { return m_cell; }
    wxPoint GetPoint() const { return m_pt; }
    wxMouseEvent GetMouseEvent() const { return m_mouseEvent; }

    void SetLinkClicked(bool linkclicked) { m_bLinkWasClicked=linkclicked; }
    bool GetLinkClicked() const { return m_bLinkWasClicked; }

    // default copy ctor, assignment operator and dtor are ok
    virtual wxEvent *Clone() const { return new MunkHtmlCellEvent(*this); }

private:
    MunkHtmlCell *m_cell;
    wxMouseEvent m_mouseEvent;
    wxPoint m_pt;

    bool m_bLinkWasClicked;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(MunkHtmlCellEvent)
};



/*!
 * Html link event
 */

class MunkHtmlLinkEvent : public wxCommandEvent
{
public:
    MunkHtmlLinkEvent() {}
    MunkHtmlLinkEvent(int id, const MunkHtmlLinkInfo &linkinfo)
        : wxCommandEvent(MunkEVT_COMMAND_HTML_LINK_CLICKED, id)
    {
        m_linkInfo = linkinfo;
    }

    const MunkHtmlLinkInfo &GetLinkInfo() const { return m_linkInfo; }

    // default copy ctor, assignment operator and dtor are ok
    virtual wxEvent *Clone() const { return new MunkHtmlLinkEvent(*this); }

private:
    MunkHtmlLinkInfo m_linkInfo;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(MunkHtmlLinkEvent)
};


typedef void (wxEvtHandler::*MunkHtmlCellEventFunction)(MunkHtmlCellEvent&);
typedef void (wxEvtHandler::*MunkHtmlLinkEventFunction)(MunkHtmlLinkEvent&);

#define MunkHtmlCellEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(MunkHtmlCellEventFunction, &func)
#define MunkHtmlLinkEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(MunkHtmlLinkEventFunction, &func)

#define MUNK_EVT_HTML_CELL_CLICKED(id, fn) \
    wx__DECLARE_EVT1(MunkEVT_COMMAND_HTML_CELL_CLICKED, id, MunkHtmlCellEventHandler(fn))
#define MUNK_EVT_HTML_CELL_HOVER(id, fn) \
    wx__DECLARE_EVT1(MunkEVT_COMMAND_HTML_CELL_HOVER, id, MunkHtmlCellEventHandler(fn))
#define MUNK_EVT_HTML_LINK_CLICKED(id, fn) \
    wx__DECLARE_EVT1(MunkEVT_COMMAND_HTML_LINK_CLICKED, id, MunkHtmlLinkEventHandler(fn))
#define MUNK_EVT_HTML_FORM_SUBMITTED(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        MunkEVT_COMMAND_HTML_FORM_SUBMITTED, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),



//-----------------------------------------------------------------------------
// MunkHtmlTableCell
//-----------------------------------------------------------------------------


struct colStruct
{
    int width, units;
            // width of the column either in pixels or percents
            // ('width' is the number, 'units' determines its meaning)
    int minWidth, maxWidth;
            // minimal/maximal column width. This is needed by HTML 4.0
            // layouting algorithm and can be determined by trying to
            // layout table cells with width=1 and width=infinity
    int leftpos, pixwidth, maxrealwidth;
            // temporary (depends on actual width of table)
};

enum cellState
{
    cellSpan,
    cellUsed,
    cellFree
};

struct cellStruct
{
    MunkHtmlContainerCell *cont;
    int colspan, rowspan;
    int minheight, valign;
    cellState flag;
    bool nowrap;
};


class MunkHtmlTableCell : public MunkHtmlContainerCell
{
protected:
    /* These are real attributes: */

    // should we draw borders or not?
    bool m_HasBorders;
    // number of columns; rows
    int m_NumCols, m_NumRows;
    // array of column information
    colStruct *m_ColsInfo;
    // 2D array of all cells in the table : m_CellInfo[row][column]
    cellStruct **m_CellInfo;
    // spaces between cells
    int m_Spacing;
    // cells internal indentation
    int m_Padding;

private:
    /* ...and these are valid only when parsing the table: */

    // number of actual column (ranging from 0..m_NumCols)
    int m_ActualCol, m_ActualRow;

    // default values (for table and row):
    wxColour m_tBkg, m_rBkg;
    wxString m_tValign, m_rValign;

    double m_PixelScale;


public:
    MunkHtmlTableCell(MunkHtmlContainerCell *parent, const MunkHtmlTag& tag, double pixel_scale = 1.0);
    virtual ~MunkHtmlTableCell();

    virtual void RemoveExtraSpacing(bool top, bool bottom);

    virtual void Layout(int w);

    void AddRow(const MunkHtmlTag& tag);
    void AddCell(MunkHtmlContainerCell *cell, const MunkHtmlTag& tag);

private:
    // Reallocates memory to given number of cols/rows
    // and changes m_NumCols/m_NumRows value to reflect this change
    // NOTE! You CAN'T change m_NumCols/m_NumRows before calling this!!
    void ReallocCols(int cols);
    void ReallocRows(int rows);

    // Computes minimal and maximal widths of columns. Needs to be called
    // only once, before first Layout().
    void ComputeMinMaxWidths();

    DECLARE_NO_COPY_CLASS(MunkHtmlTableCell)
};





///////////////////////////////////////////////////////////////////
//
//
// MunkHtmlParser, written by Ulrik Sandborg-Petersen.
//
//
//////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////
//
// MunkQDHTMLHandler
//
//////////////////////////////////////////////////////////


class MunkHtmlParsingStructure {
 public:
	MunkHtmlParsingStructure(MunkHtmlWindow *pParent);
	virtual ~MunkHtmlParsingStructure();

	// We do not own the FS
	virtual wxFileSystem *GetFS() { return m_pFS; };
	virtual void SetFS(wxFileSystem *pFS) { m_pFS = pFS; };

	// We do not own the DC
	virtual wxDC *GetDC(void) { return m_pDC; };
	virtual void SetDC(wxDC *pDC, double pixel_scale) { m_pDC = pDC; m_dblPixel_scale = pixel_scale; };

	virtual void SetHTMLBackgroundColour(const wxColour& bgcol);
	virtual wxColour GetHTMLBackgroundColour() const;

	double GetPixelScale(void) const { return m_dblPixel_scale; };

	// Returns pointer to conteiners/cells structure.
	// It should be used ONLY when printing
	virtual MunkHtmlContainerCell* GetInternalRepresentation() const { return m_Cell; }
	virtual void SetTopCell(MunkHtmlContainerCell *pCell) { m_Cell = pCell; };

	virtual bool Parse(const wxString& text, int nMagnification, std::string& error_message);


 protected:
	// This is pointer to the first cell in parsed data.  (Note: the first cell
	// is usually top one = all other cells are sub-cells of this one)
	MunkHtmlContainerCell *m_Cell;
 public:
	// This is used for forms
	MunkHtmlFormContainer *TakeOverForms();
	MunkHtmlFormContainer *m_pForms;
	MunkHtmlWindow *GetParentMunkHtmlWindow() { return m_pParentMunkHtmlWindow; };
 protected:
	
	
	// class for opening files (file system)
	wxFileSystem* m_pFS;
	wxDC *m_pDC;
	wxColour m_backgroundColour;
	double m_dblPixel_scale;
	MunkHtmlWindow *m_pParentMunkHtmlWindow;
};


enum eAnchorType {
	kATHREF,
	kATNAME
};


enum eWhiteSpaceKind {
	kWSKNormal,
	kWSKNowrap,
	kWSKPreLine,
	kWSKPre,
	kWSKPreWrap,
};

class MunkFontStringMetrics {
 public:
	long m_StringWidth;
	long m_StringHeight;
	long m_StringDescent;
	MunkFontStringMetrics(long StringWidth, long StringHeight, long StringDescent);
	~MunkFontStringMetrics();
	MunkFontStringMetrics(const MunkFontStringMetrics& other);
	MunkFontStringMetrics& operator=(const MunkFontStringMetrics& other);
 private:
	void assign(const MunkFontStringMetrics& other);
};

typedef std::map<std::string, MunkFontStringMetrics> String2MunkFontStringMetrics;

class MunkQDHTMLHandler : public MunkQDDocHandler {
	static wxRegEx m_regex_space_newline_space;
	static wxRegEx m_regex_space;
	static wxRegEx m_regex_linefeed;
	static wxRegEx m_regex_tab;
	static wxRegEx m_regex_space_spaces;


	MunkHtmlParsingStructure *m_pCanvas;
	form_id_t m_cur_form_id;
	std::string m_cur_form_select_name;
	wxDC *m_pDC;
	std::string m_chars;
	bool m_bInBody;
	MunkHtmlContainerCell *m_pCurrentContainer;
	MunkHtmlContainerCell *m_pBody;
	int m_CharHeight;
	int m_CharWidth;
	wxColour m_LinkColor;
	// basic font parameters.
	bool m_UseLink;
	// true if top of m_link_info_stack is not empty
	typedef std::stack<MunkHtmlLinkInfo> LinkInfoStack;
	LinkInfoStack m_link_info_stack;
	typedef std::stack<eAnchorType> AnchorTypeStack;
	AnchorTypeStack m_anchor_type_stack;
	typedef std::stack<bool> SmallCapsStack;
	SmallCapsStack m_smallcaps_stack;
	MunkFontAttributeStack m_HTML_font_attribute_stack;
	long m_Align;
	typedef std::map<std::string, wxFont*> String2PFontMap;
	String2PFontMap m_HTML_font_map;
	String2MunkFontStringMetrics m_FontSpaceCache; // Font characteristic string to MunkFontStringMetrics, currently only used for the string wxT(" ")
	int m_nMagnification;
	bool m_tmpLastWasSpace;
	wxChar *m_tmpStrBuf;
	size_t  m_tmpStrBufSize;
        // temporary variables used by AddText
	MunkHtmlWordCell *m_lastWordCell;
	std::string m_CurrentFontCharacteristicString;
	long m_CurrentFontSpaceWidth;
	long m_CurrentFontSpaceHeight;
	long m_CurrentFontSpaceDescent;

	// Table stuff
	typedef std::stack<MunkHtmlTableCell*> TableCellStack;
	TableCellStack m_tables_stack;
	typedef std::stack<wxString> TableAlignStack;
        TableAlignStack m_tAlignStack;
	wxString m_rAlign;
	typedef std::stack<std::pair<std::pair<long,bool>, MunkHtmlContainerCell*> > TableCellInfoStack;
	TableCellInfoStack m_table_cell_info_stack;

	// List stuff
	typedef std::stack<MunkHtmlListCell *> ListCellStack;
	int m_Numbering;
	ListCellStack m_list_cell_stack;
	typedef std::stack<std::pair<int, MunkHtmlContainerCell*> > ListCellInfoStack;
	ListCellInfoStack m_list_cell_info_stack;

	// Font stuff
        wxArrayString m_Faces;

	eWhiteSpaceKind m_current_white_space_kind;
 public:
	MunkQDHTMLHandler(MunkHtmlParsingStructure *pCanvas, int nMagnification);
	virtual ~MunkQDHTMLHandler();
	virtual void startElement(const std::string& tag, const MunkAttributeMap& attrs) throw(MunkQDException);
	virtual void endElement(const std::string& tag) throw(MunkQDException);
	virtual void startDocument(void) throw(MunkQDException);
	virtual void endDocument(void) throw(MunkQDException);
	virtual void text(const std::string& str) throw(MunkQDException);
 protected:
	MunkHtmlContainerCell *GetContainer() const { return m_pCurrentContainer; };
	MunkHtmlContainerCell *OpenContainer();
	MunkHtmlContainerCell *CloseContainer();
	MunkHtmlContainerCell *SetContainer(MunkHtmlContainerCell *pNewContainer);
	MunkHtmlWindowInterface *GetWindowInterface() { return (MunkHtmlWindowInterface*) m_pCanvas; };

	void AddHtmlTagCell(MunkMiniDOMTag *pMiniDOMTag);

	void handleChars(void);

	void AddText(const std::string& str);
	void DoAddText(wxChar *temp, int& templen, wxChar nbsp);
	void DoAddText(const wxString& txt, int& templen);


	int GetCharHeight() const {return m_CharHeight;}
	int GetCharWidth() const {return m_CharWidth;}

	void SetBackgroundImageAndBackgroundRepeat(const std::string& tag, 
						   const MunkAttributeMap& attrs,
						   const MunkHtmlTag& munkTag, wxString& css_style,
						   MunkHtmlContainerCell *pContainer);
	void SetBorders(const std::string& tag, 
			const MunkAttributeMap& attrs,
			const MunkHtmlTag& munkTag, 
			wxString& css_style,
			MunkHtmlContainerCell *pContainer);
	int GetActualFontSizeFactor() const;
	const wxColour& GetActualColor() const;
	MunkHtmlLinkInfo PopLink();
	void PushLink(const MunkHtmlLinkInfo& link);
	
	// applies current parser state (link, sub/supscript, ...) to given cell
	void ApplyStateToCell(MunkHtmlCell *cell);

	// creates font depending on m_font_attributes
	virtual wxFont* CreateCurrentFont();

	MunkHtmlScriptMode GetScriptMode() const;
	long GetScriptBaseline() const;
	bool GetFontUnderline(void) const;
	bool GetFontSmallCaps(void) const;

	// returns interface to the rendering window

	MunkHTMLFontAttributes startColor(const wxColour& clr);
	MunkHTMLFontAttributes startBold(void);
	MunkHTMLFontAttributes startEm(void);
	MunkHTMLFontAttributes startUnderline(void);	
	MunkHTMLFontAttributes startSmallCaps(void);
	MunkHTMLFontAttributes startSuperscript(int newBaseline);
	MunkHTMLFontAttributes startSubscript(int newBaseline);
	MunkHTMLFontAttributes startH1(void);
	MunkHTMLFontAttributes startH2(void);
	MunkHTMLFontAttributes startH3(void);
	MunkHTMLFontAttributes startAnchorHREF(bool bVisible, const wxColour& linkColour);
	MunkHTMLFontAttributes startAnchorNAME(void);
	MunkHTMLFontAttributes endTag(void);
	MunkHTMLFontAttributes topFontAttributeStack(void);
	wxFont *getFontFromMunkHTMLFontAttributes(const MunkHTMLFontAttributes& font_attributes, bool bUseCacheMap, const std::string& characteristic_string);
	void startMunkHTMLFontAttributeStack(void);
	void SetCharWidthHeight(void);
	void ChangeMagnification(int magnification);

	int GetAlign() const {return m_Align;}
	void SetAlign(int a) {m_Align = a;}
};


extern bool ReadString(wxString& str, wxInputStream* s, wxMBConv& conv);

#endif // ifndef _MUNK_HTML_H_
