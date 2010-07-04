/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/htmltag.cpp
// Purpose:     MunkHtmlTag class (represents single tag)
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmltag.cpp 40777 2006-08-23 19:07:15Z VS $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WXPRECOMP
    #include "wx/dynarray.h"
    #include "wx/brush.h"
    #include "wx/colour.h"
    #include "wx/dc.h"
    #include "wx/settings.h"
    #include "wx/module.h"
    #include "wx/scrolwin.h"
    #include "wx/timer.h"
    #include "wx/dcmemory.h"
    #include "wx/log.h"
    #include "wx/math.h"
    #include "wx/image.h"
    #include "wx/artprov.h"
    #include "wx/sstream.h"
    #include "wx/log.h"
    #include "wx/sizer.h"
#endif


#include "munkhtml.h"
//#include <string_func.h>


#include <fstream>

// #include "wx/html/htmlpars.h"
#include <stdio.h> // for vsscanf
#include <stdarg.h>




#ifdef __WXMAC__
#define DEFAULT_FONT_SIZE   (14)
#else
#define DEFAULT_FONT_SIZE   (13)
#endif



/*
 * This hack is necessary because:
 *
 * - Visual C++ doesn't support std::map<_K,_V>::at().
 *
 * - The MunkAttributeMap formal parameter is always const (and rightly so!)
 * 
 * - We can't use [] because it is not const.
 *
 * - Hence, we need another method.  This seems cleanest.
 *
 */
std::string getMunkAttribute(const MunkAttributeMap& attrs, const std::string& key) throw(MunkQDException)
{
	MunkAttributeMap::const_iterator ci = attrs.find(key);
	if (ci == attrs.end()) {
		throw MunkQDException("Could not find attribute with name '" + key + "'");
	} else {
		return ci->second;
	}
}


/** Split a string into a list of strings, based on a string of
 * characters to split on.
 *
 * The characters are not included in the split strings.
 *
 * If none of the characters are found, then the list containing
 * instring is returned in outlist.
 *
 * @param instring The input string.
 *
 * @param splitchars The string of characters on which to split.
 *
 * @param outlist The output list of strings.*/
void munk_split_string(const std::string& instring, const std::string& splitchars, std::list<std::string>& outlist)
{
	if (instring.find_first_not_of(splitchars, 0) == std::string::npos) {
		// Add instring as sole output in list
		outlist.push_back(instring);
		return;
	} else {
		std::string::size_type nbegin, nend;
		nbegin = instring.find_first_not_of(splitchars, 0);
		nend = instring.find_first_of(splitchars, nbegin);
		if (nend == std::string::npos) {
			outlist.push_back(instring.substr(nbegin, instring.length()-nbegin));
		} else {
			outlist.push_back(instring.substr(nbegin, nend-nbegin));
			nbegin = instring.find_first_not_of(splitchars, nend);
			while (nbegin != std::string::npos
			       && nend != std::string::npos) {
				nend = instring.find_first_of(splitchars, nbegin);
				if (nend == std::string::npos) {
					outlist.push_back(instring.substr(nbegin, instring.length()-nbegin));
					break;
				} else {
					outlist.push_back(instring.substr(nbegin, nend-nbegin));
				}
				nbegin = instring.find_first_not_of(splitchars, nend);
			}
		}
	}
}




/** Convert a long to a string.
 *
 * @param l The long to convert.
 *
 * @return The input long, converted to a string (base 10).
 */
std::string munk_long2string(long l)
{
	// Some ideas taken from http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
	char szResult[30];

	// Build up string in reverse
	char *p = szResult;
	long v = l;
	do {
		long tmp = v;
		v /= 10;
		*p++ = "9876543210123456789" [9+(tmp - v*10)];
	} while (v);

	// Add negative sign at the end if necessary	
	if (l < 0) {
		*p++ = '-';
	}

	// Terminate string
	*p-- = '\0';

	// Reverse string
	char *q = szResult;
	while (q < p) {
		char swap_c = *p;
		*p-- = *q;
		*q++ = swap_c;
	}

	return szResult;
}


/** Convert a single Unicode character (a long) to UTF-8.
 *
 * "Stolen" from utf.c in SQLite 3, which is Public Domain code.
 *
 * @param input the Unicode character
 *
 * @param output The input encoded as a single UTF-8 character.
 */
void munk_long2utf8(long c, std::string& output)
{
	output.clear();
	if (c<0x00080) {
		output += (c&0xFF);
	} else if (c<0x00800) {
		output += (0xC0 + ((c>>6)&0x1F));
		output += (0x80 + (c & 0x3F));
	} else if (c<0x10000) {
		output += (0xE0 + ((c>>12)&0x0F));
		output += (0x80 + ((c>>6) & 0x3F));
		output += (0x80 + (c & 0x3F));
	} else {
		output += (0xF0 + ((c>>18) & 0x07));
		output += (0x80 + ((c>>12) & 0x3F));
		output += (0x80 + ((c>>6) & 0x3F));
		output += (0x80 + (c & 0x3F));
	}
}





/** Check whether input string ends with end string.
 *
 * @param input The string to test
 *
 * @param end The string which may or may not be at the end of the
 * input string.
 *
 * @return True if input ends with end , false otherwise.
 */
bool munk_string_ends_with(const std::string& input, const std::string& end)
{
	// Does not work with UTF-8 in end...
	std::string::size_type input_length = input.length();
	std::string::size_type end_length = end.length();
	if (end_length > input_length) {
		return false;
	} else {
		std::string end_string = input.substr(input_length - end_length , end_length);
		return end_string == end;
	}
}

/** Convert a string to a long.
 *
 * @param str The input string, base 10.
 *
 * @return The input string, converted to a long.
 */
long munk_string2long(const std::string& str)
{
	return strtol(str.c_str(), (char **)NULL, 10);
}


/** Check whether a string is a hexadecimal string.
 *
 * This just checks whether the string only contains [0-9A-Fa-f].
 *
 * @param str The string to check for hexadecimal-hood.
 *
 * @return True iff str is a hexadecimal string.
 */
bool munk_is_hex(const std::string& str)
{
	const std::string hex_digits = "0123456789abcdefABCDEF";
	return str != "" && str.find_first_not_of(hex_digits) == std::string::npos;
}

/** Returns the long value of an arbitrary hex string.
 *
 * The input is assumed to have been sanity-checked before hand, e.g.,
 *  with is_hex().
 *
 * @param str The string to convert
 *
 * @return The long representing the input hex string.
 */
long munk_hex2long(const std::string& str)
{
	long result = 0;
	std::string::size_type strlength = str.length();
	bool bHasMetFirstNonZero = false;
	for (std::string::size_type i = 0;
	     i < strlength;
	     ++i) {
		char c = str[i];
		// Skip leading zeros
		if (c != '0') {
			bHasMetFirstNonZero = true;
		}
		if (bHasMetFirstNonZero) {
			long tmp = 0;
			if (c >= '0'
			    && c <= '9') {
				tmp = c - '0';
			} else if (c >= 'a'
				   && c <= 'f') {
				tmp = c - ('a' - 10);
			} else if (c >= 'A'
				   && c <= 'F') {
				tmp = c - ('A' - 10);
			}
			result <<= 4;
			result |= tmp;
		}
	}
	return result;
}




/** Compare two strings case-INsensitively.
 *
 * See Bjarne Stroustrup, 
 * The C++ Programming Language,
 * 3rd Edition, 1997
 * p. 591
 *
 * The standard C "tolower" function is used to evaluate
 * string-sensitivity.
 *
 * @param str1 The first string.
 *
 * @param str2 The second string.
 *
 * @return 0 if they are equal, -1 if str1 < str2, 1 if str1 > str2,
 * all less case-sensitivity.
 */

int munk_strcmp_nocase(const std::string& str1, const std::string& str2)
{
	std::string::const_iterator p = str1.begin();
	std::string::const_iterator str1end = str1.end();
	std::string::const_iterator q = str2.begin();
	std::string::const_iterator str2end = str2.end();
	while (p != str1end && q != str2end) {
		unsigned char tolop = tolower(*p);
		unsigned char toloq = tolower(*q);
		if (tolop != toloq)
			return (tolop < toloq) ? -1 : 1;
		++p;
		++q;
	}
	std::string::size_type str1size = str1.size();
	std::string::size_type str2size = str2.size();
	return (str2size == str1size) ? 0 : (str1size < str2size) ? -1 : 1;
}


/** Convert a string to an eMunkCharsets enumeration.
 *
 * Find out which eMunkCharsets enumeration constant most closely matches
 * a given string.
 *
 *
 * @param input The string-representation (probably from the user)
 * which is to be converted to eMunkCharsets.
 *
 * @param charset The output charset.  Is only defined if the function
 * returns true.
 *
 * @return True iff we were able to recognize and thus convert the
 * charset.
 *
 */
bool munk_string2charset(const std::string& input, eMunkCharsets& charset)
{
	if (munk_strcmp_nocase(input, "ascii") == 0) {
		charset = kMCSASCII;       /**< 7-bit ASCII.  */
		return true;
	} else if (munk_strcmp_nocase(input, "utf8") == 0
		   || munk_strcmp_nocase(input, "utf-8") == 0) {
 		charset = kMCSUTF8;        /**< UTF-8. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-1") == 0
		   || munk_strcmp_nocase(input, "iso_8859_1") == 0
		   || munk_strcmp_nocase(input, "iso-8859_1") == 0
		   || munk_strcmp_nocase(input, "iso-8859-1") == 0) {
		charset = kMCSISO_8859_1;  /**< ISO-8859-1. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-2") == 0
		   || munk_strcmp_nocase(input, "iso_8859_2") == 0
		   || munk_strcmp_nocase(input, "iso-8859_2") == 0
		   || munk_strcmp_nocase(input, "iso-8859-2") == 0) {
		charset = kMCSISO_8859_2;  /**< ISO-8859-2. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-3") == 0
		   || munk_strcmp_nocase(input, "iso_8859_3") == 0
		   || munk_strcmp_nocase(input, "iso-8859_3") == 0
		   || munk_strcmp_nocase(input, "iso-8859-3") == 0) {
		charset = kMCSISO_8859_3;  /**< ISO-8859-3. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-4") == 0
		   || munk_strcmp_nocase(input, "iso_8859_4") == 0
		   || munk_strcmp_nocase(input, "iso-8859_4") == 0
		   || munk_strcmp_nocase(input, "iso-8859-4") == 0) {
		charset = kMCSISO_8859_4;  /**< ISO-8859-4. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-5") == 0
		   || munk_strcmp_nocase(input, "iso_8859_5") == 0
		   || munk_strcmp_nocase(input, "iso-8859_5") == 0
		   || munk_strcmp_nocase(input, "iso-8859-5") == 0) {
		charset = kMCSISO_8859_5;  /**< ISO-8859-5. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-6") == 0
		   || munk_strcmp_nocase(input, "iso_8859_6") == 0
		   || munk_strcmp_nocase(input, "iso-8859_6") == 0
		   || munk_strcmp_nocase(input, "iso-8859-6") == 0) {
		charset = kMCSISO_8859_6;  /**< ISO-8859-6. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-7") == 0
		   || munk_strcmp_nocase(input, "iso_8859_7") == 0
		   || munk_strcmp_nocase(input, "iso-8859_7") == 0
		   || munk_strcmp_nocase(input, "iso-8859-7") == 0) {
		charset = kMCSISO_8859_7;  /**< ISO-8859-7. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-8") == 0
		   || munk_strcmp_nocase(input, "iso_8859_8") == 0
		   || munk_strcmp_nocase(input, "iso-8859_8") == 0
		   || munk_strcmp_nocase(input, "iso-8859-8") == 0) {
		charset = kMCSISO_8859_8;  /**< ISO-8859-8. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-9") == 0
		   || munk_strcmp_nocase(input, "iso_8859_9") == 0
		   || munk_strcmp_nocase(input, "iso-8859_9") == 0
		   || munk_strcmp_nocase(input, "iso-8859-9") == 0) {
		charset = kMCSISO_8859_9;  /**< ISO-8859-9. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-10") == 0
		   || munk_strcmp_nocase(input, "iso_8859_10") == 0
		   || munk_strcmp_nocase(input, "iso-8859_10") == 0
		   || munk_strcmp_nocase(input, "iso-8859-10") == 0) {
		charset = kMCSISO_8859_10;  /**< ISO-8859-10. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-13") == 0
		   || munk_strcmp_nocase(input, "iso_8859_13") == 0
		   || munk_strcmp_nocase(input, "iso-8859_13") == 0
		   || munk_strcmp_nocase(input, "iso-8859-13") == 0) {
		charset = kMCSISO_8859_13;  /**< ISO-8859-13. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-14") == 0
		   || munk_strcmp_nocase(input, "iso_8859_14") == 0
		   || munk_strcmp_nocase(input, "iso-8859_14") == 0
		   || munk_strcmp_nocase(input, "iso-8859-14") == 0) {
		charset = kMCSISO_8859_14;  /**< ISO-8859-14. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-15") == 0
		   || munk_strcmp_nocase(input, "iso_8859_15") == 0
		   || munk_strcmp_nocase(input, "iso-8859_15") == 0
		   || munk_strcmp_nocase(input, "iso-8859-15") == 0) {
		charset = kMCSISO_8859_15;  /**< ISO-8859-15. */
		return true;
	} else if (munk_strcmp_nocase(input, "iso_8859-16") == 0
		   || munk_strcmp_nocase(input, "iso_8859_16") == 0
		   || munk_strcmp_nocase(input, "iso-8859_16") == 0
		   || munk_strcmp_nocase(input, "iso-8859-16") == 0) {
		charset = kMCSISO_8859_16;  /**< ISO-8859-16. */
		return true;
	} else {
		return false;
	}
}




///////////////////////////////////////////////////////////////
//
// MunkQDParser
//
///////////////////////////////////////////////////////////////



const std::string MunkQDParser::NOTAG = "<>";

std::string MunkQDParser::state2string(eMunkQDStates e)
{
	if (e == TEXT) {
		return "TEXT";
	} else if (e == ENTITY) {
		return "ENTITY";
	} else if (e == OPEN_TAG) {
		return "OPEN_TAG";
	} else if (e == CLOSE_TAG) {
		return "CLOSE_TAG";
	} else if (e == START_TAG) {
		return "START_TAG";
	} else if (e == ATTRIBUTE_LVALUE) {
		return "ATTRIBUTE_LVALUE";
	} else if (e == ATTRIBUTE_EQUAL) {
		return "ATTRIBUTE_EQUAL";
	} else if (e == ATTRIBUTE_RVALUE) {
		return "ATTRIBUTE_RVALUE";
	} else if (e == QUOTE) {
		return "QUOTE";
	} else if (e == IN_TAG) {
		return "IN_TAG";
	} else if (e == SINGLE_TAG) {
		return "SINGLE_TAG";
	} else if (e == COMMENT) {
		return "COMMENT";
	} else if (e == DONE) {
		return "DONE";
	} else if (e == DOCTYPE) {
		return "DOCTYPE";
	} else if (e == IN_XMLDECLARATION) {
		return "IN_XMLDECLARATION";
	} else if (e == BEFORE_XMLDECLARATION) {
		return "BEFORE_XMLDECLARATION";
	} else if (e == OPEN_XMLDECLARATION) {
		return "OPEN_XMLDECLARATION";
	} else if (e == CDATA) {
		return "CDATA";
	} else {
		std::ostringstream str;
		str << "Unknown state: " << (int) e;
		throw MunkQDException(str.str());
	}
}
	


void MunkQDParser::parse(MunkQDDocHandler *pDH, std::istream *pStream) throw(MunkQDException) 
{
	bot = tok = ptr = cur = pos = lim = top = eof = 0;
	m_pInStream = pStream;
	m_pDH = pDH;
	m_tag_depth = 0;
	eMunkQDStates state = BEFORE_XMLDECLARATION;
	m_encoding = kMCSUTF8; // This is the default for XML
	m_tag_name = NOTAG;
	m_line = 1;
	m_column = 0;
	m_pDH->startDocument();
	m_end_of_line = false;

	char c = getNextChar();

	while (hasMoreInput()) {
		//std::cerr << "UP200: state = " << state2string(state) << ", c = '" << c << "'" << std::endl;

		if (state == TEXT) {
			// We are collecting text between tags.
			if (c == '&') {
				pushState(TEXT);
				m_entity = "";
				state = ENTITY;
			} else if (c == '<') {
				if (m_text.length() > 0) {
					m_pDH->text(m_text);
					m_text = "";
				}
				pushState(TEXT);
				state = START_TAG;
				m_tag_name = "";
			} else {
				m_text += c;
			}
		} else if (state == BEFORE_XMLDECLARATION) {
			if (c == '<') {
				c = getNextChar();
				if (c == '?') {
					c = getNextChar();
					if (c != 'x') {
						except("Document must start with '<?x'. It does not.");
					}
					c = getNextChar();
					if (c != 'm') {
						except("Document must start with '<?xm'. It does not.");
					}
					c = getNextChar();
					if (c != 'l') {
						except("Document must start with '<?xml'. It does not.");
					}
					c = getNextChar();
					if (!myisspace(c)) {
						except("Document must start with '<?xml' + <whitespace>. It does not.");
					}
					state = IN_XMLDECLARATION;
				} else {
					except("Document must start with <?; it does not.");
				}
			}
		} else if (state == IN_XMLDECLARATION) {
			// We are inside <?xml .... ?>
			if (c == '?') {
				c = getNextChar();
				if (c != '>') {
					except("XML declaration must end with '?>'");
				} else {
					if (m_attributes.find("encoding") != m_attributes.end()) {
						std::string strencoding = m_attributes["encoding"];
						eMunkCharsets tmpcharset;
						if (munk_string2charset(strencoding, tmpcharset)) {
							m_encoding = tmpcharset;
						} else {
							// Use UTF-8 if charset unknown.
							// FIXME: Should we do an exception?
							m_encoding = kMCSUTF8;
						}
					} else {
						m_encoding = kMCSUTF8;
					}

					state = TEXT;
				}
			} else if (myisspace(c)) {
				; // Eat up
			} else {
				pushState(IN_XMLDECLARATION);
				state = ATTRIBUTE_LVALUE;
				m_lvalue = "";
				m_rvalue = "";
				m_lvalue += c;
			}
		} else if (state == ATTRIBUTE_LVALUE) {
			if (c == '=') {
				state = ATTRIBUTE_RVALUE;
			} else if (myisspace(c)) {
				state = ATTRIBUTE_EQUAL;
			} else {
				m_lvalue += c;
			}
		} else if (state == ATTRIBUTE_EQUAL) {
			if (c == '=') {
				state = ATTRIBUTE_RVALUE;
			} else if (myisspace(c)) {
				; // Eat up
			} else {
				except("Error while processing attribute: After lvalue, there should only be whitespace or '='.");
			}
		} else if (state == ATTRIBUTE_RVALUE) {
			if (c == '\'' || c == '\"') {
				m_quote_char = c;
				state = QUOTE;
			} else if (myisspace(c)) {
				; // Eat up
			} else {
				except("Error while processing attribute: After =, there should only be whitespace or '\\'' or '\\\"'.");
			}
		} else if (state == QUOTE) {
			if (c == '\t' || c == '\n' || c == '\r') {
				// See XML Spec section 3.3.3
				// on normalization.
				m_rvalue += ' ';
			} else if (c == '&') {
				pushState(QUOTE);
				state = ENTITY;
				m_entity = "";
			} else if (c == m_quote_char) {
				m_attributes.insert(MunkAttributeMap::value_type(m_lvalue, m_rvalue));
				state = popState();
			} else {
				m_rvalue += c;
			}
		} else if (state == ENTITY) {
			if (c == ';') {
				state = popState();
				handle_entity(state);
			} else {
				m_entity += c;
			}
		} else if (state == IN_TAG) {
			if (c == '/') {
				state = SINGLE_TAG;
			} else if (c == '>') {
				++m_tag_depth;
				m_pDH->startElement(m_tag_name, m_attributes);
				eraseAttributes();
				m_tag_name = "";
				state = popState();
			} else if (myisspace(c)) {
				; // Eat up
			} else {
				pushState(IN_TAG);
				state = ATTRIBUTE_LVALUE;
				m_lvalue = "";
				m_rvalue = "";
				m_lvalue += c;
			}
		} else if (state == START_TAG) {
			// The last char was '<'. Now
			// find out whether we are doing <starttag>,
			// </endtag>, <!-- comment -->,
			// <!DOCTYPE, or <![CDATA
			eMunkQDStates topState = popState();
			if (c == '/') {
				pushState(topState);
				state = CLOSE_TAG;
			} else if (c == '?') {
				pushState(topState);
				state = OPEN_XMLDECLARATION;
				eraseAttributes();
			} else {
				pushState(topState);
				state = OPEN_TAG;
				m_tag_name = c;
				eraseAttributes();
			}
		} else if (state == OPEN_TAG) {
			if (c == '/') {
				state = SINGLE_TAG;
			} else if (c == '>') {
				++m_tag_depth;
				m_pDH->startElement(m_tag_name, m_attributes);
				eraseAttributes();
				m_tag_name = "";
				state = popState();
			} else if (myisspace(c)) {
				state = IN_TAG;
			} else {
				m_tag_name += c;
				if (c == '-' && m_tag_name == "!--") {
					pushState(TEXT);
					state = COMMENT;
				} else if (c == '[' && m_tag_name == "![CDATA[") {
					m_text = "";
					pushState(TEXT);
					state = CDATA;
				} else if (c == 'E' && m_tag_name == "!DOCTYPE") {
					pushState(TEXT);
					state = DOCTYPE;
					m_text = "";
					m_tag_name = "";
				} 
			}
		} else if (state == COMMENT) {
			if (c == '>') {
				m_text += c;
				if (munk_string_ends_with(m_text, "-->")) {
					m_pDH->comment(m_text);

					m_text = "";
					state = popState();
				} else {
					; // Nothing to do
				}
			} else {
				m_text += c;
			}
		} else if (state == CDATA) {
			if (c == '>') {
				if (munk_string_ends_with(m_text, "]]")) {
					m_pDH->text(m_text.substr(0, m_text.length() - 2));
					state = popState();
					m_text = "";
				} else {
					m_text += c;
				}
			} else {
				m_text += c;
			}
		} else if (state == DOCTYPE) {
			m_tag_name += c;
			if (c == '<') {
				m_tag_name = "";
			} else if (c == '-' && m_tag_name == "!--") {
				pushState(DOCTYPE);
				state = COMMENT;
			} else if (c == ']') {
				m_tag_name = c;
			} else if (c == '>') {
				if (m_tag_name == "]>") {
					// Is TEXT
					state = popState();
				}
			} else {
				; // Eat up
			}
		} else if (state == CLOSE_TAG) {
			// We have seen '</'
			if (c == '>') {
				m_pDH->endElement(m_tag_name);
				state = popState();
				--m_tag_depth;
				if (m_tag_depth == 0) {
					state = DONE;
				}
				m_tag_name = "";
			} else {
				m_tag_name += c;
			}
			
		} else if (state == SINGLE_TAG) {
			// We are have seen 
			// something like
			// <barefoot attr1="value" ... /
			// Now we need the final >
			if (c != '>') {
				except("While processing single tag: Expected > for tag: <" + m_tag_name + "/>");
			} else {
				m_pDH->startElement(m_tag_name, m_attributes);
				m_pDH->endElement(m_tag_name);
				if (m_tag_depth == 0) {
					state = DONE;
				} else {
					state = popState();
					m_tag_name = "";
					eraseAttributes();
				}
			}
		} else if (state == DONE) {
			eraseAttributes();
			m_pDH->endDocument();
			cleanUp();
			return;
		} else {
			except(std::string("Error: Unknown state: ") + state2string(state));
		}

		// Read next char
		c = getNextChar();	
	}
	
	if (state == DONE) {
		m_pDH->endDocument();
	} else {
		except("missing end tag");
	}
	eraseAttributes();
	cleanUp();
}
	
void MunkQDParser::except(const std::string& s) throw(MunkQDException)
{
	std::ostringstream ostr;
	ostr << s << " near line " << m_line << ", column " << m_column << '!' << std::endl;
	throw MunkQDException(ostr.str());
}

void MunkQDParser::handle_entity(eMunkQDStates state) throw(MunkQDException)
{
	if (m_entity.length() > 0 && m_entity[0] == '#') {
		long l = 0;
		if (m_entity.length() > 1
		    && (m_entity[1] == 'x' 
			|| m_entity[1] == 'X')) {
			std::string hexstring = m_entity.substr(2, std::string::npos);
			if (munk_is_hex(hexstring)) {
				l = munk_hex2long(hexstring);
			} else {
				except("Invalid hex-string in entity &" + m_entity + ";.");
			}
		} else {
			std::string decimalstring = m_entity.substr(1, std::string::npos);
			l = munk_string2long(decimalstring);
		}

		if (m_encoding == kMCSUTF8) {
			std::string utf8str;
			munk_long2utf8(l, utf8str);
			std::string::size_type i;
			std::string::size_type utf8length = utf8str.length();
			for (i = 0; i < utf8length; ++i) {
				if (state == QUOTE) {
					m_rvalue += utf8str[i];
				} else {
					m_text += utf8str[i];
				}
			}
		} else {
			if (l <= 255) {
				if (state == QUOTE) {
					m_rvalue += (char) l;
				} else {
					m_text += (char) l;
				}
			} else {
				except("Entity &" + m_entity + "; out of range; document is non-UTF8, and so we can handle only single-byte data.");
			}
		}
	} else {
		char entity_char = '\0';
		if (m_entity == "lt") {
			entity_char = '<';
		} else if (m_entity == "gt") {
			entity_char = '>';
		} else if (m_entity == "amp") {
			entity_char = '&';
		} else if (m_entity == "quot") {
			entity_char = '"';
		} else if (m_entity == "apos") {
			entity_char = '\'';
		} else {
			except("Unknown entity: &"+m_entity+";");
		}
		
		if (state == QUOTE) {
			m_rvalue += entity_char;
		} else {
			m_text += entity_char;
		}
	}
}


char MunkQDParser::getNextChar(void) throw(MunkQDException)
{
	char c;
	if ((lim - cur) < 1) fillBuffer();
	c = *cur;
	++cur;
	if (c == '\r') {
		m_end_of_line = true;
		m_column = 0;
		++m_line;
		c = '\n';
	} else if (m_end_of_line && c == '\n') {
		// We have already read \r, and this is \n that follows.
		// \r returned \n, since we need to translate
		// all of \r\n, \r, and \n to \n 
		// (see Section 2.11 of the XML spec)
		// So now we return the next character
		m_pInStream->get(c);
		m_end_of_line = false;
	} else if (c == '\n') {
		m_column = 0;
		++m_line;
	} else {
		m_end_of_line = false; // We might be doing a file with \r as the end-of-line marker.
		++m_column;
	}
	return c;
}

#define BSIZE   (1024*1024)


void MunkQDParser::fillBuffer()
{
        if(!eof)
        {
                unsigned int cnt = tok - bot;
		/*
		std::cerr << "UP205: "
			  << " cnt = " << cnt 
			  << " tok = " << (void*) tok
			  << " bot = " << (void*) bot 
			  << " cur = " << (void*) cur
			  << " eof = " << (void*) eof 
			  << " ptr = " << (void*) ptr
			  << " lim = " << (void*) lim
			  << std::endl;
		*/
                if(cnt)
                {
                        memcpy(bot, tok, lim - tok);
                        tok = bot;
                        ptr -= cnt;
                        cur -= cnt;
                        pos -= cnt;
                        lim -= cnt;
                }
                if((top - lim) < BSIZE)
                {
                        char *buf = new char[(lim - bot) + BSIZE];
                        memcpy(buf, tok, lim - tok);
                        tok = buf;
                        ptr = &buf[ptr - bot];
                        cur = &buf[cur - bot];
                        pos = &buf[pos - bot];
                        lim = &buf[lim - bot];
                        top = &lim[BSIZE];
                        delete [] bot;
                        bot = buf;
                }
                m_pInStream->read(lim, BSIZE);
                if ((cnt = m_pInStream->gcount()) != BSIZE )
                {
                        eof = &lim[cnt]; *eof++ = '\0';
                }
                lim += cnt;
        }
}

MunkQDParser::MunkQDParser()
{
	bot = tok = ptr = cur = pos = lim = top = eof = 0;
}

MunkQDParser::~MunkQDParser()
{
	cleanUp();
}







MunkHtmlParsingStructure::MunkHtmlParsingStructure(MunkHtmlWindow *pParent)
{
	m_pParentMunkHtmlWindow = pParent;
	m_Cell = 0;
	m_pFS = 0;
	m_pDC = 0;
	m_pForms = 0;
}


MunkHtmlParsingStructure::~MunkHtmlParsingStructure()
{
	delete m_Cell;
	delete m_pForms;
}


void MunkHtmlParsingStructure::SetHTMLBackgroundColour(const wxColour& bgcol)
{
	m_backgroundColour = bgcol;
}


wxColour MunkHtmlParsingStructure::GetHTMLBackgroundColour() const
{
	return m_backgroundColour;
	
}

MunkHtmlFormContainer *MunkHtmlParsingStructure::TakeOverForms()
{
	MunkHtmlFormContainer *pResult = m_pForms;
	m_pForms = 0;
	return pResult;
}


bool MunkHtmlParsingStructure::Parse(const wxString& text, std::string& error_message)
{
	bool bResult = true;
	error_message = "";
	try {
	    
		std::istringstream istr(std::string((const char*)text.mb_str(wxConvUTF8)));

		MunkQDHTMLHandler dh(this);
	    
		MunkQDParser parser;
		parser.parse(&dh, &istr);
	} catch (MunkQDException e) {
		bResult = false;
		error_message = e.what();
	} catch (...) {
		bResult = false;
		error_message = "An unknown exception occurred while parsing HTML.";
	}
	return bResult;
}




//////////////////////////////////////////////////////////////////
//
// MunkHTMLFontAttributes
//
//////////////////////////////////////////////////////////////////


MunkHTMLFontAttributes::MunkHTMLFontAttributes()
	: m_bBold(false),
	  m_bItalic(false),
	  m_bUnderline(false),
	  m_scriptBaseLine(0),
	  m_scriptMode(MunkHTML_SCRIPT_NORMAL),
	  m_sizeFactor(100),
	  m_color(*wxBLACK),
	  m_face(wxT("Arial"))
{
}

