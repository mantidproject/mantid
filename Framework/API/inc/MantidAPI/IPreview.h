// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MatrixWorkspace_fwd.h"

#include <string>

namespace Mantid {
namespace API {

/** IPreview : This is the abstract base class of the raw data previews.
 * Preview is a named basic operation that is commonly used for a given facility
 * and technique for visualizing the raw data. Preview also has a type, which
 * tells the client which kind of visualization is relevant for the returned
 * workspace.
 */
class MANTID_API_DLL IPreview {
public:
  enum class PreviewType { IVIEW = 0, PLOT1D = 1, PLOT2D = 2, SVIEW = 3 };
  virtual ~IPreview() = default;
  virtual PreviewType type() const = 0;
  virtual std::string technique() const = 0;
  virtual std::string facility() const = 0;
  virtual std::string name() const = 0;
  MatrixWorkspace_sptr view(MatrixWorkspace_sptr ws) const {
    return preview(ws);
  }

private:
  virtual MatrixWorkspace_sptr preview(MatrixWorkspace_sptr ws) const {
    return ws;
  }
};
using IPreview_uptr = std::unique_ptr<IPreview>;
} // namespace API
} // namespace Mantid
