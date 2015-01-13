#ifndef MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_
#define MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "WidgetDllOption.h"

#include <map>
#include <QtGui>
#include <string>

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------
namespace MantidQt
{
  namespace MantidWidgets
  {
    /** HintingLineEdit : A QLineEdit widget providing autocompletion.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS HintingLineEdit : public QLineEdit
    {
      Q_OBJECT
    public:
      HintingLineEdit(QWidget *parent, const std::map<std::string,std::string> &hints);
      virtual ~HintingLineEdit();
    protected:
      virtual void keyPressEvent(QKeyEvent* e);
      void updateMatches();
      void showToolTip();
      void insertSuggestion();
      void clearSuggestion();
      void nextSuggestion();
      void prevSuggestion();
      std::string m_curKey;
      std::string m_curMatch;
      std::map<std::string,std::string> m_matches;
      std::map<std::string,std::string> m_hints;
      bool m_dontComplete;
      QLabel* m_hintLabel;
    protected slots:
      void updateHints(const QString& text);
    };
  } //namespace MantidWidgets
} //namepsace MantidQt

#endif /* MANTID_MANTIDWIDGETS_HINTINGLINEEDIT_H_ */
