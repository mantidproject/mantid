// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ILatticeFunction.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics.h>

namespace Mantid {
namespace Crystal {
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS
 @date 02/06/2012
 */
class MANTID_CRYSTAL_DLL OptimizeLatticeForCellType : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "OptimizeLatticeForCellType"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Optimize lattice parameters for cell type."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"FindUBUsingFFT", "FindUBUsingIndexedPeaks", "FindUBUsingLatticeParameters"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Crystal\\Cell"; }

  API::ILatticeFunction_sptr getLatticeFunction(const std::string &cellType, const Geometry::UnitCell &cell) const;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
