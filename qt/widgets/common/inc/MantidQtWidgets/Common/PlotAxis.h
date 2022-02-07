// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QString>
#include <string>

namespace MantidQt {
namespace API {

/**
  Deals with formatting a label for a plot axis for a given type of workspace
 */
class EXPORT_OPT_MANTIDQT_COMMON PlotAxis {
public:
  /// Constructor with workspace & axis index
  PlotAxis(const Mantid::API::IMDWorkspace &workspace, const size_t index);
  /// Constructor with an IMDDimension
  PlotAxis(const Mantid::Geometry::IMDDimension &dim);
  /// Constructor with just a workspace (reverse order to above so compiler
  /// doesn't convert a
  /// a bool to an size_t and call the wrong thing
  PlotAxis(const bool plottingDistribution, const Mantid::API::MatrixWorkspace &workspace);

  /// Disable default constructor
  PlotAxis() = delete;

  /// Create a new axis title
  QString title() const;

private:
  /// Creates a title suitable for an axis attached to the given index
  void titleFromIndex(const Mantid::API::IMDWorkspace &workspace, const size_t index);
  /// Creates a title suitable for an axis attached to the given dimension
  void titleFromDimension(const Mantid::Geometry::IMDDimension &dim);
  /// Creates a title suitable for the Y data values
  void titleFromYData(const Mantid::API::MatrixWorkspace &workspace, const bool plottingDistribution);

  /// Title
  QString m_title;
};

} // namespace API
} // namespace MantidQt
