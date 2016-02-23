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

} // namespace DataObjects
} // namespace Mantid
