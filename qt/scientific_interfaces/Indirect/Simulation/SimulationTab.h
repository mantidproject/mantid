// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidQtWidgets/Spectroscopy/InelasticTab.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/System.h"

#include <MantidQtWidgets/Common/FileFinderWidget.h>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
/**
        This class defines a abstract base class for the different tabs of the
   Indirect Simulation interface.
        Any joint functionality shared between each of the tabs should be
   implemented here as well as defining
        shared member functions.

        @author Samuel Jackson, STFC
*/

class MANTIDQT_INDIRECT_DLL SimulationTab : public InelasticTab {
  Q_OBJECT

public:
  SimulationTab(QWidget *parent = nullptr);
  ~SimulationTab() override;

  void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces);
  void clearOutputPlotOptionsWorkspaces();

  void enableLoadHistoryProperty(bool doLoadHistory);
  /**
   * Legacy restore operation. It queries the supplied const settings and
   * updates tab widgets, but must not write or synchronize persistent storage.
   * See @ref settings_lifecycle.
   */
  virtual void restoreSettings(const API::FileFinderSettings &settings) = 0;

private:
  virtual void setLoadHistory(bool doLoadHistory) { UNUSED_ARG(doLoadHistory); }
};
} // namespace CustomInterfaces
} // namespace MantidQt
