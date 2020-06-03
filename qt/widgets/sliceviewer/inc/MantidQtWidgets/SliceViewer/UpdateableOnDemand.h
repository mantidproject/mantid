// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include <memory>

namespace Mantid {
namespace API {
class IPeaksWorkspace;
}
} // namespace Mantid

namespace MantidQt {
namespace SliceViewer {
/**
 * Abstract class for types that can be forced to update themselves upon
 * request.
 */
class DLLExport UpdateableOnDemand {
public:
  // Force the implementation to update itself
  virtual void performUpdate() = 0;
  // Deliver a new peaks workspace for replacement of an existing one.
  virtual void updatePeaksWorkspace(
      const std::string &toName,
      std::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace) = 0;
  // Destructor
  virtual ~UpdateableOnDemand() {}
};
} // namespace SliceViewer
} // namespace MantidQt
