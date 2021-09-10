// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <string>

namespace MantidQt {
namespace MantidWidgets {

/**
 * This struct stores the name of a global parameter which is shared across
 * ALL domains in a multi dataset fit. For example we might have two domains
 * with a FlatBackground in a composite function. If we want the value of
 * f0.f0.A0 and f1.f0.A0 to be shared (i.e. a global parameter), the parameter
 * name stored here is f0.A0 (i.e. without the domain index at the start).
 *
 * This definition was created because it is more explicit.
 */
struct EXPORT_OPT_MANTIDQT_COMMON GlobalParameter {

  GlobalParameter(std::string const &parameter);

  std::string m_parameter;
};

/**
 * This struct stores the data associated with a global tie. A global tie is
 * where a parameter of a specific domain is tied to the value of a parameter
 * in a different domain.
 */
struct EXPORT_OPT_MANTIDQT_COMMON GlobalTie {

  GlobalTie(std::string const &parameter, std::string const &tie);

  std::string toCompositeParameter(std ::string const &fullParameter) const;
  std::string toNonCompositeParameter(std ::string const &fullParameter) const;

  std::string asString() const;

  std::string m_parameter;
  std::string m_tie;
};

} // namespace MantidWidgets
} // namespace MantidQt
