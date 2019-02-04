// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_
#define MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_

#include "DllOption.h"
#include "Hint.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/System.h"

#include <QLineEdit>

#include <map>
#include <string>

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------
class QLabel;

namespace MantidQt {
namespace MantidWidgets {
/** HintingLineEdit : A QLineEdit widget providing autocompletion.
 */
class EXPORT_OPT_MANTIDQT_COMMON HintingLineEdit : public QLineEdit {
  Q_OBJECT
public:
  HintingLineEdit(QWidget *parent, std::vector<Hint> hints);
  ~HintingLineEdit() override;

protected:
  void keyPressEvent(QKeyEvent *e) override;
  void updateMatches();
  void showToolTip();
  void insertSuggestion();
  void clearSuggestion();
  void nextSuggestion();
  void prevSuggestion();
  std::vector<Hint> m_matches;
  std::vector<Hint> m_hints;
  std::string m_currentPrefix; // m_curKey;
  typename std::vector<Hint>::const_iterator m_match;
  bool m_dontComplete;
  QLabel *m_hintLabel;
protected slots:
  void updateHints(const QString &text);
  void hideHints();
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_ */
