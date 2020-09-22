// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../ISISReflectometry/Common/IClipboardFactory.h"
#include "../../ISISReflectometry/Common/Clipboard.h"
#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL ClipboardFactory
    : public IClipboardFactory {
public:
  IClipboard *createClipboard(
      boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
          subtrees,
      boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>>
          subtreeRoots) const override {
    return new Clipboard(subtrees, subtreeRoots);
  }
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
