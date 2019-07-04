// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H

#include "GUI/Batch/IBatchPresenter.h"
#include "Reduction/Slicing.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IEventPresenter

IEventPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Event' presenter
*/

class IEventPresenter {
public:
  virtual ~IEventPresenter() = default;
  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual void reductionPaused() = 0;
  virtual void reductionResumed() = 0;
  virtual void autoreductionPaused() = 0;
  virtual void autoreductionResumed() = 0;
  virtual Slicing const &slicing() const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H */
