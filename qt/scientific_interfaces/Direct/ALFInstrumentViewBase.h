// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IALFInstrumentView.h"

#include <optional>
#include <string>

#include <QWidget>

namespace MantidQt {

namespace API {
class FileFinderWidget;
}

namespace CustomInterfaces {

class IALFInstrumentPresenter;

class MANTIDQT_DIRECT_DLL ALFInstrumentViewBase : public QWidget, public IALFInstrumentView {
  Q_OBJECT

public:
  explicit ALFInstrumentViewBase(QWidget *parent = nullptr);

  QWidget *generateSampleLoadWidget() override;
  QWidget *generateVanadiumLoadWidget() override;

  void subscribePresenter(IALFInstrumentPresenter *presenter) override;

  void loadSettings() override;
  void saveSettings() override;

  void disable(std::string const &reason) override;
  void enable() override;

  std::optional<std::string> getSampleFile() const override;
  std::optional<std::string> getVanadiumFile() const override;

  void setSampleRun(std::string const &runNumber) override;
  void setVanadiumRun(std::string const &runNumber) override;

  void displayWarning(std::string const &message) override;

private slots:
  void sampleLoaded();
  void vanadiumLoaded();
  void notifyInstrumentActorReset();
  void notifyShapeChanged();

protected:
  IALFInstrumentPresenter *m_presenter;

private:
  QString m_settingsGroup;
  API::FileFinderWidget *m_sample;
  API::FileFinderWidget *m_vanadium;
};

} // namespace CustomInterfaces
} // namespace MantidQt
