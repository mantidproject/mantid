// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DetectorTube.h"
#include "DllConfig.h"

#include <optional>
#include <string>
#include <vector>

class QWidget;

namespace MantidQt {

namespace MantidWidgets {
class IInstrumentActor;
}

namespace CustomInterfaces {

class IALFInstrumentPresenter;

class MANTIDQT_DIRECT_DLL IALFInstrumentView {

public:
  virtual void setUpInstrument(std::string const &fileName) = 0;

  virtual QWidget *generateSampleLoadWidget() = 0;
  virtual QWidget *generateVanadiumLoadWidget() = 0;
  virtual QWidget *getInstrumentView() = 0;

  virtual void subscribePresenter(IALFInstrumentPresenter *presenter) = 0;

  virtual void loadSettings() = 0;
  virtual void saveSettings() = 0;

  virtual void disable(std::string const &reason) = 0;
  virtual void enable() = 0;

  virtual std::optional<std::string> getSampleFile() const = 0;
  virtual std::optional<std::string> getVanadiumFile() const = 0;

  virtual void setSampleRun(std::string const &runNumber) = 0;
  virtual void setVanadiumRun(std::string const &runNumber) = 0;

  virtual MantidWidgets::IInstrumentActor const &getInstrumentActor() const = 0;

  virtual std::vector<DetectorTube> getSelectedDetectors() const = 0;

  virtual void clearShapes() = 0;
  virtual void drawRectanglesAbove(std::vector<DetectorTube> const &tubes) = 0;

  virtual void displayWarning(std::string const &message) = 0;

  virtual ~IALFInstrumentView() = default;
};

} // namespace CustomInterfaces
} // namespace MantidQt
