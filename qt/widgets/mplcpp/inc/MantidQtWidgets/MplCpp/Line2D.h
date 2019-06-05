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
 * This type is designed to hold an existing Line2D instance and optionally
 * hold the raw data for the numpy arrays stored with the Line2D instance.
 * This object keeps the data alive if it has it.
 */
class MANTID_MPLCPP_DLL Line2D : public Artist {
public:
  // Ties together Line2D data
  struct Data {
    std::vector<double> xaxis, yaxis;
  };

public:
  explicit Line2D(Common::Python::Object obj);
  Line2D(Common::Python::Object obj, std::vector<double> &&xdataOwner,
         std::vector<double> &&ydataOwner);
  Line2D(Common::Python::Object obj, Line2D::Data &&dataOwner);
  ~Line2D() noexcept;
  Line2D(const Line2D &) = delete;
  Line2D &operator=(const Line2D &) = delete;
  Line2D(Line2D &&) = default;
  Line2D &operator=(Line2D &&);

  QColor getColor() const;

  const Data &rawData() const { return m_dataOwner; }
  void setData(std::vector<double> &&xdataOwner,
               std::vector<double> &&ydataOwner);
  void setData(Line2D::Data &&lineDataOwner);

private:
  Data m_dataOwner;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_LINE2D_H