MunkHTMLFontAttributes::MunkHTMLFontAttributes(bool bBold, bool bItalic, bool bUnderline, long scriptBaseline, MunkHtmlScriptMode scriptMode, unsigned int sizeFactor, const wxColour& color, const wxString& face)
	: m_bBold(bBold),
	  m_bItalic(bItalic),
	  m_bUnderline(bUnderline),
	  m_scriptBaseLine(scriptBaseline),
	  m_scriptMode(scriptMode),
	  m_sizeFactor(sizeFactor),
	  m_color(color),
	  m_face(face)
{
}


MunkHTMLFontAttributes::MunkHTMLFontAttributes(const MunkHTMLFontAttributes& other)
{
	copy_to_self(other);
}


MunkHTMLFontAttributes::~MunkHTMLFontAttributes()
{
}

MunkHTMLFontAttributes MunkHTMLFontAttributes::fromString(const std::string& s)
{
	bool bBold = false;
	if (s[0] == 'B') {
		bBold = true;
	}

	bool bItalic = false;
	if (s[1] == 'I') {
		bItalic = true;
	}

	bool bUnderline = false;
	if (s[2] == 'U') {
		bUnderline = true;
	}

	MunkHtmlScriptMode scriptMode = MunkHTML_SCRIPT_NORMAL;
	switch (s[3]) {
	case 'N':
		scriptMode = MunkHTML_SCRIPT_NORMAL;
		break;
	case 'B':
		scriptMode = MunkHTML_SCRIPT_SUB;
		break;
	case 'P':
		scriptMode = MunkHTML_SCRIPT_SUP;
		break;
	}

	std::string number;
	std::string::size_type index = 4;
	while (s[index] != '|') {
		number += s[index];
		++index;
	}

	long scriptBaseLine = munk_string2long(number);

	number = "";
	++index; // Skip '|'

	while (s[index] != '#') {
		number += s[index];
		++index;
	}

	long sizeFactor = munk_string2long(number);

	++index; // Skip '#'

	std::string face_name = s.substr(index, std::string::npos);
	wxString strFace = wxString(face_name.c_str(), wxConvUTF8);

	MunkHTMLFontAttributes attr(bBold, bItalic, bUnderline, scriptBaseLine, scriptMode, sizeFactor, *wxBLACK, strFace);

	return attr;
}


std::string MunkHTMLFontAttributes::toString() const
{
	std::string result;
	if (m_bBold) {
		result += 'B';
	} else {
		result += '-';
	}

	if (m_bItalic) {
		result += 'I';
	} else {
		result += '-';
	}

	if (m_bUnderline) {
		result += 'U';
	} else {
		result += '-';
	}

	switch (m_scriptMode) {
	case MunkHTML_SCRIPT_NORMAL:
		result += 'N';
		break;
	case MunkHTML_SCRIPT_SUB:
		result += 'B';
		break;
	case MunkHTML_SCRIPT_SUP:
		result += 'P';
		break;
	}

	result += munk_long2string(m_scriptBaseLine);
	result += '|';
	result += munk_long2string(m_sizeFactor);
	
	result += "#";
	result += m_face.mb_str(wxConvUTF8);

	return result;
}


long MunkHTMLFontAttributes::toLong() const
{
	long result = m_sizeFactor;

	result *= 1024;
	if (m_bBold) {
		result += 1;
	}

	result *= 2;
	if (m_bItalic) {
		result += 1;
	}

	// This is not reflected in the fonts in String2PFontTrie,
	// so it should also be left out here...
	result *= 2;
	if (m_bUnderline) {
		result += 1;
	}

	result *= 4;
	switch (m_scriptMode) {
	case MunkHTML_SCRIPT_NORMAL:
		result += 0;
		break;
	case MunkHTML_SCRIPT_SUB:
		result += 1;
	case MunkHTML_SCRIPT_SUP:
		result += 2;
	}
	
	// baseline
	result *= 1024;
	result += (m_scriptBaseLine + 512) % 1024;

	return result;
}



MunkHTMLFontAttributes& MunkHTMLFontAttributes::operator=(const MunkHTMLFontAttributes& other)
{
	copy_to_self(other);
	return *this;
}


void MunkHTMLFontAttributes::copy_to_self(const MunkHTMLFontAttributes& other)
{
	m_bBold = other.m_bBold;
	m_bItalic = other.m_bItalic;
	m_bUnderline = other.m_bUnderline;
	m_scriptBaseLine = other.m_scriptBaseLine;
	m_scriptMode = other.m_scriptMode;
	m_sizeFactor = other.m_sizeFactor;
	m_color = other.m_color;
	m_face = other.m_face;
}







//-----------------------------------------------------------------------------
// MunkHtmlTag
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS(MunkHtmlTag,wxObject)

MunkHtmlTag::MunkHtmlTag(const wxString& source, const MunkAttributeMap& attrs) : wxObject()
{
	m_Name = wxString(source.c_str(), wxConvUTF8);
	m_Name.MakeUpper();

	MunkAttributeMap::const_iterator ci = attrs.begin();
	while (ci != attrs.end()) {
		std::string attrname = ci->first;
		std::string attrvalue = ci->second;

		wxString strAttrName = wxString(attrname.c_str(), wxConvUTF8);
		strAttrName.MakeUpper();
		wxString strAttrValue = wxString(attrvalue.c_str(), wxConvUTF8);
		strAttrValue.MakeUpper();


		m_ParamNames.Add(strAttrName);
		m_ParamValues.Add(strAttrValue);

		++ci;
	}
}

MunkHtmlTag::~MunkHtmlTag()
{
}

bool MunkHtmlTag::HasParam(const wxString& par) const
{
    return (m_ParamNames.Index(par, false) != wxNOT_FOUND);
}

wxString MunkHtmlTag::GetParam(const wxString& par, bool with_commas) const
{
    int index = m_ParamNames.Index(par, false);
    if (index == wxNOT_FOUND)
        return wxEmptyString;
    if (with_commas)
    {
        // VS: backward compatibility, seems to be never used by MunkHTML...
        wxString s;
        s << wxT('"') << m_ParamValues[index] << wxT('"');
        return s;
    }
    else
        return m_ParamValues[index];
}

int MunkHtmlTag::ScanParam(const wxString& par,
			   const wxChar *format,
			   void *param) const
{
    wxString parval = GetParam(par);
    return wxSscanf(parval, format, param);
}

bool MunkHtmlTag::GetParamAsColour(const wxString& par, wxColour *clr) const
{
    wxCHECK_MSG( clr, false, _T("invalid colour argument") );

    wxString str = GetParam(par);

    // handle colours defined in HTML 4.0 first:
    if (str.length() > 1 && str[0] != _T('#'))
    {
        #define HTML_COLOUR(name, r, g, b)              \
            if (str.IsSameAs(wxT(name), false))         \
                { clr->Set(r, g, b); return true; }
        HTML_COLOUR("black",   0x00,0x00,0x00)
        HTML_COLOUR("silver",  0xC0,0xC0,0xC0)
        HTML_COLOUR("gray",    0x80,0x80,0x80)
        HTML_COLOUR("white",   0xFF,0xFF,0xFF)
        HTML_COLOUR("maroon",  0x80,0x00,0x00)
        HTML_COLOUR("red",     0xFF,0x00,0x00)
        HTML_COLOUR("purple",  0x80,0x00,0x80)
        HTML_COLOUR("fuchsia", 0xFF,0x00,0xFF)
        HTML_COLOUR("green",   0x00,0x80,0x00)
        HTML_COLOUR("lime",    0x00,0xFF,0x00)
        HTML_COLOUR("olive",   0x80,0x80,0x00)
        HTML_COLOUR("yellow",  0xFF,0xFF,0x00)
        HTML_COLOUR("navy",    0x00,0x00,0x80)
        HTML_COLOUR("blue",    0x00,0x00,0xFF)
        HTML_COLOUR("teal",    0x00,0x80,0x80)
        HTML_COLOUR("aqua",    0x00,0xFF,0xFF)
        #undef HTML_COLOUR
    }

    // then try to parse #rrggbb representations or set from other well
    // known names (note that this doesn't strictly conform to HTML spec,
    // but it doesn't do real harm -- but it *must* be done after the standard
    // colors are handled above):
    if (clr->Set(str))
        return true;

    return false;
}

bool MunkHtmlTag::GetParamAsInt(const wxString& par, int *clr) const
{
	if ( !HasParam(par) )
		return false;
	long i;
	if ( !GetParam(par).ToLong(&i) )
		return false;

	*clr = (int)i;
	return true;
}

bool MunkHtmlTag::GetParamAsLengthInInches(const wxString& par, double *inches) const
{
    if (!HasParam(par)) return false;
    wxString parStr = GetParam(par);
    bool succ;
    if (parStr.Right(2).Upper() == wxT("IN")) {
	    wxString doubleString = parStr.Left(parStr.Len() - 2);
	    succ = doubleString.ToDouble(inches);
    } else {
	    // At the moment, we only know how to deal with inches.
	    succ = false;
    }
    return succ;
}

wxString MunkHtmlTag::GetAllParams() const
{
    // VS: this function is for backward compatibility only,
    //     never used by MunkHTML
    wxString s;
    size_t cnt = m_ParamNames.GetCount();
    for (size_t i = 0; i < cnt; i++)
    {
        s << m_ParamNames[i];
        s << wxT('=');
        if (m_ParamValues[i].Find(wxT('"')) != wxNOT_FOUND)
            s << wxT('\'') << m_ParamValues[i] << wxT('\'');
        else
            s << wxT('"') << m_ParamValues[i] << wxT('"');
    }
    return s;
}



/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_list.cpp
// Purpose:     MunkHtml module for lists
// Author:      Vaclav Slavik
// RCS-ID:      $Id: m_list.cpp 38788 2006-04-18 08:11:26Z ABX $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif


//-----------------------------------------------------------------------------
// MunkHtmlListmarkCell
//-----------------------------------------------------------------------------

class MunkHtmlListmarkCell : public MunkHtmlCell
{
    private:
        wxBrush m_Brush;
    public:
        MunkHtmlListmarkCell(wxDC *dc, const wxColour& clr);
        void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                  MunkHtmlRenderingInfo& info);

    DECLARE_NO_COPY_CLASS(MunkHtmlListmarkCell)
};

MunkHtmlListmarkCell::MunkHtmlListmarkCell(wxDC* dc, const wxColour& clr) : MunkHtmlCell(), m_Brush(clr, wxSOLID)
{
    m_Width =  dc->GetCharHeight();
    m_Height = dc->GetCharHeight();
    // bottom of list mark is lined with bottom of letters in next cell
    m_Descent = m_Height / 3;
}



void MunkHtmlListmarkCell::Draw(wxDC& dc, int x, int y,
                              int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                              MunkHtmlRenderingInfo& WXUNUSED(info))
{
    dc.SetBrush(m_Brush);
    dc.DrawEllipse(x + m_PosX + m_Width / 3, y + m_PosY + m_Height / 3,
                   (m_Width / 3), (m_Width / 3));
}

//-----------------------------------------------------------------------------
// MunkHtmlListCell
//-----------------------------------------------------------------------------

MunkHtmlListCell::MunkHtmlListCell(MunkHtmlContainerCell *parent) : MunkHtmlContainerCell(parent)
{
    m_NumRows = 0;
    m_RowInfo = 0;
    m_ListmarkWidth = 0;
}

MunkHtmlListCell::~MunkHtmlListCell()
{
    if (m_RowInfo) free(m_RowInfo);
}

int MunkHtmlListCell::ComputeMaxBase(MunkHtmlCell *cell)
{
    if(!cell)
        return 0;

    MunkHtmlCell *child = cell->GetFirstChild();

    while(child)
    {
        int base = ComputeMaxBase( child );
        if ( base > 0 ) return base + child->GetPosY();
        child = child->GetNext();
    }

    return cell->GetHeight() - cell->GetDescent();
}

void MunkHtmlListCell::Layout(int w)
{
    MunkHtmlCell::Layout(w);

    ComputeMinMaxWidths();
    m_Width = wxMax(m_Width, wxMin(w, GetMaxTotalWidth()));

    int s_width = m_Width - m_IndentLeft;

    int vpos = 0;
    for (int r = 0; r < m_NumRows; r++)
    {
        // do layout first time to layout contents and adjust pos
        m_RowInfo[r].mark->Layout(m_ListmarkWidth);
        m_RowInfo[r].cont->Layout(s_width - m_ListmarkWidth);

        const int base_mark = ComputeMaxBase( m_RowInfo[r].mark );
        const int base_cont = ComputeMaxBase( m_RowInfo[r].cont );
        const int adjust_mark = vpos + wxMax(base_cont-base_mark,0);
        const int adjust_cont = vpos + wxMax(base_mark-base_cont,0);

        m_RowInfo[r].mark->SetPos(m_IndentLeft, adjust_mark);
        m_RowInfo[r].cont->SetPos(m_IndentLeft + m_ListmarkWidth, adjust_cont);

        vpos = wxMax(adjust_mark + m_RowInfo[r].mark->GetHeight(),
                     adjust_cont + m_RowInfo[r].cont->GetHeight());
    }
    m_Height = vpos;
}

void MunkHtmlListCell::AddRow(MunkHtmlContainerCell *mark, MunkHtmlContainerCell *cont)
{
    ReallocRows(++m_NumRows);
    m_RowInfo[m_NumRows - 1].mark = mark;
    m_RowInfo[m_NumRows - 1].cont = cont;
}

void MunkHtmlListCell::ReallocRows(int rows)
{
    m_RowInfo = (MunkHtmlListItemStruct*) realloc(m_RowInfo, sizeof(MunkHtmlListItemStruct) * rows);
    m_RowInfo[rows - 1].mark = NULL;
    m_RowInfo[rows - 1].cont = NULL;
    m_RowInfo[rows - 1].minWidth = 0;
    m_RowInfo[rows - 1].maxWidth = 0;

    m_NumRows = rows;
}

void MunkHtmlListCell::ComputeMinMaxWidths()
{
    if (m_NumRows == 0) return;

    m_MaxTotalWidth = 0;
    m_Width = 0;

    for (int r = 0; r < m_NumRows; r++)
    {
        MunkHtmlListItemStruct& row = m_RowInfo[r];
        row.mark->Layout(1);
        row.cont->Layout(1);
        int maxWidth = row.cont->GetMaxTotalWidth();
        int width = row.cont->GetWidth();
        if (row.mark->GetWidth() > m_ListmarkWidth)
            m_ListmarkWidth = row.mark->GetWidth();
        if (maxWidth > m_MaxTotalWidth)
            m_MaxTotalWidth = maxWidth;
        if (width > m_Width)
            m_Width = width;
    }
    m_Width += m_ListmarkWidth + m_IndentLeft;
    m_MaxTotalWidth += m_ListmarkWidth + m_IndentLeft;
}

//-----------------------------------------------------------------------------
// MunkHtmlListcontentCell
//-----------------------------------------------------------------------------

class MunkHtmlListcontentCell : public MunkHtmlContainerCell
{
public:
    MunkHtmlListcontentCell(MunkHtmlContainerCell *p) : MunkHtmlContainerCell(p) {}
    virtual void Layout(int w) {
        // Reset top indentation, fixes <li><p>
        SetIndent(0, MunkHTML_INDENT_TOP);
        MunkHtmlContainerCell::Layout(w);
    }
};


//-----------------------------------------------------------------------------
// MunkHtmlLineBreakCell
//-----------------------------------------------------------------------------


MunkHtmlLineBreakCell::MunkHtmlLineBreakCell(wxDC *dc, MunkHtmlCell *pPreviousCell)
{
	m_Width = 0;
	m_Height = dc->GetCharHeight() / 2;
}

MunkHtmlLineBreakCell::~MunkHtmlLineBreakCell()
{
}

void MunkHtmlLineBreakCell::Layout(int w)
{
	MunkHtmlCell::Layout(w);
}




/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_hline.cpp
// Purpose:     MunkHtml module for horizontal line (HR tag)
// Author:      Vaclav Slavik
// RCS-ID:      $Id: m_hline.cpp 38788 2006-04-18 08:11:26Z ABX $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
// MunkHtmlLineCell
//-----------------------------------------------------------------------------

class MunkHtmlLineCell : public MunkHtmlCell
{
    public:
        MunkHtmlLineCell(int size, bool shading) : MunkHtmlCell() {m_Height = size; m_HasShading = shading;}
        void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                  MunkHtmlRenderingInfo& info);
        void Layout(int w)
            { m_Width = w; MunkHtmlCell::Layout(w); }

    private:
        // Should we draw 3-D shading or not
      bool m_HasShading;

      DECLARE_NO_COPY_CLASS(MunkHtmlLineCell)
};


void MunkHtmlLineCell::Draw(wxDC& dc, int x, int y,
                          int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                          MunkHtmlRenderingInfo& WXUNUSED(info))
{
    wxBrush mybrush(wxT("GREY"), (m_HasShading) ? wxTRANSPARENT : wxSOLID);
    wxPen mypen(wxT("GREY"), 1, wxSOLID);
    dc.SetBrush(mybrush);
    dc.SetPen(mypen);
    dc.DrawRectangle(x + m_PosX, y + m_PosY, m_Width, m_Height);
}


/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_image.cpp
// Purpose:     MunkHtml module for displaying images
// Author:      Vaclav Slavik
// RCS-ID:      $Id: m_image.cpp 41819 2006-10-09 17:51:07Z VZ $
// Copyright:   (c) 1999 Vaclav Slavik, Joel Lucsy
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////




WX_DECLARE_OBJARRAY(int, MunkCoordArray);
#include "wx/arrimpl.cpp" // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(MunkCoordArray)


// ---------------------------------------------------------------------------
// MunkHtmlImageMapAreaCell
//                  0-width, 0-height cell that represents single area in
//                  imagemap (it's GetLink is called from MunkHtmlImageCell's)
// ---------------------------------------------------------------------------

class MunkHtmlImageMapAreaCell : public MunkHtmlCell
{
    public:
        enum celltype { CIRCLE, RECT, POLY };
    protected:
        MunkCoordArray coords;
        celltype type;
        int radius;
    public:
        MunkHtmlImageMapAreaCell( celltype t, wxString &coords, double pixel_scale = 1.0);
        virtual MunkHtmlLinkInfo *GetLink( int x = 0, int y = 0 ) const;
        void Draw(wxDC& WXUNUSED(dc),
                  int WXUNUSED(x), int WXUNUSED(y),
                  int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                  MunkHtmlRenderingInfo& WXUNUSED(info)) {}


    DECLARE_NO_COPY_CLASS(MunkHtmlImageMapAreaCell)
};





MunkHtmlImageMapAreaCell::MunkHtmlImageMapAreaCell( MunkHtmlImageMapAreaCell::celltype t, wxString &incoords, double pixel_scale )
{
    int i;
    wxString x = incoords, y;

    type = t;
    while ((i = x.Find( ',' )) != wxNOT_FOUND)
    {
        coords.Add( (int)(pixel_scale * (double)wxAtoi( x.Left( i ).c_str())) );
        x = x.Mid( i + 1 );
    }
    coords.Add( (int)(pixel_scale * (double)wxAtoi( x.c_str())) );
}

MunkHtmlLinkInfo *MunkHtmlImageMapAreaCell::GetLink( int x, int y ) const
{
    switch (type)
    {
        case RECT:
            {
                int l, t, r, b;

                l = coords[ 0 ];
                t = coords[ 1 ];
                r = coords[ 2 ];
                b = coords[ 3 ];
                if (x >= l && x <= r && y >= t && y <= b)
                {
                    return m_Link;
                }
                break;
            }
        case CIRCLE:
            {
                int l, t, r;
                double  d;

                l = coords[ 0 ];
                t = coords[ 1 ];
                r = coords[ 2 ];
                d = sqrt( (double) (((x - l) * (x - l)) + ((y - t) * (y - t))) );
                if (d < (double)r)
                {
                    return m_Link;
                }
            }
            break;
        case POLY:
            {
                if (coords.GetCount() >= 6)
                {
                    int intersects = 0;
                    int wherex = x;
                    int wherey = y;
                    int totalv = coords.GetCount() / 2;
                    int totalc = totalv * 2;
                    int xval = coords[totalc - 2];
                    int yval = coords[totalc - 1];
                    int end = totalc;
                    int pointer = 1;

                    if ((yval >= wherey) != (coords[pointer] >= wherey))
                    {
                        if ((xval >= wherex) == (coords[0] >= wherex))
                        {
                            intersects += (xval >= wherex) ? 1 : 0;
                        }
                        else
                        {
                            intersects += ((xval - (yval - wherey) *
                                            (coords[0] - xval) /
                                            (coords[pointer] - yval)) >= wherex) ? 1 : 0;
                        }
                    }

                    while (pointer < end)
                    {
                        yval = coords[pointer];
                        pointer += 2;
                        if (yval >= wherey)
                        {
                            while ((pointer < end) && (coords[pointer] >= wherey))
                            {
                                pointer += 2;
                            }
                            if (pointer >= end)
                            {
                                break;
                            }
                            if ((coords[pointer - 3] >= wherex) ==
                                    (coords[pointer - 1] >= wherex)) {
                                intersects += (coords[pointer - 3] >= wherex) ? 1 : 0;
                            }
                            else
                            {
                                intersects +=
                                    ((coords[pointer - 3] - (coords[pointer - 2] - wherey) *
                                      (coords[pointer - 1] - coords[pointer - 3]) /
                                      (coords[pointer] - coords[pointer - 2])) >= wherex) ? 1 : 0;
                            }
                        }
                        else
                        {
                            while ((pointer < end) && (coords[pointer] < wherey))
                            {
                                pointer += 2;
                            }
                            if (pointer >= end)
                            {
                                break;
                            }
                            if ((coords[pointer - 3] >= wherex) ==
                                    (coords[pointer - 1] >= wherex))
                            {
                                intersects += (coords[pointer - 3] >= wherex) ? 1 : 0;
                            }
                            else
                            {
                                intersects +=
                                    ((coords[pointer - 3] - (coords[pointer - 2] - wherey) *
                                      (coords[pointer - 1] - coords[pointer - 3]) /
                                      (coords[pointer] - coords[pointer - 2])) >= wherex) ? 1 : 0;
                            }
                        }
                    }
                    if ((intersects & 1) != 0)
                    {
                        return m_Link;
                    }
                }
            }
            break;
    }

    if (m_Next)
    {
        MunkHtmlImageMapAreaCell  *a = (MunkHtmlImageMapAreaCell*)m_Next;
        return a->GetLink( x, y );
    }
    return NULL;
}








//--------------------------------------------------------------------------------
// MunkHtmlImageMapCell
//                  0-width, 0-height cell that represents map from imagemaps
//                  it is always placed before MunkHtmlImageMapAreaCells
//                  It responds to Find(MunkHTML_COND_ISIMAGEMAP)
//--------------------------------------------------------------------------------


class MunkHtmlImageMapCell : public MunkHtmlCell
{
    public:
        MunkHtmlImageMapCell( wxString &name );
    protected:
        wxString m_Name;
    public:
        virtual MunkHtmlLinkInfo *GetLink( int x = 0, int y = 0 ) const;
        virtual const MunkHtmlCell *Find( int cond, const void *param ) const;
        void Draw(wxDC& WXUNUSED(dc),
                  int WXUNUSED(x), int WXUNUSED(y),
                  int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                  MunkHtmlRenderingInfo& WXUNUSED(info)) {}

    DECLARE_NO_COPY_CLASS(MunkHtmlImageMapCell)
};


MunkHtmlImageMapCell::MunkHtmlImageMapCell( wxString &name )
{
    m_Name = name ;
}

MunkHtmlLinkInfo *MunkHtmlImageMapCell::GetLink( int x, int y ) const
{
    MunkHtmlImageMapAreaCell  *a = (MunkHtmlImageMapAreaCell*)m_Next;
    if (a)
        return a->GetLink( x, y );
    return MunkHtmlCell::GetLink( x, y );
}

const MunkHtmlCell *MunkHtmlImageMapCell::Find( int cond, const void *param ) const
{
    if (cond == MunkHTML_COND_ISIMAGEMAP)
    {
        if (m_Name == *((wxString*)(param)))
            return this;
    }
    return MunkHtmlCell::Find(cond, param);
}





//--------------------------------------------------------------------------------
// MunkHtmlImageCell
//                  Image/bitmap
//--------------------------------------------------------------------------------

class MunkHtmlImageCell : public MunkHtmlCell
{
public:
	MunkHtmlImageCell(MunkHtmlWindowInterface *windowIface,
			  wxFSFile *input, int w = wxDefaultCoord, int h = wxDefaultCoord,
			  double scale = 1.0, int align = MunkHTML_ALIGN_BOTTOM,
			  const wxString& mapname = wxEmptyString);
	virtual ~MunkHtmlImageCell();
	void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
		  MunkHtmlRenderingInfo& info);
	virtual MunkHtmlLinkInfo *GetLink(int x = 0, int y = 0) const;
	
	void SetImage(const wxImage& img);

private:
	wxBitmap           *m_bitmap;
	int                 m_bmpW, m_bmpH;
	bool                m_showFrame:1;
	MunkHtmlWindowInterface *m_windowIface;
	double              m_scale;
	MunkHtmlImageMapCell *m_imageMap;
	wxString            m_mapName;
	
	DECLARE_NO_COPY_CLASS(MunkHtmlImageCell)
};



//----------------------------------------------------------------------------
// MunkHtmlImageCell
//----------------------------------------------------------------------------


MunkHtmlImageCell::MunkHtmlImageCell(MunkHtmlWindowInterface *windowIface,
                                 wxFSFile *input,
                                 int w, int h, double scale, int align,
                                 const wxString& mapname) : MunkHtmlCell()
{
    m_windowIface = windowIface;
    m_scale = scale;
    m_showFrame = false;
    m_bitmap = NULL;
    m_bmpW = w;
    m_bmpH = h;
    m_imageMap = NULL;
    m_mapName = mapname;
    SetCanLiveOnPagebreak(false);

    if ( m_bmpW && m_bmpH )
    {
        if ( input )
        {
            wxInputStream *s = input->GetStream();

            if ( s )
            {
                {
                    wxImage image(*s, wxBITMAP_TYPE_ANY);
                    if ( image.Ok() )
                        SetImage(image);
                }
            }
        }
        else // input==NULL, use "broken image" bitmap
        {
            if ( m_bmpW == wxDefaultCoord && m_bmpH == wxDefaultCoord )
            {
                m_bmpW = 29;
                m_bmpH = 31;
            }
            else
            {
                m_showFrame = true;
                if ( m_bmpW == wxDefaultCoord ) m_bmpW = 31;
                if ( m_bmpH == wxDefaultCoord ) m_bmpH = 33;
            }
            m_bitmap =
                new wxBitmap(wxArtProvider::GetBitmap(wxART_MISSING_IMAGE));
        }
    }
    //else: ignore the 0-sized images used sometimes on the Web pages

    m_Width = (int)(scale * (double)m_bmpW);
    m_Height = (int)(scale * (double)m_bmpH);

    switch (align)
    {
        case MunkHTML_ALIGN_TOP :
            m_Descent = m_Height;
            break;
        case MunkHTML_ALIGN_CENTER :
            m_Descent = m_Height / 2;
            break;
        case MunkHTML_ALIGN_BOTTOM :
        default :
            m_Descent = 0;
            break;
    }
 }

void MunkHtmlImageCell::SetImage(const wxImage& img)
{
#if !defined(__WXMSW__) || wxUSE_WXDIB
    if ( img.Ok() )
    {
        delete m_bitmap;

        int ww, hh;
        ww = img.GetWidth();
        hh = img.GetHeight();

        if ( m_bmpW == wxDefaultCoord )
            m_bmpW = ww;
        if ( m_bmpH == wxDefaultCoord )
            m_bmpH = hh;

        // Only scale the bitmap at the rendering stage,
        // so we don't lose quality twice
/*
        if ((m_bmpW != ww) || (m_bmpH != hh))
        {
            wxImage img2 = img.Scale(m_bmpW, m_bmpH);
            m_bitmap = new wxBitmap(img2);
        }
        else
*/
            m_bitmap = new wxBitmap(img);
    }
#endif
}



MunkHtmlImageCell::~MunkHtmlImageCell()
{
    delete m_bitmap;
}


void MunkHtmlImageCell::Draw(wxDC& dc, int x, int y,
                           int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                           MunkHtmlRenderingInfo& WXUNUSED(info))
{
    if ( m_showFrame )
    {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawRectangle(x + m_PosX, y + m_PosY, m_Width, m_Height);
        x++, y++;
    }
    if ( m_bitmap )
    {
        // We add in the scaling from the desired bitmap width
        // and height, so we only do the scaling once.
        double imageScaleX = 1.0;
        double imageScaleY = 1.0;
        if (m_bmpW != m_bitmap->GetWidth())
            imageScaleX = (double) m_bmpW / (double) m_bitmap->GetWidth();
        if (m_bmpH != m_bitmap->GetHeight())
            imageScaleY = (double) m_bmpH / (double) m_bitmap->GetHeight();

        double us_x, us_y;
        dc.GetUserScale(&us_x, &us_y);
        dc.SetUserScale(us_x * m_scale * imageScaleX, us_y * m_scale * imageScaleY);

        dc.DrawBitmap(*m_bitmap, (int) ((x + m_PosX) / (m_scale*imageScaleX)),
                                 (int) ((y + m_PosY) / (m_scale*imageScaleY)), true);
        dc.SetUserScale(us_x, us_y);
    }
}

MunkHtmlLinkInfo *MunkHtmlImageCell::GetLink( int x, int y ) const
{
    if (m_mapName.empty())
        return MunkHtmlCell::GetLink( x, y );
    if (!m_imageMap)
    {
        MunkHtmlContainerCell *p, *op;
        op = p = GetParent();
        while (p)
        {
            op = p;
            p = p->GetParent();
        }
        p = op;
        MunkHtmlCell *cell = (MunkHtmlCell*)p->Find(MunkHTML_COND_ISIMAGEMAP,
                                                (const void*)(&m_mapName));
        if (!cell)
        {
            ((wxString&)m_mapName).Clear();
            return MunkHtmlCell::GetLink( x, y );
        }
        {   // dirty hack, ask Joel why he fills m_ImageMap in this place
            // THE problem is that we're in const method and we can't modify m_ImageMap
            MunkHtmlImageMapCell **cx = (MunkHtmlImageMapCell**)(&m_imageMap);
            *cx = (MunkHtmlImageMapCell*)cell;
        }
    }
    return m_imageMap->GetLink(x, y);
}


class MunkHtmlAnchorCell : public MunkHtmlCell
{
private:
    wxString m_AnchorName;

public:
    MunkHtmlAnchorCell(const wxString& name) : MunkHtmlCell()
        { m_AnchorName = name; }
    void Draw(wxDC& WXUNUSED(dc),
              int WXUNUSED(x), int WXUNUSED(y),
              int WXUNUSED(view_y1), int WXUNUSED(view_y2),
              MunkHtmlRenderingInfo& WXUNUSED(info)) {}

