#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace DataObjects {

/// Construct from ISpectrum.
Histogram1D::Histogram1D(const ISpectrum &other) : ISpectrum(other) {
  dataY() = other.readY();
  dataE() = other.readE();
}

/// Assignment from ISpectrum.
Histogram1D &Histogram1D::operator=(const ISpectrum &rhs) {
  ISpectrum::operator=(rhs);
  dataY() = rhs.readY();
  dataE() = rhs.readE();
  return *this;
}

void Histogram1D::clearData() {
  MantidVec &yValues = this->dataY();
  std::fill(yValues.begin(), yValues.end(), 0.0);
  MantidVec &eValues = this->dataE();
  std::fill(eValues.begin(), eValues.end(), 0.0);
}

} // namespace DataObjects
} // namespace Mantid
