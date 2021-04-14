// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidSINQ/DllConfig.h"

namespace Mantid {
namespace Poldi {

/** IPoldiFunction1D :

    This is an auxilliary interface that has to be implemented by
    functions that are supposed to be used for POLDI fits.

    This way the calculation of a theoretical diffractogram can be
    performed by the corresponding functions, which may behave differently
    depending on their nature. For examples see the following classes:

        PoldiSpectrumConstantBackground
        PoldiSpectrumDomainFunction
        Poldi2DFunction

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 07/01/2015
  */
class MANTID_SINQ_DLL IPoldiFunction1D {
public:
  virtual ~IPoldiFunction1D() = default;

  virtual void poldiFunction1D(const std::vector<int> &indices, const API::FunctionDomain1D &domain,
                               API::FunctionValues &values) const = 0;
};

} // namespace Poldi
} // namespace Mantid