    virtual const MunkHtmlCell* Find(int condition, const void* param) const
    {
        if ((condition == MunkHTML_COND_ISANCHOR) &&
            (m_AnchorName == (*((const wxString*)param))))
        {
            return this;
        }
        else
        {
            return MunkHtmlCell::Find(condition, param);
        }
    }

    DECLARE_NO_COPY_CLASS(MunkHtmlAnchorCell)
};






/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/htmlcell.cpp
// Purpose:     MunkHtmlCell - basic element of HTML output
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmlcell.cpp 46507 2007-06-17 17:58:20Z VS $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>

//-----------------------------------------------------------------------------
// Helper classes
//-----------------------------------------------------------------------------

void MunkHtmlSelection::Set(const wxPoint& fromPos, const MunkHtmlCell *fromCell,
                          const wxPoint& toPos, const MunkHtmlCell *toCell)
{
    m_fromCell = fromCell;
    m_toCell = toCell;
    m_fromPos = fromPos;
    m_toPos = toPos;
}

void MunkHtmlSelection::Set(const MunkHtmlCell *fromCell, const MunkHtmlCell *toCell)
{
    wxPoint p1 = fromCell ? fromCell->GetAbsPos() : wxDefaultPosition;
    wxPoint p2 = toCell ? toCell->GetAbsPos() : wxDefaultPosition;
    if ( toCell )
    {
        p2.x += toCell->GetWidth();
        p2.y += toCell->GetHeight();
    }
    Set(p1, fromCell, p2, toCell);
}

wxColour
MunkDefaultHtmlRenderingStyle::
GetSelectedTextColour(const wxColour& WXUNUSED(clr))
{
    return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
}

wxColour
MunkDefaultHtmlRenderingStyle::
GetSelectedTextBgColour(const wxColour& WXUNUSED(clr))
{
    return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
}


//-----------------------------------------------------------------------------
// MunkHtmlCell
//-----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(MunkHtmlCell, wxObject)

MunkHtmlCell::MunkHtmlCell() : wxObject()
{
    m_Next = NULL;
    m_Parent = NULL;
    m_Width = m_Height = m_Descent = 0;
    m_ScriptMode = MunkHTML_SCRIPT_NORMAL;        // <sub> or <sup> mode
    m_ScriptBaseline = 0;                       // <sub> or <sup> baseline
    m_CanLiveOnPagebreak = true;
    m_Link = NULL;
}

MunkHtmlCell::~MunkHtmlCell()
{
    delete m_Link;
}

// Update the descent value when whe are in a <sub> or <sup>.
// prevbase is the parent base
void MunkHtmlCell::SetScriptMode(MunkHtmlScriptMode mode, long previousBase)
{
    m_ScriptMode = mode;

    if (mode == MunkHTML_SCRIPT_SUP)
        m_ScriptBaseline = previousBase - (m_Height + 1) / 2;
    else if (mode == MunkHTML_SCRIPT_SUB)
        m_ScriptBaseline = previousBase + (m_Height + 1) / 6;
    else
        m_ScriptBaseline = 0;

    m_Descent += m_ScriptBaseline;
}

#if WXWIN_COMPATIBILITY_2_6

struct MunkHtmlCellOnMouseClickCompatHelper;

static MunkHtmlCellOnMouseClickCompatHelper *gs_helperOnMouseClick = NULL;

// helper for routing calls to new ProcessMouseClick() method to deprecated
// OnMouseClick() method
struct MunkHtmlCellOnMouseClickCompatHelper
{
    MunkHtmlCellOnMouseClickCompatHelper(MunkHtmlWindowInterface *window_,
                                       const wxPoint& pos_,
                                       const wxMouseEvent& event_)
        : window(window_), pos(pos_), event(event_), retval(false)
    {
    }

    bool CallOnMouseClick(MunkHtmlCell *cell)
    {
        MunkHtmlCellOnMouseClickCompatHelper *oldHelper = gs_helperOnMouseClick;
        gs_helperOnMouseClick = this;
        cell->OnMouseClick
              (
                window ? window->GetHTMLWindow() : NULL,
                pos.x, pos.y,
                event
              );
        gs_helperOnMouseClick = oldHelper;
        return retval;
    }

    MunkHtmlWindowInterface *window;
    const wxPoint& pos;
    const wxMouseEvent& event;
    bool retval;
};
#endif // WXWIN_COMPATIBILITY_2_6

bool MunkHtmlCell::ProcessMouseClick(MunkHtmlWindowInterface *window,
                                   const wxPoint& pos,
                                   const wxMouseEvent& event)
{
    wxCHECK_MSG( window, false, _T("window interface must be provided") );

#if WXWIN_COMPATIBILITY_2_6
    // NB: this hack puts the body of ProcessMouseClick() into OnMouseClick()
    //     (for which it has to pass the arguments and return value via a
    //     helper variable because these two methods have different
    //     signatures), so that old code overriding OnMouseClick will continue
    //     to work
    MunkHtmlCellOnMouseClickCompatHelper compat(window, pos, event);
    return compat.CallOnMouseClick(this);
}

void MunkHtmlCell::OnMouseClick(wxWindow *, int, int, const wxMouseEvent& event)
{
    wxCHECK_RET( gs_helperOnMouseClick, _T("unexpected call to OnMouseClick") );
    MunkHtmlWindowInterface *window = gs_helperOnMouseClick->window;
    const wxPoint& pos = gs_helperOnMouseClick->pos;
#endif // WXWIN_COMPATIBILITY_2_6

    MunkHtmlLinkInfo *lnk = GetLink(pos.x, pos.y);
    bool retval = false;

    if (lnk)
    {
        MunkHtmlLinkInfo lnk2(*lnk);
        lnk2.SetEvent(&event);
        lnk2.SetHtmlCell(this);

        window->OnHTMLLinkClicked(lnk2);
        retval = true;
    }

#if WXWIN_COMPATIBILITY_2_6
    gs_helperOnMouseClick->retval = retval;
#else
    return retval;
#endif // WXWIN_COMPATIBILITY_2_6
}

#if WXWIN_COMPATIBILITY_2_6
wxCursor MunkHtmlCell::GetCursor() const
{
    return wxNullCursor;
}
#endif // WXWIN_COMPATIBILITY_2_6

wxCursor MunkHtmlCell::GetMouseCursor(MunkHtmlWindowInterface *window) const
{
#if WXWIN_COMPATIBILITY_2_6
    // NB: Older versions of wx used GetCursor() virtual method in place of
    //     GetMouseCursor(interface). This code ensures that user code that
    //     overriden GetCursor() continues to work. The trick is that the base
    //     MunkHtmlCell::GetCursor() method simply returns wxNullCursor, so we
    //     know that GetCursor() was overriden iff it returns valid cursor.
    wxCursor cur = GetCursor();
    if (cur.Ok())
        return cur;
#endif // WXWIN_COMPATIBILITY_2_6

    if ( GetLink() )
    {
        return window->GetHTMLCursor(MunkHtmlWindowInterface::HTMLCursor_Link);
    }
    else
    {
        return window->GetHTMLCursor(MunkHtmlWindowInterface::HTMLCursor_Default);
    }
}


bool MunkHtmlCell::AdjustPagebreak(int *pagebreak, int render_height,
                                 wxArrayInt& WXUNUSED(known_pagebreaks))
{
    if ((!m_CanLiveOnPagebreak) &&
                m_PosY < *pagebreak && m_PosY + m_Height > *pagebreak)
    {
        *pagebreak = m_PosY;
        return true;
    }

    return false;
}



void MunkHtmlCell::SetLink(const MunkHtmlLinkInfo& link)
{
    if (m_Link) delete m_Link;
    m_Link = NULL;
    if (link.GetHref() != wxEmptyString)
        m_Link = new MunkHtmlLinkInfo(link);
}


void MunkHtmlCell::Layout(int WXUNUSED(w))
{
    SetPos(0, 0);
}



const MunkHtmlCell* MunkHtmlCell::Find(int WXUNUSED(condition), const void* WXUNUSED(param)) const
{
    return NULL;
}


MunkHtmlCell *MunkHtmlCell::FindCellByPos(wxCoord x, wxCoord y,
                                      unsigned flags) const
{
    if ( x >= 0 && x < m_Width && y >= 0 && y < m_Height )
    {
        return wxConstCast(this, MunkHtmlCell);
    }
    else
    {
        if ((flags & MunkHTML_FIND_NEAREST_AFTER) &&
                (y < 0 || (y < 0+m_Height && x < 0+m_Width)))
            return wxConstCast(this, MunkHtmlCell);
        else if ((flags & MunkHTML_FIND_NEAREST_BEFORE) &&
                (y >= 0+m_Height || (y >= 0 && x >= 0)))
            return wxConstCast(this, MunkHtmlCell);
        else
            return NULL;
    }
}


wxPoint MunkHtmlCell::GetAbsPos(MunkHtmlCell *rootCell) const
{
    wxPoint p(m_PosX, m_PosY);
    for (MunkHtmlCell *parent = m_Parent; parent && parent != rootCell;
         parent = parent->m_Parent)
    {
        p.x += parent->m_PosX;
        p.y += parent->m_PosY;
    }
    return p;
}

MunkHtmlCell *MunkHtmlCell::GetRootCell() const
{
    MunkHtmlCell *c = wxConstCast(this, MunkHtmlCell);
    while ( c->m_Parent )
        c = c->m_Parent;
    return c;
}

unsigned MunkHtmlCell::GetDepth() const
{
    unsigned d = 0;
    for (MunkHtmlCell *p = m_Parent; p; p = p->m_Parent)
        d++;
    return d;
}

bool MunkHtmlCell::IsBefore(MunkHtmlCell *cell) const
{
    const MunkHtmlCell *c1 = this;
    const MunkHtmlCell *c2 = cell;
    unsigned d1 = GetDepth();
    unsigned d2 = cell->GetDepth();

    if ( d1 > d2 )
        for (; d1 != d2; d1-- )
            c1 = c1->m_Parent;
    else if ( d1 < d2 )
        for (; d1 != d2; d2-- )
            c2 = c2->m_Parent;

    if ( cell == this )
        return true;

    while ( c1 && c2 )
    {
        if ( c1->m_Parent == c2->m_Parent )
        {
            while ( c1 )
            {
                if ( c1 == c2 )
                    return true;
                c1 = c1->GetNext();
            }
            return false;
        }
        else
        {
            c1 = c1->m_Parent;
            c2 = c2->m_Parent;
        }
    }

    wxFAIL_MSG(_T("Cells are in different trees"));
    return false;
}


//-----------------------------------------------------------------------------
// MunkHtmlWordCell
//-----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(MunkHtmlWordCell, MunkHtmlCell)

MunkHtmlWordCell::MunkHtmlWordCell(const wxString& word, const wxDC& dc) : MunkHtmlCell()
{
    m_Word = word;
    dc.GetTextExtent(m_Word, &m_Width, &m_Height, &m_Descent);
    SetCanLiveOnPagebreak(false);
    m_allowLinebreak = true;
}

void MunkHtmlWordCell::SetPreviousWord(MunkHtmlWordCell *cell)
{
    if ( cell && m_Parent == cell->m_Parent &&
         !wxIsspace(cell->m_Word.Last()) && !wxIsspace(m_Word[0u]) )
    {
        m_allowLinebreak = false;
    }
}

// Splits m_Word into up to three parts according to selection, returns
// substring before, in and after selection and the points (in relative coords)
// where s2 and s3 start:
void MunkHtmlWordCell::Split(const wxDC& dc,
                           const wxPoint& selFrom, const wxPoint& selTo,
                           unsigned& pos1, unsigned& pos2) const
{
    wxPoint pt1 = (selFrom == wxDefaultPosition) ?
                   wxDefaultPosition : selFrom - GetAbsPos();
    wxPoint pt2 = (selTo == wxDefaultPosition) ?
                   wxPoint(m_Width, wxDefaultCoord) : selTo - GetAbsPos();

    // if the selection is entirely within this cell, make sure pt1 < pt2 in
    // order to make the rest of this function simpler:
    if ( selFrom != wxDefaultPosition && selTo != wxDefaultPosition &&
         selFrom.x > selTo.x )
    {
        wxPoint tmp = pt1;
        pt1 = pt2;
        pt2 = tmp;
    }

    unsigned len = m_Word.length();
    unsigned i = 0;
    pos1 = 0;

    // adjust for cases when the start/end position is completely
    // outside the cell:
    if ( pt1.y < 0 )
        pt1.x = 0;
    if ( pt2.y >= m_Height )
        pt2.x = m_Width;

    // before selection:
    // (include character under caret only if in first half of width)
#ifdef __WXMAC__
    // implementation using PartialExtents to support fractional widths
    wxArrayInt widths ;
    dc.GetPartialTextExtents(m_Word,widths) ;
    while( i < len && pt1.x >= widths[i] )
        i++ ;
    if ( i < len )
    {
        int charW = (i > 0) ? widths[i] - widths[i-1] : widths[i];
        if ( widths[i] - pt1.x < charW/2 )
            i++;
    }
#else // !__WXMAC__
    wxCoord charW, charH;
    while ( pt1.x > 0 && i < len )
    {
        dc.GetTextExtent(m_Word[i], &charW, &charH);
        pt1.x -= charW;
        if ( pt1.x >= -charW/2 )
        {
            pos1 += charW;
            i++;
        }
    }
#endif // __WXMAC__/!__WXMAC__

    // in selection:
    // (include character under caret only if in first half of width)
    unsigned j = i;
#ifdef __WXMAC__
    while( j < len && pt2.x >= widths[j] )
        j++ ;
    if ( j < len )
    {
        int charW = (j > 0) ? widths[j] - widths[j-1] : widths[j];
        if ( widths[j] - pt2.x < charW/2 )
            j++;
    }
#else // !__WXMAC__
    pos2 = pos1;
    pt2.x -= pos2;
    while ( pt2.x > 0 && j < len )
    {
        dc.GetTextExtent(m_Word[j], &charW, &charH);
        pt2.x -= charW;
        if ( pt2.x >= -charW/2 )
        {
            pos2 += charW;
            j++;
        }
    }
#endif // __WXMAC__/!__WXMAC__

    pos1 = i;
    pos2 = j;
}

void MunkHtmlWordCell::SetSelectionPrivPos(const wxDC& dc, MunkHtmlSelection *s) const
{
    unsigned p1, p2;

    Split(dc,
          this == s->GetFromCell() ? s->GetFromPos() : wxDefaultPosition,
          this == s->GetToCell() ? s->GetToPos() : wxDefaultPosition,
          p1, p2);

    wxPoint p(0, m_Word.length());

    if ( this == s->GetFromCell() )
        p.x = p1; // selection starts here
    if ( this == s->GetToCell() )
        p.y = p2; // selection ends here

    if ( this == s->GetFromCell() )
        s->SetFromPrivPos(p);
    if ( this == s->GetToCell() )
        s->SetToPrivPos(p);
}


static void SwitchSelState(wxDC& dc, MunkHtmlRenderingInfo& info,
                           bool toSelection)
{
    wxColour fg = info.GetState().GetFgColour();
    wxColour bg = info.GetState().GetBgColour();

    if ( toSelection )
    {
        dc.SetBackgroundMode(wxSOLID);
	wxColour selFg = info.GetStyle().GetSelectedTextColour(fg);
	if (selFg != wxNullColour) {
		dc.SetTextForeground(selFg);
	}
	wxColour selBg = info.GetStyle().GetSelectedTextBgColour(bg);
	if (selBg != wxNullColour) {
		dc.SetTextBackground(selBg);
		dc.SetBackground(wxBrush(selBg,
					 wxSOLID));
	}
    } else if (info.GetState().GetBgColour() != wxNullColour
	       && info.GetState().GetBgColour() != info.GetWindowBackgroundColour()) {
        dc.SetBackgroundMode(wxSOLID);
	if (fg != wxNullColour) {
		dc.SetTextForeground(fg);
	}
	if (bg != wxNullColour) {
		dc.SetTextBackground(bg);
		dc.SetBackground(wxBrush(bg, wxSOLID));
	}
    } 
    else
    {
        dc.SetBackgroundMode(wxTRANSPARENT);
	if (fg != wxNullColour) {
		dc.SetTextForeground(fg);
	}
	if (bg != wxNullColour) {
		dc.SetTextBackground(bg);
		dc.SetBackground(wxBrush(bg, wxSOLID));
	}
    }
}


void MunkHtmlWordCell::Draw(wxDC& dc, int x, int y,
                          int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                          MunkHtmlRenderingInfo& info)
{
#if 0 // useful for debugging
    dc.SetPen(*wxBLACK_PEN);
    dc.DrawRectangle(x+m_PosX,y+m_PosY,m_Width /* VZ: +1? */ ,m_Height);
#endif

    bool drawSelectionAfterCell = false;

    if ( info.GetState().GetSelectionState() == MunkHTML_SEL_CHANGING )
    {
        // Selection changing, we must draw the word piecewise:
        MunkHtmlSelection *s = info.GetSelection();
        wxString txt;
        int w, h;
        int ofs = 0;

        wxPoint priv = (this == s->GetFromCell()) ?
                           s->GetFromPrivPos() : s->GetToPrivPos();

        // NB: this is quite a hack: in order to compute selection boundaries
        //     (in word's characters) we must know current font, which is only
        //     possible inside rendering code. Therefore we update the
        //     information here and store it in MunkHtmlSelection so that
        //     ConvertToText can use it later:
        if ( priv == wxDefaultPosition )
        {
            SetSelectionPrivPos(dc, s);
            priv = (this == s->GetFromCell()) ?
                    s->GetFromPrivPos() : s->GetToPrivPos();
        }

        int part1 = priv.x;
        int part2 = priv.y;

        if ( part1 > 0 )
        {
            txt = m_Word.Mid(0, part1);
            dc.DrawText(txt, x + m_PosX, y + m_PosY);
            dc.GetTextExtent(txt, &w, &h);
            ofs += w;
        }

        SwitchSelState(dc, info, true);

        txt = m_Word.Mid(part1, part2-part1);
        dc.DrawText(txt, ofs + x + m_PosX, y + m_PosY);

        if ( (size_t)part2 < m_Word.length() )
        {
            dc.GetTextExtent(txt, &w, &h);
            ofs += w;
            SwitchSelState(dc, info, false);
            txt = m_Word.Mid(part2);
            dc.DrawText(txt, ofs + x + m_PosX, y + m_PosY);
        }
        else
            drawSelectionAfterCell = true;
    }
    else
    {
        MunkHtmlSelectionState selstate = info.GetState().GetSelectionState();
        // Not changing selection state, draw the word in single mode:
        if ( selstate != MunkHTML_SEL_OUT &&
             dc.GetBackgroundMode() != wxSOLID )
        {
            SwitchSelState(dc, info, true);
        }
        else if ( selstate == MunkHTML_SEL_OUT &&
                  dc.GetBackgroundMode() == wxSOLID )
        {
            SwitchSelState(dc, info, false);
        }
        dc.DrawText(m_Word, x + m_PosX, y + m_PosY);
        drawSelectionAfterCell = (selstate != MunkHTML_SEL_OUT);
    }

    if (info.GetUnderline()) {
	    wxColour penColour = info.GetState().GetFgColour();
	    if (penColour == wxNullColour) {
		    penColour = *wxBLACK;
	    }
	    dc.SetPen(wxPen(penColour,
			    1, // width
			    wxSOLID));
	    dc.DrawLine(x + m_PosX, y + m_PosY + m_Height - m_Descent, x + m_PosX + m_Width + 1, y + m_PosY + m_Height - m_Descent);
	    dc.SetPen(wxNullPen);
    }

    // NB: If the text is justified then there is usually some free space
    //     between adjacent cells and drawing the selection only onto cells
    //     would result in ugly unselected spaces. The code below detects
    //     this special case and renders the selection *outside* the sell,
    //     too.
    if ( m_Parent->GetAlignHor() == MunkHTML_ALIGN_JUSTIFY &&
         drawSelectionAfterCell )
    {
        MunkHtmlCell *nextCell = m_Next;
        while ( nextCell && nextCell->IsFormattingCell() )
            nextCell = nextCell->GetNext();
        if ( nextCell )
        {
            int nextX = nextCell->GetPosX();
            if ( m_PosX + m_Width < nextX )
            {
                dc.SetBrush(dc.GetBackground());
                dc.SetPen(*wxTRANSPARENT_PEN);
                dc.DrawRectangle(x + m_PosX + m_Width, y + m_PosY,
                                 nextX - m_PosX - m_Width, m_Height);
            }
        }
    }
}


wxString MunkHtmlWordCell::ConvertToText(MunkHtmlSelection *s) const
{
    if ( s && (this == s->GetFromCell() || this == s->GetToCell()) )
    {
        wxPoint priv = this == s->GetFromCell() ? s->GetFromPrivPos()
                                                : s->GetToPrivPos();

        // VZ: we may be called before we had a chance to re-render ourselves
        //     and in this case GetFrom/ToPrivPos() is not set yet -- assume
        //     that this only happens in case of a double/triple click (which
        //     seems to be the case now) and so it makes sense to select the
        //     entire contents of the cell in this case
        //
        // TODO: but this really needs to be fixed in some better way later...
        if ( priv != wxDefaultPosition )
        {
            int part1 = priv.x;
            int part2 = priv.y;
	    if ( part1 == part2 ) 
		    return wxEmptyString;
            return m_Word.Mid(part1, part2-part1);
        }
        //else: return the whole word below
    }

    return m_Word;
}

wxCursor MunkHtmlWordCell::GetMouseCursor(MunkHtmlWindowInterface *window) const
{
    if ( !GetLink() )
    {
        return window->GetHTMLCursor(MunkHtmlWindowInterface::HTMLCursor_Text);
    }
    else
    {
        return MunkHtmlCell::GetMouseCursor(window);
    }
}


//-----------------------------------------------------------------------------
// MunkHtmlContainerCell
//-----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(MunkHtmlContainerCell, MunkHtmlCell)

MunkHtmlContainerCell::MunkHtmlContainerCell(MunkHtmlContainerCell *parent) : MunkHtmlCell()
{
	m_bIsInlineBlock = false;
	m_Cells = m_LastCell = NULL;
	m_Parent = parent;
	m_MaxTotalWidth = 0;
	if (m_Parent) m_Parent->InsertCell(this);
	m_AlignHor = MunkHTML_ALIGN_LEFT;
	m_AlignVer = MunkHTML_ALIGN_BOTTOM;
	m_IndentLeft = m_IndentRight = m_IndentTop = m_IndentBottom = 0;
	m_IndentFirstLine = 0;
	m_WidthFloat = 100; m_WidthFloatUnits = MunkHTML_UNITS_PERCENT;
	m_UseBkColour = false;
	m_UseBorder = false;
	m_MinHeight = 0;
	m_MinHeightAlign = MunkHTML_ALIGN_TOP;
	m_LastLayout = -1;
}

void MunkHtmlContainerCell::SetBackgroundColour(const wxColour& clr)
{
	if (clr != wxNullColour) {
		m_UseBkColour = true; 
		m_BkColour = clr;
	} else {
		m_UseBkColour = false; 
		m_BkColour = wxNullColour;
	}
}

MunkHtmlContainerCell::~MunkHtmlContainerCell()
{
    MunkHtmlCell *cell = m_Cells;
    while ( cell )
    {
        MunkHtmlCell *cellNext = cell->GetNext();
        delete cell;
        cell = cellNext;
    }
}



void MunkHtmlContainerCell::SetIndent(int i, int what, int units)
{
    int val = (units == MunkHTML_UNITS_PIXELS) ? i : -i;
    if (what & MunkHTML_INDENT_LEFT) m_IndentLeft = val;
    if (what & MunkHTML_INDENT_RIGHT) m_IndentRight = val;
    if (what & MunkHTML_INDENT_TOP) m_IndentTop = val;
    if (what & MunkHTML_INDENT_BOTTOM) m_IndentBottom = val;
    m_LastLayout = -1;
}



int MunkHtmlContainerCell::GetIndent(int ind) const
{
    if (ind & MunkHTML_INDENT_LEFT) return m_IndentLeft;
    else if (ind & MunkHTML_INDENT_RIGHT) return m_IndentRight;
    else if (ind & MunkHTML_INDENT_TOP) return m_IndentTop;
    else if (ind & MunkHTML_INDENT_BOTTOM) return m_IndentBottom;
    else return -1; /* BUG! Should not be called... */
}




int MunkHtmlContainerCell::GetIndentUnits(int ind) const
{
    bool p = false;
    if (ind & MunkHTML_INDENT_LEFT) p = m_IndentLeft < 0;
    else if (ind & MunkHTML_INDENT_RIGHT) p = m_IndentRight < 0;
    else if (ind & MunkHTML_INDENT_TOP) p = m_IndentTop < 0;
    else if (ind & MunkHTML_INDENT_BOTTOM) p = m_IndentBottom < 0;
    if (p) return MunkHTML_UNITS_PERCENT;
    else return MunkHTML_UNITS_PIXELS;
}


bool MunkHtmlContainerCell::AdjustPagebreak(int *pagebreak, int render_height,
                                          wxArrayInt& known_pagebreaks)
{
    if (!m_CanLiveOnPagebreak)
	    return MunkHtmlCell::AdjustPagebreak(pagebreak, render_height, known_pagebreaks);

    MunkHtmlCell *c = GetFirstChild();
    bool rt = false;
    int pbrk = *pagebreak - m_PosY;
    while (c)
    {
	    if (c->AdjustPagebreak(&pbrk, render_height, known_pagebreaks))
            rt = true;
        c = c->GetNext();
    }
    if (rt)
        *pagebreak = pbrk + m_PosY;
    return rt;
}


