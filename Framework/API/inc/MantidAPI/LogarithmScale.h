// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ITransformScale.h"

namespace Mantid {
namespace API {
/*Base class  representing a logarithm scaling transformation acting on a
  one-dimensional grid domain

  @author Jose Borreguero
  @date Aug/28/2012
*/

class MANTID_API_DLL LogarithmScale : public ITransformScale {
public:
  LogarithmScale() : m_base(M_E) {};
  const std::string name() const override { return "LogarithmScale"; }
  void transform(std::vector<double> &gd) override;
  void setBase(const double base);
  /// The scaling transformation. First and last elements of the grid remain
  /// unchanged

private:
  double m_base; // base of the logarithm

}; // class LogarithmScale

} // namespace API
} // namespace Mantid
