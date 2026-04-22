// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlternateCSPythonLexer.h"
#include "MantidKernel/ConfigService.h"

/**
 * Construct a lexer with a font to be used for all text styles
 * @param font A font to used for the text
 */
AlternateCSPythonLexer::AlternateCSPythonLexer(const QFont &font) : QsciLexerPython(), m_font(font) {
  auto &config = Mantid::Kernel::ConfigService::Instance();
  std::string apply_dark_theme = "False";
  if (config.hasProperty("editors.apply_dark_theme")) {
    apply_dark_theme = config.getString("editors.apply_dark_theme");
  }
  m_isDarkMode = (apply_dark_theme == "True");
}

/**
 * Returns the foreground colour of the text for a style.
 * @param style An enum defining the type of element encountered
 * @return A QColor for this element type
 */
QColor AlternateCSPythonLexer::defaultColor(int style) const {
  return m_isDarkMode ? defaultColorDark(style) : defaultColorLight(style);
}

/**
 * Returns the foreground colour of the text for a light theme style.
 * @param style An enum defining the type of element encountered
 * @return A QColor for this element type
 */
QColor AlternateCSPythonLexer::defaultColorLight(int style) const {
  switch (style) {
  case Default:
    return QColor(0x00, 0x00, 0x00);

  case Comment:
    return QColor(0xad, 0xad, 0xad);

  case Number:
    return QColor(0x80, 0x00, 0x00);

  case DoubleQuotedString:
  case SingleQuotedString:
    return QColor(0x00, 0xaa, 0x00);

  case DoubleQuotedFString:
  case SingleQuotedFString:
    return QColor(0x00, 0xaa, 0x00);

  case Keyword:
    return QColor(0x00, 0x00, 0xff);

  case TripleSingleQuotedString:
  case TripleDoubleQuotedString:
    return QColor(0x00, 0xaa, 0x00);

  case TripleSingleQuotedFString:
  case TripleDoubleQuotedFString:
    return QColor(0x00, 0xaa, 0x00);

  case ClassName:
    return QColor(0x00, 0x00, 0x00);

  case FunctionMethodName:
    return QColor(0x00, 0x7f, 0x7f);

  case Operator:
  case Identifier:
    return QColor(0x00, 0x00, 0x00);

  case CommentBlock:
    return QColor(0xad, 0xad, 0xad);

  case UnclosedString:
    return QColor(0x00, 0x00, 0x00);

  case HighlightedIdentifier:
    return QColor(0x40, 0x70, 0x90);

  case Decorator:
    return QColor(0x80, 0x50, 0x00);
  }

  return QsciLexer::defaultColor(style);
}

/**
 * Returns the foreground colour of the text for a dark theme style.
 * @param style An enum defining the type of element encountered
 * @return A QColor for this element type
 */
QColor AlternateCSPythonLexer::defaultColorDark(int style) const {
  switch (style) {
  case Default:
    return QColor(0xFF, 0xFF, 0xFF);

  case Comment:
    return QColor(0x72, 0x86, 0xB7);

  case Number:
    return QColor(0xFF, 0xC6, 0x8F);

  case DoubleQuotedString:
  case SingleQuotedString:
    return QColor(0x98, 0xC3, 0x78);

  case DoubleQuotedFString:
  case SingleQuotedFString:
    return QColor(0xE0, 0x6D, 0x76);

  case Keyword:
    return QColor(0xC6, 0x78, 0xDD);

  case TripleSingleQuotedString:
  case TripleDoubleQuotedString:
    return QColor(0xCE, 0x91, 0x78);

  case TripleSingleQuotedFString:
  case TripleDoubleQuotedFString:
    return QColor(0xE0, 0x6D, 0x76);

  case ClassName:
    return QColor(0xFF, 0xEE, 0xAE);

  case FunctionMethodName:
    return QColor(0xBB, 0xDA, 0xFF);

  case Operator:
    return QColor(0x99, 0xFF, 0xFF);

  case Identifier:
    return QColor(0xFF, 0xFF, 0xFF);

  case CommentBlock:
    return QColor(0xad, 0xad, 0xad);

  case UnclosedString:
    return QColor(0xFF, 0xFF, 0xFF);

  case HighlightedIdentifier:
    return QColor(0x00, 0x3F, 0x8E);

  case Decorator:
    return QColor(0xDC, 0xDC, 0xAA);
  }

  return QsciLexer::defaultColor(style);
}

/**
 * Returns the font of the text.
 * @param style Unused.
 * @return A QFont for this element type
 */
QFont AlternateCSPythonLexer::defaultFont(int style) const {
  Q_UNUSED(style);
  return m_font;
}

///**
// * Returns the keywords used by the lexer for a given set.
// * @param set The keyword set to retrieve (1 is for keywords)
// * @return A string containing the keywords
// */
const char *AlternateCSPythonLexer::keywords(int set) const {
  if (set == 1) {
    // Retrieve default Python keywords from qscintilla
    const char *defaultKeywords = QsciLexerPython::keywords(1);
    static std::string combinedKeywords;
    std::ostringstream stream;
    stream << defaultKeywords << " " << customKeywords;

    combinedKeywords = stream.str();
    return combinedKeywords.c_str();
  }

  return QsciLexerPython::keywords(set);
}

QColor AlternateCSPythonLexer::defaultPaper(int style) const {
  Q_UNUSED(style);
  return m_isDarkMode ? QColor(30, 30, 30) : QColor(255, 255, 255);
}
