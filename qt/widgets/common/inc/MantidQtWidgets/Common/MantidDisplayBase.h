// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DistributionOptions.h"
#include "MantidQtWidgets/Common/GraphOptions.h"
#include <QStringList>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

template <class Key, class T> class QHash;
template <class Key, class T> class QMultiMap;
template <class T> class QList;
class QString;
class QWidget;
class Table;
class MultiLayer;
class MantidMatrix;

namespace MantidQt {
namespace MantidWidgets {

class MantidWSIndexDialog;

/**
\class  MantidBase
\brief  Contains display methods which will be used by
QWorkspaceDockView.
\author Lamar Moore
\date   24-08-2016
\version 1.0
*/
class DLLExport MantidDisplayBase {
public:
  virtual ~MantidDisplayBase() = default;

  // Data display and saving methods
  virtual void updateRecentFilesList(const QString &fname) = 0;
  virtual void enableSaveNexus(const QString &wsName) = 0;
  virtual void disableSaveNexus() = 0;
  virtual void deleteWorkspaces(const QStringList &wsNames = QStringList()) = 0;
  virtual void importWorkspace() = 0;
  virtual MantidMatrix *importMatrixWorkspace(const Mantid::API::MatrixWorkspace_sptr workspace, int lower = -1,
                                              int upper = -1, bool showDlg = true) = 0;
  virtual void importWorkspace(const QString &wsName, bool showDlg = true, bool makeVisible = true) = 0;
  virtual void renameWorkspace(QStringList = QStringList()) = 0;
  virtual void showMantidInstrumentSelected() = 0;
  virtual Table *createDetectorTable(const QString &wsName, const std::vector<int> &indices,
                                     bool include_data = false) = 0;
  virtual void importBoxDataTable() = 0;
  virtual void showListData() = 0;
  virtual void importTransposed() = 0;

  // Algorithm Display and Execution Methods
  virtual Mantid::API::IAlgorithm_sptr createAlgorithm(const QString &algName, int version = -1) = 0;
  virtual void showAlgorithmDialog(const QString &algName, int version = -1) = 0;
  virtual void showAlgorithmDialog(const QString &algName, QHash<QString, QString> paramList,
                                   Mantid::API::AlgorithmObserver *obs = nullptr, int version = -1) = 0;
  virtual void executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) = 0;
  virtual bool executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg, const bool wait = false) = 0;

  virtual Mantid::API::Workspace_const_sptr getWorkspace(const QString &workspaceName) = 0;

  virtual QWidget *getParent() = 0;

  // Plotting Methods
  virtual MultiLayer *plot1D(const QMultiMap<QString, std::set<int>> &toPlot, bool spectrumPlot,
                             MantidQt::DistributionFlag distr = MantidQt::DistributionDefault, bool errs = false,
                             MultiLayer *plotWindow = nullptr, bool clearWindow = false, bool waterfallPlot = false,
                             const QString &log = "", const std::set<double> &customLogValues = std::set<double>()) = 0;
  virtual void drawColorFillPlots(const QStringList &wsNames,
                                  GraphOptions::CurveType curveType = GraphOptions::ColorMap) = 0;
  virtual void showMDPlot() = 0;
  virtual MultiLayer *plotSubplots(const QMultiMap<QString, std::set<int>> &toPlot,
                                   MantidQt::DistributionFlag distr = MantidQt::DistributionDefault, bool errs = false,
                                   MultiLayer *plotWindow = nullptr) = 0;
  virtual void plotSurface(bool accepted, int plotIndex, const QString &axisName, const QString &logName,
                           const std::set<double> &customLogValues, const QList<QString> &workspaceNames) = 0;
  virtual void plotContour(bool accepted, int plotIndex, const QString &axisName, const QString &logName,
                           const std::set<double> &customLogValues, const QList<QString> &workspaceNames) = 0;

  // Interface Methods
  virtual void showSpectrumViewer() = 0;
  virtual void showSliceViewer() = 0;
  virtual void showLogFileWindow() = 0;
  virtual void showSampleMaterialWindow() = 0;
  virtual void showAlgorithmHistory() = 0;

  virtual MantidWSIndexDialog *createWorkspaceIndexDialog(int flags, const QStringList &wsNames, bool showWaterfall,
                                                          bool showPlotAll, bool showTiledOpt,
                                                          bool isAdvanced = false) = 0;

  virtual void updateProject() = 0;
  virtual void showCritical(const QString & /*unused*/) {}
};
} // namespace MantidWidgets
} // namespace MantidQt
