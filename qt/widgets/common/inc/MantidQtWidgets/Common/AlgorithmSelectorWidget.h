#ifndef MANTID_MANTIDWIDGETS_ALGORITHMSELECTORWIDGET_H_
#define MANTID_MANTIDWIDGETS_ALGORITHMSELECTORWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "DllOption.h"

#include <QComboBox>
#include <QPoint>
#include <QString>
#include <QTreeWidget>

#include <vector>
#include <Poco/NObserver.h>

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------
class QPushButton;

namespace Mantid {
namespace API {
struct Algorithm_descriptor;
}
}
namespace MantidQt {
namespace MantidWidgets {

class AlgorithmTreeWidget;
class FindAlgComboBox;

/**
 * Represents the algorithm selected by the user
 * Contains name and version
 */
struct SelectedAlgorithm {
  QString name;
  int version;
  /// implicit conversion to QString
  operator QString() { return name; }
  /// constructor
  SelectedAlgorithm(const QString nameIn, const int versionIn)
      : name(nameIn), version(versionIn){};
};

//============================================================================
/** A widget consisting of a ComboBox and a TreeWidget
 * to allow a user to select an algorithm either by category
 * or by typing.

  @date 2012-03-06
  */
class EXPORT_OPT_MANTIDQT_COMMON AlgorithmSelectorWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(bool showExecuteButton READ showExecuteButton WRITE
                 showExecuteButton)

public:
  AlgorithmSelectorWidget(QWidget *parent);
  ~AlgorithmSelectorWidget() override;
  SelectedAlgorithm getSelectedAlgorithm();
  void setSelectedAlgorithm(QString &algName);
  bool showExecuteButton() const;
  void showExecuteButton(const bool);

public slots:
  void update();
  void executeSelected();
  void findAlgTextChanged(const QString &text);
  void treeSelectionChanged();

signals:
  void algorithmFactoryUpdateReceived();
  void executeAlgorithm(const QString &, int);
  void algorithmSelectionChanged(const QString &, int);

protected:
  AlgorithmTreeWidget *m_tree;
  FindAlgComboBox *m_findAlg;
  QPushButton *m_execButton;

private:
  /// Callback for AlgorithmFactory update notifications
  void handleAlgorithmFactoryUpdate(
      Mantid::API::AlgorithmFactoryUpdateNotification_ptr);
  /// Observes algorithm factory update notifications
  Poco::NObserver<AlgorithmSelectorWidget,
                  Mantid::API::AlgorithmFactoryUpdateNotification>
      m_updateObserver;
  // Flag to indicate that we are updating
  bool m_updateInProgress;
};

//============================================================================
/** Tree widget with the categories and algorithms listed
 *
 */
class AlgorithmTreeWidget : public QTreeWidget {
  Q_OBJECT
public:
  AlgorithmTreeWidget(QWidget *w) : QTreeWidget(w) {}
  ~AlgorithmTreeWidget() override {}
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;
  SelectedAlgorithm getSelectedAlgorithm();

public slots:
  void update();

signals:
  /// Signal emitted when the widget requests that we execute that algorithm
  void executeAlgorithm(const QString &, int);

private:
  QPoint m_dragStartPosition;
};

//============================================================================
/** ComboBox for finding algorithms
 *
 */
class FindAlgComboBox : public QComboBox {
  Q_OBJECT
public:
  ~FindAlgComboBox() override {}
  SelectedAlgorithm getSelectedAlgorithm();

signals:
  void enterPressed();

public slots:
  void update();

protected:
  void keyPressEvent(QKeyEvent *e) override;

private:
  using AlgNamesType = std::vector<Mantid::API::AlgorithmDescriptor>;
  void addAliases(AlgNamesType &algNamesList);
  QString stripAlias(const QString &text) const;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_ALGORITHMSELECTORWIDGET_H_ */

/**
Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
