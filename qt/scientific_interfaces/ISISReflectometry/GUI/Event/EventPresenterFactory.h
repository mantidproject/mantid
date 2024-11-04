// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "EventPresenter.h"
#include "IEventPresenter.h"
#include "IEventView.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class EventPresenterFactory {
public:
  std::unique_ptr<IEventPresenter> make(IEventView *view) { return std::make_unique<EventPresenter>(view); }
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
