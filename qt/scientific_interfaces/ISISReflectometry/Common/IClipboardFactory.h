// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IClipboard.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class DLLExport IClipboardFactory {
public:
  virtual ~IClipboardFactory(){};
  virtual IClipboard *createClipboard(
      boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
          subtrees,
      boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>>
          subtreeRoots) const = 0;
  virtual IClipboard *createClipboard() const = 0;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt