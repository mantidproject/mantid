// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Clipboard.h"
#include "DllConfig.h"
#include "IClipboardFactory.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class DLLExport ClipboardFactory : public IClipboardFactory {
public:
  IClipboard *createClipboard(
      boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
          subtrees,
      boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>>
          subtreeRoots) const {
    return new Clipboard(subtrees, subtreeRoots);
  }
  IClipboard *createClipboard() const override { return new Clipboard(); }
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
