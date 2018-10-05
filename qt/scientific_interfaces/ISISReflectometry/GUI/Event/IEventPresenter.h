// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H

#include "../../IReflBatchPresenter.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IEventPresenter

IReflEventPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Event' presenter
*/

enum class SliceType { None, UniformEven, Uniform, Custom, LogValue };

class IEventPresenter {
public:
  virtual ~IEventPresenter() = default;
  virtual void acceptMainPresenter(IReflBatchPresenter *mainPresenter) = 0;
  virtual void onReductionPaused() = 0;
  virtual void onReductionResumed() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H */
