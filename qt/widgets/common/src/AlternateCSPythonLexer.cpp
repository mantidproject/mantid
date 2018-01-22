#include "MantidQtWidgets/Common/AlternateCSPythonLexer.h"
#include <QApplication>

/**
 * Returns the foreground colour of the text for a style.
 * @param style An enum defining the type of element encountered
 * @return A QColor for this element type
 */
QColor AlternateCSPythonLexer::defaultColor(int style) const {
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

  case Keyword:
    return QColor(0x00, 0x00, 0xff);

  case TripleSingleQuotedString:
  case TripleDoubleQuotedString:
    return QColor(0x00, 0xaa, 0x00);

  case ClassName:
    return QColor(0x00, 0x00, 0x00);

  case FunctionMethodName:
    return QColor(0x00, 0x7f, 0x7f);

  case Operator:
  case Identifier:
    break;

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
 *  Returns the font of the text for a style.
 * @param style An enum defining the type of element encountered
 * @return A QFont for this element type
 */
QFont AlternateCSPythonLexer::defaultFont(int style) const {
  Q_UNUSED(style);
  return QApplication::font();
}