void MunkHtmlContainerCell::Layout(int w)
{
	MunkHtmlCell::Layout(w);

    if (m_LastLayout == w)
	    return;
    m_LastLayout = w;

    // VS: Any attempt to layout with negative or zero width leads to hell,
    // but we can't ignore such attempts completely, since it sometimes
    // happen (e.g. when trying how small a table can be). The best thing we
    // can do is to set the width of child cells to zero
    if (w < 1)
    {
       m_Width = 0;
       for (MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
            cell->Layout(0);
            // this does two things: it recursively calls this code on all
            // child contrainers and resets children's position to (0,0)
       return;
    }

    MunkHtmlCell *nextCell;
    long xpos = 0, ypos = m_IndentTop;
    int xdelta = 0, ybasicpos = 0, ydiff;
    int s_width, nextWordWidth, s_indent;
    int ysizeup = 0, ysizedown = 0;
    int MaxLineWidth = 0;
    int curLineWidth = 0;
    m_MaxTotalWidth = 0;
    bool bIsFirstLine = true;

    /*

    WIDTH ADJUSTING :

    */

    if (m_WidthFloatUnits == MunkHTML_UNITS_PERCENT)
    {
        if (m_WidthFloat < 0) m_Width = (100 + m_WidthFloat) * w / 100;
        else m_Width = m_WidthFloat * w / 100;
    }
    else
    {
        if (m_WidthFloat < 0) m_Width = w + m_WidthFloat;
        else m_Width = m_WidthFloat;
    }

    if (m_Cells)
    {
        int l = (m_IndentLeft < 0) ? (-m_IndentLeft * m_Width / 100) : m_IndentLeft;
        int r = (m_IndentRight < 0) ? (-m_IndentRight * m_Width / 100) : m_IndentRight;
        for (MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
            cell->Layout(m_Width - (l + r));
    }
    if (IsInlineBlock()) {
	    int maxWidth = 0;
	    for (MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext()) {
		    int cell_width = cell->GetWidth();
		    if (cell_width > maxWidth) {
			    maxWidth = cell_width;
		    }
	    }
	    m_Width = maxWidth;
    }


    /*

    LAYOUTING :

    */

    // adjust indentation:
    s_indent = (m_IndentLeft < 0) ? (-m_IndentLeft * m_Width / 100) : m_IndentLeft;
    s_width = m_Width - s_indent - ((m_IndentRight < 0) ? (-m_IndentRight * m_Width / 100) : m_IndentRight);


    if (m_IndentFirstLine != 0) {
	    xpos = m_IndentFirstLine;
    }



    // my own layouting:
    MunkHtmlCell *cell = m_Cells,
               *line = m_Cells;
    while (cell != NULL)
    {
        switch (m_AlignVer)
        {
            case MunkHTML_ALIGN_TOP :      ybasicpos = 0; break;
            case MunkHTML_ALIGN_BOTTOM :   ybasicpos = - cell->GetHeight(); break;
            case MunkHTML_ALIGN_CENTER :   ybasicpos = - cell->GetHeight() / 2; break;
        }
        ydiff = cell->GetHeight() + ybasicpos;

        if (cell->GetDescent() + ydiff > ysizedown) ysizedown = cell->GetDescent() + ydiff;
        if (ybasicpos + cell->GetDescent() < -ysizeup) ysizeup = - (ybasicpos + cell->GetDescent());

        // layout nonbreakable run of cells:
        cell->SetPos(xpos, ybasicpos + cell->GetDescent());

        xpos += cell->GetWidth();

        if (!cell->IsTerminalCell()
	    && !cell->IsInlineBlock()) 
        {
            // Container cell indicates new line
            if (curLineWidth > m_MaxTotalWidth)
                m_MaxTotalWidth = curLineWidth;

            if (wxMax(cell->GetWidth(), cell->GetMaxTotalWidth()) > m_MaxTotalWidth)
                m_MaxTotalWidth = cell->GetMaxTotalWidth();
            curLineWidth = 0;
        }
        else
            // Normal cell, add maximum cell width to line width
            curLineWidth += cell->GetMaxTotalWidth();

        cell = cell->GetNext();

        // compute length of the next word that would be added:
        nextWordWidth = 0;
        if (cell)
        {
            nextCell = cell;
            do
            {
                nextWordWidth += nextCell->GetWidth();
                nextCell = nextCell->GetNext();
            } while (nextCell && !nextCell->IsLinebreakAllowed());
        }

        // force new line if occurred:
        if ((cell == NULL) ||
            ((xpos + nextWordWidth > s_width) && cell->IsLinebreakAllowed())
	    || cell->ForceLineBreak())
        {
            if (xpos > MaxLineWidth) MaxLineWidth = xpos;
            if (ysizeup < 0) ysizeup = 0;
            if (ysizedown < 0) ysizedown = 0;
	    if (IsInlineBlock()) {
		    switch (m_AlignHor) {
		    case MunkHTML_ALIGN_LEFT :
		    case MunkHTML_ALIGN_JUSTIFY :
			    xdelta = 0;
			    break;
		    case MunkHTML_ALIGN_RIGHT :
			    xdelta = 0 + (s_width - xpos);
			    break;
		    case MunkHTML_ALIGN_CENTER :
			    xdelta = 0 + (s_width - xpos) / 2;
			    break;
		    }
	    } else {
		    switch (m_AlignHor) {
		    case MunkHTML_ALIGN_LEFT :
		    case MunkHTML_ALIGN_JUSTIFY :
			    xdelta = 0;
			    break;
		    case MunkHTML_ALIGN_RIGHT :
			    xdelta = 0 + (s_width - xpos);
			    break;
		    case MunkHTML_ALIGN_CENTER :
			    xdelta = 0 + (s_width - xpos) / 2;
			    break;
		    }
	    }
            if (xdelta < 0) xdelta = 0;
	    if (!IsInlineBlock()) {
		    xdelta += s_indent;
	    }

            ypos += ysizeup;

            if (m_AlignHor != MunkHTML_ALIGN_JUSTIFY || cell == NULL)
            {
                while (line != cell)
                {
                    line->SetPos(line->GetPosX() + xdelta,
                                   ypos + line->GetPosY());
                    line = line->GetNext();
                }
            }
            else // align == justify
            {
                // we have to distribute the extra horz space between the cells
                // on this line

                // an added complication is that some cells have fixed size and
                // shouldn't get any increment (it so happens that these cells
                // also don't allow line break on them which provides with an
                // easy way to test for this) -- and neither should the cells
                // adjacent to them as this could result in a visible space
                // between two cells separated by, e.g. font change, cell which
                // is wrong

                int step = s_width - xpos;
                if ( step > 0 )
                {
                    // first count the cells which will get extra space
                    int total = -1;
		    bool bIsLastLine = false;

                    const MunkHtmlCell *c;
                    if ( line != cell )
                    {
                        for ( c = line; c != cell; c = c->GetNext() )
                        {
                            if ( c->IsLinebreakAllowed() )
                            {
                                total++;
                            }
                        }

			// This is a crude way of getting to know whether
			// this is the last line, and it doesn't always
			// work.
			if (c == NULL) {
			    bIsLastLine = true;
			}
                    }

		    // If this is the last line, don't do the justify thing.
		    if (bIsLastLine) {
		        total = 0;
		    }

                    // and now extra space to those cells which merit it
                    if ( total )
                    {
                        // first visible cell on line is not moved:
                        while (line !=cell && !line->IsLinebreakAllowed())
                        {
                            line->SetPos(line->GetPosX() + s_indent,
                                         line->GetPosY() + ypos);
                            line = line->GetNext();
                        }

                        if (line != cell)
                        {
                            line->SetPos(line->GetPosX() + s_indent,
                                         line->GetPosY() + ypos);

                            line = line->GetNext();
                        }

                        for ( int n = 0; line != cell; line = line->GetNext() )
                        {
                            if ( line->IsLinebreakAllowed() )
                            {
                                // offset the next cell relative to this one
                                // thus increasing our size
                                n++;
                            }

                            line->SetPos(line->GetPosX() + s_indent +
                                           ((n * step) / total),
                                           line->GetPosY() + ypos);
                        }
                    }
                    else
                    {
                        // this will cause the code to enter "else branch" below:
                        step = 0;
                    }
                }
                // else branch:
                if ( step <= 0 ) // no extra space to distribute
                {
                    // just set the indent properly
                    while (line != cell)
                    {
                        line->SetPos(line->GetPosX() + s_indent,
                                     line->GetPosY() + ypos);
                        line = line->GetNext();
                    }
                }
            }

            ypos += ysizedown;
            xpos = 0;
            ysizeup = ysizedown = 0;
            line = cell;
	    bIsFirstLine = false;
        }
    }

    // setup height & width, depending on container layout:
    m_Height = ypos + (ysizedown + ysizeup) + m_IndentBottom;
    m_Descent = 0;

    if (m_Height < m_MinHeight)
    {
        if (m_MinHeightAlign != MunkHTML_ALIGN_TOP)
        {
            int diff = m_MinHeight - m_Height;
            if (m_MinHeightAlign == MunkHTML_ALIGN_CENTER) diff /= 2;
            cell = m_Cells;
            while (cell)
            {
                cell->SetPos(cell->GetPosX(), cell->GetPosY() + diff);
                cell = cell->GetNext();
            }
        }
        m_Height = m_MinHeight;
    }

    if (curLineWidth > m_MaxTotalWidth)
        m_MaxTotalWidth = curLineWidth;

    m_MaxTotalWidth += s_indent + ((m_IndentRight < 0) ? (-m_IndentRight * m_Width / 100) : m_IndentRight);
    MaxLineWidth += s_indent + ((m_IndentRight < 0) ? (-m_IndentRight * m_Width / 100) : m_IndentRight);
    if (IsInlineBlock()) {
	    m_Width = m_MaxTotalWidth;
    } else {
	    if (m_Width < MaxLineWidth) m_Width = MaxLineWidth;
    }

    m_LastLayout = w;
}

void MunkHtmlContainerCell::UpdateRenderingStatePre(MunkHtmlRenderingInfo& info,
                                                  MunkHtmlCell *cell) const
{
    MunkHtmlSelection *s = info.GetSelection();
    if (!s) return;
    if (s->GetFromCell() == cell || s->GetToCell() == cell)
    {
        info.GetState().SetSelectionState(MunkHTML_SEL_CHANGING);
    }
}

void MunkHtmlContainerCell::UpdateRenderingStatePost(MunkHtmlRenderingInfo& info,
                                                   MunkHtmlCell *cell) const
{
    MunkHtmlSelection *s = info.GetSelection();
    if (!s) return;
    if (s->GetToCell() == cell)
        info.GetState().SetSelectionState(MunkHTML_SEL_OUT);
    else if (s->GetFromCell() == cell)
        info.GetState().SetSelectionState(MunkHTML_SEL_IN);
}

#define mMin(a, b) (((a) < (b)) ? (a) : (b))
#define mMax(a, b) (((a) < (b)) ? (b) : (a))

void MunkHtmlContainerCell::Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                               MunkHtmlRenderingInfo& info)
{
#if 0 // useful for debugging
    dc.SetPen(*wxRED_PEN);
    dc.DrawRectangle(x+m_PosX,y+m_PosY,m_Width,m_Height);
#endif

    int xlocal = x + m_PosX;
    int ylocal = y + m_PosY;

    if (m_UseBkColour)
    {
        wxBrush myb = wxBrush(m_BkColour, wxSOLID);

        int real_y1 = mMax(ylocal, view_y1);
        int real_y2 = mMin(ylocal + m_Height - 1, view_y2);

        dc.SetBrush(myb);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(xlocal, real_y1, m_Width, real_y2 - real_y1 + 1);
    }

    if (m_UseBorder)
    {
        wxPen mypen1(m_BorderColour1, 1, wxSOLID);
        wxPen mypen2(m_BorderColour2, 1, wxSOLID);

        dc.SetPen(mypen1);
        dc.DrawLine(xlocal, ylocal, xlocal, ylocal + m_Height - 1);
        dc.DrawLine(xlocal, ylocal, xlocal + m_Width, ylocal);
        dc.SetPen(mypen2);
        dc.DrawLine(xlocal + m_Width - 1, ylocal, xlocal +  m_Width - 1, ylocal + m_Height - 1);
        dc.DrawLine(xlocal, ylocal + m_Height - 1, xlocal + m_Width, ylocal + m_Height - 1);
    }

    if (m_Cells)
    {
        // draw container's contents:
        for (MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
        {

            // optimize drawing: don't render off-screen content:
            if ((ylocal + cell->GetPosY() <= view_y2) &&
                (ylocal + cell->GetPosY() + cell->GetHeight() > view_y1)) {
                // the cell is visible, draw it:
                UpdateRenderingStatePre(info, cell);
                cell->Draw(dc,
                           xlocal, ylocal, view_y1, view_y2,
                           info);
                UpdateRenderingStatePost(info, cell);
            } else if (ylocal > view_y2) {
		    break;
	    } else {
                // the cell is off-screen, proceed with font+color+etc.
                // changes only:
                cell->DrawInvisible(dc, xlocal, ylocal, info);
            }
        }
    }
}



void MunkHtmlContainerCell::DrawInvisible(wxDC& dc, int x, int y,
                                        MunkHtmlRenderingInfo& info)
{
    if (m_Cells)
    {
        for (MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
        {
		MunkHtmlSelection *s = info.GetSelection();
		if (s) {
			UpdateRenderingStatePre(info, cell);
		}
		cell->DrawInvisible(dc, x + m_PosX, y + m_PosY, info);
		s = info.GetSelection();
		if (s) {
			UpdateRenderingStatePost(info, cell);
		}
        }
    }
}


wxColour MunkHtmlContainerCell::GetBackgroundColour()
{
    if (m_UseBkColour)
        return m_BkColour;
    else
        return wxNullColour;
}



MunkHtmlLinkInfo *MunkHtmlContainerCell::GetLink(int x, int y) const
{
    MunkHtmlCell *cell = FindCellByPos(x, y);

    // VZ: I don't know if we should pass absolute or relative coords to
    //     MunkHtmlCell::GetLink()? As the base class version just ignores them
    //     anyhow, it hardly matters right now but should still be clarified
    return cell ? cell->GetLink(x, y) : NULL;
}



void MunkHtmlContainerCell::InsertCell(MunkHtmlCell *f)
{
    if (!m_Cells) m_Cells = m_LastCell = f;
    else
    {
        m_LastCell->SetNext(f);
        m_LastCell = f;
        if (m_LastCell) while (m_LastCell->GetNext()) m_LastCell = m_LastCell->GetNext();
    }
    f->SetParent(this);
    m_LastLayout = -1;
}



void MunkHtmlContainerCell::SetAlign(const MunkHtmlTag& tag)
{
	wxString alg;
	if (tag.HasParam(wxT("ALIGN"))) {
		alg = tag.GetParam(wxT("ALIGN"));
	} else if (tag.HasParam(wxT("TEXT_ALIGN"))) {
		alg = tag.GetParam(wxT("TEXT_ALIGN"));
	}

	if (!alg.IsEmpty()) {
		alg.MakeUpper();
		if (alg == wxT("CENTER"))
			SetAlignHor(MunkHTML_ALIGN_CENTER);
		else if (alg == wxT("LEFT"))
			SetAlignHor(MunkHTML_ALIGN_LEFT);
		else if (alg == wxT("JUSTIFY"))
			SetAlignHor(MunkHTML_ALIGN_JUSTIFY);
		else if (alg == wxT("RIGHT"))
			SetAlignHor(MunkHTML_ALIGN_RIGHT);
		m_LastLayout = -1;
	}
}



void MunkHtmlContainerCell::SetWidthFloat(const MunkHtmlTag& tag, double pixel_scale)
{
    if (tag.HasParam(wxT("WIDTH")))
    {
        int wdi;
        wxString wd = tag.GetParam(wxT("WIDTH"));

        if (wd[wd.length()-1] == wxT('%'))
        {
            wxSscanf(wd.c_str(), wxT("%i%%"), &wdi);
            SetWidthFloat(wdi, MunkHTML_UNITS_PERCENT);
        }
        else
        {
            wxSscanf(wd.c_str(), wxT("%i"), &wdi);
            SetWidthFloat((int)(pixel_scale * (double)wdi), MunkHTML_UNITS_PIXELS);

        }
        m_LastLayout = -1;
    }
}



const MunkHtmlCell* MunkHtmlContainerCell::Find(int condition, const void* param) const
{
    if (m_Cells)
    {
        for (MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext())
        {
            const MunkHtmlCell *r = cell->Find(condition, param);
            if (r) return r;
        }
    }
    return NULL;
}


MunkHtmlCell *MunkHtmlContainerCell::FindCellByPos(wxCoord x, wxCoord y,
                                               unsigned flags) const
{
    if ( flags & MunkHTML_FIND_EXACT )
    {
        for ( const MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext() )
        {
            int cx = cell->GetPosX(),
                cy = cell->GetPosY();

            if ( (cx <= x) && (cx + cell->GetWidth() > x) &&
                 (cy <= y) && (cy + cell->GetHeight() > y) )
            {
                return cell->FindCellByPos(x - cx, y - cy, flags);
            }
        }
    }
    else if ( flags & MunkHTML_FIND_NEAREST_AFTER )
    {
        MunkHtmlCell *c;
        for ( const MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext() )
        {
            if ( cell->IsFormattingCell() )
                continue;
            int cellY = cell->GetPosY();
            if (!( y < cellY || (y < cellY + cell->GetHeight() &&
                                 x < cell->GetPosX() + cell->GetWidth()) ))
                continue;

            c = cell->FindCellByPos(x - cell->GetPosX(), y - cellY, flags);
            if (c) return c;
        }
    }
    else if ( flags & MunkHTML_FIND_NEAREST_BEFORE )
    {
        MunkHtmlCell *c2, *c = NULL;
        for ( const MunkHtmlCell *cell = m_Cells; cell; cell = cell->GetNext() )
        {
            if ( cell->IsFormattingCell() )
                continue;
            int cellY = cell->GetPosY();
            if (!( cellY + cell->GetHeight() <= y ||
                   (y >= cellY && x >= cell->GetPosX()) ))
                break;
            c2 = cell->FindCellByPos(x - cell->GetPosX(), y - cellY, flags);
            if (c2)
                c = c2;
        }
        if (c) return c;
    }

    return NULL;
}


bool MunkHtmlContainerCell::ProcessMouseClick(MunkHtmlWindowInterface *window,
                                            const wxPoint& pos,
                                            const wxMouseEvent& event)
{
#if WXWIN_COMPATIBILITY_2_6
    MunkHtmlCellOnMouseClickCompatHelper compat(window, pos, event);
    return compat.CallOnMouseClick(this);
}

void MunkHtmlContainerCell::OnMouseClick(wxWindow*,
                                       int, int, const wxMouseEvent& event)
{
    wxCHECK_RET( gs_helperOnMouseClick, _T("unexpected call to OnMouseClick") );
    MunkHtmlWindowInterface *window = gs_helperOnMouseClick->window;
    const wxPoint& pos = gs_helperOnMouseClick->pos;
#endif // WXWIN_COMPATIBILITY_2_6

    bool retval = false;
    MunkHtmlCell *cell = FindCellByPos(pos.x, pos.y);
    if ( cell )
        retval = cell->ProcessMouseClick(window, pos, event);

#if WXWIN_COMPATIBILITY_2_6
    gs_helperOnMouseClick->retval = retval;
#else
    return retval;
#endif // WXWIN_COMPATIBILITY_2_6
}


MunkHtmlCell *MunkHtmlContainerCell::GetFirstTerminal() const
{
    if ( m_Cells )
    {
        MunkHtmlCell *c2;
        for (MunkHtmlCell *c = m_Cells; c; c = c->GetNext())
        {
            c2 = c->GetFirstTerminal();
            if ( c2 )
                return c2;
        }
    }
    return NULL;
}

MunkHtmlCell *MunkHtmlContainerCell::GetLastTerminal() const
{
    if ( m_Cells )
    {
        // most common case first:
        MunkHtmlCell *c = m_LastCell->GetLastTerminal();
        if ( c )
            return c;

        MunkHtmlCell *ctmp;
        MunkHtmlCell *c2 = NULL;
        for (c = m_Cells; c; c = c->GetNext())
        {
            ctmp = c->GetLastTerminal();
            if ( ctmp )
                c2 = ctmp;
        }
        return c2;
    }
    else
        return NULL;
}


static bool IsEmptyContainer(MunkHtmlContainerCell *cell)
{
    for ( MunkHtmlCell *c = cell->GetFirstChild(); c; c = c->GetNext() )
    {
        if ( !c->IsTerminalCell() || !c->IsFormattingCell() )
            return false;
    }
    return true;
}

void MunkHtmlContainerCell::RemoveExtraSpacing(bool top, bool bottom)
{
    if ( top )
        SetIndent(0, MunkHTML_INDENT_TOP);
    if ( bottom )
        SetIndent(0, MunkHTML_INDENT_BOTTOM);

    if ( m_Cells )
    {
        MunkHtmlCell *c;
        MunkHtmlContainerCell *cont;
        if ( top )
        {
            for ( c = m_Cells; c; c = c->GetNext() )
            {
                if ( c->IsTerminalCell() )
                {
                    if ( !c->IsFormattingCell() )
                        break;
                }
                else
                {
                    cont = (MunkHtmlContainerCell*)c;
                    if ( IsEmptyContainer(cont) )
                    {
                        cont->SetIndent(0, MunkHTML_INDENT_VERTICAL);
                    }
                    else
                    {
                        cont->RemoveExtraSpacing(true, false);
                        break;
                    }
                }
            }
        }

        if ( bottom )
        {
            wxArrayPtrVoid arr;
            for ( c = m_Cells; c; c = c->GetNext() )
                arr.Add((void*)c);

            for ( int i = arr.GetCount() - 1; i >= 0; i--)
            {
                c = (MunkHtmlCell*)arr[i];
                if ( c->IsTerminalCell() )
                {
                    if ( !c->IsFormattingCell() )
                        break;
                }
                else
                {
                    cont = (MunkHtmlContainerCell*)c;
                    if ( IsEmptyContainer(cont) )
                    {
                        cont->SetIndent(0, MunkHTML_INDENT_VERTICAL);
                    }
                    else
                    {
                        cont->RemoveExtraSpacing(false, true);
                        break;
                    }
                }
            }
        }
    }
}




// --------------------------------------------------------------------------
// MunkHtmlColourCell
// --------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(MunkHtmlColourCell, MunkHtmlCell)

void MunkHtmlColourCell::Draw(wxDC& dc,
                            int x, int y,
                            int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                            MunkHtmlRenderingInfo& info)
{
    DrawInvisible(dc, x, y, info);
}

void MunkHtmlColourCell::DrawInvisible(wxDC& dc,
                                     int WXUNUSED(x), int WXUNUSED(y),
                                     MunkHtmlRenderingInfo& info)
{
    MunkHtmlRenderingState& state = info.GetState();
    if (m_Flags & MunkHTML_CLR_FOREGROUND)
    {
	    if (m_Colour != state.GetFgColour()) {
		    state.SetFgColour(m_Colour);
	    } 
	    if (state.GetSelectionState() != MunkHTML_SEL_IN) {
		    dc.SetTextForeground(m_Colour);
	    } else {
		    dc.SetTextForeground(info.GetStyle().GetSelectedTextColour(m_Colour));
	    }
    }
    if (m_Flags & MunkHTML_CLR_BACKGROUND)
    {
	    if (state.GetBgColour() != m_Colour) {
		    state.SetBgColour(m_Colour);
	    }

	    if (state.GetSelectionState() != MunkHTML_SEL_IN) {
		    if (m_Colour != wxNullColour) {
			    dc.SetTextBackground(m_Colour);
			    dc.SetBackground(m_Brush);
			    dc.SetBackgroundMode(wxSOLID);
		    }
	    } else {
		    wxColour c = info.GetStyle().GetSelectedTextBgColour(m_Colour);
		    if (c != wxNullColour) {
			    dc.SetTextBackground(c);
			    dc.SetBackground(wxBrush(c, wxSOLID));
		    }
	    }
    }
}




// ---------------------------------------------------------------------------
// MunkHtmlFontCell
// ---------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(MunkHtmlFontCell, MunkHtmlCell)

void MunkHtmlFontCell::Draw(wxDC& dc,
                          int WXUNUSED(x), int WXUNUSED(y),
                          int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                          MunkHtmlRenderingInfo& info)
{
    dc.SetFont(m_Font);
    info.SetUnderline(m_bUnderline);
}

void MunkHtmlFontCell::DrawInvisible(wxDC& dc, int WXUNUSED(x), int WXUNUSED(y),
                                   MunkHtmlRenderingInfo& info)
{
    dc.SetFont(m_Font);
    info.SetUnderline(m_bUnderline);
}




BEGIN_EVENT_TABLE(MunkHtmlButtonPanel, wxPanel)
    EVT_BUTTON(wxID_ANY, MunkHtmlButtonPanel::OnButtonClicked)
END_EVENT_TABLE()

MunkHtmlButtonPanel::MunkHtmlButtonPanel(MunkHtmlWindow *pParent, int id, const wxString& strLabel, form_id_t form_id)
: wxPanel(pParent, id),
	m_form_id(form_id),
	m_strLabel(strLabel),
	m_pParent(pParent)
{
	wxBoxSizer *pSizer = new wxBoxSizer(wxVERTICAL);
	m_pButton = new wxButton(this, wxID_ANY, strLabel, wxDefaultPosition, wxDefaultSize);

#ifdef __WXMAC__
	// We meed a border of 3 on Mac, otherwise it will overlap
	// with other stuff.
	int border = 3;
#else
	int border = 0;
#endif
	pSizer->Add( m_pButton, 0, wxALL, border);

	SetSizer(pSizer);

	Fit();
}

MunkHtmlButtonPanel::~MunkHtmlButtonPanel()
{
}
	
void MunkHtmlButtonPanel::OnButtonClicked(wxCommandEvent& event)
{
	m_pParent->OnFormSubmitClicked(m_form_id, m_strLabel);
}



MunkHtmlRadioBoxPanel::MunkHtmlRadioBoxPanel(bool bEnable, int selection, wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& point , const wxSize& size, const wxArrayString& choices, int majorDimension, long style)
	: wxPanel(parent, id)
{
	wxBoxSizer *pSizer = new wxBoxSizer(wxVERTICAL);
	m_pRadioBox = new wxRadioBox(this, wxID_ANY, label, point, size, choices, majorDimension, style);
	m_pRadioBox->Enable(bEnable);

	m_pRadioBox->SetSelection(selection);

#ifdef __WXMAC__
	// We meed a border of 3 on Mac, otherwise it will overlap
	// with other stuff.
	int border = 3;
#else
	int border = 0;
#endif
	pSizer->Add( m_pRadioBox, 1, wxALL|wxEXPAND, border);

	SetSizer(pSizer);

	Fit();
}

MunkHtmlRadioBoxPanel::~MunkHtmlRadioBoxPanel()
{
}




MunkHtmlComboBoxPanel::MunkHtmlComboBoxPanel(int selection, wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& point , const wxSize& size, const wxArrayString& choices, long style)
	: wxPanel(parent, id)
{
	wxBoxSizer *pSizer = new wxBoxSizer(wxVERTICAL);
	m_pComboBox = new wxComboBox(this, wxID_ANY, label, point, size, choices, style);

	m_pComboBox->SetSelection(selection);

#ifdef __WXMAC__
	// We meed a border of 3 on Mac, otherwise it will overlap
	// with other stuff.
	int border = 3;
#else
	int border = 0;
#endif
	pSizer->Add( m_pComboBox, 1, wxALL|wxEXPAND, border);

	SetSizer(pSizer);

	Fit();
}

MunkHtmlComboBoxPanel::~MunkHtmlComboBoxPanel()
{
}


// ---------------------------------------------------------------------------
// MunkHtmlWidgetCell
// ---------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(MunkHtmlWidgetCell, MunkHtmlCell)


MunkHtmlWidgetCell::MunkHtmlWidgetCell(wxWindow *wnd, int w)
{
    int sx, sy;
    m_Wnd = wnd;
    m_Wnd->GetSize(&sx, &sy);
    m_Width = sx;
    m_Height = sy;
    m_WidthFloat = w;
}


void MunkHtmlWidgetCell::Draw(wxDC& WXUNUSED(dc),
                            int x, int y,
                            int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                            MunkHtmlRenderingInfo& WXUNUSED(info))
{
    int absx = 0, absy = 0, stx, sty;
    MunkHtmlCell *c = this;

    while (c)
    {
        absx += c->GetPosX();
        absy += c->GetPosY();
        c = c->GetParent();
    }

    wxScrolledWindow *scrolwin =
        wxDynamicCast(m_Wnd->GetParent(), wxScrolledWindow);
    wxCHECK_RET( scrolwin,
                 _T("widget cells can only be placed in MunkHtmlWindow") );

    scrolwin->GetViewStart(&stx, &sty);

    m_Wnd->SetSize(absx - MunkHTML_SCROLL_STEP * stx,
                   absy  - MunkHTML_SCROLL_STEP * sty,
                   m_Width, m_Height,
		   wxSIZE_FORCE);

    /*    
    // Doesn't work on Mac OS X!
    m_Wnd->Refresh(true);
    m_Wnd->Update();
    */
}



void MunkHtmlWidgetCell::DrawInvisible(wxDC& WXUNUSED(dc),
                                     int WXUNUSED(x), int WXUNUSED(y),
                                     MunkHtmlRenderingInfo& WXUNUSED(info))
{
    int absx = 0, absy = 0, stx, sty;
    MunkHtmlCell *c = this;

    while (c)
    {
        absx += c->GetPosX();
        absy += c->GetPosY();
        c = c->GetParent();
    }

    wxScrolledWindow *scrolwin =
	    wxDynamicCast(m_Wnd->GetParent(), wxScrolledWindow);
    wxCHECK_RET( scrolwin,
		 _T("widget cells can only be placed in MunkHtmlWindow") );
    
    scrolwin->GetViewStart(&stx, &sty);
    m_Wnd->SetSize(absx - MunkHTML_SCROLL_STEP * stx,
		   absy  - MunkHTML_SCROLL_STEP * sty,
		   m_Width, m_Height,
		   wxSIZE_FORCE);
    
}


void MunkHtmlWidgetCell::Layout(int w)
{
	int sx, sy;
	m_Wnd->Fit();
	m_Wnd->GetSize(&sx, &sy);
	m_Width = sx;
	m_Height = sy;
	
    if (m_WidthFloat != 0)
    {
        m_Width = (w * m_WidthFloat) / 100;
        m_Wnd->SetSize(m_Width, m_Height);
    }

    MunkHtmlCell::Layout(w);
}



// ----------------------------------------------------------------------------
// MunkHtmlTerminalCellsInterator
// ----------------------------------------------------------------------------

const MunkHtmlCell* MunkHtmlTerminalCellsInterator::operator++()
{
    if ( !m_pos )
        return NULL;

    do
    {
        if ( m_pos == m_to )
        {
            m_pos = NULL;
            return NULL;
        }

        if ( m_pos->GetNext() )
            m_pos = m_pos->GetNext();
        else
        {
            // we must go up the hierarchy until we reach container where this
            // is not the last child, and then go down to first terminal cell:
            while ( m_pos->GetNext() == NULL )
            {
                m_pos = m_pos->GetParent();
                if ( !m_pos )
                    return NULL;
            }
            m_pos = m_pos->GetNext();
        }
        while ( m_pos->GetFirstChild() != NULL )
            m_pos = m_pos->GetFirstChild();
    } while ( !m_pos->IsTerminalCell() );

    return m_pos;
}




/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/htmlwin.cpp
// Purpose:     MunkHtmlWindow class for parsing & displaying HTML (implementation)
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmlwin.cpp 46060 2007-05-16 07:17:11Z VS $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WXPRECOMP
    #include "wx/list.h"
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/dcclient.h"
    #include "wx/frame.h"
    #include "wx/dcmemory.h"
    #include "wx/timer.h"
    #include "wx/settings.h"
    #include "wx/dataobj.h"
#endif

#include "wx/clipbrd.h"

#include "wx/arrimpl.cpp"
#include "wx/listimpl.cpp"

// HTML events:
IMPLEMENT_DYNAMIC_CLASS(MunkHtmlLinkEvent, wxCommandEvent)
IMPLEMENT_DYNAMIC_CLASS(MunkHtmlCellEvent, wxCommandEvent)

DEFINE_EVENT_TYPE(MunkEVT_COMMAND_HTML_CELL_CLICKED)
DEFINE_EVENT_TYPE(MunkEVT_COMMAND_HTML_CELL_HOVER)
DEFINE_EVENT_TYPE(MunkEVT_COMMAND_HTML_LINK_CLICKED)


#if wxUSE_CLIPBOARD
// ----------------------------------------------------------------------------
// MunkHtmlWinAutoScrollTimer: the timer used to generate a stream of scroll
// events when a captured mouse is held outside the window
// ----------------------------------------------------------------------------

class MunkHtmlWinAutoScrollTimer : public wxTimer
{
public:
    MunkHtmlWinAutoScrollTimer(wxScrolledWindow *win,
                      wxEventType eventTypeToSend,
                      int pos, int orient)
    {
        m_win = win;
        m_eventType = eventTypeToSend;
        m_pos = pos;
        m_orient = orient;
    }

    virtual void Notify();

private:
    wxScrolledWindow *m_win;
    wxEventType m_eventType;
    int m_pos,
        m_orient;

    DECLARE_NO_COPY_CLASS(MunkHtmlWinAutoScrollTimer)
};

void MunkHtmlWinAutoScrollTimer::Notify()
{
    // only do all this as long as the window is capturing the mouse
    if ( wxWindow::GetCapture() != m_win )
    {
        Stop();
    }
    else // we still capture the mouse, continue generating events
    {
        // first scroll the window if we are allowed to do it
        wxScrollWinEvent event1(m_eventType, m_pos, m_orient);
        event1.SetEventObject(m_win);
        if ( m_win->GetEventHandler()->ProcessEvent(event1) )
        {
            // and then send a pseudo mouse-move event to refresh the selection
            wxMouseEvent event2(wxEVT_MOTION);
            wxGetMousePosition(&event2.m_x, &event2.m_y);

            // the mouse event coordinates should be client, not screen as
            // returned by wxGetMousePosition
            wxWindow *parentTop = m_win;
            while ( parentTop->GetParent() )
                parentTop = parentTop->GetParent();
            wxPoint ptOrig = parentTop->GetPosition();
            event2.m_x -= ptOrig.x;
            event2.m_y -= ptOrig.y;

            event2.SetEventObject(m_win);

            // FIXME: we don't fill in the other members - ok?
            m_win->GetEventHandler()->ProcessEvent(event2);
        }
        else // can't scroll further, stop
        {
            Stop();
        }
    }
}

#endif // wxUSE_CLIPBOARD



//-----------------------------------------------------------------------------
// MunkHtmlHistoryItem
//-----------------------------------------------------------------------------

// item of history list
class WXDLLIMPEXP_HTML MunkHtmlHistoryItem
{
public:
    MunkHtmlHistoryItem(const wxString& p, const wxString& a) {m_Page = p, m_Anchor = a, m_Pos = 0;}
    int GetPos() const {return m_Pos;}
    void SetPos(int p) {m_Pos = p;}
    const wxString& GetPage() const {return m_Page;}
    const wxString& GetAnchor() const {return m_Anchor;}

private:
    wxString m_Page;
    wxString m_Anchor;
    int m_Pos;
};


//-----------------------------------------------------------------------------
// our private arrays:
//-----------------------------------------------------------------------------

WX_DECLARE_OBJARRAY(MunkHtmlHistoryItem, MunkHtmlHistoryArray);
WX_DEFINE_OBJARRAY(MunkHtmlHistoryArray)


//-----------------------------------------------------------------------------
// MunkHtmlWindowMouseHelper
//-----------------------------------------------------------------------------

MunkHtmlWindowMouseHelper::MunkHtmlWindowMouseHelper(MunkHtmlWindowInterface *iface)
    : m_tmpMouseMoved(false),
      m_tmpLastLink(NULL),
      m_tmpLastCell(NULL),
      m_interface(iface)
{
}

void MunkHtmlWindowMouseHelper::HandleMouseMoved()
{
    m_tmpMouseMoved = true;
}

bool MunkHtmlWindowMouseHelper::HandleMouseClick(MunkHtmlCell *rootCell,
                                               const wxPoint& pos,
                                               const wxMouseEvent& event)
{
    if (!rootCell)
        return false;

    MunkHtmlCell *cell = rootCell->FindCellByPos(pos.x, pos.y);
    // this check is needed because FindCellByPos returns terminal cell and
    // containers may have empty borders -- in this case NULL will be
    // returned
    if (!cell)
        return false;

    // adjust the coordinates to be relative to this cell:
    wxPoint relpos = pos - cell->GetAbsPos(rootCell);

    return OnCellClicked(cell, relpos.x, relpos.y, event);
}

void MunkHtmlWindowMouseHelper::HandleIdle(MunkHtmlCell *rootCell,
                                         const wxPoint& pos)
{
    MunkHtmlCell *cell = rootCell ? rootCell->FindCellByPos(pos.x, pos.y) : NULL;

    if (cell != m_tmpLastCell)
    {
        MunkHtmlLinkInfo *lnk = NULL;
        if (cell)
        {
            // adjust the coordinates to be relative to this cell:
            wxPoint relpos = pos - cell->GetAbsPos(rootCell);
            lnk = cell->GetLink(relpos.x, relpos.y);
        }

        wxCursor cur;
        if (cell)
            cur = cell->GetMouseCursor(m_interface);
        else
            cur = m_interface->GetHTMLCursor(
                        MunkHtmlWindowInterface::HTMLCursor_Default);

        m_interface->GetHTMLWindow()->SetCursor(cur);

        if (lnk != m_tmpLastLink)
        {
            if (lnk)
                m_interface->SetHTMLStatusText(lnk->GetHref());
            else
                m_interface->SetHTMLStatusText(wxEmptyString);

            m_tmpLastLink = lnk;
        }

        m_tmpLastCell = cell;
    }
    else // mouse moved but stayed in the same cell
    {
        if ( cell )
        {
            OnCellMouseHover(cell, pos.x, pos.y);
        }
    }

    m_tmpMouseMoved = false;
}

bool MunkHtmlWindowMouseHelper::OnCellClicked(MunkHtmlCell *cell,
                                            wxCoord x, wxCoord y,
                                            const wxMouseEvent& event)
{
    MunkHtmlCellEvent ev(MunkEVT_COMMAND_HTML_CELL_CLICKED,
                       m_interface->GetHTMLWindow()->GetId(),
                       cell, wxPoint(x,y), event);

    if (!m_interface->GetHTMLWindow()->GetEventHandler()->ProcessEvent(ev))
    {
        // if the event wasn't handled, do the default processing here:

        wxASSERT_MSG( cell, _T("can't be called with NULL cell") );

        cell->ProcessMouseClick(m_interface, ev.GetPoint(), ev.GetMouseEvent());
    }

    // true if a link was clicked, false otherwise
    return ev.GetLinkClicked();
}

void MunkHtmlWindowMouseHelper::OnCellMouseHover(MunkHtmlCell * cell,
                                               wxCoord x,
                                               wxCoord y)
{
    MunkHtmlCellEvent ev(MunkEVT_COMMAND_HTML_CELL_HOVER,
                       m_interface->GetHTMLWindow()->GetId(),
                       cell, wxPoint(x,y), wxMouseEvent());
    m_interface->GetHTMLWindow()->GetEventHandler()->ProcessEvent(ev);
}




//-----------------------------------------------------------------------------
// MunkHtmlWindow
//-----------------------------------------------------------------------------

wxCursor *MunkHtmlWindow::ms_cursorLink = NULL;
wxCursor *MunkHtmlWindow::ms_cursorText = NULL;

void MunkHtmlWindow::CleanUpStatics()
{
    wxDELETE(ms_cursorLink);
    wxDELETE(ms_cursorText);
}

void MunkHtmlWindow::Init()
{
    m_tmpCanDrawLocks = 0;
    m_FS = new wxFileSystem();
#if wxUSE_STATUSBAR
    m_RelatedStatusBar = -1;
#endif // wxUSE_STATUSBAR
    m_RelatedFrame = NULL;
    m_TitleFormat = wxT("%s");
    m_OpenedPage = m_OpenedAnchor = m_OpenedPageTitle = wxEmptyString;
    m_Cell = NULL;
    //m_Parser = new MunkHtmlWinParser(this);
    //m_Parser->SetFS(m_FS);
    m_HistoryPos = -1;
    m_HistoryOn = true;
    m_History = new MunkHtmlHistoryArray;
    SetBorders(0);
    m_selection = NULL;
    m_makingSelection = false;
#if wxUSE_CLIPBOARD
    m_timerAutoScroll = NULL;
    m_lastDoubleClick = 0;
#endif // wxUSE_CLIPBOARD
    m_backBuffer = NULL;
    m_eraseBgInOnPaint = false;
    m_tmpSelFromCell = NULL;
    m_pForms = 0;
}

bool MunkHtmlWindow::Create(wxWindow *parent, wxWindowID id,
                          const wxPoint& pos, const wxSize& size,
                          long style, const wxString& name)
{
    if (!wxScrolledWindow::Create(parent, id, pos, size,
                                  style | wxVSCROLL | wxHSCROLL,
                                  name))
        return false;

    std::string dummy_error_message;
    SetPage(wxT("<?xml version='1.0' encoding='utf-8'?><html><body></body></html>"), dummy_error_message);
    
    return true;
}


MunkHtmlWindow::~MunkHtmlWindow()
{
#if wxUSE_CLIPBOARD
    StopAutoScrolling();
#endif // wxUSE_CLIPBOARD
    HistoryClear();

    delete m_selection;

    delete m_Cell;

    //delete m_Parser;
    delete m_FS;
    delete m_History;
    delete m_backBuffer;

    delete m_pForms;
}


void MunkHtmlWindow::OnFormSubmitClicked(form_id_t form_id, const wxString& strLabel)
{
	if (m_pForms == 0) {
		return;
	}

	MunkHtmlForm *pForm = m_pForms->getForm(form_id);

	if (pForm == 0) {
		return;
	}
	
	std::map<std::string, std::string> REQUEST = pForm->getREQUEST();

	std::string action = pForm->getAction();
	
	std::string method = pForm->getMethod();

	method = "GET"; // FIXME: Always does GET, no matter what!

	OnFormAction(REQUEST, method, action);
}


void MunkHtmlWindow::OnFormAction(const std::map<std::string, std::string>& REQUEST, const std::string& method, const std::string& action)
{
	// Must be overridden in descendant in order to do what it must do
}

void MunkHtmlWindow::SetRelatedFrame(wxFrame* frame, const wxString& format)
{
    m_RelatedFrame = frame;
    m_TitleFormat = format;
}



#if wxUSE_STATUSBAR
void MunkHtmlWindow::SetRelatedStatusBar(int bar)
{
    m_RelatedStatusBar = bar;
}
#endif // wxUSE_STATUSBAR




bool MunkHtmlWindow::SetPage(const wxString& source, std::string& error_message)
{
    m_OpenedPage = m_OpenedAnchor = m_OpenedPageTitle = wxEmptyString;
    return DoSetPage(source, error_message);
}

void MunkHtmlWindow::SetForms(MunkHtmlFormContainer *pForms)
{ 
	delete m_pForms;
	m_pForms = pForms; 
};

bool MunkHtmlWindow::DoSetPage(const wxString& source, std::string& error_message)
{
	MunkHtmlFormElement::ResetNextID();

	m_strPageSource = source;

	wxDELETE(m_selection);

	// we will soon delete all the cells, so clear pointers to them:
	m_tmpSelFromCell = NULL;

	// ...and run the parser on it:
	wxClientDC *dc = new wxClientDC(this);
	dc->SetMapMode(wxMM_TEXT);
	//SetBackgroundColour(wxColour(0xFF, 0xFF, 0xFF));
	SetHTMLBackgroundColour(wxNullColour);
	SetBackgroundImage(wxNullBitmap);

	//m_Parser->SetDC(dc);
	if (m_Cell) {
		delete m_Cell;
		m_Cell = NULL;
	}

	// Clear canvas
	//Clear();

	bool bResult = true;
	error_message = "";
	MunkHtmlParsingStructure ps(this);
	ps.SetDC(dc, 1.0);
	ps.SetFS(GetFS());
	ps.SetHTMLBackgroundColour(this->GetHTMLBackgroundColour());
	bResult = ps.Parse(m_strPageSource, error_message);
	SetTopCell(ps.GetInternalRepresentation());
	SetForms(ps.TakeOverForms());
	ps.SetTopCell(0); // Make sure we don't delete the cells in the ps destructor
	    
	Scroll(0,0);

	delete dc;
	if (bResult) {
		m_Cell->SetIndent(m_Borders, MunkHTML_INDENT_ALL, MunkHTML_UNITS_PIXELS);
		m_Cell->SetAlignHor(MunkHTML_ALIGN_CENTER);
		CreateLayout();
	}
	if (m_tmpCanDrawLocks == 0) {
		Refresh();
		Update();
	}

	// This is necessary on Windows in order to be able to
	// use anchors reliably (!)

	// (But this should be done at the application level, not
	// here! So we comment it out again.)

	// SetFocus();

	return true;
}

// utility function: read a wxString from a wxInputStream
bool ReadString(wxString& str, wxInputStream* s, wxMBConv& conv)
{
	// Read contents into strContents
	wxString strContents;
	wxStringOutputStream sos(&strContents);
	s->Read(sos);

	str = strContents;
	
	std::string strHTML = std::string((const char*) strContents.mb_str(wxConvUTF8));
	return true;
}



bool MunkHtmlWindow::LoadPage(const wxString& location, std::string& error_message)
{
    wxBusyCursor busyCursor;

    wxFSFile *f;
    bool rt_val;
    // bool needs_refresh = false;
    bool needs_refresh = true;

    m_tmpCanDrawLocks++;
    if (m_HistoryOn && (m_HistoryPos != -1))
    {
        // store scroll position into history item:
        int x, y;
        GetViewStart(&x, &y);
        (*m_History)[m_HistoryPos].SetPos(y);
    }

    if (location[0] == wxT('#'))
    {
        // local anchor:
        wxString anch = location.Mid(1) /*1 to end*/;
        m_tmpCanDrawLocks--;
        rt_val = ScrollToAnchor(anch);
        m_tmpCanDrawLocks++;
    }
    else if (location.Find(wxT('#')) != wxNOT_FOUND && location.BeforeFirst(wxT('#')) == m_OpenedPage)
    {
        wxString anch = location.AfterFirst(wxT('#'));
        m_tmpCanDrawLocks--;
        rt_val = ScrollToAnchor(anch);
        m_tmpCanDrawLocks++;
    }
    else if (location.Find(wxT('#')) != wxNOT_FOUND &&
             (m_FS->GetPath() + location.BeforeFirst(wxT('#'))) == m_OpenedPage)
    {
        wxString anch = location.AfterFirst(wxT('#'));
        m_tmpCanDrawLocks--;
        rt_val = ScrollToAnchor(anch);
        m_tmpCanDrawLocks++;
    }

    else
    {
        needs_refresh = true;
#if wxUSE_STATUSBAR
        // load&display it:
        if (m_RelatedStatusBar != -1)
        {
            m_RelatedFrame->SetStatusText(_("Connecting..."), m_RelatedStatusBar);
            Refresh(false);
        }
#endif // wxUSE_STATUSBAR

        f = m_FS->OpenFile(location);
	if (f == NULL) {
		error_message = (const char*) (wxString(wxT("Could not open file with URL:")) + location).mb_str(wxConvUTF8);
		m_tmpCanDrawLocks--;
		return false;
	} 
	
        if (f == NULL) {
		//wxLogError(_("Unable to open requested HTML document: %s"), location.c_str());
		m_tmpCanDrawLocks--;
		SetHTMLStatusText(wxEmptyString);
		return false;
        } else {
		wxString src = wxEmptyString;
		
#if wxUSE_STATUSBAR
		if (m_RelatedStatusBar != -1) {
			wxString msg = _("Loading : ") + location;
			m_RelatedFrame->SetStatusText(msg, m_RelatedStatusBar);
			Refresh(false);
		}
#endif // wxUSE_STATUSBAR

		wxInputStream *s = f->GetStream();
		wxString doc;
		
		if (s == NULL) {
			//wxLogError(_("Cannot open HTML document: %s"), f->GetLocation().c_str());
			src = wxEmptyString;
		}
		
	    // NB: We convert input file to wchar_t here in Unicode mode, based on
	    //     either Content-Type header or <meta> tags. In ANSI mode, we don't
	    //     do it as it is done by MunkHtmlParser (for this reason, we add <meta>
	    //     tag if we used Content-Type header).
#if wxUSE_UNICODE
		wxString tmpdoc;
		ReadString(tmpdoc, s, wxConvUTF8); // Assume UTF-8
		src = tmpdoc;
#else // !wxUSE_UNICODE
		ReadString(doc, s, wxConvLibc);
		// add meta tag if we obtained this through http:
		if (!f->GetMimeType().empty()) {
			wxString hdr;
			wxString mime = f->GetMimeType();
			hdr.Printf(_T("<meta http-equiv=\"Content-Type\" content=\"%s\">"), mime.c_str());
			src = hdr+doc;
		}
#endif

	    

            m_FS->ChangePathTo(f->GetLocation());
            rt_val = SetPage(src, error_message);
            m_OpenedPage = f->GetLocation();
            if (rt_val && f->GetAnchor() != wxEmptyString)
            {
                ScrollToAnchor(f->GetAnchor());
            }

            delete f;

#if wxUSE_STATUSBAR
            if (m_RelatedStatusBar != -1)
                m_RelatedFrame->SetStatusText(_("Done"), m_RelatedStatusBar);
#endif // wxUSE_STATUSBAR
        }
    }

    if (m_HistoryOn) // add this page to history there:
    {
        int c = m_History->GetCount() - (m_HistoryPos + 1);

        if (m_HistoryPos < 0 ||
            (*m_History)[m_HistoryPos].GetPage() != m_OpenedPage ||
            (*m_History)[m_HistoryPos].GetAnchor() != m_OpenedAnchor)
        {
            m_HistoryPos++;
            for (int i = 0; i < c; i++)
                m_History->RemoveAt(m_HistoryPos);
            m_History->Add(new MunkHtmlHistoryItem(m_OpenedPage, m_OpenedAnchor));
        }
    }

    if (m_OpenedPageTitle == wxEmptyString)
        OnSetTitle(wxFileNameFromPath(m_OpenedPage));

    if (needs_refresh) {
	    m_tmpCanDrawLocks--;
	    Refresh();
	    Update();
    } else {
	    m_tmpCanDrawLocks--;
    }

    return rt_val;
}


bool MunkHtmlWindow::LoadFile(const wxFileName& filename, std::string& error_message)
{
    wxString url = wxFileSystem::FileNameToURL(filename);
    return LoadPage(url, error_message);
}


bool MunkHtmlWindow::ScrollToAnchor(const wxString& anchor)
{
    const MunkHtmlCell *c = m_Cell->Find(MunkHTML_COND_ISANCHOR, &anchor);
    if (!c)
    {
	    //wxLogWarning(_("HTML anchor %s does not exist."), anchor.c_str());
	    Scroll(0, 0);
        return false;
    }
    else
    {
        int y;

        for (y = 0; c != NULL; c = c->GetParent()) y += c->GetPosY();
        Scroll(-1, (y / MunkHTML_SCROLL_STEP) - 1);

        //::wxLogWarning(_("HTML anchor %s found at y = %d."), anchor.c_str(), y);

        m_OpenedAnchor = anchor;

        return true;
    }
}


void MunkHtmlWindow::OnSetTitle(const wxString& title)
{
    if (m_RelatedFrame)
    {
        wxString tit;
        tit.Printf(m_TitleFormat, title.c_str());
        m_RelatedFrame->SetTitle(tit);
    }
    m_OpenedPageTitle = title;
}





void MunkHtmlWindow::CreateLayout()
{
    int ClientWidth, ClientHeight;

    if (!m_Cell) return;

    if ( HasFlag(MunkHW_SCROLLBAR_NEVER) )
    {
        SetScrollbars(1, 1, 0, 0); // always off
        GetClientSize(&ClientWidth, &ClientHeight);
        m_Cell->Layout(ClientWidth);
    }
    else // !MunkHW_SCROLLBAR_NEVER
    {
        GetClientSize(&ClientWidth, &ClientHeight);

        m_Cell->Layout(ClientWidth);
        if (ClientHeight < m_Cell->GetHeight() + GetCharHeight())
        {
		/*cheat: top-level frag is always container*/
            SetScrollbars(
                  MunkHTML_SCROLL_STEP, MunkHTML_SCROLL_STEP,
                  m_Cell->GetWidth() / MunkHTML_SCROLL_STEP,
                  (m_Cell->GetHeight() + GetCharHeight()) / MunkHTML_SCROLL_STEP);
#ifdef LOGGING_ENABLED
	    //::wxLogWarning(wxT("UP300: toplevelcell width, height =(%d,%d)"), m_Cell->GetWidth(), m_Cell->GetHeight() + GetCharHeight());
#endif
	    GetClientSize(&ClientWidth, &ClientHeight);
	    m_Cell->Layout(ClientWidth);
            SetScrollbars(
                  MunkHTML_SCROLL_STEP, MunkHTML_SCROLL_STEP,
                  m_Cell->GetWidth() / MunkHTML_SCROLL_STEP,
                  (m_Cell->GetHeight() + GetCharHeight()) / MunkHTML_SCROLL_STEP);
        }
        else /* we fit into window, no need for scrollbars */
        {
            SetScrollbars(MunkHTML_SCROLL_STEP, 1, m_Cell->GetWidth() / MunkHTML_SCROLL_STEP, 0); // disable...
            GetClientSize(&ClientWidth, &ClientHeight);
            m_Cell->Layout(ClientWidth); // ...and relayout
        }
    }
}



bool MunkHtmlWindow::HistoryBack()
{
    wxString a, l;

    if (m_HistoryPos < 1) return false;

    // store scroll position into history item:
    int x, y;
    GetViewStart(&x, &y);
    (*m_History)[m_HistoryPos].SetPos(y);

    // go to previous position:
    m_HistoryPos--;

    l = (*m_History)[m_HistoryPos].GetPage();
    a = (*m_History)[m_HistoryPos].GetAnchor();
    m_HistoryOn = false;
    m_tmpCanDrawLocks++;
    std::string dummy_error_message;
    if (a == wxEmptyString) LoadPage(l, dummy_error_message);
    else LoadPage(l + wxT("#") + a, dummy_error_message);
    m_HistoryOn = true;
    m_tmpCanDrawLocks--;
    Scroll(0, (*m_History)[m_HistoryPos].GetPos());
    Refresh();
    Update();
    return true;
}

bool MunkHtmlWindow::HistoryCanBack()
{
    if (m_HistoryPos < 1) return false;
    return true ;
}


bool MunkHtmlWindow::HistoryForward()
{
    wxString a, l;

    if (m_HistoryPos == -1) return false;
    if (m_HistoryPos >= (int)m_History->GetCount() - 1)return false;

    m_OpenedPage = wxEmptyString; // this will disable adding new entry into history in LoadPage()

    m_HistoryPos++;
    l = (*m_History)[m_HistoryPos].GetPage();
    a = (*m_History)[m_HistoryPos].GetAnchor();
    m_HistoryOn = false;
    m_tmpCanDrawLocks++;
    std::string dummy_error_message;
    if (a == wxEmptyString) LoadPage(l, dummy_error_message);
    else LoadPage(l + wxT("#") + a, dummy_error_message);
    m_HistoryOn = true;
    m_tmpCanDrawLocks--;
    Scroll(0, (*m_History)[m_HistoryPos].GetPos());
    Refresh();
    Update();
    return true;
}

bool MunkHtmlWindow::HistoryCanForward()
{
    if (m_HistoryPos == -1) return false;
    if (m_HistoryPos >= (int)m_History->GetCount() - 1)return false;
    return true ;
}


void MunkHtmlWindow::HistoryClear()
{
    m_History->Empty();
    m_HistoryPos = -1;
}


bool MunkHtmlWindow::IsSelectionEnabled() const
{
#if wxUSE_CLIPBOARD
    return !HasFlag(MunkHW_NO_SELECTION);
#else
    return false;
#endif
}


#if wxUSE_CLIPBOARD
wxString MunkHtmlWindow::DoSelectionToText(MunkHtmlSelection *sel)
{
    if ( !sel )
        return wxEmptyString;
    
    wxClientDC dc(this);
    wxString text;
 
     MunkHtmlTerminalCellsInterator i(sel->GetFromCell(), sel->GetToCell());
     const MunkHtmlCell *prev = NULL;
 
     while ( i )
     {
         // When converting HTML content to plain text, the entire paragraph
         // (container in MunkHTML) goes on single line. A new paragraph (that
         // should go on its own line) has its own container. Therefore, the
         // simplest way of detecting where to insert newlines in plain text
         // is to check if the parent container changed -- if it did, we moved
         // to a new paragraph.
         if ( prev && prev->GetParent() != i->GetParent() )
             text << wxT('\n');
 
         // NB: we don't need to pass the selection to ConvertToText() in the
         //     middle of the selected text; it's only useful when only part of
         //     a cell is selected
         text << i->ConvertToText(sel);
 
        prev = *i;
        ++i;
    }
    return text;
}

wxString MunkHtmlWindow::ToText()
{
    if (m_Cell)
    {
        MunkHtmlSelection sel;
        sel.Set(m_Cell->GetFirstTerminal(), m_Cell->GetLastTerminal());
        return DoSelectionToText(&sel);
    }
    else
        return wxEmptyString;
}

#endif // wxUSE_CLIPBOARD

bool MunkHtmlWindow::CopySelection(ClipboardType t)
{
#if wxUSE_CLIPBOARD
    if ( m_selection )
    {
#if defined(__UNIX__) && !defined(__WXMAC__)
        wxTheClipboard->UsePrimarySelection(t == Primary);
#else // !__UNIX__
        // Primary selection exists only under X11, so don't do anything under
        // the other platforms when we try to access it
        //
        // TODO: this should be abstracted at wxClipboard level!
        if ( t == Primary )
            return false;
#endif // __UNIX__/!__UNIX__

        if ( wxTheClipboard->Open() )
        {
            const wxString txt(SelectionToText());
            wxTheClipboard->SetData(new wxTextDataObject(txt));
            wxTheClipboard->Close();
            /*
		    wxLogTrace(_T("wxhtmlselection"),
                       _("Copied to clipboard:\"%s\""), txt.c_str());
	    */

            return true;
        }
    }
#else
    wxUnusedVar(t);
#endif // wxUSE_CLIPBOARD

    return false;
}


void MunkHtmlWindow::OnLinkClicked(const MunkHtmlLinkInfo& link)
{
    MunkHtmlLinkEvent event(GetId(), link);
    event.SetEventObject(this);
    if (!GetEventHandler()->ProcessEvent(event))
    {
        // the default behaviour is to load the URL in this window
        const wxMouseEvent *e = event.GetLinkInfo().GetEvent();
	std::string dummy_error_message;

        if (e == NULL || e->LeftUp())
		LoadPage(event.GetLinkInfo().GetHref(), dummy_error_message);
    }
}

void MunkHtmlWindow::PaintBackground(wxDC& dc)
{
  
    if ( !m_bmpBg.Ok() )
    {
        dc.SetBackground(wxBrush(GetBackgroundColour(), wxSOLID));
        dc.Clear();

        return;
    }

    // if the image is not fully opaque, we have to erase the background before
    // drawing it, however avoid doing it for opaque images as this would just
    // result in extra flicker without any other effect as background is
    // completely covered anyhow
    if ( m_bmpBg.GetMask() ) {
	    dc.SetBackground(wxBrush(GetBackgroundColour(), wxSOLID));
	    dc.Clear();
    }

    const wxSize sizeWin(GetClientSize());
    const wxSize sizeBmp(m_bmpBg.GetWidth(), m_bmpBg.GetHeight());
    for ( wxCoord x = 0; x < sizeWin.x; x += sizeBmp.x )
    {
        for ( wxCoord y = 0; y < sizeWin.y; y += sizeBmp.y )
        {
            dc.DrawBitmap(m_bmpBg, x, y, true /* use mask */);
        }
    }
}

void MunkHtmlWindow::OnEraseBackground(wxEraseEvent& event)
{
  // Do nothing.
}

void MunkHtmlWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
  if ( m_bmpBg.Ok() ) {
    Refresh(true, NULL);
  }

    wxPaintDC dc(this);

    if (m_tmpCanDrawLocks > 0 || m_Cell == NULL) {
        return;
    }

    int x, y;
    GetViewStart(&x, &y);
    //wxRect rect = GetUpdateRegion().GetBox();

    wxSize sz = GetSize();
    wxRect rect = wxRect(0,0, sz.x, sz.y);


    wxMemoryDC dcm;
    if ( !m_backBuffer )
        m_backBuffer = new wxBitmap(sz.x, sz.y);
    dcm.SelectObject(*m_backBuffer);

    PaintBackground(dcm);

    PrepareDC(dcm);
    dcm.SetMapMode(wxMM_TEXT);
    dcm.SetBackgroundMode(wxTRANSPARENT);

    MunkHtmlRenderingInfo rinfo(GetHTMLBackgroundColour());
    MunkDefaultHtmlRenderingStyle rstyle;
    rinfo.SetSelection(m_selection);
    rinfo.SetStyle(&rstyle);
    m_Cell->Draw(dcm, 0, 0,
                 y * MunkHTML_SCROLL_STEP + rect.GetTop(),
                 y * MunkHTML_SCROLL_STEP + rect.GetBottom(),
                 rinfo);

//#define DEBUG_HTML_SELECTION
#ifdef DEBUG_HTML_SELECTION
    {
    int xc, yc, x, y;
    wxGetMousePosition(&xc, &yc);
    ScreenToClient(&xc, &yc);
    CalcUnscrolledPosition(xc, yc, &x, &y);
    MunkHtmlCell *at = m_Cell->FindCellByPos(x, y);
    MunkHtmlCell *before =
        m_Cell->FindCellByPos(x, y, MunkHTML_FIND_NEAREST_BEFORE);
    MunkHtmlCell *after =
        m_Cell->FindCellByPos(x, y, MunkHTML_FIND_NEAREST_AFTER);

    dcm.SetBrush(*wxTRANSPARENT_BRUSH);
    dcm.SetPen(*wxBLACK_PEN);
    if (at)
        dcm.DrawRectangle(at->GetAbsPos(),
                          wxSize(at->GetWidth(),at->GetHeight()));
    dcm.SetPen(*wxGREEN_PEN);
    if (before)
        dcm.DrawRectangle(before->GetAbsPos().x+1, before->GetAbsPos().y+1,
                          before->GetWidth()-2,before->GetHeight()-2);
    dcm.SetPen(*wxRED_PEN);
    if (after)
        dcm.DrawRectangle(after->GetAbsPos().x+2, after->GetAbsPos().y+2,
                          after->GetWidth()-4,after->GetHeight()-4);
    }
#endif

    dcm.SetDeviceOrigin(0,0);
    dc.SetDeviceOrigin(0,0);
    dc.DestroyClippingRegion();
    dc.Blit(0, rect.GetTop(),
            sz.x, rect.GetBottom() - rect.GetTop() + 1,
            &dcm,
            0, rect.GetTop());
}






void MunkHtmlWindow::OnSize(wxSizeEvent& event)
{
    wxDELETE(m_backBuffer);

    wxScrolledWindow::OnSize(event);
    CreateLayout();

    // Recompute selection if necessary:
    if ( m_selection )
    {
        m_selection->Set(m_selection->GetFromCell(),
                         m_selection->GetToCell());
        m_selection->ClearPrivPos();
    }

    Refresh();
    Update();
}


void MunkHtmlWindow::OnMouseMove(wxMouseEvent& WXUNUSED(event))
{
    MunkHtmlWindowMouseHelper::HandleMouseMoved();
}

void MunkHtmlWindow::OnMouseDown(wxMouseEvent& event)
{
#if wxUSE_CLIPBOARD
    if ( event.LeftDown() && IsSelectionEnabled() )
    {
        const long TRIPLECLICK_LEN = 200; // 0.2 sec after doubleclick
        if ( wxGetLocalTimeMillis() - m_lastDoubleClick <= TRIPLECLICK_LEN )
        {
            SelectLine(CalcUnscrolledPosition(event.GetPosition()));

            (void) CopySelection();
        }
        else
        {
            m_makingSelection = true;

            if ( m_selection )
            {
                wxDELETE(m_selection);
                Refresh();
            }
            m_tmpSelFromPos = CalcUnscrolledPosition(event.GetPosition());
            m_tmpSelFromCell = NULL;

            CaptureMouse();
        }
    }
#else
    wxUnusedVar(event);
#endif // wxUSE_CLIPBOARD
}

void MunkHtmlWindow::OnMouseUp(wxMouseEvent& event)
{
#if wxUSE_CLIPBOARD
    if ( m_makingSelection )
    {
        ReleaseMouse();
        m_makingSelection = false;

        // if m_selection=NULL, the user didn't move the mouse far enough from
        // starting point and the mouse up event is part of a click, the user
        // is not selecting text:
        if ( m_selection )
        {
            CopySelection(Primary);

            // we don't want mouse up event that ended selecting to be
            // handled as mouse click and e.g. follow hyperlink:
            return;
        }
    }
#endif // wxUSE_CLIPBOARD

    SetFocus();

    wxPoint pos = CalcUnscrolledPosition(event.GetPosition());
    MunkHtmlWindowMouseHelper::HandleMouseClick(m_Cell, pos, event);
}

#if wxUSE_CLIPBOARD
void MunkHtmlWindow::OnMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
    if ( !m_makingSelection )
        return;

    // discard the selecting operation
    m_makingSelection = false;
    wxDELETE(m_selection);
    m_tmpSelFromCell = NULL;
    Refresh();
}
#endif // wxUSE_CLIPBOARD


void MunkHtmlWindow::OnInternalIdle()
{
	Update();
    wxWindow::OnInternalIdle();

    if (m_Cell != NULL && DidMouseMove())
    {
#ifdef DEBUG_HTML_SELECTION
        Refresh();
#endif
        int xc, yc, x, y;
        wxGetMousePosition(&xc, &yc);
        ScreenToClient(&xc, &yc);
        CalcUnscrolledPosition(xc, yc, &x, &y);

        MunkHtmlCell *cell = m_Cell->FindCellByPos(x, y);

        // handle selection update:
        if ( m_makingSelection )
        {
            if ( !m_tmpSelFromCell )
                m_tmpSelFromCell = m_Cell->FindCellByPos(
                                         m_tmpSelFromPos.x,m_tmpSelFromPos.y);

            // NB: a trick - we adjust selFromPos to be upper left or bottom
            //     right corner of the first cell of the selection depending
            //     on whether the mouse is moving to the right or to the left.
            //     This gives us more "natural" behaviour when selecting
            //     a line (specifically, first cell of the next line is not
            //     included if you drag selection from left to right over
            //     entire line):
            wxPoint dirFromPos;
            if ( !m_tmpSelFromCell )
            {
                dirFromPos = m_tmpSelFromPos;
            }
            else
            {
                dirFromPos = m_tmpSelFromCell->GetAbsPos();
                if ( x < m_tmpSelFromPos.x )
                {
                    dirFromPos.x += m_tmpSelFromCell->GetWidth();
                    dirFromPos.y += m_tmpSelFromCell->GetHeight();
                }
            }
            bool goingDown = dirFromPos.y < y ||
                             (dirFromPos.y == y && dirFromPos.x < x);

            // determine selection span:
            if ( /*still*/ !m_tmpSelFromCell )
            {
                if (goingDown)
                {
                    m_tmpSelFromCell = m_Cell->FindCellByPos(
                                         m_tmpSelFromPos.x,m_tmpSelFromPos.y,
                                         MunkHTML_FIND_NEAREST_AFTER);
                    if (!m_tmpSelFromCell)
                        m_tmpSelFromCell = m_Cell->GetFirstTerminal();
                }
                else
                {
                    m_tmpSelFromCell = m_Cell->FindCellByPos(
                                         m_tmpSelFromPos.x,m_tmpSelFromPos.y,
                                         MunkHTML_FIND_NEAREST_BEFORE);
                    if (!m_tmpSelFromCell)
                        m_tmpSelFromCell = m_Cell->GetLastTerminal();
                }
            }

            MunkHtmlCell *selcell = cell;
            if (!selcell)
            {
                if (goingDown)
                {
                    selcell = m_Cell->FindCellByPos(x, y,
                                                 MunkHTML_FIND_NEAREST_BEFORE);
                    if (!selcell)
                        selcell = m_Cell->GetLastTerminal();
                }
                else
                {
                    selcell = m_Cell->FindCellByPos(x, y,
                                                 MunkHTML_FIND_NEAREST_AFTER);
                    if (!selcell)
                        selcell = m_Cell->GetFirstTerminal();
                }
            }

            // NB: it may *rarely* happen that the code above didn't find one
            //     of the cells, e.g. if MunkHtmlWindow doesn't contain any
            //     visible cells.
            if ( selcell && m_tmpSelFromCell )
            {
                if ( !m_selection )
                {
                    // start selecting only if mouse movement was big enough
                    // (otherwise it was meant as mouse click, not selection):
                    const int PRECISION = 2;
                    wxPoint diff = m_tmpSelFromPos - wxPoint(x,y);
                    if (abs(diff.x) > PRECISION || abs(diff.y) > PRECISION)
                    {
                        m_selection = new MunkHtmlSelection();
                    }
                }
                if ( m_selection )
                {
                    if ( m_tmpSelFromCell->IsBefore(selcell) )
                    {
                        m_selection->Set(m_tmpSelFromPos, m_tmpSelFromCell,
                                         wxPoint(x,y), selcell);
                    }
                    else
                    {
                        m_selection->Set(wxPoint(x,y), selcell,
                                         m_tmpSelFromPos, m_tmpSelFromCell);
                    }
                    m_selection->ClearPrivPos();
                    Refresh();
                }
            }
        }

        // handle cursor and status bar text changes:

        // NB: because we're passing in 'cell' and not 'm_Cell' (so that the
        //     leaf cell lookup isn't done twice), we need to adjust the
        //     position for the new root:
        wxPoint posInCell(x, y);
        if (cell)
            posInCell -= cell->GetAbsPos();
        MunkHtmlWindowMouseHelper::HandleIdle(cell, posInCell);
    }
}

#if wxUSE_CLIPBOARD
void MunkHtmlWindow::StopAutoScrolling()
{
    if ( m_timerAutoScroll )
    {
        wxDELETE(m_timerAutoScroll);
    }
}

void MunkHtmlWindow::OnMouseEnter(wxMouseEvent& event)
{
    StopAutoScrolling();
    event.Skip();
}

void MunkHtmlWindow::OnMouseLeave(wxMouseEvent& event)
{
    // don't prevent the usual processing of the event from taking place
    event.Skip();

    // when a captured mouse leave a scrolled window we start generate
    // scrolling events to allow, for example, extending selection beyond the
    // visible area in some controls
    if ( wxWindow::GetCapture() == this )
    {
        // where is the mouse leaving?
        int pos, orient;
        wxPoint pt = event.GetPosition();
        if ( pt.x < 0 )
        {
            orient = wxHORIZONTAL;
            pos = 0;
        }
        else if ( pt.y < 0 )
        {
            orient = wxVERTICAL;
            pos = 0;
        }
        else // we're lower or to the right of the window
        {
            wxSize size = GetClientSize();
            if ( pt.x > size.x )
            {
                orient = wxHORIZONTAL;
                pos = GetVirtualSize().x / MunkHTML_SCROLL_STEP;
            }
            else if ( pt.y > size.y )
            {
                orient = wxVERTICAL;
                pos = GetVirtualSize().y / MunkHTML_SCROLL_STEP;
            }
            else // this should be impossible
            {
                // but seems to happen sometimes under wxMSW - maybe it's a bug
                // there but for now just ignore it

                //wxFAIL_MSG( _T("can't understand where has mouse gone") );

                return;
            }
        }

        // only start the auto scroll timer if the window can be scrolled in
        // this direction
        if ( !HasScrollbar(orient) )
            return;

        delete m_timerAutoScroll;
        m_timerAutoScroll = new MunkHtmlWinAutoScrollTimer
                                (
                                    this,
                                    pos == 0 ? wxEVT_SCROLLWIN_LINEUP
                                             : wxEVT_SCROLLWIN_LINEDOWN,
                                    pos,
                                    orient
                                );
        m_timerAutoScroll->Start(50); // FIXME: make configurable
    }
}

void MunkHtmlWindow::OnKeyUp(wxKeyEvent& event)
{
    if ( IsSelectionEnabled() &&
            (event.GetKeyCode() == 'C' && event.CmdDown()) )
    {
        wxClipboardTextEvent evt(wxEVT_COMMAND_TEXT_COPY, GetId());

        evt.SetEventObject(this);

        GetEventHandler()->ProcessEvent(evt);
    }
}

void MunkHtmlWindow::OnCopy(wxCommandEvent& WXUNUSED(event))
{
    (void) CopySelection();
}

void MunkHtmlWindow::OnClipboardEvent(wxClipboardTextEvent& WXUNUSED(event))
{
    (void) CopySelection();
}

void MunkHtmlWindow::OnDoubleClick(wxMouseEvent& event)
{
    // select word under cursor:
    if ( IsSelectionEnabled() )
    {
        SelectWord(CalcUnscrolledPosition(event.GetPosition()));

        (void) CopySelection(Primary);

        m_lastDoubleClick = wxGetLocalTimeMillis();
    }
    else
        event.Skip();
}

void MunkHtmlWindow::SelectWord(const wxPoint& pos)
{
    if ( m_Cell )
    {
        MunkHtmlCell *cell = m_Cell->FindCellByPos(pos.x, pos.y);
        if ( cell )
        {
            delete m_selection;
            m_selection = new MunkHtmlSelection();
            m_selection->Set(cell, cell);
            RefreshRect(wxRect(CalcScrolledPosition(cell->GetAbsPos()),
                               wxSize(cell->GetWidth(), cell->GetHeight())));
        }
    }
}

void MunkHtmlWindow::SelectLine(const wxPoint& pos)
{
    if ( m_Cell )
    {
        MunkHtmlCell *cell = m_Cell->FindCellByPos(pos.x, pos.y);
        if ( cell )
        {
            // We use following heuristic to find a "line": let the line be all
            // cells in same container as the cell under mouse cursor that are
            // neither completely above nor completely bellow the clicked cell
            // (i.e. are likely to be words positioned on same line of text).

            int y1 = cell->GetAbsPos().y;
            int y2 = y1 + cell->GetHeight();
            int y;
            const MunkHtmlCell *c;
            const MunkHtmlCell *before = NULL;
            const MunkHtmlCell *after = NULL;

            // find last cell of line:
            for ( c = cell->GetNext(); c; c = c->GetNext())
            {
                y = c->GetAbsPos().y;
                if ( y + c->GetHeight() > y1 && y < y2 )
                    after = c;
                else
                    break;
            }
            if ( !after )
                after = cell;

            // find first cell of line:
            for ( c = cell->GetParent()->GetFirstChild();
                    c && c != cell; c = c->GetNext())
            {
                y = c->GetAbsPos().y;
                if ( y + c->GetHeight() > y1 && y < y2 )
                {
                    if ( ! before )
                        before = c;
                }
                else
                    before = NULL;
            }
            if ( !before )
                before = cell;

            delete m_selection;
            m_selection = new MunkHtmlSelection();
            m_selection->Set(before, after);

            Refresh();
        }
    }
}

void MunkHtmlWindow::SelectAll()
{
    if ( m_Cell )
    {
        delete m_selection;
        m_selection = new MunkHtmlSelection();
        m_selection->Set(m_Cell->GetFirstTerminal(), m_Cell->GetLastTerminal());
        Refresh();
    }
}

#endif // wxUSE_CLIPBOARD



IMPLEMENT_DYNAMIC_CLASS(MunkHtmlWindow,wxScrolledWindow)

BEGIN_EVENT_TABLE(MunkHtmlWindow, wxScrolledWindow)
    EVT_SIZE(MunkHtmlWindow::OnSize)
    EVT_LEFT_DOWN(MunkHtmlWindow::OnMouseDown)
    EVT_LEFT_UP(MunkHtmlWindow::OnMouseUp)
    EVT_RIGHT_UP(MunkHtmlWindow::OnMouseUp)
    EVT_MOTION(MunkHtmlWindow::OnMouseMove)
    EVT_ERASE_BACKGROUND(MunkHtmlWindow::OnEraseBackground)
    EVT_PAINT(MunkHtmlWindow::OnPaint)
#if wxUSE_CLIPBOARD
    EVT_LEFT_DCLICK(MunkHtmlWindow::OnDoubleClick)
    EVT_ENTER_WINDOW(MunkHtmlWindow::OnMouseEnter)
    EVT_LEAVE_WINDOW(MunkHtmlWindow::OnMouseLeave)
    EVT_MOUSE_CAPTURE_LOST(MunkHtmlWindow::OnMouseCaptureLost)
    EVT_KEY_UP(MunkHtmlWindow::OnKeyUp)
    EVT_MENU(wxID_COPY, MunkHtmlWindow::OnCopy)
    EVT_TEXT_COPY(wxID_ANY, MunkHtmlWindow::OnClipboardEvent)
#endif // wxUSE_CLIPBOARD
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
// MunkHtmlWindowInterface implementation in MunkHtmlWindow
//-----------------------------------------------------------------------------

void MunkHtmlWindow::SetHTMLWindowTitle(const wxString& title)
{
    OnSetTitle(title);
}

void MunkHtmlWindow::OnHTMLLinkClicked(const MunkHtmlLinkInfo& link)
{
    OnLinkClicked(link);
}

MunkHtmlOpeningStatus MunkHtmlWindow::OnHTMLOpeningURL(MunkHtmlURLType type,
                                                   const wxString& url,
                                                   wxString *redirect) const
{
    return OnOpeningURL(type, url, redirect);
}

wxPoint MunkHtmlWindow::HTMLCoordsToWindow(MunkHtmlCell *WXUNUSED(cell),
                                         const wxPoint& pos) const
{
    return CalcScrolledPosition(pos);
}

wxWindow* MunkHtmlWindow::GetHTMLWindow()
{
    return this;
}

wxColour MunkHtmlWindow::GetHTMLBackgroundColour() const
{
	return m_HTML_background_colour;
}

void MunkHtmlWindow::SetHTMLBackgroundColour(const wxColour& clr)
{
	m_HTML_background_colour = clr;
	SetBackgroundColour(clr);
}

void MunkHtmlWindow::SetHTMLBackgroundImage(const wxBitmap& bmpBg)
{
    SetBackgroundImage(bmpBg);
    SetHTMLBackgroundColour(wxNullColour);
}

void MunkHtmlWindow::SetHTMLStatusText(const wxString& text)
{
#if wxUSE_STATUSBAR
    if (m_RelatedStatusBar != -1)
        m_RelatedFrame->SetStatusText(text, m_RelatedStatusBar);
#else
    wxUnusedVar(text);
#endif // wxUSE_STATUSBAR
}

/*static*/
wxCursor MunkHtmlWindow::GetDefaultHTMLCursor(HTMLCursor type)
{
    switch (type)
    {
        case HTMLCursor_Link:
            if ( !ms_cursorLink )
                ms_cursorLink = new wxCursor(wxCURSOR_HAND);
            return *ms_cursorLink;

        case HTMLCursor_Text:
            if ( !ms_cursorText )
                ms_cursorText = new wxCursor(wxCURSOR_IBEAM);
            return *ms_cursorText;

        case HTMLCursor_Default:
        default:
            return *wxSTANDARD_CURSOR;
    }
}

wxCursor MunkHtmlWindow::GetHTMLCursor(HTMLCursor type) const
{
    return GetDefaultHTMLCursor(type);
}


/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_tables.cpp
// Purpose:     MunkHtml module for tables
// Author:      Vaclav Slavik
// RCS-ID:      $Id: m_tables.cpp 48722 2007-09-16 11:19:46Z VZ $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#define TABLE_BORDER_CLR_1  wxColour(0xC5, 0xC2, 0xC5)
#define TABLE_BORDER_CLR_2  wxColour(0x62, 0x61, 0x62)


//-----------------------------------------------------------------------------
// MunkHtmlTableCell
//-----------------------------------------------------------------------------



MunkHtmlTableCell::MunkHtmlTableCell(MunkHtmlContainerCell *parent, const MunkHtmlTag& tag, double pixel_scale)
 : MunkHtmlContainerCell(parent)
{
    m_PixelScale = pixel_scale;
    m_HasBorders =
            (tag.HasParam(wxT("BORDER")) && tag.GetParam(wxT("BORDER")) != wxT("0"));
    m_ColsInfo = NULL;
    m_NumCols = m_NumRows = 0;
    m_CellInfo = NULL;
    m_ActualCol = m_ActualRow = -1;

    /* scan params: */
    if (tag.HasParam(wxT("BGCOLOR")))
    {
        tag.GetParamAsColour(wxT("BGCOLOR"), &m_tBkg);
        if (m_tBkg.Ok())
            SetBackgroundColour(m_tBkg);
    }
    if (tag.HasParam(wxT("VALIGN")))
        m_tValign = tag.GetParam(wxT("VALIGN"));
    else
        m_tValign = wxEmptyString;
    if (!tag.GetParamAsInt(wxT("CELLSPACING"), &m_Spacing))
        m_Spacing = 2;
    if (!tag.GetParamAsInt(wxT("CELLPADDING"), &m_Padding))
        m_Padding = 3;
    m_Spacing = (int)(m_PixelScale * (double)m_Spacing);
    m_Padding = (int)(m_PixelScale * (double)m_Padding);

    if (m_HasBorders)
        SetBorder(TABLE_BORDER_CLR_1, TABLE_BORDER_CLR_2);
}



MunkHtmlTableCell::~MunkHtmlTableCell()
{
    if (m_ColsInfo) free(m_ColsInfo);
    if (m_CellInfo)
    {
        for (int i = 0; i < m_NumRows; i++)
            free(m_CellInfo[i]);
        free(m_CellInfo);
    }
}


void MunkHtmlTableCell::RemoveExtraSpacing(bool WXUNUSED(top),
                                         bool WXUNUSED(bottom))
{
    // Don't remove any spacing in the table -- it's always desirable,
    // because it's part of table's definition.
    // (If MunkHtmlContainerCell::RemoveExtraSpacing() was applied to tables,
    // then upper left cell of a table would be positioned above other cells
    // if the table was the first element on the page.)
}

void MunkHtmlTableCell::ReallocCols(int cols)
{
    int i,j;

    for (i = 0; i < m_NumRows; i++)
    {
        m_CellInfo[i] = (cellStruct*) realloc(m_CellInfo[i], sizeof(cellStruct) * cols);
        for (j = m_NumCols; j < cols; j++)
            m_CellInfo[i][j].flag = cellFree;
    }

    m_ColsInfo = (colStruct*) realloc(m_ColsInfo, sizeof(colStruct) * cols);
    for (j = m_NumCols; j < cols; j++)
    {
           m_ColsInfo[j].width = 0;
           m_ColsInfo[j].units = MunkHTML_UNITS_PERCENT;
           m_ColsInfo[j].minWidth = m_ColsInfo[j].maxWidth = -1;
    }

    m_NumCols = cols;
}



void MunkHtmlTableCell::ReallocRows(int rows)
{
    m_CellInfo = (cellStruct**) realloc(m_CellInfo, sizeof(cellStruct*) * rows);
    for (int row = m_NumRows; row < rows ; row++)
    {
        if (m_NumCols == 0)
            m_CellInfo[row] = NULL;
        else
        {
            m_CellInfo[row] = (cellStruct*) malloc(sizeof(cellStruct) * m_NumCols);
            for (int col = 0; col < m_NumCols; col++)
                m_CellInfo[row][col].flag = cellFree;
        }
    }
    m_NumRows = rows;
}


void MunkHtmlTableCell::AddRow(const MunkHtmlTag& tag)
{
    m_ActualCol = -1;
    // VS: real allocation of row entry is done in AddCell in order
    //     to correctly handle empty rows (i.e. "<tr></tr>")
    //     m_ActualCol == -1 indicates that AddCell has to allocate new row.

    // scan params:
    m_rBkg = m_tBkg;
    if (tag.HasParam(wxT("BGCOLOR")))
        tag.GetParamAsColour(wxT("BGCOLOR"), &m_rBkg);
    if (tag.HasParam(wxT("VALIGN")))
        m_rValign = tag.GetParam(wxT("VALIGN"));
    else
        m_rValign = m_tValign;
}



void MunkHtmlTableCell::AddCell(MunkHtmlContainerCell *cell, const MunkHtmlTag& tag)
{
    // Is this cell in new row?
    // VS: we can't do it in AddRow, see my comment there
    if (m_ActualCol == -1)
    {
        if (m_ActualRow + 1 > m_NumRows - 1)
            ReallocRows(m_ActualRow + 2);
        m_ActualRow++;
    }

    // cells & columns:
    do
    {
        m_ActualCol++;
    } while ((m_ActualCol < m_NumCols) &&
             (m_CellInfo[m_ActualRow][m_ActualCol].flag != cellFree));

    if (m_ActualCol > m_NumCols - 1)
        ReallocCols(m_ActualCol + 1);

    int r = m_ActualRow, c = m_ActualCol;

    m_CellInfo[r][c].cont = cell;
    m_CellInfo[r][c].colspan = 1;
    m_CellInfo[r][c].rowspan = 1;
    m_CellInfo[r][c].flag = cellUsed;
    m_CellInfo[r][c].minheight = 0;
    m_CellInfo[r][c].valign = MunkHTML_ALIGN_TOP;

    /* scan for parameters: */

    // width:
    {
        if (tag.HasParam(wxT("WIDTH")))
        {
            wxString wd = tag.GetParam(wxT("WIDTH"));

            if (wd[wd.length()-1] == wxT('%')) {
		    if ( wxSscanf(wd.c_str(), wxT("%i%%"), &m_ColsInfo[c].width) == 1 ) {
			    m_ColsInfo[c].units = MunkHTML_UNITS_PERCENT;
		    }
	    } else {
		    long width;
		    if ( wd.ToLong(&width) ) {
			    m_ColsInfo[c].width = (int)(m_PixelScale * (double)width);
			    m_ColsInfo[c].units = MunkHTML_UNITS_PIXELS;
                 }
		    
	    }
        }
    }


    // spanning:
    {
        tag.GetParamAsInt(wxT("COLSPAN"), &m_CellInfo[r][c].colspan);
        tag.GetParamAsInt(wxT("ROWSPAN"), &m_CellInfo[r][c].rowspan);

        // VS: the standard says this about col/rowspan:
        //     "This attribute specifies the number of rows spanned by the
        //     current cell. The default value of this attribute is one ("1").
        //     The value zero ("0") means that the cell spans all rows from the
        //     current row to the last row of the table." All mainstream
        //     browsers act as if 0==1, though, and so does MunkHTML.
        if (m_CellInfo[r][c].colspan < 1)
            m_CellInfo[r][c].colspan = 1;
        if (m_CellInfo[r][c].rowspan < 1)
            m_CellInfo[r][c].rowspan = 1;

        if ((m_CellInfo[r][c].colspan > 1) || (m_CellInfo[r][c].rowspan > 1))
        {
            int i, j;

            if (r + m_CellInfo[r][c].rowspan > m_NumRows)
                ReallocRows(r + m_CellInfo[r][c].rowspan);
            if (c + m_CellInfo[r][c].colspan > m_NumCols)
                ReallocCols(c + m_CellInfo[r][c].colspan);
            for (i = r; i < r + m_CellInfo[r][c].rowspan; i++)
                for (j = c; j < c + m_CellInfo[r][c].colspan; j++)
                    m_CellInfo[i][j].flag = cellSpan;
            m_CellInfo[r][c].flag = cellUsed;
        }
    }

    //background color:
    {
        wxColour bk = m_rBkg;
        if (tag.HasParam(wxT("BGCOLOR")))
            tag.GetParamAsColour(wxT("BGCOLOR"), &bk);
        if (bk.Ok())
            cell->SetBackgroundColour(bk);
    }
    if (m_HasBorders)
        cell->SetBorder(TABLE_BORDER_CLR_2, TABLE_BORDER_CLR_1);

    // vertical alignment:
    {
        wxString valign;
        if (tag.HasParam(wxT("VALIGN")))
            valign = tag.GetParam(wxT("VALIGN"));
        else
            valign = m_tValign;
        valign.MakeUpper();
        if (valign == wxT("TOP"))
            m_CellInfo[r][c].valign = MunkHTML_ALIGN_TOP;
        else if (valign == wxT("BOTTOM"))
            m_CellInfo[r][c].valign = MunkHTML_ALIGN_BOTTOM;
        else m_CellInfo[r][c].valign = MunkHTML_ALIGN_CENTER;
    }

    // nowrap
    if (tag.HasParam(wxT("NOWRAP")))
        m_CellInfo[r][c].nowrap = true;
    else
        m_CellInfo[r][c].nowrap = false;

    cell->SetIndent(m_Padding, MunkHTML_INDENT_ALL, MunkHTML_UNITS_PIXELS);
}

void MunkHtmlTableCell::ComputeMinMaxWidths()
{
    if (m_NumCols == 0 || m_ColsInfo[0].minWidth != wxDefaultCoord) return;

    m_MaxTotalWidth = 0;
    int percentage = 0;
    for (int c = 0; c < m_NumCols; c++)
    {
        for (int r = 0; r < m_NumRows; r++)
        {
            cellStruct& cell = m_CellInfo[r][c];
            if (cell.flag == cellUsed)
            {
                cell.cont->Layout(2*m_Padding + 1);
                int maxWidth = cell.cont->GetMaxTotalWidth();
                int width = cell.nowrap?maxWidth:cell.cont->GetWidth();
                width -= (cell.colspan-1) * m_Spacing;
                maxWidth -= (cell.colspan-1) * m_Spacing;
                // HTML 4.0 says it is acceptable to distribute min/max
                width /= cell.colspan;
                maxWidth /= cell.colspan;
                for (int j = 0; j < cell.colspan; j++) {
                    if (width > m_ColsInfo[c+j].minWidth)
                        m_ColsInfo[c+j].minWidth = width;
                    if (maxWidth > m_ColsInfo[c+j].maxWidth)
                        m_ColsInfo[c+j].maxWidth = maxWidth;
                }
            }
        }
        // Calculate maximum table width, required for nested tables
        if (m_ColsInfo[c].units == MunkHTML_UNITS_PIXELS)
            m_MaxTotalWidth += wxMax(m_ColsInfo[c].width, m_ColsInfo[c].minWidth);
        else if ((m_ColsInfo[c].units == MunkHTML_UNITS_PERCENT) && (m_ColsInfo[c].width != 0))
            percentage += m_ColsInfo[c].width;
        else
            m_MaxTotalWidth += m_ColsInfo[c].maxWidth;
    }

    if (percentage >= 100)
    {
        // Table would have infinite length
        // Make it ridiculous large
        m_MaxTotalWidth = 0xFFFFFF;
    }
    else
        m_MaxTotalWidth = m_MaxTotalWidth * 100 / (100 - percentage);

    m_MaxTotalWidth += (m_NumCols + 1) * m_Spacing;
}

void MunkHtmlTableCell::Layout(int w)
{
    ComputeMinMaxWidths();

    MunkHtmlCell::Layout(w);

    /*

    WIDTH ADJUSTING :

    */

    if (m_WidthFloatUnits == MunkHTML_UNITS_PERCENT)
    {
        if (m_WidthFloat < 0)
        {
            if (m_WidthFloat < -100)
                m_WidthFloat = -100;
            m_Width = (100 + m_WidthFloat) * w / 100;
        }
        else
        {
            if (m_WidthFloat > 100)
                m_WidthFloat = 100;
            m_Width = m_WidthFloat * w / 100;
        }
    }
    else
    {
        if (m_WidthFloat < 0) m_Width = w + m_WidthFloat;
        else m_Width = m_WidthFloat;
    }


    /*

    LAYOUTING :

    */

    /* 1.  setup columns widths:

           The algorithm tries to keep the table size less than w if possible.
       */
    {
        int wpix = m_Width - (m_NumCols + 1) * m_Spacing;
        int i, j;

        // 1a. setup fixed-width columns:
        for (i = 0; i < m_NumCols; i++)
            if (m_ColsInfo[i].units == MunkHTML_UNITS_PIXELS)
            {
                m_ColsInfo[i].pixwidth = wxMax(m_ColsInfo[i].width,
                                               m_ColsInfo[i].minWidth);
                wpix -= m_ColsInfo[i].pixwidth;
            }

        // 1b. Calculate maximum possible width if line wrapping would be disabled
        // Recalculate total width if m_WidthFloat is zero to keep tables as small
        // as possible.
        int maxWidth = 0;
        for (i = 0; i < m_NumCols; i++)
            if (m_ColsInfo[i].width == 0)
            {
                maxWidth += m_ColsInfo[i].maxWidth;
            }

        if (!m_WidthFloat)
        {
            // Recalculate table width since no table width was initially given
            int newWidth = m_Width - wpix +  maxWidth;

            // Make sure that floating-width columns will have the right size.
            // Calculate sum of all floating-width columns
            int percentage = 0;
            for (i = 0; i < m_NumCols; i++)
                if ((m_ColsInfo[i].units == MunkHTML_UNITS_PERCENT) && (m_ColsInfo[i].width != 0))
                    percentage += m_ColsInfo[i].width;

            if (percentage >= 100)
                newWidth = w;
            else
                newWidth = newWidth * 100 / (100 - percentage);

            newWidth = wxMin(newWidth, w - (m_NumCols + 1) * m_Spacing);
            wpix -= m_Width - newWidth;
            m_Width = newWidth;
        }


        // 1c. setup floating-width columns:
        int wtemp = wpix;
        for (i = 0; i < m_NumCols; i++)
            if ((m_ColsInfo[i].units == MunkHTML_UNITS_PERCENT) && (m_ColsInfo[i].width != 0))
            {
                m_ColsInfo[i].pixwidth = wxMin(m_ColsInfo[i].width, 100) * wpix / 100;

                // Make sure to leave enough space for the other columns
                int minRequired = 0;
                for (j = 0; j < m_NumCols; j++)
                {
                    if ((m_ColsInfo[j].units == MunkHTML_UNITS_PERCENT && j > i) ||
                        !m_ColsInfo[j].width)
                        minRequired += m_ColsInfo[j].minWidth;
                }
                m_ColsInfo[i].pixwidth = wxMax(wxMin(wtemp - minRequired, m_ColsInfo[i].pixwidth), m_ColsInfo[i].minWidth);

                wtemp -= m_ColsInfo[i].pixwidth;
            }
        wpix = wtemp;

        // 1d. setup default columns (no width specification supplied):
        // The algorithm assigns calculates the maximum possible width if line
        // wrapping would be disabled and assigns column width as a fraction
        // based upon the maximum width of a column
        // FIXME: I'm not sure if this algorithm is conform to HTML standard,
        //        though it seems to be much better than the old one

        for (i = j = 0; i < m_NumCols; i++)
            if (m_ColsInfo[i].width == 0) j++;
        if (wpix < 0)
            wpix = 0;

        // Assign widths
        for (i = 0; i < m_NumCols; i++)
            if (m_ColsInfo[i].width == 0)
            {
                // Assign with, make sure not to drop below minWidth
                if (maxWidth)
                    m_ColsInfo[i].pixwidth = (int)(wpix * (m_ColsInfo[i].maxWidth / (float)maxWidth) + 0.5);
                else
                    m_ColsInfo[i].pixwidth = wpix / j;

                // Make sure to leave enough space for the other columns
                int minRequired = 0;
                int r;
                for (r = i + 1; r < m_NumCols; r++)
                {
                    if (!m_ColsInfo[r].width)
                        minRequired += m_ColsInfo[r].minWidth;
                }
                m_ColsInfo[i].pixwidth = wxMax(wxMin(wpix - minRequired, m_ColsInfo[i].pixwidth), m_ColsInfo[i].minWidth);

                if (maxWidth)
                {
                    if (m_ColsInfo[i].pixwidth > (wpix * (m_ColsInfo[i].maxWidth / (float)maxWidth) + 0.5))
                    {
                        int diff = (int)(m_ColsInfo[i].pixwidth - (wpix * m_ColsInfo[i].maxWidth / (float)maxWidth + 0.5));
                        maxWidth += diff - m_ColsInfo[i].maxWidth;
                    }
                    else
                        maxWidth -= m_ColsInfo[i].maxWidth;
                }
                wpix -= m_ColsInfo[i].pixwidth;
            }
    }

    /* 2.  compute positions of columns: */
    {
        int wpos = m_Spacing;
        for (int i = 0; i < m_NumCols; i++)
        {
            m_ColsInfo[i].leftpos = wpos;
            wpos += m_ColsInfo[i].pixwidth + m_Spacing;
        }

        // add the remaining space to the last column
        if (m_NumCols > 0 && wpos < m_Width)
            m_ColsInfo[m_NumCols-1].pixwidth += m_Width - wpos;
    }

    /* 3.  sub-layout all cells: */
    {
        int *ypos = new int[m_NumRows + 1];

        int actcol, actrow;
        int fullwid;
        MunkHtmlContainerCell *actcell;

        ypos[0] = m_Spacing;
        for (actrow = 1; actrow <= m_NumRows; actrow++) ypos[actrow] = -1;
        for (actrow = 0; actrow < m_NumRows; actrow++)
        {
            if (ypos[actrow] == -1) ypos[actrow] = ypos[actrow-1];
            // 3a. sub-layout and detect max height:

            for (actcol = 0; actcol < m_NumCols; actcol++) {
                if (m_CellInfo[actrow][actcol].flag != cellUsed) continue;
                actcell = m_CellInfo[actrow][actcol].cont;
                fullwid = 0;
                for (int i = actcol; i < m_CellInfo[actrow][actcol].colspan + actcol; i++)
                    fullwid += m_ColsInfo[i].pixwidth;
                fullwid += (m_CellInfo[actrow][actcol].colspan - 1) * m_Spacing;
                actcell->SetMinHeight(m_CellInfo[actrow][actcol].minheight, m_CellInfo[actrow][actcol].valign);
                actcell->Layout(fullwid);

                if (ypos[actrow] + actcell->GetHeight() + m_CellInfo[actrow][actcol].rowspan * m_Spacing > ypos[actrow + m_CellInfo[actrow][actcol].rowspan])
                    ypos[actrow + m_CellInfo[actrow][actcol].rowspan] =
                            ypos[actrow] + actcell->GetHeight() + m_CellInfo[actrow][actcol].rowspan * m_Spacing;
            }
        }

        for (actrow = 0; actrow < m_NumRows; actrow++)
        {
            // 3b. place cells in row & let'em all have same height:

            for (actcol = 0; actcol < m_NumCols; actcol++)
            {
                if (m_CellInfo[actrow][actcol].flag != cellUsed) continue;
                actcell = m_CellInfo[actrow][actcol].cont;
                actcell->SetMinHeight(
                                 ypos[actrow + m_CellInfo[actrow][actcol].rowspan] - ypos[actrow] -  m_Spacing,
                                 m_CellInfo[actrow][actcol].valign);
                fullwid = 0;
                for (int i = actcol; i < m_CellInfo[actrow][actcol].colspan + actcol; i++)
                    fullwid += m_ColsInfo[i].pixwidth;
                fullwid += (m_CellInfo[actrow][actcol].colspan - 1) * m_Spacing;
                actcell->Layout(fullwid);
                actcell->SetPos(m_ColsInfo[actcol].leftpos, ypos[actrow]);
            }
        }
        m_Height = ypos[m_NumRows];
        delete[] ypos;
    }

    /* 4. adjust table's width if it was too small: */
    if (m_NumCols > 0)
    {
        int twidth = m_ColsInfo[m_NumCols-1].leftpos +
                     m_ColsInfo[m_NumCols-1].pixwidth + m_Spacing;
        if (twidth > m_Width)
            m_Width = twidth;
    }
}






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


MunkQDHTMLHandler::MunkQDHTMLHandler(MunkHtmlParsingStructure *pCanvas)
	: m_chars(""),
	  m_bInBody(false),
	  m_pCurrentContainer(0),
	  m_CharHeight(0),
	  m_CharWidth(0),
	  m_UseLink(false)
{
	m_cur_form_id = 0;
	m_pCanvas = pCanvas;
	m_pDC = m_pCanvas->GetDC(); // new wxClientDC(pCanvas);
	m_Align = MunkHTML_ALIGN_LEFT;
	m_tmpStrBuf = NULL;
	m_tmpStrBufSize = 0;
	m_tmpLastWasSpace = false;
	m_nMagnification = 100;
	m_Numbering = 0;
	m_lastWordCell = NULL;
        m_tAlignStack.push(wxEmptyString);
	m_rAlign = wxEmptyString;
	startMunkHTMLFontAttributeStack();
	m_smallcaps_stack.push(false);
	SetCharWidthHeight();

	OpenContainer();

	// Do it again, so that there always is a top
	OpenContainer();

	m_pCurrentContainer->InsertCell(new MunkHtmlColourCell(GetActualColor()));
	wxColour windowColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW) ;


	wxColour backgroundColour = m_pCanvas->GetHTMLBackgroundColour();
	if (backgroundColour != wxNullColour) {
		m_pCurrentContainer->InsertCell
			(
			 new MunkHtmlColourCell
			 (m_pCanvas ? backgroundColour : windowColour,
			  MunkHTML_CLR_BACKGROUND
			  )
			 );
	} else {
		// std::cerr << "UP261: background colour == wxNullColour " << std::endl;
	}

	m_pCurrentContainer->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), false));
}



