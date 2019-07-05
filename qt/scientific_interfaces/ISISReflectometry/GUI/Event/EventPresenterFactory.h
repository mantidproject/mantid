// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
#include "Common/DllConfig.h"
#include "EventPresenter.h"
#include "IEventPresenter.h"
#include "IEventView.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class EventPresenterFactory {
public:
  std::unique_ptr<IEventPresenter> make(IEventView *view) {
    return std::make_unique<EventPresenter>(view);
  }
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
