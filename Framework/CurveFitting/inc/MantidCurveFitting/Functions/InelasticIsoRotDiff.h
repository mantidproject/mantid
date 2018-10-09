// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INELASTICISOROTDIFF_H_
#define MANTID_INELASTICISOROTDIFF_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
// Mantid headers from other projects
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"
// 3rd party library headers (N/A)
// standard library headers (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date 25/09/2016
*/

/**
 * @brief Inelastic part of the IsoRotDiff function.
 */
class DLLExport InelasticIsoRotDiff : public API::ParamFunction,
                                      public API::IFunction1D {
public:
  InelasticIsoRotDiff();

  /// overwrite IFunction base class methods
  void init() override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "InelasticIsoRotDiff"; }

  /// overwrite IFunction base class methods
  const std::string category() const override { return "QuasiElastic"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif // MANTID_INELASTICISOROTDIFF_H_