MunkQDHTMLHandler::~MunkQDHTMLHandler()
{
	delete[] m_tmpStrBuf;
}


void MunkQDHTMLHandler::startElement(const std::string& tag, const MunkAttributeMap& attrs) throw(MunkQDException)
{
	handleChars();

	if (tag == "a") {
		if (attrs.find("name") != attrs.end()) {
			wxString name(getMunkAttribute(attrs, "name").c_str(), wxConvUTF8);
			startAnchorNAME();
			GetContainer()->InsertCell(new MunkHtmlAnchorCell(name));
			m_anchor_type_stack.push(kATNAME);
		} else if (attrs.find("href") != attrs.end()) {
			wxString href(getMunkAttribute(attrs, "href").c_str(), wxConvUTF8);
			bool bVisible = true;
			if (attrs.find("visible") != attrs.end()) {
				std::string visible_bool = attrs.find("visible")->second;
				if (visible_bool == "false") {
					bVisible = false;
				}
			}

			MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
			
			// Support COLOR tag on <a> element.
			// This is non-standard, but useful.
			// NONSTANDARD
			wxColour linkColour = *wxBLUE; // standard is BLUE
			if (munkTag.HasParam(wxT("COLOR"))) {
				wxColour clrFgr;
				munkTag.GetParamAsColour(wxT("COLOR"), &clrFgr);
				if (clrFgr.Ok()) {
					linkColour = clrFgr;
				}
			}

			startAnchorHREF(bVisible, linkColour);
			
			GetContainer()->InsertCell(new MunkHtmlColourCell(GetActualColor()));
			GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));


			// Support BGCOLOR tag on <a> element.
			// This is non-standard, but useful.
			// NONSTANDARD		
			if (munkTag.HasParam(wxT("BGCOLOR"))) {
				wxColour clrBkg;
				munkTag.GetParamAsColour(wxT("BGCOLOR"), &clrBkg);
				if (clrBkg.Ok()) {
					GetContainer()->InsertCell(new MunkHtmlColourCell(clrBkg, MunkHTML_CLR_BACKGROUND));
				}
			}


			PushLink(MunkHtmlLinkInfo(href, 
						  wxT(""))); // target
			m_anchor_type_stack.push(kATHREF);
		} else {
			throw MunkQDException(std::string("Anchor start-tag <a ....> without either href or name! Please add either href or name"));

		}
	} else if (tag == "p") {
		if (GetContainer()->GetFirstChild() != NULL){
				CloseContainer();
				OpenContainer();
		}
		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
		GetContainer()->SetAlign(munkTag);

		// Support BGCOLOR tag on <p> element.
		// This is non-standard, but useful.
		// NONSTANDARD		
		if (munkTag.HasParam(wxT("BGCOLOR"))) {
			wxColour clrBkg;
			munkTag.GetParamAsColour(wxT("BGCOLOR"), &clrBkg);
			if (clrBkg.Ok()) {
				GetContainer()->SetBackgroundColour(clrBkg);
			}
		}

		// NONSTANDARD		
		if (munkTag.HasParam(wxT("MARGIN_LEFT"))) {
			double marginLeft;
			if (!munkTag.GetParamAsLengthInInches(wxT("MARGIN_LEFT"), &marginLeft)) {
				marginLeft = 0.0;
			}
			// Convert to pixels, based on 72 dpi
			// FIXME: What about printing?
			marginLeft = (marginLeft*72.0);

			GetContainer()->SetIndent((int) marginLeft, MunkHTML_INDENT_LEFT, MunkHTML_UNITS_PIXELS);
		} else {
			GetContainer()->SetIndent(0, MunkHTML_INDENT_LEFT, MunkHTML_UNITS_PIXELS);
		}

		// NONSTANDARD		
		if (munkTag.HasParam(wxT("MARGIN_RIGHT"))) {
			double marginRight;
			if (!munkTag.GetParamAsLengthInInches(wxT("MARGIN_RIGHT"), &marginRight)) {
				marginRight = 0.0;
			}
			// Convert to pixels, based on 72 dpi
			// FIXME: What about printing?
			marginRight = (marginRight*72.0);

			GetContainer()->SetIndent((int) marginRight, MunkHTML_INDENT_RIGHT, MunkHTML_UNITS_PIXELS);
		} else {
			GetContainer()->SetIndent(0, MunkHTML_INDENT_RIGHT, MunkHTML_UNITS_PIXELS);
		}



		// NONSTANDARD		
		if (munkTag.HasParam(wxT("TEXT_INDENT"))) {
			double firstLineIndent;
			if (!munkTag.GetParamAsLengthInInches(wxT("TEXT_INDENT"), &firstLineIndent)) {
				firstLineIndent = 0.0;
			}
			// Convert to pixels, based on 72 dpi
			// FIXME: What about printing?
			firstLineIndent = (firstLineIndent*72);

			GetContainer()->SetFirstLineIndent(firstLineIndent);
		} else {
			GetContainer()->SetFirstLineIndent(0);
		}

		// NONSTANDARD		
		if (munkTag.HasParam(wxT("MARGIN_TOP"))) {
			double marginTop;
			if (!munkTag.GetParamAsLengthInInches(wxT("MARGIN_TOP"), &marginTop)) {
				marginTop = 0.0;
			}
			// Convert to pixels, based on 72 dpi
			// FIXME: What about printing?
			marginTop = (marginTop*72.0);

			GetContainer()->SetIndent((int) marginTop, MunkHTML_INDENT_TOP, MunkHTML_UNITS_PIXELS);
		} else {
			GetContainer()->SetIndent(GetCharHeight(), MunkHTML_INDENT_TOP, MunkHTML_UNITS_PIXELS);
		}


		// NONSTANDARD		
		if (munkTag.HasParam(wxT("MARGIN_BOTTOM"))) {
			double marginBottom;
			if (!munkTag.GetParamAsLengthInInches(wxT("MARGIN_BOTTOM"), &marginBottom)) {
				marginBottom = 0.0;
			}
			// Convert to pixels, based on 72 dpi
			// FIXME: What about printing?
			marginBottom = (marginBottom*72.0);

			GetContainer()->SetIndent((int) marginBottom, MunkHTML_INDENT_BOTTOM, MunkHTML_UNITS_PIXELS);
		} else {
			GetContainer()->SetIndent(0, MunkHTML_INDENT_BOTTOM, MunkHTML_UNITS_PIXELS);
		}


		
	} else if (tag == "br") {
		GetContainer()->InsertCell(new MunkHtmlLineBreakCell(m_pDC, GetContainer()->GetFirstChild()));
	} else if (tag == "form") {
		if (m_pCanvas->m_pForms == 0) {
			m_pCanvas->m_pForms = new MunkHtmlFormContainer();
		}
		++ m_cur_form_id;

		std::string method;
		if (attrs.find("method") != attrs.end()) {
			method = getMunkAttribute(attrs, "method");
		} else {
			method = "GET";
		}

		std::string action;
		if (attrs.find("action") != attrs.end()) {
			action = getMunkAttribute(attrs, "action");
		} else {
			action = "";
		}


		m_pCanvas->m_pForms->addForm(m_cur_form_id, method, action);
	} else if (tag == "input") {
		std::string type;
		eMunkHtmlFormElementKind fe_kind = kFEHidden;
		
		if (attrs.find("type") != attrs.end()) {
			type = getMunkAttribute(attrs, "type");
			if (type == "submit") {
				fe_kind = kFESubmit;
			} else if (type == "hidden") {
				fe_kind = kFEHidden;
			} else {
				fe_kind = kFEHidden;
			}
		} else {
			type = "hidden";
			fe_kind = kFEHidden;
		}

		std::string name;
		if (attrs.find("name") != attrs.end()) {
			name = getMunkAttribute(attrs, "name");
		} else {
			name = "hidden_name";
		}

		std::string value;
		if (attrs.find("value") != attrs.end()) {
			value = getMunkAttribute(attrs, "value");
		} else {
			value = "hidden_value";
		}

		bool bChecked = false;
		if (attrs.find("checked") != attrs.end()) {
			bChecked = true;
		} else {
			bChecked = false;
		}

		std::string label;
		if (attrs.find("label") != attrs.end()) {
			label = getMunkAttribute(attrs, "label");
		} else {
			label = "";
		}

		if (fe_kind == kFESubmit) {
			name = "submit";
			label = value;
		}

		MunkHtmlForm *pForm = m_pCanvas->m_pForms->getForm(m_cur_form_id);
		if (pForm != 0) {
			pForm->addFormElement(name, fe_kind);

			MunkHtmlFormElement *pFormElement = pForm->getFormElement(name);
			pFormElement->addValueLabelPair(value, label);

			if (fe_kind == kFESubmit) {
				MunkHtmlWidgetCell *pWidgetCell = 0;

				pWidgetCell = pFormElement->realizeCell(m_pCanvas->GetParentMunkHtmlWindow());
				if (pWidgetCell != 0) {
					GetContainer()->InsertCell(pWidgetCell);
				}
			}
		}
	} else if (tag == "option") {
		std::string value;
		if (attrs.find("value") != attrs.end()) {
			value = getMunkAttribute(attrs, "value");
		} else {
			value = "hidden_value";
		}

		bool bSelected = false;
		if (attrs.find("selected") != attrs.end()) {
			bSelected = true;
		} else {
			bSelected = false;
		}

		// NONSTANDARD: <option  ... label="{mylabel}"/>
		// instead of <option ...>{mylabel}</option> !
		std::string label;
		if (attrs.find("label") != attrs.end()) {
			label = getMunkAttribute(attrs, "label");
		} else {
			label = "";
		}

		MunkHtmlForm *pForm = m_pCanvas->m_pForms->getForm(m_cur_form_id);
		if (pForm != 0) {
			MunkHtmlFormElement *pFormElement = pForm->getFormElement(m_cur_form_select_name);
			pFormElement->addValueLabelPair(value, label, bSelected);
		}
	} else if (tag == "select") {
		eMunkHtmlFormElementKind fe_kind = kFESelect;
		
		std::string name;
		if (attrs.find("name") != attrs.end()) {
			name = getMunkAttribute(attrs, "name");
		} else {
			name = "hidden_name";
		}

		m_cur_form_select_name = name;

		MunkHtmlForm *pForm = m_pCanvas->m_pForms->getForm(m_cur_form_id);
		if (pForm != 0) {
			pForm->addFormElement(name, kFESelect);
		}
	} else if (tag == "radiobox") {
		// NONSTANDARD:
		// <radiobox name="{myname} disabled=\"true|false\"">
		//   <option value="{myvalue}" label="{mylabel}" selected="true"/>
		//   <option value="{myvalue}" label="{mylabel}"/>
		// </radiobox>
		eMunkHtmlFormElementKind fe_kind = kFERadioBox;
		
		std::string name;
		if (attrs.find("name") != attrs.end()) {
			name = getMunkAttribute(attrs, "name");
		} else {
			name = "hidden_name";
		}

		m_cur_form_select_name = name;

		bool bDisabled = false;
		if (attrs.find("disabled") != attrs.end()) {
			if (getMunkAttribute(attrs, "disabled") == "true") {
				bDisabled = true;
			};
		} else {
			bDisabled = false;
		}


		MunkHtmlForm *pForm = m_pCanvas->m_pForms->getForm(m_cur_form_id);
		if (pForm != 0) {
			pForm->addFormElement(name, kFERadioBox);
			
			MunkHtmlFormElement *pRadioBox = pForm->getFormElement(name);
			pRadioBox->setDisabled(bDisabled);
		}
	} else if (tag == "div") {
		CloseContainer();
		OpenContainer();

		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);

		// NONSTANDARD          
		int padding;
		if (munkTag.HasParam(wxT("PADDING"))) {
			if (!munkTag.GetParamAsInt(wxT("PADDING"), &padding)) {
				padding = 0;
			}
		} else {
			padding = 0;
		}
		GetContainer()->SetIndent(padding, MunkHTML_INDENT_ALL, MunkHTML_UNITS_PIXELS);

		OpenContainer();
		OpenContainer();
	} else if (tag == "b") {
		startBold();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "em") {
		startEm();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "i") {
		startEm();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "u") {
		startUnderline();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "sc") {
		// NONSTANDARD
		m_smallcaps_stack.push(true);
	} else if (tag == "sup") {
		int oldbase = GetScriptBaseline();


		MunkHtmlContainerCell *cont = GetContainer();
		MunkHtmlCell *c = cont->GetLastChild();

		startSuperscript(oldbase + c ? c->GetScriptBaseline() : 0);

		cont->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "sub") {
		int oldbase = GetScriptBaseline();


		MunkHtmlContainerCell *cont = GetContainer();
		MunkHtmlCell *c = cont->GetLastChild();

		startSubscript(oldbase + c ? c->GetScriptBaseline() : 0);

		cont->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "h1" || tag == "h2" || tag == "h3") {
		if (tag == "h1") {
			startH1();
		} else if (tag == "h2") {
			startH2();
		} else {
			startH3();
		}
		MunkHtmlContainerCell *c;
		c = GetContainer();
		if (c->GetFirstChild())	{
			CloseContainer();
			OpenContainer();
			c = GetContainer();
		}
		c = GetContainer();

		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);

		c->SetAlign(munkTag);
		c->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
		c->SetIndent(GetCharHeight(), MunkHTML_INDENT_TOP);
		SetAlign(c->GetAlignHor());
	} else if (tag == "img") {
		if (attrs.find("src") != attrs.end()) {
			int w = wxDefaultCoord, h = wxDefaultCoord;
			int al;
			MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
			wxFSFile *str;
			wxString tmp(getMunkAttribute(attrs, "src").c_str(), wxConvUTF8);
			wxString mn = wxEmptyString;

			str = m_pCanvas->GetFS()->OpenFile(tmp, wxFS_SEEKABLE|wxFS_READ);

			if (munkTag.HasParam(wxT("WIDTH")))
				munkTag.GetParamAsInt(wxT("WIDTH"), &w);
			if (munkTag.HasParam(wxT("HEIGHT")))
				munkTag.GetParamAsInt(wxT("HEIGHT"), &h);
			al = MunkHTML_ALIGN_BOTTOM;
			if (munkTag.HasParam(wxT("ALIGN"))) {
				wxString alstr = munkTag.GetParam(wxT("ALIGN"));
				alstr.MakeUpper();  // for the case alignment was in ".."
				if (alstr == wxT("TEXTTOP"))
					al = MunkHTML_ALIGN_TOP;
				else if ((alstr == wxT("CENTER")) || (alstr == wxT("ABSCENTER")))
					al = MunkHTML_ALIGN_CENTER;
			}
			if (munkTag.HasParam(wxT("USEMAP"))) {
				mn = munkTag.GetParam( wxT("USEMAP") );
				if (mn.GetChar(0) == wxT('#')) {
					mn = mn.Mid( 1 );
				}
			}
			MunkHtmlImageCell *cel = new MunkHtmlImageCell(
                                          GetWindowInterface(),
                                          str, w, h,
					  1.0, // Pixel scale
                                          al, mn);
			ApplyStateToCell(cel);
			cel->SetId(munkTag.GetParam(wxT("id"))); // may be empty
			GetContainer()->InsertCell(cel);
			if (str) {
				delete str;
			}
		}
	} else if (tag == "font") {
		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);

		// Stay with the current one if <font> tag doesn't have COLOR
		wxColour newColor = GetActualColor();

		if (munkTag.HasParam(wxT("COLOR"))){
			wxColour clr;
			if (munkTag.GetParamAsColour(wxT("COLOR"), &clr)) {
				newColor = clr;
			}
		}
		
		int newSizeFactor = GetActualFontSizeFactor();
		if (munkTag.HasParam(wxT("SIZE"))) {
			int tmp = 0;
			wxChar c = munkTag.GetParam(wxT("SIZE")).GetChar(0);
			if (munkTag.GetParamAsInt(wxT("SIZE"), &tmp)) {
				int nCurrentSizeFactor = newSizeFactor;
				int nNewPointSize = 0;
				if (c == wxT('+') || c == wxT('-')) {
					int nCurrentPointSize = ((m_nMagnification * DEFAULT_FONT_SIZE * nCurrentSizeFactor) / 10000);
					nNewPointSize = nCurrentPointSize + tmp;
				} else {
					nNewPointSize = tmp;
				}

				if (nNewPointSize < 7) {
					nNewPointSize = 7;
				} else if (nNewPointSize > 32) {
					nNewPointSize = 32;
				}
				newSizeFactor = (10000 * nNewPointSize) / (m_nMagnification * DEFAULT_FONT_SIZE);
			}
		}

		bool bHasFace = false;
		wxString strFaceName = wxT("Arial");
		if (attrs.find("face") != attrs.end()) {
			std::list<std::string> face_name_list;

			std::string face_list_string = getMunkAttribute(attrs, "face");
			munk_split_string(face_list_string, ",", face_name_list);
			
			// Take first.
			std::list<std::string>::const_iterator ci = face_name_list.begin();
			std::string face_name = "Arial";
			if (ci == face_name_list.end()) {
				; // Just go with Arial
			} else {
				face_name = *ci;

				// Only use it if it is non-empty
				bHasFace = true;
			}

			strFaceName = wxString(face_name.c_str(), wxConvUTF8);
		}

		MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
		current_font_attributes.m_color = newColor;
		current_font_attributes.m_sizeFactor = newSizeFactor;

		if (bHasFace) {
			current_font_attributes.m_face = strFaceName;
		}
		
		m_HTML_font_attribute_stack.push(current_font_attributes);

		GetContainer()->InsertCell(new MunkHtmlColourCell(GetActualColor()));
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "hr") {
		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);

		MunkHtmlContainerCell *c;
		int sz;
		bool HasShading;
		
		CloseContainer();
		c = OpenContainer();
		
		//c->SetIndent(GetCharHeight(), MunkHTML_INDENT_VERTICAL);
		c->SetIndent(0, MunkHTML_INDENT_VERTICAL);
		c->SetAlignHor(MunkHTML_ALIGN_CENTER);
		c->SetAlign(munkTag);
		c->SetWidthFloat(munkTag);
		sz = 2;
		munkTag.GetParamAsInt(wxT("SIZE"), &sz);
		HasShading = !(munkTag.HasParam(wxT("NOSHADE")));
		int myHeight = (int)((double)sz * 1.0);

		c->InsertCell(new MunkHtmlLineCell(myHeight, HasShading));
		
		CloseContainer();
		OpenContainer();
	} else if (tag == "pagebreak") {
		int al = GetContainer()->GetAlignHor();
		MunkHtmlContainerCell *c;

		CloseContainer();
		c = OpenContainer();
		c->SetAlignHor(al);

		c->InsertCell(new MunkHtmlPagebreakCell());
		
		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
		c->SetAlign(munkTag);
		c->SetMinHeight(GetCharHeight());
	} else if (tag == "table" || tag == "tr" || tag == "td" || tag == "th") {
		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
		MunkHtmlContainerCell *c;
		if (tag == "table") {
			int oldAlign = GetAlign();
			
			MunkHtmlContainerCell *oldcont;
			oldcont = c = OpenContainer();

			m_table_cell_info_stack.push(std::make_pair(oldAlign, oldcont));
			
			MunkHtmlTableCell *pTable = new MunkHtmlTableCell(c, munkTag);
			m_tables_stack.push(pTable);
			
			// width:
			{
				if (munkTag.HasParam(wxT("WIDTH"))) {
					wxString wd = munkTag.GetParam(wxT("WIDTH"));
					
					if (wd[wd.length()-1] == wxT('%')) {
						int width = 0;
						wxSscanf(wd.c_str(), wxT("%i%%"), &width);
						pTable->SetWidthFloat(width, MunkHTML_UNITS_PERCENT);
					} else {
						int width = 0;
						wxSscanf(wd.c_str(), wxT("%i"), &width);
						pTable->SetWidthFloat((int)(1.0 * width), MunkHTML_UNITS_PIXELS);
					}
				} else {
					pTable->SetWidthFloat(0, MunkHTML_UNITS_PIXELS);
				}
			}
			if (munkTag.HasParam(wxT("ALIGN"))) {
				m_tAlignStack.push(munkTag.GetParam(wxT("ALIGN")));
			} else {
				m_tAlignStack.push(wxEmptyString);
			}
		} else if (!m_tables_stack.empty()) {
			// new row in table
			if (munkTag.GetName() == wxT("TR")) {
				m_tables_stack.top()->AddRow(munkTag);
				m_rAlign = m_tAlignStack.top();
				if (munkTag.HasParam(wxT("ALIGN")))
					m_rAlign = munkTag.GetParam(wxT("ALIGN"));
			}  else { // new cell
				c = SetContainer(new MunkHtmlContainerCell(m_tables_stack.top()));
				m_tables_stack.top()->AddCell(c, munkTag);

				OpenContainer();

				if (munkTag.GetName() == wxT("TH")) /*header style*/
					SetAlign(MunkHTML_ALIGN_CENTER);
				else
					SetAlign(MunkHTML_ALIGN_LEFT);
				
				wxString als;

				als = m_rAlign;
				if (munkTag.HasParam(wxT("ALIGN")))
					als = munkTag.GetParam(wxT("ALIGN"));
				als.MakeUpper();
				if (als == wxT("RIGHT"))
					SetAlign(MunkHTML_ALIGN_RIGHT);
				else if (als == wxT("LEFT"))
					SetAlign(MunkHTML_ALIGN_LEFT);
				else if (als == wxT("CENTER"))
					SetAlign(MunkHTML_ALIGN_CENTER);
				
				OpenContainer();
			}
		}
	} else if (tag == "dl") {
		if (GetContainer()->GetFirstChild() != NULL) {
			CloseContainer();
			OpenContainer();
		}
		GetContainer()->SetIndent(GetCharHeight(), MunkHTML_INDENT_TOP);
	} else if (tag == "dt") {
		MunkHtmlContainerCell *c;
		CloseContainer();
		c = OpenContainer();
		c->SetAlignHor(MunkHTML_ALIGN_LEFT);
		c->SetMinHeight(GetCharHeight());
        } else if (tag == "dd") {
		MunkHtmlContainerCell *c;
		CloseContainer();
		c = OpenContainer();
		c->SetIndent(3 * GetCharWidth(), MunkHTML_INDENT_LEFT);
	} else if (tag == "li") {
		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
		
		MunkHtmlContainerCell *c;

		// List Item:
		if (!m_list_cell_stack.empty()) {
			MunkHtmlListCell *pList = m_list_cell_stack.top();

			c = SetContainer(new MunkHtmlContainerCell(pList));
			c->SetAlignVer(MunkHTML_ALIGN_TOP);

			MunkHtmlContainerCell *mark = c;
			c->SetWidthFloat(1 * GetCharWidth(), MunkHTML_UNITS_PIXELS);
			if (m_Numbering == 0) {
				// Centering gives more space after the bullet
				c->SetAlignHor(MunkHTML_ALIGN_CENTER);
				c->InsertCell(new MunkHtmlListmarkCell(m_pDC, GetActualColor()));
			} else {
				c->SetAlignHor(MunkHTML_ALIGN_RIGHT);
				wxString markStr;
				markStr.Printf(wxT("%i. "), m_Numbering);
				c->InsertCell(new MunkHtmlWordCell(markStr, *(m_pDC)));
			}
			CloseContainer();

			c = OpenContainer();
			
			pList->AddRow(mark, c);
			c = OpenContainer();
			SetContainer(new MunkHtmlListcontentCell(c));
			
			if (m_Numbering != 0) {
				++m_Numbering;
			}
		}
	} else if (tag == "ul" || tag == "ol") {
		CloseContainer();

		MunkHtmlContainerCell *c;

		int oldnum = 0;
		if (m_list_cell_info_stack.empty()) {
			oldnum = 0;
		} else {
			oldnum = m_list_cell_info_stack.top().first;
		}

		if (tag == "ul") {
			m_Numbering = 0;
		} else {
			MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
			if (munkTag.HasParam(wxT("START"))) {
				// If we have the "start" attribute,
				// use it instead of "1"
				munkTag.GetParamAsInt(wxT("START"), &m_Numbering);
			} else {
				// If we don't have "start", just use "1"
				m_Numbering = 1;
			}
		}

		MunkHtmlContainerCell *oldcont;
		oldcont = c = OpenContainer();

		m_list_cell_info_stack.push(std::make_pair(oldnum, oldcont));
		
		MunkHtmlListCell *pList = new MunkHtmlListCell(c);
		pList->SetIndent(1 * GetCharWidth(), MunkHTML_INDENT_LEFT);
		m_list_cell_stack.push(pList);
		SetContainer(pList);
	} else if (tag == "html") {
		; // Do nothing
	} else if (tag == "head") {
		; // Do nothing
	} else if (tag == "title") {
		; // Do nothing
	} else if (tag == "meta") {
		; // Do nothing
	} else if (tag == "body") {
		m_bInBody = true;
		MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
		
		// NONSTANDARD          
		int padding;
		if (munkTag.HasParam(wxT("PADDING"))) {
			if (!munkTag.GetParamAsInt(wxT("PADDING"), &padding)) {
				padding = 10;
			}
		} else {
			padding = 10;
		}
		if (m_pCanvas->GetParentMunkHtmlWindow() != NULL) {
			m_pCanvas->GetParentMunkHtmlWindow()->SetBorders(padding);
		}

		if (attrs.find("background") != attrs.end()) {
			int w = wxDefaultCoord, h = wxDefaultCoord;
			MunkHtmlTag munkTag(wxString(tag.c_str(), wxConvUTF8), attrs);
			wxFSFile *str;
			wxString tmp(getMunkAttribute(attrs, "background").c_str(), wxConvUTF8);
			wxString mn = wxEmptyString;

			str = m_pCanvas->GetFS()->OpenFile(tmp, wxFS_SEEKABLE|wxFS_READ);
			if (str) {
				wxInputStream *s = str->GetStream();
				if (s) {
					wxImage image(*s, wxBITMAP_TYPE_ANY);
					if (image.Ok()) {
						if (m_pCanvas->GetParentMunkHtmlWindow() != NULL) {
							m_pCanvas->GetParentMunkHtmlWindow()->SetHTMLBackgroundImage(wxBitmap(image, -1));
						}
					} else {
						// std::cerr << "UP257: image was not OK!" << std::endl;
					}
				}
				m_pCanvas->SetHTMLBackgroundColour(wxNullColour);
				if (m_pCanvas->GetParentMunkHtmlWindow() != NULL) {
					m_pCanvas->GetParentMunkHtmlWindow()->SetHTMLBackgroundColour(wxNullColour);
				}

				delete str;
			}
		} else {
			// No background image. Set background to white.
			m_pCanvas->SetHTMLBackgroundColour(*wxWHITE);
			if (m_pCanvas->GetParentMunkHtmlWindow() != NULL) {
				m_pCanvas->GetParentMunkHtmlWindow()->SetHTMLBackgroundColour(*wxWHITE);
			}
		}

	} else {
		throw MunkQDException(std::string("Unknown start-tag: <") + tag + ">");
	}
}


