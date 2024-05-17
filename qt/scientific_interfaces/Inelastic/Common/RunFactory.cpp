// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunFactory.h"

#include "IRunSubscriber.h"
#include "RunPresenter.h"
#include "RunView.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

std::unique_ptr<IRunPresenter> RunFactory::createPresenter(IRunSubscriber *subscriber, QWidget *parent) const {
  return std::make_unique<RunPresenter>(subscriber, new RunView(parent));
}

} // namespace CustomInterfaces
} // namespace MantidQt