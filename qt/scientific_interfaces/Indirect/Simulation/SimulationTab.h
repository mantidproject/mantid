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

#include <QSettings>
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

  void setOutputPlotOptionsPresenter(std::unique_ptr<OutputPlotOptionsPresenter> presenter);
  void setOutputPlotOptionsWorkspaces(std::vector<std::string> const &outputWorkspaces);
  void clearOutputPlotOptionsWorkspaces();

  virtual void loadSettings(const QSettings &settings) = 0;

private:
  std::unique_ptr<OutputPlotOptionsPresenter> m_plotOptionsPresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
