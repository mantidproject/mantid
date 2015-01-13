#ifndef MANTID_MANTIDWIDGETS_ALGORITHMSELECTORWIDGET_H_
#define MANTID_MANTIDWIDGETS_ALGORITHMSELECTORWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "WidgetDllOption.h"

#include <Poco/NObserver.h>

#include <QtGui>

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------
namespace MantidQt
{
namespace MantidWidgets
{

  class AlgorithmTreeWidget;
  class FindAlgComboBox;

  //============================================================================
  /** A widget consisting of a ComboBox and a TreeWidget
   * to allow a user to select an algorithm either by category
   * or by typing.

    @date 2012-03-06
    */
  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS AlgorithmSelectorWidget : public QWidget
  {
    Q_OBJECT
    Q_PROPERTY(bool showExecuteButton READ showExecuteButton WRITE showExecuteButton)

  public:
    AlgorithmSelectorWidget(QWidget *parent);
    virtual ~AlgorithmSelectorWidget();
    void getSelectedAlgorithm(QString& algName, int& version);
    QString getSelectedAlgorithm();
    void setSelectedAlgorithm(QString & algName);
    bool showExecuteButton() const;
    void showExecuteButton(const bool);

  public slots:
    void update();
    void executeSelected();
    void findAlgTextChanged(const QString& text);
    void treeSelectionChanged();

signals:
    void algorithmFactoryUpdateReceived();
    void executeAlgorithm(const QString &, int);
    void algorithmSelectionChanged(const QString &, int);

  protected:
    AlgorithmTreeWidget *m_tree;
    FindAlgComboBox* m_findAlg;
    QPushButton *m_execButton;

  private:
    /// Callback for AlgorithmFactory update notifications
    void handleAlgorithmFactoryUpdate(Mantid::API::AlgorithmFactoryUpdateNotification_ptr);
    /// Observes algorithm factory update notifications
    Poco::NObserver<AlgorithmSelectorWidget,
                    Mantid::API::AlgorithmFactoryUpdateNotification> m_updateObserver;
    // Flag to indicate that we are updating
    bool m_updateInProgress;
  };

  //============================================================================
  /** Tree widget with the categories and algorithms listed
   *
   */
  class AlgorithmTreeWidget:public QTreeWidget
  {
    Q_OBJECT
  public:
    AlgorithmTreeWidget(QWidget *w):QTreeWidget(w){}
    virtual ~AlgorithmTreeWidget() {}
    void mousePressEvent (QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void getSelectedAlgorithm(QString& algName, int& version);

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
  class FindAlgComboBox: public QComboBox
  {
    Q_OBJECT
  public:
    virtual ~FindAlgComboBox() {}
    void getSelectedAlgorithm(QString& algName, int& version);

  signals:
    void enterPressed();

  public slots:
    void update();

  protected:
    void keyPressEvent(QKeyEvent *e);
  };




} // namespace MantidWidgets
} // namespace MantidQt

#endif  /* MANTID_MANTIDWIDGETS_ALGORITHMSELECTORWIDGET_H_ */


/**
Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
