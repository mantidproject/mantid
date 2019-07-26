// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IBATCHPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_IBATCHPRESENTERFACTORY_H

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
class IBatchPresenter;
class IBatchView;

class IBatchPresenterFactory {
public:
  virtual ~IBatchPresenterFactory() = default;
  virtual std::unique_ptr<IBatchPresenter> make(IBatchView *view) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_IBATCHPRESENTERFACTORY_H
