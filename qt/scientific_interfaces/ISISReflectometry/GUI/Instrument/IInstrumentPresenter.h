// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Instrument.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IBatchPresenter;

/** @class IInstrumentPresenter

IInstrumentPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Instrument' presenter
*/
class IInstrumentPresenter {
public:
  virtual ~IInstrumentPresenter() = default;

  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual Instrument const &instrument() const = 0;
  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual void notifyInstrumentChanged(std::string const &instrumentName) = 0;
  virtual void restoreDefaults() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
