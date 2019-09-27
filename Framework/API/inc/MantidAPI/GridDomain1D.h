// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_GRIDDOMAIN1D_H_
#define MANTID_API_GRIDDOMAIN1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/GridDomain.h"
#include <stdexcept>
#include <string>

#include <stdexcept>
#include <string>

namespace Mantid {
namespace API {
/*Base class that represents a one dimensional grid domain,
  from which a function may take its arguments.

  @author Jose Borreguero
  @date Aug/28/2012
*/

class MANTID_API_DLL GridDomain1D : public API::GridDomain {
public:
  /// initialize
  void initialize(double &startX, double &endX, size_t &n, std::string scaling);
  /// number of grid point	s
  size_t size() const override { return m_points.size(); }
  /// number of dimensions in the grid
  size_t nDimensions() { return 1; }
  void reScale(const std::string &scaling);
  void SetScalingName(const std::string scaling);
  std::vector<double> &getPoints() { return m_points; }

private:
  std::string m_scaling;
  std::vector<double> m_points;

}; // class IGridDomain

/// typedef for a shared pointer
using GridDomain1D_sptr = boost::shared_ptr<GridDomain1D>;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_GRIDDOMAIN1D_H_*/
