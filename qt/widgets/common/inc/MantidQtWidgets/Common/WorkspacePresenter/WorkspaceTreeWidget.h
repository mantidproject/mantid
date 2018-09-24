#ifndef MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGET_H
#define MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGET_H

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/MantidTreeWidget.h"

#include <MantidAPI/ExperimentInfo.h>
#include <MantidAPI/IAlgorithm_fwd.h>
#include <MantidAPI/IMDEventWorkspace_fwd.h>
#include <MantidAPI/IMDWorkspace.h>
#include <MantidAPI/IPeaksWorkspace_fwd.h>
#include <MantidAPI/ITableWorkspace_fwd.h>
#include <MantidAPI/MatrixWorkspace_fwd.h>
#include <MantidAPI/WorkspaceGroup_fwd.h>

#include <MantidQtWidgets/Common/WorkspacePresenter/IWorkspaceDockView.h>
#include <QDockWidget>
#include <QHash>
#include <QMap>
#include <QMetaType>
#include <QMutex>
#include <boost/shared_ptr.hpp>
#include <map>

class QMainWindow;
class QLabel;
class QFileDialog;
class QLineEdit;
class QActionGroup;
class QMenu;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressBar;
class QVBoxLayout;
class QHBoxLayout;
class QSignalMapper;
class QSortFilterProxyModel;

using TopLevelItems = std::map<std::string, Mantid::API::Workspace_sptr>;
Q_DECLARE_METATYPE(TopLevelItems)

