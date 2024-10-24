// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Batch/IBatchPresenter.h"
#include "Reduction/Slicing.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class IEventPresenter

IEventPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Event' presenter
*/

class IEventPresenter {
public:
  virtual ~IEventPresenter() = default;
  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual Slicing const &slicing() const = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