void MunkQDHTMLHandler::endElement(const std::string& tag) throw(MunkQDException)
{
	handleChars();

	if (tag == "a") {
		if (m_anchor_type_stack.empty()) {
			throw MunkQDException("</a> encountered without start-tag <a>. Please inform the publisher of this software.");
		}
		eAnchorType anchor_type = m_anchor_type_stack.top();
		m_anchor_type_stack.pop();		
		if (anchor_type == kATHREF) {
			PopLink();
		} else { // kATNAME
			; // Nothing to do
		}
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
		GetContainer()->InsertCell(new MunkHtmlColourCell(GetActualColor()));
		GetContainer()->InsertCell(new MunkHtmlColourCell(GetContainer()->GetBackgroundColour(), MunkHTML_CLR_BACKGROUND));
	} else if (tag == "p") {
	} else if (tag == "br") {
		; // Nothing to do
	} else if (tag == "form") {
		; // Nothing to do
	} else if (tag == "radiobox"
		   || tag == "select") {
		// Realize radio box
		MunkHtmlForm *pForm = m_pCanvas->m_pForms->getForm(m_cur_form_id);
		MunkHtmlWidgetCell *pWidgetCell = 0;

		if (pForm != 0) {
			MunkHtmlFormElement *pFormElement = pForm->getFormElement(m_cur_form_select_name);
			if (pFormElement != 0) {
				pWidgetCell = pFormElement->realizeCell(m_pCanvas->GetParentMunkHtmlWindow());
			}
		}
		if (pWidgetCell != 0) {
			GetContainer()->InsertCell(pWidgetCell);
		} else {
			MUNK_ASSERT_THROW(false,
					  "pWidgetCell == 0");
		}
	} else if (tag == "input") {
		; // Nothing to do
	} else if (tag == "option") {
		; // Nothing to do
	} else if (tag == "div") {
		CloseContainer();
		CloseContainer();
	} else if (tag == "b") {
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "em") {
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "i") {
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "u") {
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "sc") {
		// NONSTANDARD small caps
		m_smallcaps_stack.pop();
	} else if (tag == "sup") {
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "sub") {
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
	} else if (tag == "pagebreak") {
		// Nothing to do; all was done at the start of the tag.
	} else if (tag == "h1"
		   || tag == "h2"
		   || tag == "h3") {
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
		CloseContainer();
		OpenContainer();
		MunkHtmlContainerCell *c = GetContainer();
		c->SetIndent(GetCharHeight(), MunkHTML_INDENT_TOP);
	} else if (tag == "table") {
		std::pair<int, MunkHtmlContainerCell*> mypair = 
			m_table_cell_info_stack.top();
		m_table_cell_info_stack.pop();
		m_tAlignStack.pop();

		SetAlign(mypair.first);
		SetContainer(mypair.second);
		CloseContainer();
		m_tables_stack.pop();
	} else if (tag == "td" || tag == "tr" || tag == "th") {
		; // Nothing to do
	} else if (tag == "hr") {
		; // Do nothing
	} else if (tag == "dl") {
		if (GetContainer()->GetFirstChild() != NULL) {
			CloseContainer();
			OpenContainer();
		}
		GetContainer()->SetIndent(GetCharHeight(), MunkHTML_INDENT_TOP);
	} else if (tag == "dt") {
		; // Do nothing
        } else if (tag == "dd") {
		; // Do nothing
	} else if (tag == "li") {
		if (!m_list_cell_stack.empty()) {
			CloseContainer();
			CloseContainer();
			CloseContainer();

		}
	} else if (tag == "ol" || tag == "ul") {
		SetContainer(m_list_cell_stack.top());
		CloseContainer();

		m_list_cell_stack.pop();

		std::pair<int, MunkHtmlContainerCell*> mypair = m_list_cell_info_stack.top();
		m_list_cell_info_stack.pop();
		
		SetContainer(mypair.second);
		CloseContainer();
		OpenContainer();
		m_Numbering = mypair.first;
	} else if (tag == "html") {
		; // Do nothing
	} else if (tag == "head") {
		; // Do nothing
	} else if (tag == "title") {
		; // Do nothing
	} else if (tag == "meta") {
		; // Do nothing
	} else if (tag == "img") {
		; // Do nothing
	} else if (tag == "font") {
		endTag();
		GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
		GetContainer()->InsertCell(new MunkHtmlColourCell(GetActualColor()));
	} else if (tag == "body") {
		m_bInBody = false;
	} else {
		throw MunkQDException(std::string("Unknown end-tag: </") + tag + ">");
	}
}


