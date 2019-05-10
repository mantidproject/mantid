// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Line2D.h"
#include "MantidPythonInterface/core/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidQtWidgets/MplCpp/ColorConverter.h"

#include <QColor>

using Mantid::PythonInterface::Converters::VectorToNDArray;
using Mantid::PythonInterface::Converters::WrapReadOnly;
using Mantid::PythonInterface::GlobalInterpreterLock;
using MantidQt::Widgets::MplCpp::ColorConverter;
using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Contruct a wrapper around an existing matplotlib Line2D instance.
 * Usally Axes.plot is used to create one of these so that an the Line2D
 * object already contains the data. This object assumes the Line2D
 * instance contains a view on to the data in the 2 vectors passed here so
 * this object becomes the owner of this data. The Line2D s detached from on
 * a canvas on destruction of this object as the data will be destroyed.
 * Only accepts r-value references to signal ownership transfer.
 * @param obj An existing Line2D instance
 * @param xdataOwner The source data for X. It is moved into this object
 * @param ydataOwner The source data for Y. It is moved into this object
 */
Line2D::Line2D(Python::Object obj, std::vector<double> &&xdataOwner,
               std::vector<double> &&ydataOwner)
    : Artist(std::move(obj)), m_dataOwner{std::move(xdataOwner),
                                          std::move(ydataOwner)} {
  assert(!m_dataOwner.xaxis.empty());
  assert(!m_dataOwner.yaxis.empty());
}

/**
 * @brief Contruct a wrapper around an existing matplotlib.lines.Line2D instance
 * This object owns the data that is part of the Line2D instance. It assumes the
 * Line2D contains numpy arrays that are simple a view on to the existing data
 * that is part of the given Y and X arrays.
 * Only accepts r-value references to signal ownership transfer
 * @param obj  An existing Line2D instance
 * @param dataOwner The source data for X,Y. It is moved into this object
 */
Line2D::Line2D(Python::Object obj, Line2D::Data &&dataOwner)
    : Artist(std::move(obj)), m_dataOwner(std::move(dataOwner)) {
  assert(!m_dataOwner.xaxis.empty());
  assert(!m_dataOwner.yaxis.empty());
}

/**
 * The data is being deleted so the the line is removed from the axes
 * if it is present
 */
Line2D::~Line2D() noexcept {
  // If the Line2D has not been gutted by a std::move() then
  // detach the line
  if (!m_dataOwner.xaxis.empty()) {
    try {
      this->remove();
    } catch (...) {
      // line is not attached to an axes
    }
  }
}

/**
 * Move assign from another Line2D instance
 * @param rhs The temporary object to assign from
 * @return A reference to this object
 */
Line2D &Line2D::operator=(Line2D &&rhs) {
  // This objects old data is being destroyed so the
  // line is removed.
  this->remove();
  m_dataOwner = std::move(rhs.m_dataOwner);
  return *this;
}

/**
 * @return A QColor defining the color of the artist
 */
QColor Line2D::getColor() const {
  GlobalInterpreterLock lock;
  return ColorConverter::toRGB(pyobj().attr("get_color")());
}

/**
 * Set new data for the line. Only accepts r-value references to signal
 * ownership transfer
 * @param xdataOwner A vector of new data for X. This container owns the data
 * for the line.
 * @param ydataOwner A vector of new data for Y. This container owns the data
 * for the line.
 */
void Line2D::setData(std::vector<double> &&xdataOwner,
                     std::vector<double> &&ydataOwner) {
  setData(Line2D::Data{std::move(xdataOwner), std::move(ydataOwner)});
}

/**
 * Set new data for the line. Overload taking a single Data object
 * Only accepts r-value references to signal ownership transfer
 * @param lineDataOwner A Data struct of new data for X/Y.
 */
void Line2D::setData(Line2D::Data &&lineDataOwner) {
  assert(!lineDataOwner.xaxis.empty());
  assert(!lineDataOwner.yaxis.empty());

  GlobalInterpreterLock lock;
  // Wrap the vector data in a numpy facade to avoid a copy.
  // The vector still owns the data and is kept alive by this object
  VectorToNDArray<double, WrapReadOnly> wrapNDArray;
  Python::Object xarray{Python::NewRef(wrapNDArray(lineDataOwner.xaxis))},
      yarray{Python::NewRef(wrapNDArray(lineDataOwner.yaxis))};
  pyobj().attr("set_data")(xarray, yarray);
  m_dataOwner = std::move(lineDataOwner);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
