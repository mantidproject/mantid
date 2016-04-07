#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace DataObjects {

void Histogram1D::clearData() {
  MantidVec &yValues = this->dataY();
  std::fill(yValues.begin(), yValues.end(), 0.0);
  MantidVec &eValues = this->dataE();
  std::fill(eValues.begin(), eValues.end(), 0.0);
}

/// Sets the x data.
/// @param X :: vector of X data
void Histogram1D::setX(const MantidVec &X) { refX.access() = X; }

/// Sets the x data.
/// @param X :: vector of X data
void Histogram1D::setX(const MantidVecPtr &X) { refX = X; }

/// Sets the x data
/// @param X :: vector of X data
void Histogram1D::setX(const MantidVecPtr::ptr_type &X) { refX = X; }

/// Returns the x data
MantidVec &Histogram1D::dataX() { return refX.access(); }

/// Returns the x data const
const MantidVec &Histogram1D::dataX() const { return *refX; }

/// Returns the x data const
const MantidVec &Histogram1D::readX() const { return *refX; }

/// Returns a pointer to the x data
MantidVecPtr Histogram1D::ptrX() const { return refX; }

} // namespace DataObjects
} // namespace Mantid
