// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/Functions/CubicSpline.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {
/** Convolution of two workspaces
 */

class MANTID_CURVEFITTING_DLL ConvolveWorkspaces : public API::Algorithm {
public:
  ConvolveWorkspaces() : API::Algorithm() {}
  ~ConvolveWorkspaces() override {}
  /// Algorithm's name
  const std::string name() const override { return "ConvolveWorkspaces"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Convolution of two workspaces."; }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void convolve(MantidVec &xValues, const MantidVec &Y1, const MantidVec &Y2, MantidVec &out) const;
  std::unique_ptr<API::Progress> m_progress = nullptr;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
