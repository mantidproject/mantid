// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "AsciiSaver.h"
#include "Common/DllConfig.h"
#include "ISavePresenter.h"
#include "ISaveView.h"
#include "SavePresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class SavePresenterFactory {
public:
  std::unique_ptr<ISavePresenter> make(ISaveView *view) {
    return std::make_unique<SavePresenter>(view, std::make_unique<AsciiSaver>());
  }
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt