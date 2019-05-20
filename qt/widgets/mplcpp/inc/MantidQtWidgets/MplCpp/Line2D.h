// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_LINE2D_H
#define MPLCPP_LINE2D_H

#include "MantidQtWidgets/MplCpp/Artist.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include <QColor>
#include <vector>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Line2D holds an instance of a matplotlib Line2D type.
 * This type is designed to hold an existing Line2D instance that contains
 * data in numpy arrays that do not own their data but have a view on to an
 * existing vector of data. This object keeps the data alive.
 */
class MANTID_MPLCPP_DLL Line2D : public Artist {
public:
  Line2D(Common::Python::Object obj, std::vector<double> xdataOwner,
         std::vector<double> ydataOwner);
  ~Line2D() noexcept;
  // not copyable
  Line2D(const Line2D &) = delete;
  Line2D &operator=(const Line2D &) = delete;
  // movable
  Line2D(Line2D &&) = default;
  Line2D &operator=(Line2D &&) = default;

  QColor getColor() const;

private:
  // Containers that own the data making up the line
  std::vector<double> m_xOwner, m_yOwner;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_LINE2D_H
