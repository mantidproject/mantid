// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** @class ReflCommandBase

ReflCommandBase is an interface which defines the functions any data processor
action needs to support. Defines a IReflTablePresenter that will be notified.
*/
class CommandBase : public Command {
public:
  CommandBase(DataProcessorPresenter *tablePresenter) : m_presenter(tablePresenter) {
    if (!tablePresenter) {
      throw std::invalid_argument("Invalid abstract presenter");
    }
  };
  CommandBase(const QDataProcessorWidget &widget) : CommandBase(widget.getPresenter()) {}

protected:
  DataProcessorPresenter *const m_presenter;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt