// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFInstrumentModel.h"
#include "DetectorTube.h"
#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <optional>
#include <string>
#include <vector>

#include <QWidget>

namespace MantidQt::CustomInterfaces {

class ALFInstrumentWidget;
class IALFInstrumentView;
class IALFAnalysisPresenter;

class MANTIDQT_DIRECT_DLL IALFInstrumentPresenter {

public:
  virtual QWidget *getSampleLoadWidget() = 0;
  virtual QWidget *getVanadiumLoadWidget() = 0;
  virtual ALFInstrumentWidget *getInstrumentView() = 0;

  virtual void subscribeAnalysisPresenter(IALFAnalysisPresenter *presenter) = 0;

  virtual void loadSettings() = 0;
  virtual void saveSettings() = 0;

  virtual void loadSample() = 0;
  virtual void loadVanadium() = 0;

  virtual void notifyInstrumentActorReset() = 0;
  virtual void notifyShapeChanged() = 0;
  virtual void notifyTubesSelected(std::vector<DetectorTube> const &tubes) = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentPresenter final : public IALFInstrumentPresenter {

public:
  ALFInstrumentPresenter(IALFInstrumentView *view, std::unique_ptr<IALFInstrumentModel> model);

  QWidget *getSampleLoadWidget() override;
  QWidget *getVanadiumLoadWidget() override;
  ALFInstrumentWidget *getInstrumentView() override;

  void subscribeAnalysisPresenter(IALFAnalysisPresenter *presenter) override;

  void loadSettings() override;
  void saveSettings() override;

  void loadSample() override;
  void loadVanadium() override;

  void notifyInstrumentActorReset() override;
  void notifyShapeChanged() override;
  void notifyTubesSelected(std::vector<DetectorTube> const &tubes) override;

private:
  Mantid::API::MatrixWorkspace_sptr loadAndNormalise(const std::string &pathToRun);
  void generateLoadedWorkspace();

  void updateInstrumentViewFromModel();
  void updateAnalysisViewFromModel();

  IALFAnalysisPresenter *m_analysisPresenter;

  IALFInstrumentView *m_view;
  std::unique_ptr<IALFInstrumentModel> m_model;
};

} // namespace MantidQt::CustomInterfaces
