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
  InstrumentPresenterFactory(IFileHandler *fileHandler) : m_fileHandler(fileHandler) {}

  std::unique_ptr<IInstrumentPresenter> make(IInstrumentView *view) {
    return std::make_unique<InstrumentPresenter>(view, Instrument(), m_fileHandler);
  }

private:
  IFileHandler *m_fileHandler;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt