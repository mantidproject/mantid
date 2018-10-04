#include "MantidQtWidgets/MplCpp/Line2D.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Contruct a wrapper around an existing matplotlib.lines.Line2D instance
 * This object owns the data that is part of the Line2D instance. It assumes the
 * Line2D contains numpy arrays that are simple a view on to the existing data
 * that is part of the given Y and X arrays.
 * @param obj An existing Line2D instance
 * @param xdataOwner The source data for X. It is moved into this object
 * @param ydataOwner The source data for Y. It is moved into this object
 */
Line2D::Line2D(Python::Object obj, std::vector<double> xdataOwner,
               std::vector<double> ydataOwner)
    : Artist(obj), m_xOwner(std::move(xdataOwner)),
      m_yOwner(std::move(ydataOwner)) {}

/**
 * The data is being deleted so the the line is removed from the axes
 * if it is present
 */
Line2D::~Line2D() {
  try {
    this->remove();
  } catch (Python::ErrorAlreadySet &) {
    // line is not attached to an axes
  }
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