namespace MantidQt {
namespace MantidWidgets {
class MantidDisplayBase;
class MantidTreeWidgetItem;
class MantidTreeWidget;

/**
\class  WorkspaceTreeWidget
\author Lamar Moore
\date   24-08-2016
\version 1.1


Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
class EXPORT_OPT_MANTIDQT_COMMON WorkspaceTreeWidget
    : public QWidget,
      public IWorkspaceDockView {
  Q_OBJECT
public:
  explicit WorkspaceTreeWidget(MantidQt::MantidWidgets::MantidDisplayBase *mdb,
                               QWidget *parent = nullptr);
  ~WorkspaceTreeWidget();
  void dropEvent(QDropEvent *de) override;

  MantidQt::MantidWidgets::WorkspacePresenterWN_wptr
  getPresenterWeakPtr() override;

  SortDirection getSortDirection() const override;
  SortCriteria getSortCriteria() const override;
  void sortWorkspaces(SortCriteria criteria, SortDirection direction) override;

  MantidQt::MantidWidgets::StringList
  getSelectedWorkspaceNames() const override;
  // Horrible second function to get a return value as QStringList directly
  QStringList getSelectedWorkspaceNamesAsQList() const;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const override;

  bool askUserYesNo(const std::string &caption,
                    const std::string &message) const override;
  void showCriticalUserMessage(const std::string &caption,
                               const std::string &message) const override;

  void showLoadDialog() override;
  void showLiveDataDialog() override;
  void
  showRenameDialog(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void enableDeletePrompt(bool enable) override;
  bool isPromptDelete() const override;
  bool deleteConfirmation() const override;
  void
  deleteWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void clearView() override;
  std::string getFilterText() const override;
  SaveFileType getSaveFileType() const override;
  void saveWorkspace(SaveFileType type) override;
  void
  saveWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void filterWorkspaces(const std::string &filterText) override;
  void recordWorkspaceRename(const std::string &oldName,
                             const std::string &newName) override;
  void refreshWorkspaces() override;

  // Context Menu Handlers
  void popupContextMenu() override;
  void showWorkspaceData() override;
  void saveToProgram() override;
  void showInstrumentView() override;
  void plotSpectrum(std::string type) override;
  void showColourFillPlot() override;
  void showDetectorsTable() override;
  void showBoxDataTable() override;
  void showVatesGUI() override;
  void showMDPlot() override;
  void showListData() override;
  void showSpectrumViewer() override;
  void showSliceViewer() override;
  void showLogs() override;
  void showSampleMaterialWindow() override;
  void showAlgorithmHistory() override;
  void showTransposed() override;
  void convertToMatrixWorkspace() override;
  void convertMDHistoToMatrixWorkspace() override;

  bool executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                             const bool wait = true) override;

private:
  bool hasUBMatrix(const std::string &wsName);
  void addSaveMenuOption(QString algorithmString, QString menuEntryName = "");
  void setTreeUpdating(const bool state);
  inline bool isTreeUpdating() const { return m_treeUpdating; }
  void updateTree(const TopLevelItems &items) override;
  void populateTopLevel(const TopLevelItems &topLevelItems,
                        const QStringList &expanded);
  MantidTreeWidgetItem *
  addTreeEntry(const std::pair<std::string, Mantid::API::Workspace_sptr> &item,
               QTreeWidgetItem *parent = nullptr);
  bool shouldBeSelected(QString name) const;
  void createWorkspaceMenuActions();
  void createSortMenuActions();
  void setItemIcon(QTreeWidgetItem *item, const std::string &wsID);

  void addMatrixWorkspaceMenuItems(
      QMenu *menu,
      const Mantid::API::MatrixWorkspace_const_sptr &matrixWS) const;
  void addMDEventWorkspaceMenuItems(
      QMenu *menu,
      const Mantid::API::IMDEventWorkspace_const_sptr &mdeventWS) const;
  void addMDHistoWorkspaceMenuItems(
      QMenu *menu, const Mantid::API::IMDWorkspace_const_sptr &WS) const;
  void addPeaksWorkspaceMenuItems(
      QMenu *menu, const Mantid::API::IPeaksWorkspace_const_sptr &WS) const;
  void addWorkspaceGroupMenuItems(QMenu *menu) const;
  void addTableWorkspaceMenuItems(QMenu *menu) const;
  void addClearMenuItems(QMenu *menu, const QString &wsName);

  void excludeItemFromSort(MantidTreeWidgetItem *item);

  void setupWidgetLayout();
  void setupLoadButtonMenu();
  void setupConnections();

  MantidQt::MantidWidgets::MantidItemSortScheme
  whichCriteria(SortCriteria criteria);

public slots:
  void clickedWorkspace(QTreeWidgetItem *, int);
  void saveWorkspaceCollection();
  void onClickDeleteWorkspaces();
  void renameWorkspace();
  void populateChildData(QTreeWidgetItem *item);
  void onClickSaveToProgram(const QString &name);
  void sortAscending();
  void sortDescending();
  void chooseByName();
  void chooseByLastModified();
  void chooseByMemorySize();
  void keyPressEvent(QKeyEvent *) override;

protected slots:
  void popupMenu(const QPoint &pos);
  void workspaceSelected();

private slots:
  void handleShowSaveAlgorithm();
  void treeSelectionChanged();
  void onClickGroupButton();
  void onClickLoad();
  void onLoadAccept();
  void onClickLiveData();
  void onClickShowData();
  void onClickShowInstrument();
  void onClickShowBoxData();
  void onClickShowVates();
  void onClickShowMDPlot();
  void onClickShowListData();
  void onClickShowSpectrumViewer();
  void onClickShowSliceViewer();
  void onClickShowFileLog();
  void onClickSaveNexusWorkspace();
  void onClickShowTransposed();
  void onClickPlotSpectra();
  void onClickPlotSpectraErr();
  void onClickPlotAdvanced();
  void onClickDrawColorFillPlot();
  void onClickShowDetectorTable();
  void onClickConvertToMatrixWorkspace();
  void onClickConvertMDHistoToMatrixWorkspace();
  void onClickShowAlgHistory();
  void onClickShowSampleMaterial();
  void onClickClearUB();
  void incrementUpdateCount();
  void filterWorkspaceTree(const QString &text);

private:
  MantidQt::MantidWidgets::WorkspacePresenterVN_sptr m_presenter;

protected:
  MantidTreeWidget *m_tree;
  QPoint m_menuPosition;
  QString selectedWsName;
  QMenu *m_loadMenu, *m_saveToProgram;
  QSignalMapper *m_programMapper;
  QAction *m_program, *m_saveNexus, *m_rename, *m_delete;

private:
  QString m_programName;
  MantidDisplayBase *const m_mantidDisplayModel;

  std::string m_filteredText;
  QPushButton *m_loadButton;
  QPushButton *m_saveButton;
  QPushButton *m_deleteButton;
  QPushButton *m_groupButton;
  QPushButton *m_sortButton;
  QLineEdit *m_workspaceFilter;
  QActionGroup *m_sortChoiceGroup;
  QFileDialog *m_saveFolderDialog;

  QMenu *m_sortMenu, *m_saveMenu;
  // Context-menu actions
  QAction *m_showData, *m_showInst, *m_plotSpec, *m_plotSpecErr,
      *m_plotAdvanced, *m_showDetectors, *m_showBoxData, *m_showVatesGui,
      *m_showSpectrumViewer, *m_showSliceViewer, *m_colorFill, *m_showLogs,
      *m_showSampleMaterial, *m_showHist, *m_showMDPlot, *m_showListData,
      *m_ascendingSortAction, *m_descendingSortAction, *m_byNameChoice,
      *m_byLastModifiedChoice, *m_showTransposed, *m_convertToMatrixWorkspace,
      *m_convertMDHistoToMatrixWorkspace, *m_clearUB;

  QAtomicInt m_updateCount;
  bool m_treeUpdating;
  bool m_promptDelete;
  SaveFileType m_saveFileType;
  SortCriteria m_sortCriteria;
  SortDirection m_sortDirection;
  /// Temporarily keeps names of selected workspaces during tree update
  /// in order to restore selection after update
  QStringList m_selectedNames;
  /// Keep a map of renamed workspaces between updates
  QHash<QString, QString> m_renameMap;
  /// A mutex to lock m_renameMap and m_selectedNames for reading/writing
  mutable QMutex m_mutex;

private slots:
  void handleUpdateTree(const TopLevelItems &);
  void handleClearView();
signals:
  void signalClearView();
  void signalUpdateTree(const TopLevelItems &);
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGET_H
