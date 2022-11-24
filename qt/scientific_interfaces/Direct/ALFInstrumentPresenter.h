// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFInstrumentModel.h"
#include "DllConfig.h"
#include "MantidGeometry/IDetector.h"

#include <optional>
#include <string>
#include <utility>

#include <QWidget>

namespace MantidQt {

namespace MantidWidgets {
class InstrumentWidget;
} // namespace MantidWidgets

namespace CustomInterfaces {

class IALFInstrumentView;
class IALFAnalysisPresenter;

class MANTIDQT_DIRECT_DLL IALFInstrumentPresenter {

public:
  virtual QWidget *getLoadWidget() = 0;
  virtual MantidWidgets::InstrumentWidget *getInstrumentView() = 0;

  virtual void subscribeAnalysisPresenter(IALFAnalysisPresenter *presenter) = 0;

  virtual void loadRunNumber() = 0;

  virtual void extractSingleTube(Mantid::Geometry::IDetector_const_sptr detector) = 0;
  virtual void averageTube(Mantid::Geometry::IDetector_const_sptr detector) = 0;

  virtual bool checkDataIsExtracted() const = 0;

  virtual std::string extractedWsName() const = 0;
};

class MANTIDQT_DIRECT_DLL ALFInstrumentPresenter final : public IALFInstrumentPresenter {

public:
  ALFInstrumentPresenter(IALFInstrumentView *view, std::unique_ptr<IALFInstrumentModel> model);

  QWidget *getLoadWidget() override;
  MantidWidgets::InstrumentWidget *getInstrumentView() override;

  void subscribeAnalysisPresenter(IALFAnalysisPresenter *presenter) override;

  void loadRunNumber() override;

  void extractSingleTube(Mantid::Geometry::IDetector_const_sptr detector) override;
  void averageTube(Mantid::Geometry::IDetector_const_sptr detector) override;

  bool checkDataIsExtracted() const override;

  std::string extractedWsName() const override;

private:
  std::optional<std::string> loadAndTransform(const std::string &run);

  IALFAnalysisPresenter *m_analysisPresenter;

  IALFInstrumentView *m_view;
  std::unique_ptr<IALFInstrumentModel> m_model;
};

} // namespace CustomInterfaces
} // namespace MantidQt
