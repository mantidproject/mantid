// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "RunPresenter.h"

#include <QWidget>

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IRunSubscriber;

class RunFactory {
public:
  std::unique_ptr<IRunPresenter> createPresenter(IRunSubscriber *subscriber, QWidget *parent) const;
};

} // namespace CustomInterfaces
} // namespace MantidQt