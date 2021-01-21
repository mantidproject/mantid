// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {
//---------------------------------------------------------------------------

class MANTID_CURVEFITTING_DLL EstimatePeakErrors : public API::Algorithm {
public:
  EstimatePeakErrors();

  const std::string name() const override;
  const std::string summary() const override;

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"Fit", "EstimateFitParameters"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
