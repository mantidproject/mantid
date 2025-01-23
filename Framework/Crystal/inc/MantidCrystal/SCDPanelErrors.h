// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidCrystal/DllConfig.h"
#include <cmath>

namespace Mantid {
namespace Crystal {
/*
@author Vickie Lynch, SNS
@date 7/25/2016
*/
class MANTID_CRYSTAL_DLL SCDPanelErrors : public API::ParamFunction, public API::IFunction1D {
public:
  /// Constructor
  SCDPanelErrors();

  /// overwrite IFunction base class methods
  std::string name() const override { return "SCDPanelErrors"; }
  const std::string category() const override { return "General"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  ///  function derivatives
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const IFunction::Attribute &value) override;

  /// Move detectors with parameters
  void moveDetector(double x, double y, double z, double rotx, double roty, double rotz, double scalex, double scaley,
                    std::string detname, const API::Workspace_sptr &inputW) const;

private:
  /// Call the appropriate load function
  void load(const std::string &fname);

  /// Load the points from a Workspace
  void loadWorkspace(const std::string &wsName) const;

  /// Load the points from a Workspace
  void loadWorkspace(std::shared_ptr<API::Workspace> ws) const;

  /// Clear all data
  void clear() const;

  /// Evaluate the function for a list of arguments and given scaling factor
  void eval(double xshift, double yshift, double zshift, double xrotate, double yrotate, double zrotate, double scalex,
            double scaley, double *out, const double *xValues, const size_t nData, double tShift) const;

  /// Fill in the workspace and bank names
  void setupData() const;

  /// The default value for the workspace index
  static const int defaultIndexValue;

  /// Temporary workspace holder
  mutable std::shared_ptr<API::Workspace> m_workspace;

  /// Stores bank
  mutable std::string m_bank;

  /// Flag of completing data setup
  mutable bool m_setupFinished;
};

} // namespace Crystal
} // namespace Mantid
