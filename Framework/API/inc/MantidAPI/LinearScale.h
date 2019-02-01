// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_LINEARSCALE_H_
#define MANTID_API_LINEARSCALE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ITransformScale.h"

namespace Mantid {
namespace API {
/*Base class  representing a linear scaling transformation acting on a
  one-dimensional grid domain

  @author Jose Borreguero
  @date Aug/28/2012
*/

class MANTID_API_DLL LinearScale : public API::ITransformScale {
public:
  /// The scaling transformation. First and last elements of the grid remain
  /// unchanged
  const std::string name() const override { return "LinearScale"; }
  void transform(std::vector<double> &gd) override;
}; // class LinearScale

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_LINEARSCALE_H_*/
