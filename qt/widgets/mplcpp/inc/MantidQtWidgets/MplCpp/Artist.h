// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_ARTIST_H
#define MPLCPP_ARTIST_H

#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Wraps a matplotlib.artist object with a C++ interface
 */
class MANTID_MPLCPP_DLL Artist : public Python::InstanceHolder {
public:
  // Holds a reference to the matplotlib artist object
  explicit Artist(Python::Object obj);

  void set(Python::Dict kwargs);
  void remove();
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_ARTIST_H
