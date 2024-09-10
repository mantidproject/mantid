// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Axes.h"
#include "MantidQtWidgets/MplCpp/ErrorbarContainer.h"

#include <boost/none_t.hpp>
#include <optional>

#include <QHash>
#include <QVariant>

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
  Line2D plot(const Mantid::API::MatrixWorkspace_sptr &workspace, const size_t wkspIndex, const QString &lineColour,
              const QString &label, const std::optional<QHash<QString, QVariant>> &otherKwargs = std::nullopt);
  ErrorbarContainer errorbar(const Mantid::API::MatrixWorkspace_sptr &workspace, const size_t wkspIndex,
                             const QString &lineColour, const QString &label,
                             const std::optional<QHash<QString, QVariant>> &otherKwargs = std::nullopt);
  void pcolormesh(const Mantid::API::MatrixWorkspace_sptr &workspace,
                  const std::optional<QHash<QString, QVariant>> &otherKwargs = std::nullopt);
  ///@}

  /// @name Artist removal/replacement
  ///@{
  bool removeWorkspaceArtists(const Mantid::API::MatrixWorkspace_sptr &ws);
  bool replaceWorkspaceArtists(const Mantid::API::MatrixWorkspace_sptr &newWS);
  ///@}
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
