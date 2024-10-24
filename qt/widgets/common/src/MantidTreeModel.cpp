// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MantidTreeModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"
#include <Poco/ActiveResult.h>
#include <QMessageBox>
#include <qcoreapplication.h>

using namespace std;
using namespace MantidQt;
using namespace MantidWidgets;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("WorkspaceWidget");
}

MantidTreeModel::MantidTreeModel() = default;

// Data display and saving methods
void MantidTreeModel::deleteWorkspaces(const QStringList &wsNames) {
  try {
    if (!wsNames.isEmpty()) {
      auto alg = createAlgorithm("DeleteWorkspaces");
      alg->setLogging(false);
      std::vector<std::string> vecWsNames;
      vecWsNames.reserve(wsNames.size());
      foreach (auto wsName, wsNames) { vecWsNames.emplace_back(wsName.toStdString()); }
      alg->setProperty("WorkspaceList", vecWsNames);
      executeAlgorithmAsync(alg);
    }
  } catch (...) {
    QMessageBox::warning(nullptr, "", "Could not delete selected workspaces.");
  }
}

void MantidTreeModel::renameWorkspace(QStringList wsName) {
  // Determine the algorithm
  QString algName("RenameWorkspace");
  if (wsName.size() > 1)
    algName = "RenameWorkspaces";

  QHash<QString, QString> presets;
  if (wsName.size() > 1) {
    presets["InputWorkspaces"] = wsName.join(",");
  } else {
    presets["InputWorkspace"] = wsName[0];
  }
  showAlgorithmDialog(algName, presets);
}

// Algorithm Display and Execution Methods
Mantid::API::IAlgorithm_sptr MantidTreeModel::createAlgorithm(const QString &algName, int version) {
  Mantid::API::IAlgorithm_sptr alg;
  try {
    alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(), version);
  } catch (...) {
    QString message = "Cannot create algorithm \"" + algName + "\"";
    if (version != -1) {
      message += " version " + QString::number(version);
    }
    QMessageBox::warning(nullptr, "", message);
    alg = Mantid::API::IAlgorithm_sptr();
  }
  return alg;
}
void MantidTreeModel::showAlgorithmDialog(const QString &algName, int version) {
  // Check if Alg is valid
  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, version);
  if (!alg)
    return;
  MantidQt::API::InterfaceManager interfaceManager;
  MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialog(alg, nullptr, false);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

void MantidTreeModel::showAlgorithmDialog(const QString &algName, QHash<QString, QString> paramList,
                                          Mantid::API::AlgorithmObserver *obs, int version) {
  // Get latest version of the algorithm
  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, version);
  if (!alg)
    return;

  try {
    for (QHash<QString, QString>::Iterator it = paramList.begin(); it != paramList.end(); ++it) {
      alg->setPropertyValue(it.key().toStdString(), it.value().toStdString());
    }
  } catch (std::exception &ex) {
    g_log.error() << "Error setting the properties for algotithm " << algName.toStdString() << ": " << ex.what()
                  << '\n';
    return;
  }
  MantidQt::API::AlgorithmDialog *dlg = createAlgorithmDialog(alg);
  if (obs) {
    dlg->addAlgorithmObserver(obs);
  }

  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

/**
 * This creates an algorithm dialog (the default property entry thingie).
 * Helper function not required by interface
 */
MantidQt::API::AlgorithmDialog *MantidTreeModel::createAlgorithmDialog(const Mantid::API::IAlgorithm_sptr &alg) {
  QHash<QString, QString> presets;
  QStringList enabled;

  // If a property was explicitly set show it as preset in the dialog
  const std::vector<Mantid::Kernel::Property *> props = alg->getProperties();
  std::vector<Mantid::Kernel::Property *>::const_iterator p = props.begin();
  for (; p != props.end(); ++p) {
    if (!(**p).isDefault()) {
      QString property_name = QString::fromStdString((**p).name());
      presets.insert(property_name, QString::fromStdString((**p).value()));
      enabled.append(property_name);
    }
  }

  // Check if a workspace is selected in the dock and set this as a preference
  // for the input workspace
  // This is an optional message displayed at the top of the GUI.
  QString optional_msg(alg->summary().c_str());

  MantidQt::API::InterfaceManager interfaceManager;
  MantidQt::API::AlgorithmDialog *dlg =
      interfaceManager.createDialog(alg, nullptr, false, presets, optional_msg, enabled);
  return dlg;
}

void MantidTreeModel::executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) { executeAlgorithmAsync(alg); }

bool MantidTreeModel::executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg, const bool wait) {
  if (wait) {
    Poco::ActiveResult<bool> result(alg->executeAsync());
    while (!result.available()) {
      QCoreApplication::processEvents();
    }
    result.wait();

    try {
      return result.data();
    } catch (Poco::NullPointerException &) {
      return false;
    }
  } else {
    try {
      alg->executeAsync();
    } catch (Poco::NoThreadAvailableException &) {
      g_log.error() << "No thread was available to run the " << alg->name() << " algorithm in the background.\n";
      return false;
    }
    return true;
  }
}

Workspace_const_sptr MantidTreeModel::getWorkspace(const QString &workspaceName) {
  if (AnalysisDataService::Instance().doesExist(workspaceName.toStdString())) {
    return AnalysisDataService::Instance().retrieve(workspaceName.toStdString());
  }
  Workspace_const_sptr empty;
  return empty;
}

