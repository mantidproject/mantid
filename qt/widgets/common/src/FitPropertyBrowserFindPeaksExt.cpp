// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/FitPropertyBrowserFindPeaksExt.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"

#include <QEventLoop>
#include <QHash>
#include <QString>
#include <QStringList>

namespace MantidQt::MantidWidgets {

using namespace Mantid::API;

void FindPeakConvolveStrategy::initialise(const std::string &wsName, const int workspaceIndex,
                                          const std::string &peakListName, const int FWHM,
                                          AlgorithmFinishObserver *obs) {
  m_peakListName = peakListName;
  m_FWHM = FWHM;
  const double peakExtent{m_FWHM * (6.0 / 2.355)}; // Approx convert from FWHM to peak extent, assuming gaussian
  QHash<QString, QString> defaultValues;
  defaultValues["InputWorkspace"] = QString::fromStdString(wsName);
  defaultValues["OutputWorkspace"] = QString::fromStdString(m_peakListName);
  defaultValues["StartWorkspaceIndex"] = QString::number(workspaceIndex);
  defaultValues["EndWorkspaceIndex"] = QString::number(workspaceIndex);
  defaultValues["EstimatedPeakExtent"] = QString::number(peakExtent);
  const QStringList enabledParams{"EstimatedPeakExtent"};
  const QStringList disabledParams{"InputWorkspace", "OutputWorkspace", "StartWorkspaceIndex", "EndWorkspaceIndex"};
  API::InterfaceManager interfaceMgr;
  m_dlg = interfaceMgr.createDialogFromName("FindPeaksConvolve", -1, nullptr, false, defaultValues, QString(),
                                            enabledParams, disabledParams);
  m_dlg->setShowKeepOpen(false);
  m_dlg->disableExitButton();
  m_obs = obs;
  m_dlg->addAlgorithmObserver(m_obs);
}

void FindPeakConvolveStrategy::execute() {
  m_dlg->show();
  QEventLoop loop;
  connect(m_obs, SIGNAL(algCompletedSignal()), &loop, SLOT(quit()));
  loop.exec();

  Mantid::API::WorkspaceGroup_sptr groupWs = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(
      Mantid::API::AnalysisDataService::Instance().retrieve(m_peakListName));

  Mantid::API::ITableWorkspace_sptr peakCentreWs =
      std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(groupWs->getItem("PeakCentre"));
  m_peakCentres = std::make_unique<std::vector<double>>();
  m_peakCentres->reserve(peakCentreWs->columnCount());
  for (size_t i{1}; i < peakCentreWs->columnCount(); i++) {
    m_peakCentres->push_back(peakCentreWs->Double(0, i));
  }

  Mantid::API::ITableWorkspace_sptr peakHeightWs =
      std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(groupWs->getItem("PeakYPosition"));
  m_peakHeights = std::make_unique<std::vector<double>>();
  m_peakHeights->reserve(peakHeightWs->columnCount());
  for (size_t i{1}; i < peakHeightWs->columnCount(); i++) {
    m_peakHeights->push_back(peakHeightWs->Double(0, i));
  }
  m_peakWidths = std::make_unique<std::vector<double>>(m_peakCentres->size(), m_FWHM);
}

void FindPeakDefaultStrategy::execute() {
  m_alg->execute();
  Mantid::API::ITableWorkspace_sptr ws = std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(m_peakListName));

  m_peakCentres = std::make_unique<Mantid::API::ColumnVector<double>>(ws->getVector("centre"));
  m_peakWidths = std::make_unique<Mantid::API::ColumnVector<double>>(ws->getVector("width"));
  m_peakHeights = std::make_unique<Mantid::API::ColumnVector<double>>(ws->getVector("height"));
}

void FindPeakDefaultStrategy::initialise(const std::string &wsName, const int workspaceIndex,
                                         const std::string &peakListName, const int FWHM,
                                         AlgorithmFinishObserver *obs) {
  UNUSED_ARG(obs);
  m_peakListName = peakListName;
  const QString setting =
      QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("curvefitting.findPeaksTolerance"));
  const int Tolerance{setting.isEmpty() ? 4 : setting.toInt()};

  m_alg = Mantid::API::AlgorithmManager::Instance().create("FindPeaks");
  m_alg->initialize();
  m_alg->setPropertyValue("InputWorkspace", wsName);
  m_alg->setProperty("WorkspaceIndex", workspaceIndex);
  m_alg->setPropertyValue("PeaksList", m_peakListName);
  m_alg->setProperty("FWHM", FWHM);
  m_alg->setProperty("Tolerance", Tolerance);
}
} // namespace MantidQt::MantidWidgets
