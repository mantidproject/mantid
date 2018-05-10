#ifndef MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_
#define MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_

#include "DllOption.h"
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

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTIDQT_COMMON HintingLineEdit : public QLineEdit {
  Q_OBJECT
public:
  HintingLineEdit(QWidget *parent,
                  const std::map<std::string, std::string> &hints);
  ~HintingLineEdit() override;

protected:
  void keyPressEvent(QKeyEvent *e) override;
  void updateMatches();
  void showToolTip();
  void insertSuggestion();
  void clearSuggestion();
  void nextSuggestion();
  void prevSuggestion();
  std::string m_curKey;
  std::string m_curMatch;
  std::map<std::string, std::string> m_matches;
  std::map<std::string, std::string> m_hints;
  bool m_dontComplete;
  QLabel *m_hintLabel;
protected slots:
  void updateHints(const QString &text);
  void hideHints();
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_ */
