// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLSAVEPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLSAVEPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IReflSaveTabPresenter.h"
#include "IReflSaveTabView.h"
#include "ReflAsciiSaver.h"
#include "ReflSaveTabPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class SavePresenterFactory {
public:
  std::unique_ptr<IReflSaveTabPresenter> make(IReflSaveTabView *view) {
    return Mantid::Kernel::make_unique<ReflSaveTabPresenter>(
        view, Mantid::Kernel::make_unique<ReflAsciiSaver>());
  }
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_REFLSETTINGSPRESENTERFACTORY_H
