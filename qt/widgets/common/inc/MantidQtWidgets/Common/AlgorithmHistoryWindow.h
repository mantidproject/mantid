// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMHISTORYWINDOW_H
#define ALGORITHMHISTORYWINDOW_H

#include "DllOption.h"

#include "MantidAPI/HistoryItem.h"
#include "MantidAPI/HistoryView.h"
#include "MantidAPI/ScriptBuilder.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/EnvironmentHistory.h"

#include <QAbstractListModel>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QTreeWidget>

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
class QMouseEvent;
class QLineEdit;
class QLabel;
class QFileDialog;

//------------------------------------------------------------------------------
// Mantid Forward declarations
//------------------------------------------------------------------------------

namespace Mantid {
namespace API {
class Workspace;
}
} // namespace Mantid

class AlgHistoryItem : public QTreeWidgetItem, public Mantid::API::HistoryItem {
public:
  AlgHistoryItem(const QStringList &names,
                 Mantid::API::AlgorithmHistory_const_sptr algHistory,
                 AlgHistoryItem *parent = nullptr)
      : QTreeWidgetItem(parent, names, UserType), Mantid::API::HistoryItem(
                                                      algHistory) {}
};

class AlgHistoryTreeWidget : public QTreeWidget {
  Q_OBJECT

signals:
  void updateAlgorithmHistoryWindow(
      Mantid::API::AlgorithmHistory_const_sptr algHistory);
  void unrollAlgorithmHistory(const std::vector<int> &indicies);
  void rollAlgorithmHistory(int index);

public:
  /// Constructor
  explicit AlgHistoryTreeWidget(QWidget *w)
      : QTreeWidget(w), m_algName(""), m_nVersion(0) {
    connect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
            SLOT(onItemChanged(QTreeWidgetItem *, int)));
  }
  void
  populateAlgHistoryTreeWidget(const Mantid::API::WorkspaceHistory &wsHist);

protected:
  void selectionChanged(const QItemSelection &selected,
                        const QItemSelection &deselected) override;

private slots:
  void onItemChanged(QTreeWidgetItem *item, int index);

private:
  void treeSelectionChanged();
  void itemChecked(QTreeWidgetItem *item, int index);
  void itemUnchecked(QTreeWidgetItem *item, int index);
  void populateNestedHistory(AlgHistoryItem *parentWidget,
                             Mantid::API::AlgorithmHistory_sptr history);
  void uncheckAllChildren(QTreeWidgetItem *item, int index);
  QString concatVersionwithName(const std::string &name, const int version);

  const static int UNROLL_COLUMN_INDEX = 1;
  QString m_algName;
  int m_nVersion;
};

class AlgExecSummaryGrpBox : public QGroupBox {
  Q_OBJECT
public:
  explicit AlgExecSummaryGrpBox(QWidget *w);
  AlgExecSummaryGrpBox(QString /*title*/, QWidget *w);
  ~AlgExecSummaryGrpBox() override;
  void setData(const double execDuration,
               const Mantid::Types::Core::DateAndTime execDate);

private:
  QLineEdit *getAlgExecDurationCtrl() const { return m_execDurationEdit; }
  QLineEdit *getAlgExecDateCtrl() const { return m_execDateTimeEdit; }

private:
  QLabel *m_execDurationlabel;
  QLineEdit *m_execDurationEdit;
  QLabel *m_Datelabel;
  QLineEdit *m_execDateTimeEdit;
  QString m_algexecDuration;
};

class AlgEnvHistoryGrpBox : public QGroupBox {
  Q_OBJECT
public:
  explicit AlgEnvHistoryGrpBox(QWidget *w);
  AlgEnvHistoryGrpBox(QString /*title*/, QWidget *w);
  ~AlgEnvHistoryGrpBox() override;

  QLineEdit *getosNameEdit() const { return m_osNameEdit; }
  QLineEdit *getosVersionEdit() const { return m_osVersionEdit; }
  QLineEdit *getfrmworkVersionEdit() const { return m_frmwkVersnEdit; }
  void
  fillEnvHistoryGroupBox(const Mantid::Kernel::EnvironmentHistory &envHist);

private:
  QLabel *m_osNameLabel;
  QLineEdit *m_osNameEdit;
  QLabel *m_osVersionLabel;
  QLineEdit *m_osVersionEdit;
  QLabel *m_frmworkVersionLabel;
  QLineEdit *m_frmwkVersnEdit;
};

class AlgHistoryProperties;
class EXPORT_OPT_MANTIDQT_COMMON AlgorithmHistoryWindow : public QDialog {
  Q_OBJECT
signals:
  void updateAlgorithmHistoryWindow(QString algName);

public:
  AlgorithmHistoryWindow(
      QWidget *parent,
      const boost::shared_ptr<const Mantid::API::Workspace> /*wsptr*/);
  AlgorithmHistoryWindow(QWidget *parent, const QString &workspaceName);
  ~AlgorithmHistoryWindow() override;

  void closeEvent(QCloseEvent *ce) override;

public slots:
  void updateAll(Mantid::API::AlgorithmHistory_const_sptr algHistmakeory);
  void doUnroll(const std::vector<int> &unrollIndicies);
  void doRoll(int index);

  void copytoClipboard();
  void writeToScriptFile();

private:
  AlgExecSummaryGrpBox *createExecSummaryGrpBox();
  AlgEnvHistoryGrpBox *
  createEnvHistGrpBox(const Mantid::Kernel::EnvironmentHistory &envHistory);
  AlgHistoryProperties *createAlgHistoryPropWindow();

  QFileDialog *createScriptDialog(const QString &algName);
  void
  updateExecSummaryGrpBox(Mantid::API::AlgorithmHistory_const_sptr algHistory);
  void updateAlgHistoryProperties(
      Mantid::API::AlgorithmHistory_const_sptr algHistory);

  std::string getScriptVersionMode();

private:
  const Mantid::API::WorkspaceHistory &m_algHist;
  QLabel *m_scriptVersionLabel;
  QComboBox *m_scriptComboMode;
  QPushButton *m_scriptButtonFile;
  QPushButton *m_scriptButtonClipboard;
  AlgHistoryTreeWidget *m_Historytree;
  AlgHistoryProperties *m_histPropWindow;
  AlgExecSummaryGrpBox *m_execSumGrpBox;
  AlgEnvHistoryGrpBox *m_envHistGrpBox;
  QString m_wsName;
  boost::shared_ptr<Mantid::API::HistoryView> m_view;
};

class AlgHistoryProperties : public QObject {
  Q_OBJECT

public:
  AlgHistoryProperties(
      QWidget *w,
      const std::vector<Mantid::Kernel::PropertyHistory_sptr> &propHist);

  void displayAlgHistoryProperties();
  void clearData();

  void setAlgProperties(
      const std::vector<Mantid::Kernel::PropertyHistory_sptr> &histProp);
  const Mantid::Kernel::PropertyHistories &getAlgProperties();

public slots:
  void popupMenu(const QPoint &pos);
  void copySelectedItemText();

public:
  QTreeWidget *m_histpropTree;

private:
  QAction *m_copyAction;
  QMenu *m_contextMenu;
  QString m_selectedItemText;

  std::vector<Mantid::Kernel::PropertyHistory_sptr> m_Histprop;
};

#endif // ALGORITHMHISTORYWINDOW_H
