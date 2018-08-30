#ifndef MANTIDQT_MANTIDWIDGETS_MANTIDTREEMODEL_H
#define MANTIDQT_MANTIDWIDGETS_MANTIDTREEMODEL_H
//----------------------------------
// Includes
//----------------------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/MantidDisplayBase.h"

#include <QHash>
#include <QObject>

//----------------------------------
// Forward declarations
//----------------------------------

namespace MantidQt {
namespace MantidWidgets {

class MantidWSIndexDialog;

/**
\class MantidTreeModel

\author Elliot Oram
\date 18-12-2017
\version 1.0

Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class EXPORT_OPT_MANTIDQT_COMMON MantidTreeModel
    : public QObject,
      public MantidQt::MantidWidgets::MantidDisplayBase {
  Q_OBJECT

public:
  /// DefaultConstructor
  MantidTreeModel();

  // Data display and saving methods
  void updateRecentFilesList(const QString &fname) override;
  void deleteWorkspaces(const QStringList &wsNames = QStringList()) override;

  // Algorithm Display and Execution Methods
  Mantid::API::IAlgorithm_sptr createAlgorithm(const QString &algName,
                                               int version = -1) override;

  bool executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                             const bool wait = false) override;

  Mantid::API::Workspace_const_sptr
  getWorkspace(const QString &workspaceName) override;
  QWidget *getParent() override;

  MantidQt::API::AlgorithmDialog *
  createAlgorithmDialog(Mantid::API::IAlgorithm_sptr alg);

  // Plotting Methods
  MultiLayer *
  plotSubplots(const QMultiMap<QString, std::set<int>> &toPlot,
               MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
               bool errs = false, MultiLayer *plotWindow = nullptr) override;

public slots:
  // Data display and saving methods
  void enableSaveNexus(const QString &wsName) override;
  void disableSaveNexus() override;
  void importWorkspace() override;

  MantidMatrix *
  importMatrixWorkspace(const Mantid::API::MatrixWorkspace_sptr workspace,
                        int lower = -1, int upper = -1,
                        bool showDlg = true) override;

  void importWorkspace(const QString &wsName, bool showDlg = true,
                       bool makeVisible = true) override;

  void showMantidInstrumentSelected() override;
  Table *createDetectorTable(const QString &wsName,
                             const std::vector<int> &indices,
                             bool include_data = false) override;
  void importBoxDataTable() override;
  void showListData() override;
  void importTransposed() override;
  void renameWorkspace(QStringList = QStringList()) override;

  // Algorithm Display and Execution Methods
  void showAlgorithmDialog(const QString &algName, int version = -1) override;
  void showAlgorithmDialog(const QString &algName,
                           QHash<QString, QString> paramList,
                           Mantid::API::AlgorithmObserver *obs = nullptr,
                           int version = -1) override;
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) override;

  // Plotting Methods
  MultiLayer *
  plot1D(const QMultiMap<QString, std::set<int>> &toPlot, bool spectrumPlot,
         MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
         bool errs = false, MultiLayer *plotWindow = nullptr,
         bool clearWindow = false, bool waterfallPlot = false,
         const QString &log = "",
         const std::set<double> &customLogValues = std::set<double>()) override;

  void drawColorFillPlots(
      const QStringList &wsNames,
      GraphOptions::CurveType curveType = GraphOptions::ColorMap) override;

  void showMDPlot() override;

  void plotSurface(bool accepted, int plotIndex, const QString &axisName,
                   const QString &logName,
                   const std::set<double> &customLogValues,
                   const QList<QString> &workspaceNames) override;

  void plotContour(bool accepted, int plotIndex, const QString &axisName,
                   const QString &logName,
                   const std::set<double> &customLogValues,
                   const QList<QString> &workspaceNames) override;

  // Inteface methods
  // ONLY REQUIRED TO STATIFY MantidDisplayBase INTERFACE
  void showVatesSimpleInterface() override;
  void showSpectrumViewer() override;
  void showSliceViewer() override;
  void showLogFileWindow() override;
  void showSampleMaterialWindow() override;
  void showAlgorithmHistory() override;

  MantidQt::MantidWidgets::MantidWSIndexDialog *createWorkspaceIndexDialog(
      int flags, const QStringList &wsNames, bool showWaterfall,
      bool showPlotAll, bool showTiledOpt, bool isAdvanced = false) override;

  void updateProject() override;
  void showCritical(const QString &) override;

#ifdef MAKE_VATES
  bool doesVatesSupportOpenGL() override;
#endif

private:
  // overide copy operations
  MantidTreeModel(const MantidTreeModel &);
  MantidTreeModel &operator=(const MantidTreeModel &);
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQT_MANTIDWIDGETS_MANTIDTREEMODEL_H