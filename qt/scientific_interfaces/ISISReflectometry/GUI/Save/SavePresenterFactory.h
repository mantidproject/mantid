// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "FileSaver.h"
#include "GUI/Common/IFileHandler.h"
#include "ISavePresenter.h"
#include "ISaveView.h"
#include "SaveAlgorithmRunner.h"
#include "SavePresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class SavePresenterFactory {
public:
  SavePresenterFactory(IFileHandler *fileHandler) : m_fileHandler(fileHandler) {}

  std::unique_ptr<ISavePresenter> make(ISaveView *view) {
    return std::make_unique<SavePresenter>(
        view, std::make_unique<FileSaver>(std::make_unique<SaveAlgorithmRunner>(), m_fileHandler));
  }

private:
  IFileHandler *m_fileHandler;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
