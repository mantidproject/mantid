// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class IBatchPresenter;
class IBatchView;

class IBatchPresenterFactory {
public:
  virtual ~IBatchPresenterFactory() = default;
  virtual std::unique_ptr<IBatchPresenter> make(IBatchView *view) = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
