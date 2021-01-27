// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Crystal {

/** SCDCalibratePanels2ObjFunc : TODO: DESCRIPTION
 */
class MANTID_CRYSTAL_DLL SCDCalibratePanels2ObjFunc : public API::ParamFunction,
                                                      public API::IFunction1D {
public:
  /// Constructor for init input vector
  SCDCalibratePanels2ObjFunc();

  /// overwrite based class name
  std::string name() const override { return "SCDCalibratePanels2ObjFunc"; }

  /// set category
  const std::string category() const override { return "General"; }

  /// base objective function
  void function1D(double *out, const double *xValues,
                  const size_t order) const override;

private:
  /// temp workspace holder
  mutable std::shared_ptr<API::Workspace> m_ws;
  mutable std::string m_cmpt;

  const bool LOGCHILDALG{false};
  const Mantid::Kernel::V3D UNSET_HKL{0, 0, 0};
  const double PI{3.1415926535897932384626433832795028841971693993751058209};

  /// helper functions
  void moveInstruentComponentBy(double deltaX, double deltaY, double deltaZ,
                                std::string componentName,
                                const API::Workspace_sptr &ws) const;

  void rotateInstrumentComponentBy(double rotVx, double rotVy, double rotVz,
                                   double rotAng, std::string componentName,
                                   const API::Workspace_sptr &ws) const;
};

} // namespace Crystal
} // namespace Mantid
