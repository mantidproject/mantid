// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : TextEditor.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QTextEdit>

class Graph;

class TextEditor : public QTextEdit {
  Q_OBJECT

public:
  explicit TextEditor(Graph *g);
  ~TextEditor() override;
  void formatText(const QString &prefix, const QString &postfix);

signals:
  void textEditorDeleted();
public slots:
  void addSymbol(const QString &letter);

private:
  void closeEvent(QCloseEvent *e) override;
  QString d_initial_text;
  QWidget *d_target;
};

#endif