void MunkQDHTMLHandler::startDocument(void) throw(MunkQDException)
{
	m_chars = "";
	m_bInBody = false;
}


void MunkQDHTMLHandler::endDocument(void) throw(MunkQDException)
{
	MunkHtmlContainerCell *top;

	//CloseContainer();
	//OpenContainer();
	
	top = m_pCurrentContainer;
	while (top->GetParent()) top = top->GetParent();
	top->RemoveExtraSpacing(true, true);
	
	m_pCanvas->SetTopCell(top);
}

void MunkQDHTMLHandler::AddText(const std::string& str)
{
	wxString txt(str.c_str(), wxConvUTF8);
	size_t i = 0,
		x,
		lng = wxStrlen(txt);
	register wxChar d;
	int templen = 0;
	wxChar nbsp = 160;
	
	if (lng+1 > m_tmpStrBufSize) {
		delete[] m_tmpStrBuf;
		m_tmpStrBuf = new wxChar[lng+1];
		m_tmpStrBufSize = lng+1;
	}
	wxChar *temp = m_tmpStrBuf;
	
	if (m_tmpLastWasSpace) {
		while ((i < lng) &&
		       ((txt[i] == wxT('\n')) || (txt[i] == wxT('\r')) || (txt[i] == wxT(' ')) ||
			(txt[i] == wxT('\t')))) i++;
	}
	
	while (i < lng) {
		x = 0;
		d = temp[templen++] = txt[i];
		if ((d == wxT('\n')) || (d == wxT('\r')) || (d == wxT(' ')) || (d == wxT('\t'))) {
			i++, x++;
			while ((i < lng) && ((txt[i] == wxT('\n')) || (txt[i] == wxT('\r')) ||
					     (txt[i] == wxT(' ')) || (txt[i] == wxT('\t')))) i++, x++;
		}
		else i++;
		
		if (x) {
			temp[templen-1] = wxT(' ');
			DoAddText(temp, templen, nbsp);
			m_tmpLastWasSpace = true;
		}
	}

	if (templen && (templen > 1 || temp[0] != wxT(' '))) {
		DoAddText(temp, templen, nbsp);
		m_tmpLastWasSpace = false;
	}
}


