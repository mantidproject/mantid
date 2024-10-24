// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/System.h"

#include <QComboBox>
#include <QPoint>
#include <QString>
#include <QTreeWidget>

#include <Poco/NObserver.h>
#include <vector>

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------
class QPushButton;

namespace Mantid {
namespace API {
struct Algorithm_descriptor;
}
} // namespace Mantid
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
  SelectedAlgorithm(const QString &nameIn, const int versionIn) : name(nameIn), version(versionIn) {};
};

//============================================================================
/** A widget consisting of a ComboBox and a TreeWidget
 * to allow a user to select an algorithm either by category
 * or by typing.

  @date 2012-03-06
  */
class EXPORT_OPT_MANTIDQT_COMMON AlgorithmSelectorWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(bool showExecuteButton READ showExecuteButton WRITE showExecuteButton)

public:
  AlgorithmSelectorWidget(QWidget *parent);
  ~AlgorithmSelectorWidget() override;
  SelectedAlgorithm getSelectedAlgorithm();
  void setSelectedAlgorithm(QString &algName);
  bool showExecuteButton() const;
  void showExecuteButton(const bool /*val*/);

public slots:
  void update();
  void executeSelected();
  void findAlgTextChanged(const QString &text);
  void treeSelectionChanged();

signals:
  void algorithmFactoryUpdateReceived();
  void executeAlgorithm(const QString & /*_t1*/, int /*_t2*/);
  void algorithmSelectionChanged(const QString & /*_t1*/, int /*_t2*/);

protected:
  AlgorithmTreeWidget *m_tree;
  FindAlgComboBox *m_findAlg;
  QPushButton *m_execButton;

private:
  /// Callback for AlgorithmFactory update notifications
  void handleAlgorithmFactoryUpdate(Mantid::API::AlgorithmFactoryUpdateNotification_ptr /*unused*/);
  /// Observes algorithm factory update notifications
  Poco::NObserver<AlgorithmSelectorWidget, Mantid::API::AlgorithmFactoryUpdateNotification> m_updateObserver;
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
  ~AlgorithmTreeWidget() override = default;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;
  SelectedAlgorithm getSelectedAlgorithm();

public slots:
  void update();

signals:
  /// Signal emitted when the widget requests that we execute that algorithm
  void executeAlgorithm(const QString & /*_t1*/, int /*_t2*/);

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
  ~FindAlgComboBox() override = default;
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
