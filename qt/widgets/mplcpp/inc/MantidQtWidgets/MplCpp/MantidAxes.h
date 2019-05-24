// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_MANTID_AXES_H
#define MPLCPP_MANTID_AXES_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Axes.h"
#include "MantidQtWidgets/MplCpp/ErrorbarContainer.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * C++ wrapper around a matplotlib MantidAxes object instance
 */
class MANTID_MPLCPP_DLL MantidAxes : public Axes {
public:
  MantidAxes(Common::Python::Object pyObj);

  /// @name Plot creation functions
  ///@{
  Line2D plot(const Mantid::API::MatrixWorkspace_sptr &workspace,
              const size_t wkspIndex, const QString lineColour,
              const QString label);
  ErrorbarContainer errorbar(const Mantid::API::MatrixWorkspace_sptr &workspace,
                             const size_t wkspIndex, const QString lineColour,
                             const QString label);
  ///@}

  /// @name Artist removal/replacement
  ///@{
  void removeWorkspaceArtists(const Mantid::API::MatrixWorkspace_sptr &ws);
  void replaceWorkspaceArtists(const Mantid::API::MatrixWorkspace_sptr &newWS);
  ///@}
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_AXES_H
