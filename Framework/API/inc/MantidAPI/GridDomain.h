// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_GRIDDOMAIN_H_
#define MANTID_API_GRIDDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"

namespace Mantid {
namespace API {
/*Base class that represents the grid domain from which a function may take its
  arguments.
  Grids are multidimensional objects, grids are a composition of grids.

  @author Jose Borreguero
  @date Aug/28/2012
*/

class MANTID_API_DLL GridDomain : public API::FunctionDomain {
public:
  /// number of grid points
  size_t size() const override;
  /// number of dimensions in the grid
  size_t nDimensions();
  /// get the grid at specified index
  boost::shared_ptr<GridDomain> getGrid(size_t index);
  /// re-scale all grids
  void reScale(const std::string &scaling);

private:
  /// composition of grids
  std::vector<boost::shared_ptr<GridDomain>> m_grids;

}; // class IGridDomain

/// typedef for a shared pointer
using GridDomain_sptr = boost::shared_ptr<GridDomain>;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_GRIDDOMAIN_H_*/
