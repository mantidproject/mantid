#include "MantidQtWidgets/Common/AlternateCSPythonLexer.h"

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
  return QsciLexer::defaultFont(style);

  //  switch (style) {
  //  case Comment:
  //#if defined(Q_OS_WIN)
  //    f = QFont("Comic Sans MS", 9);
  //#elif defined(Q_OS_MAC)
  //    f = QFont("Comic Sans MS", 12);
  //#else
  //    f = QFont("Bitstream Vera Serif", 9);
  //#endif
  //    break;

  //  case DoubleQuotedString:
  //  case SingleQuotedString:
  //  case UnclosedString:
  //#if defined(Q_OS_WIN)
  //    f = QFont("Courier New", 10);
  //#elif defined(Q_OS_MAC)
  //    f = QFont("Courier", 12);
  //#else
  //    f = QFont("Bitstream Vera Sans Mono", 9);
  //#endif
  //    break;

  //  case Keyword:
  //  case ClassName:
  //  case FunctionMethodName:
  //  case Operator:
  //    f = QsciLexer::defaultFont(style);
  //    f.setBold(true);
  //    break;

  //  default:
  //    f = QsciLexer::defaultFont(style);
  //  }

  //  return f;
}
