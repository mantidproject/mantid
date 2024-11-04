// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../Reduction/Instrument.h"
#include "Common/DllConfig.h"
#include "IInstrumentPresenter.h"
#include "IInstrumentView.h"
#include "InstrumentPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class InstrumentPresenterFactory {
public:
  InstrumentPresenterFactory(IFileHandler *fileHandler, IReflMessageHandler *messageHandler)
      : m_fileHandler(fileHandler), m_messageHandler(messageHandler) {}

  std::unique_ptr<IInstrumentPresenter> make(IInstrumentView *view) {
    return std::make_unique<InstrumentPresenter>(view, Instrument(), m_fileHandler, m_messageHandler);
  }

private:
  IFileHandler *m_fileHandler;
  IReflMessageHandler *m_messageHandler;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
