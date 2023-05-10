// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFAlgorithmManager.h"
#include "ALFData.h"
#include "ALFInstrumentModel.h"
#include "DetectorTube.h"
#include "DllConfig.h"
#include "IALFAlgorithmManagerSubscriber.h"
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

class MANTIDQT_DIRECT_DLL ALFInstrumentPresenter final : public IALFInstrumentPresenter,
                                                         public IALFAlgorithmManagerSubscriber {

public:
  ALFInstrumentPresenter(IALFInstrumentView *view, std::unique_ptr<IALFInstrumentModel> model,
                         std::unique_ptr<IALFAlgorithmManager> algorithmManager);

  QWidget *getSampleLoadWidget() override;
  QWidget *getVanadiumLoadWidget() override;
  ALFInstrumentWidget *getInstrumentView() override;

  void subscribeAnalysisPresenter(IALFAnalysisPresenter *presenter) override;

  void loadSettings() override;
  void saveSettings() override;

  void loadSample() override;
  void loadVanadium() override;

  void notifyAlgorithmError(std::string const &message) override;
  void notifyLoadComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void notifyNormaliseByCurrentComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void notifyRebinToWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void notifyDivideComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void notifyReplaceSpecialValuesComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void notifyConvertUnitsComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;

  void notifyCreateWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void notifyScaleXComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void notifyRebunchComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;

  void notifyInstrumentActorReset() override;
  void notifyShapeChanged() override;
  void notifyTubesSelected(std::vector<DetectorTube> const &tubes) override;

private:
  void generateLoadedWorkspace();
  void normaliseSampleByVanadium();
  void convertSampleToDSpacing(Mantid::API::MatrixWorkspace_sptr const &workspace);

  std::optional<std::string> getFileFromView() const;

  void loadAndNormalise();

  void updateRunInViewFromModel();

  void updateInstrumentViewFromModel();
  void updateAnalysisViewFromModel();

  ALFData m_dataSwitch;

  IALFAnalysisPresenter *m_analysisPresenter;

  IALFInstrumentView *m_view;
  std::unique_ptr<IALFInstrumentModel> m_model;
  std::unique_ptr<IALFAlgorithmManager> m_algorithmManager;
};

} // namespace MantidQt::CustomInterfaces