//===========================================================
// Interface require functions
//===========================================================
// The following functions have not been implemented as they
// require other code to be ported to the workbench
// In the case of functions that return, they return nullptr

void MantidTreeModel::updateRecentFilesList(const QString &fname) { /*Not require until tool bar is created*/
  Q_UNUSED(fname);
}
void MantidTreeModel::enableSaveNexus(const QString &wsName) { /*handled by widget*/
  Q_UNUSED(wsName);
}
void MantidTreeModel::disableSaveNexus() { /* handled by widget*/
}

void MantidTreeModel::showSpectrumViewer() {}
void MantidTreeModel::showSliceViewer() {}
void MantidTreeModel::showAlgorithmHistory() {}
void MantidTreeModel::showMDPlot() {}
QWidget *MantidTreeModel::getParent() { return new QWidget(nullptr); }
void MantidTreeModel::updateProject() {}
void MantidTreeModel::showCritical(const QString &msg) { Q_UNUSED(msg); }
void MantidTreeModel::showMantidInstrumentSelected() {}
void MantidTreeModel::importBoxDataTable() {}
void MantidTreeModel::showListData() {}
void MantidTreeModel::importTransposed() {}
void MantidTreeModel::showLogFileWindow() {}
void MantidTreeModel::showSampleMaterialWindow() {}
void MantidTreeModel::importWorkspace() {}

MantidMatrix *MantidTreeModel::importMatrixWorkspace(const Mantid::API::MatrixWorkspace_sptr workspace, int lower,
                                                     int upper, bool showDlg) {
  Q_UNUSED(workspace);
  Q_UNUSED(lower);
  Q_UNUSED(upper);
  Q_UNUSED(showDlg);
  return nullptr;
}

void MantidTreeModel::importWorkspace(const QString &wsName, bool showDlg, bool makeVisible) {
  Q_UNUSED(wsName);
  Q_UNUSED(showDlg);
  Q_UNUSED(makeVisible);
}

Table *MantidTreeModel::createDetectorTable(const QString &wsName, const std::vector<int> &indices, bool include_data) {
  Q_UNUSED(wsName);
  Q_UNUSED(indices);
  Q_UNUSED(include_data);
  return nullptr;
}

MultiLayer *MantidTreeModel::plot1D(const QMultiMap<QString, std::set<int>> &toPlot, bool spectrumPlot,
                                    MantidQt::DistributionFlag distr, bool errs, MultiLayer *plotWindow,
                                    bool clearWindow, bool waterfallPlot, const QString &log,
                                    const std::set<double> &customLogValues) {
  Q_UNUSED(toPlot);
  Q_UNUSED(spectrumPlot);
  Q_UNUSED(distr);
  Q_UNUSED(errs);
  Q_UNUSED(plotWindow);
  Q_UNUSED(clearWindow);
  Q_UNUSED(waterfallPlot);
  Q_UNUSED(log);
  Q_UNUSED(customLogValues);
  return nullptr;
}

void MantidTreeModel::drawColorFillPlots(const QStringList &wsNames, GraphOptions::CurveType curveType) {
  Q_UNUSED(wsNames);
  Q_UNUSED(curveType);
}

MultiLayer *MantidTreeModel::plotSubplots(const QMultiMap<QString, std::set<int>> &toPlot,
                                          MantidQt::DistributionFlag distr, bool errs, MultiLayer *plotWindow) {
  Q_UNUSED(toPlot);
  Q_UNUSED(distr);
  Q_UNUSED(errs);
  Q_UNUSED(plotWindow);

  return nullptr;
}

void MantidTreeModel::plotSurface(bool accepted, int plotIndex, const QString &axisName, const QString &logName,
                                  const std::set<double> &customLogValues, const QList<QString> &workspaceNames) {
  Q_UNUSED(accepted);
  Q_UNUSED(plotIndex);
  Q_UNUSED(axisName);
  Q_UNUSED(logName);
  Q_UNUSED(customLogValues);
  Q_UNUSED(workspaceNames);
}

void MantidTreeModel::plotContour(bool accepted, int plotIndex, const QString &axisName, const QString &logName,
                                  const std::set<double> &customLogValues, const QList<QString> &workspaceNames) {
  Q_UNUSED(accepted);
  Q_UNUSED(plotIndex);
  Q_UNUSED(axisName);
  Q_UNUSED(logName);
  Q_UNUSED(customLogValues);
  Q_UNUSED(workspaceNames);
}

MantidQt::MantidWidgets::MantidWSIndexDialog *
MantidTreeModel::createWorkspaceIndexDialog(int flags, const QStringList &wsNames, bool showWaterfall, bool showPlotAll,
                                            bool showTiledOpt, bool isAdvanced) {
  Q_UNUSED(flags);
  Q_UNUSED(wsNames);
  Q_UNUSED(showWaterfall);
  Q_UNUSED(showPlotAll);
  Q_UNUSED(showTiledOpt);
  Q_UNUSED(isAdvanced);
  // Return empty MantidWSIndexDialog
  return new MantidWSIndexDialog(nullptr, Qt::WindowFlags(), QList<QString>());
}
