// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_ALTERNATECSPYTHONLEXER_H
#define MANTIDQT_WIDGETS_ALTERNATECSPYTHONLEXER_H

#include <Qsci/qscilexerpython.h>

/**
 * Defines a Python lexer with a alternative colour scheme
 * to the standard one provided by QsciLexerPython.
 */
class AlternateCSPythonLexer : public QsciLexerPython {
public:
  AlternateCSPythonLexer(const QFont &font);

  QColor defaultColor(int style) const override;
  QFont defaultFont(int style) const override;

private:
  QFont m_font;
};

#endif // MANTIDQT_WIDGETS_ALTERNATECSPYTHONLEXER_H