void MunkQDHTMLHandler::DoAddText(wxChar *temp, int& templen, wxChar nbsp)
{
    temp[templen] = 0;
    templen = 0;
#if !wxUSE_UNICODE
    if (m_EncConv)
        m_EncConv->Convert(temp);
#endif
    size_t len = wxStrlen(temp);
    for (size_t j = 0; j < len; j++)
    {
        if (temp[j] == nbsp)
            temp[j] = wxT(' ');
    }

    MunkHtmlCell *c = new MunkHtmlWordCell(temp, *(m_pDC));

    ApplyStateToCell(c);

    m_pCurrentContainer->InsertCell(c);
    ((MunkHtmlWordCell*)c)->SetPreviousWord(m_lastWordCell);
    m_lastWordCell = (MunkHtmlWordCell*)c;
}




void MunkQDHTMLHandler::handleChars(void)
{
	if (!m_bInBody) {
		m_chars = "";
		return; // Nothing to do when not within <body>...</body>.
	}
	if (m_chars.empty()) {
		return; // Nothing to do
	} else if (GetFontSmallCaps()) {
		bool bSmallCapsTagOn = false;
		std::string::size_type length = m_chars.length();
		std::string tmp;
		for (std::string::size_type i = 0;
		     i < length;
		     ++i) {
			char c = m_chars[i];
			if (isalpha(c)) {
				if (isupper(c)) {
					if (bSmallCapsTagOn) {
						if (!tmp.empty()) {
							AddText(tmp);
							tmp = "";
						}
						endTag();
						GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
						bSmallCapsTagOn = false;
						tmp += c;
					} else {
						// isalpha() && isupper() && !bSmallCapsTagOn
						tmp += c;
					}
				} else {
					// isalpha(c) && !isupper(c)
					if (bSmallCapsTagOn) {
						tmp += (char) toupper(c);
					} else {
						if (!tmp.empty()) {
							AddText(tmp);
							tmp = "";
						}	
						startSmallCaps();
						GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
						bSmallCapsTagOn = true;
						tmp += toupper(c);
					}
				}
			} else {
				// !isalpha(c)
				if (bSmallCapsTagOn) {
					tmp += c;
				} else {
					if (!tmp.empty()) {
						AddText(tmp);
						tmp = "";
					}
					startSmallCaps();
					GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
					bSmallCapsTagOn = true;
					tmp += c;
				}
			}
		}
		if (!tmp.empty()) {
			AddText(tmp);
		}
		if (bSmallCapsTagOn) {
			endTag();
			GetContainer()->InsertCell(new MunkHtmlFontCell(CreateCurrentFont(), GetFontUnderline()));
		}
		m_chars = "";
		return;
	} else {
		AddText(m_chars);
		m_chars = "";
		return;
	}


	/*
	else if (!is_other_than_whitespace(m_chars)) {
		AddText(" ");
			
		// Clean up for next text() call.
		m_chars = "";
		return;
	} else {
		std::list<std::string> token_list;
		munk_split_string(m_chars, " \n\t\r", token_list);
		std::list<std::string>::const_iterator ci = token_list.begin();
		std::list<std::string>::const_iterator cend = token_list.end();
		while (ci != cend) {
			std::string text = *ci;
			++ci;
			bool bAddSpaceAfter = true;
			if (ci == cend) {
				if (is_whitespace(m_chars[m_chars.length()-1])) {
					; // Leave it as true
				} else {
					bAddSpaceAfter = false;
				}
			}
			AddText(text, bAddSpaceAfter);
		}

		AddText(m_chars);

		// Clean up for next text() call.
		m_chars = "";
	}
	*/
}

void MunkQDHTMLHandler::text(const std::string& str) throw(MunkQDException)
{
	m_chars += str;
}


MunkHtmlContainerCell *MunkQDHTMLHandler::OpenContainer()
{
	m_pCurrentContainer = new MunkHtmlContainerCell(m_pCurrentContainer);
	m_pCurrentContainer->SetAlignHor(m_Align);
	m_tmpLastWasSpace = true;
	return m_pCurrentContainer;
}


MunkHtmlContainerCell *MunkQDHTMLHandler::SetContainer(MunkHtmlContainerCell *pNewContainer)
{
	m_pCurrentContainer = pNewContainer;
	m_tmpLastWasSpace = true;
	return m_pCurrentContainer;
}


MunkHtmlContainerCell *MunkQDHTMLHandler::CloseContainer()
{
 	m_pCurrentContainer = m_pCurrentContainer->GetParent();
	return m_pCurrentContainer;
}


MunkHtmlPagebreakCell::MunkHtmlPagebreakCell()
{
	m_Width = 1;
	m_Height = 10;
	m_bHasSeenPagebreak = false;
	SetCanLiveOnPagebreak(false);
}

MunkHtmlPagebreakCell::~MunkHtmlPagebreakCell()
{
}

bool MunkHtmlPagebreakCell::AdjustPagebreak(int *pagebreak, int render_height,
                                 wxArrayInt& WXUNUSED(known_pagebreaks))
{
	int real_position = *pagebreak + m_Parent->GetPosY();
	if (*pagebreak > 0 && *pagebreak < (render_height-1)) {
		if (m_bHasSeenPagebreak) {
			return false;
		} else {
			m_bHasSeenPagebreak = true;
			*pagebreak = 0;
			return true;
		}
	} else {
		return false;
	}
}


void MunkHtmlPagebreakCell::Layout(int w)
{
	MunkHtmlCell::Layout(w);

	m_PosY = m_Parent->GetPosY();

	m_bHasSeenPagebreak = false;
}







void MunkHtmlPagebreakCell::Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
				 MunkHtmlRenderingInfo& info)
{
	// Do nothing.
}




void MunkQDHTMLHandler::SetCharWidthHeight()
{
	// Get extent
	wxCoord nWidth, nHeight;
	wxCoord nDescent, nExternalLeading;
	m_pDC->SetFont(*(CreateCurrentFont()));
	m_pDC->GetTextExtent(wxT("A"),
			     &nWidth, &nHeight,
			     &nDescent, &nExternalLeading);
	m_CharWidth = nWidth;
	m_CharHeight = nHeight;
	
	m_pDC->SetFont(wxNullFont);
}


void MunkQDHTMLHandler::ApplyStateToCell(MunkHtmlCell *cell)
{
	// set the link:
	if (m_UseLink) {
		cell->SetLink(m_link_info_stack.top());
	}
		
	// apply current script mode settings:
	cell->SetScriptMode(GetScriptMode(), GetScriptBaseline());
}

int MunkQDHTMLHandler::GetActualFontSizeFactor() const
{
	return m_HTML_font_attribute_stack.top().m_sizeFactor;
}


const wxColour& MunkQDHTMLHandler::GetActualColor() const
{
	return m_HTML_font_attribute_stack.top().m_color;
}

MunkHtmlLinkInfo MunkQDHTMLHandler::PopLink()
{
	if (m_link_info_stack.empty()) {
		throw MunkQDException(std::string("PopLink() called with no link info in stack.\nPlease inform the publisher of this program."));
	} else {
		MunkHtmlLinkInfo mytop = m_link_info_stack.top();
		m_link_info_stack.pop();
		if (m_link_info_stack.empty()) {
			m_UseLink = false;
		} else {
			m_UseLink = (m_link_info_stack.top().GetHref() != wxEmptyString);
		}
		return mytop;
	}
}

void MunkQDHTMLHandler::PushLink(const MunkHtmlLinkInfo& link)
{
	m_link_info_stack.push(link);
	m_UseLink = (link.GetHref() != wxEmptyString);
}


void MunkQDHTMLHandler::startMunkHTMLFontAttributeStack(void)
{
	// First clear it...
	while (!m_HTML_font_attribute_stack.empty()) {
		m_HTML_font_attribute_stack.pop();
	}
	
	// Then push back a normal, non-decorated, black MunkHTMLFontAttributes object.
	m_HTML_font_attribute_stack.push(MunkHTMLFontAttributes(false, // bold
								false, // italic
								false, // underline
								0, // baseline
								MunkHTML_SCRIPT_NORMAL, // scriptMode
								100,  // sizeFactor
								*wxBLACK)); // color
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startBold(void)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_bBold = true;

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startEm(void)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_bItalic = !current_font_attributes.m_bItalic;

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}

bool MunkQDHTMLHandler::GetFontUnderline(void) const 
{

	if (m_HTML_font_attribute_stack.empty()) {
		return false;
	} else {
		MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
		return current_font_attributes.m_bUnderline;
	}
}

MunkHTMLFontAttributes MunkQDHTMLHandler::startUnderline(void)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_bUnderline = true;

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startSmallCaps(void)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_sizeFactor = (int) (current_font_attributes.m_sizeFactor * 0.75);

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startSuperscript(int newBaseline)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_scriptMode = MunkHTML_SCRIPT_SUP;
	current_font_attributes.m_sizeFactor = (int) (current_font_attributes.m_sizeFactor * 0.56);
	current_font_attributes.m_scriptBaseLine = newBaseline;

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startSubscript(int newBaseline)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_scriptMode = MunkHTML_SCRIPT_SUB;
	current_font_attributes.m_sizeFactor = (int) (current_font_attributes.m_sizeFactor * 0.56);
	current_font_attributes.m_scriptBaseLine = newBaseline;

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startH1(void)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_bBold = true;
	current_font_attributes.m_sizeFactor = 130;

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startH2(void)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_bBold = true;
	current_font_attributes.m_sizeFactor = 110;

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startH3(void)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	current_font_attributes.m_bBold = true;
	current_font_attributes.m_sizeFactor = 100;

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startAnchorNAME(void)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	// Just push the same.
	// This is done because we wish to be able to pop any </a>.
	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::startAnchorHREF(bool bVisible, const wxColour& linkColour)
{
	MunkHTMLFontAttributes current_font_attributes = m_HTML_font_attribute_stack.top();
	if (bVisible) {
		current_font_attributes.m_color = linkColour;
	}

	m_HTML_font_attribute_stack.push(current_font_attributes);
	return current_font_attributes;
}


MunkHTMLFontAttributes MunkQDHTMLHandler::endTag(void)
{
	m_HTML_font_attribute_stack.pop();
	return m_HTML_font_attribute_stack.top();
}

MunkHTMLFontAttributes MunkQDHTMLHandler::topFontAttributeStack(void)
{
	return m_HTML_font_attribute_stack.top();
}


wxFont *MunkQDHTMLHandler::getFontFromMunkHTMLFontAttributes(const MunkHTMLFontAttributes& font_attributes, bool bUseCacheMap)
{
	std::string characteristic_string = font_attributes.toString();
	wxFont *pResult = 0;

	if (bUseCacheMap) {
		String2PFontMap::iterator it = m_HTML_font_map.find(characteristic_string);
		if (it != m_HTML_font_map.end()) {
			pResult = it->second;
		}
	}

	if (pResult == 0) {
		int nPointSize = ((int)(((m_nMagnification * DEFAULT_FONT_SIZE * font_attributes.m_sizeFactor)) * m_pCanvas->GetPixelScale())) / 10000;
		/*
		int nFontStyle = (font_attributes.m_bItalic) ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL;

		wxFontWeight fontWeight = (font_attributes.m_bBold) ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL;
		
		wxFont *pNewFont = new wxFont(nPointSize,
					      wxFONTFAMILY_ROMAN,
					      nFontStyle,
					      fontWeight,
					      false,
					      wxT("Arial"),
					      wxFONTENCODING_DEFAULT);
		*/

		int nFontFlags = 0;
		if (font_attributes.m_bItalic) {

			nFontFlags |= wxFONTFLAG_ITALIC;
		}
		if (font_attributes.m_bBold) {
			nFontFlags |= wxFONTFLAG_BOLD;
		}
		nFontFlags |= wxFONTFLAG_ANTIALIASED;

		wxFont *pNewFont = wxFont::New(nPointSize,
					       wxFONTFAMILY_ROMAN,
					       nFontFlags,
					       font_attributes.m_face,
					       wxFONTENCODING_DEFAULT);

		if (bUseCacheMap) {
			m_HTML_font_map.insert(std::make_pair(characteristic_string, pNewFont));
		}
		pResult = pNewFont;
	}
	return pResult;
}


MunkHtmlScriptMode MunkQDHTMLHandler::GetScriptMode() const
{
	return m_HTML_font_attribute_stack.top().m_scriptMode;
}

long MunkQDHTMLHandler::GetScriptBaseline() const
{
	return m_HTML_font_attribute_stack.top().m_scriptBaseLine;
}

bool MunkQDHTMLHandler::GetFontSmallCaps() const
{
	return m_smallcaps_stack.top();
}


void MunkQDHTMLHandler::LoadFonts(int magnification, wxDC *pInDC)
{
	m_nMagnification = magnification;

	// magnification is a percentage
	//int pointSize = (int) ((DEFAULT_FONT_SIZE * magnification) / 100);

	// now round size to a multiple of 10
	//pointSize = ((pointSize + 5) / 10) * 10;
	
	wxDC *pDC = pInDC;

	// Set map mode
	pDC->SetMapMode(wxMM_TEXT);

	String2PFontMap::iterator it 
		= m_HTML_font_map.begin();
	while (it != m_HTML_font_map.end()) {
		std::string characteristic_string = it->first;
		MunkHTMLFontAttributes font_attr = MunkHTMLFontAttributes::fromString(characteristic_string);
		wxFont *pNewFont = this->getFontFromMunkHTMLFontAttributes(font_attr,
									   false); // Use cache? No: We are going to replace it...
		delete it->second;
		it->second = pNewFont;
		++it;
	}
}

wxFont *MunkQDHTMLHandler::CreateCurrentFont()
{
	wxFont *pResult = getFontFromMunkHTMLFontAttributes(m_HTML_font_attribute_stack.top(),
							    true); // Use map
	m_pDC->SetFont(*pResult);
	return pResult;
}



//////////////////////////////////////////////////////////
//
// MunkHtmlFormContainer
//
//////////////////////////////////////////////////////////
MunkHtmlFormContainer::MunkHtmlFormContainer()
{
}


MunkHtmlFormContainer::~MunkHtmlFormContainer()
{
	std::map<form_id_t, MunkHtmlForm*>::iterator it =  m_forms.begin();
	while (it != m_forms.end()) {
		delete it->second;
		++it;
	}
	m_forms.clear();
}


void MunkHtmlFormContainer::addForm(form_id_t new_form_id, const std::string& method, const std::string& action)
{
	MunkHtmlForm *pForm = new MunkHtmlForm(new_form_id, method, action);
	m_forms.insert(std::make_pair(new_form_id, pForm));
}


MunkHtmlForm *MunkHtmlFormContainer::getForm(form_id_t form_id)
{
	std::map<form_id_t, MunkHtmlForm*>::iterator it =  m_forms.find(form_id);
	if (it == m_forms.end()) {
		return 0;
	} else {
		return it->second;
	}
}


std::list<MunkHtmlForm*> MunkHtmlFormContainer::getFormList()
{
	std::list<MunkHtmlForm*> result;
	std::map<form_id_t, MunkHtmlForm*>::iterator it =  m_forms.begin();
	while (it != m_forms.end()) {
		result.push_back(it->second);
		++it;
	}
	
	return result;
}
 







//////////////////////////////////////////////////////////
//
// MunkHtmlForm
//
//////////////////////////////////////////////////////////

MunkHtmlForm::MunkHtmlForm(form_id_t form_id, const std::string& method, const std::string& action)
	: m_form_id(form_id),
	  m_method(method),
	  m_action(action)
{
	// FIXME: Implement
}


MunkHtmlForm::~MunkHtmlForm()
{
	std::map<form_element_name, MunkHtmlFormElement*>::iterator it =  m_form_elements.begin();
	while (it != m_form_elements.end()) {
		delete it->second;
		++it;
	}
	m_form_elements.clear();
}


// Silently does not add if already there
void MunkHtmlForm::addFormElement(const std::string& name, eMunkHtmlFormElementKind kind)
{
	if (m_form_elements.find(name) != m_form_elements.end()) {
		return; // Silently do not add if already there
	} else {
		MunkHtmlFormElement *pFormElement = new MunkHtmlFormElement(m_form_id, kind);
		m_form_elements.insert(std::make_pair(name, pFormElement));
	}
}


MunkHtmlFormElement *MunkHtmlForm::getFormElement(const std::string& name)
{
	std::map<form_element_name, MunkHtmlFormElement*>::iterator it =  m_form_elements.find(name);
	if (it == m_form_elements.end()) {
		return 0;
	} else {
		return it->second;
	}
}


std::map<std::string, std::string> MunkHtmlForm::getREQUEST(void)
{
	std::map<std::string, std::string> result;
	std::map<form_element_name, MunkHtmlFormElement*>::iterator it =  m_form_elements.begin();
	while (it != m_form_elements.end()) {
		std::string name = it->first;
		MunkHtmlFormElement *pFormElement = it->second;
		std::string value = pFormElement->getValue();

		result.insert(std::make_pair(name, value));
		
		++it;
	}

	return result;
}


std::list<MunkHtmlFormElement*> MunkHtmlForm::getFormElementList()
{
	std::list<MunkHtmlFormElement*> result;
	std::map<form_element_name, MunkHtmlFormElement*>::iterator it =  m_form_elements.begin();
	while (it != m_form_elements.end()) {
		result.push_back(it->second);
		++it;
	}
	
	return result;
}




//////////////////////////////////////////////////////////
//
// MunkHtmlFormElement
//
//////////////////////////////////////////////////////////

int MunkHtmlFormElement::m_next_id = 20000;

MunkHtmlFormElement::MunkHtmlFormElement(form_id_t form_id, eMunkHtmlFormElementKind kind)
{
	m_form_id = form_id;
	m_kind = kind;
	m_selected_index = 0;
	m_pButtonPanel = 0;
	m_pRadioBoxPanel = 0;
	m_pComboBoxPanel = 0;
	m_bDisabled = false;
}


MunkHtmlFormElement::~MunkHtmlFormElement()
{
	m_value_label_pair_list.clear();

	// We do not own the m_pWidgetCell, so thing to do here.
}


std::string MunkHtmlFormElement::getValue()
{
	if (m_kind == kFESubmit) {
		std::string str_value;
		if (m_value_label_pair_list.empty()) {
			str_value = "submit";
		} else {
			str_value = m_value_label_pair_list.begin()->first;
		}
		return str_value;
	} else if (m_kind == kFERadioBox
		   || m_kind == kFESelect) {
		int selected_index;
		if (m_kind == kFERadioBox) {
			selected_index = m_pRadioBoxPanel->GetSelection();
		} else {
			selected_index = m_pComboBoxPanel->GetSelection();
		}
		std::list<std::pair<std::string, std::string> >::const_iterator ci = m_value_label_pair_list.begin();
		int count = 0;
		std::string value = ci->first;
		while (count <= selected_index
		       && ci != m_value_label_pair_list.end()) {
			value = ci->first;

			++count;
			++ci;
		}

		return value;
	} else if (m_kind == kFEHidden) {
		std::string str_value;
		if (m_value_label_pair_list.empty()) {
			str_value = "null";
		} else {
			str_value = m_value_label_pair_list.begin()->first;
		}
		return str_value;
	} else {
		return "null";
	}
}

 // Get selected value
void MunkHtmlFormElement::addValueLabelPair(const std::string& value, const std::string& label, bool bSelected)
{
	m_value_label_pair_list.push_back(std::pair<std::string, std::string>(value, label));
	if (bSelected) {
		m_selected_index = m_value_label_pair_list.size() - 1;
	}
}


MunkHtmlWidgetCell *MunkHtmlFormElement::realizeCell(MunkHtmlWindow *pParent)
{

	MUNK_ASSERT_THROW(pParent != 0,
			  "realizeCell(0)");

	if (m_kind == kFESubmit) {
		std::string str_label;
		if (m_value_label_pair_list.empty()) {
			str_label = "Submit";
			m_value_label_pair_list.push_back(std::make_pair(str_label, str_label));
		} else {
			str_label = m_value_label_pair_list.begin()->second;
		}
		wxString strLabel(str_label.c_str(), wxConvUTF8);

		MunkHtmlButtonPanel *pButtonPanel = new MunkHtmlButtonPanel(pParent, MunkHtmlFormElement::GetNextID(), strLabel, m_form_id);

		// pButtonPanel->Show(true);

		m_pWidgetCell = new MunkHtmlWidgetCell(pButtonPanel, 0);
		return m_pWidgetCell;
	} else if (m_kind == kFEHidden) {
		return 0; // Nothing to create.
	} else if (m_kind == kFERadioBox) {
		wxArrayString arrLabels;

		std::list<std::pair<std::string, std::string> >::const_iterator ci = m_value_label_pair_list.begin();
		while (ci != m_value_label_pair_list.end()) {
			std::string str_label = ci->second;
			
			wxString strLabel(str_label.c_str(), wxConvUTF8);

			arrLabels.Add(strLabel);
			
			++ci;
		}
		m_pRadioBoxPanel = 
			new MunkHtmlRadioBoxPanel(!m_bDisabled,
						  m_selected_index,
						  pParent, 
						  MunkHtmlFormElement::GetNextID(),
						  wxEmptyString, // label
						  wxDefaultPosition,
						  wxDefaultSize,
						  arrLabels,
						  0,
						  wxRA_SPECIFY_ROWS);

		//m_pRadioBoxPanel->Show(true);

		m_pWidgetCell = new MunkHtmlWidgetCell(m_pRadioBoxPanel, 0);
		return m_pWidgetCell;
	} else if (m_kind == kFESelect) {
		wxArrayString arrLabels;

		std::list<std::pair<std::string, std::string> >::const_iterator ci = m_value_label_pair_list.begin();
		while (ci != m_value_label_pair_list.end()) {
			std::string str_label = ci->second;
			
			wxString strLabel(str_label.c_str(), wxConvUTF8);

			arrLabels.Add(strLabel);
			
			++ci;
		}

		m_pComboBoxPanel = 
			new MunkHtmlComboBoxPanel(m_selected_index,
						  pParent, 
						  MunkHtmlFormElement::GetNextID(),
						  arrLabels[m_selected_index],
						  wxDefaultPosition,
						  wxDefaultSize,
						  arrLabels,
						  wxCB_READONLY);

		//m_pComboBoxPanel->Show(true);

		m_pWidgetCell = new MunkHtmlWidgetCell(m_pComboBoxPanel, 0);
		return m_pWidgetCell;
	} else {
		return 0;
	}
}
