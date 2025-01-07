// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * PeakHKLErrors.h
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */

#pragma once

#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Crystal {

/**

  @author Ruth Mikkelson, SNS,ORNL
  @date 01/26/2013
 */
class MANTID_CRYSTAL_DLL PeakHKLErrors : public API::ParamFunction, public API::IFunction1D {
public:
  PeakHKLErrors();

  std::string name() const override { return std::string("PeakHKLErrors"); };

  virtual int version() const { return 1; };

  const std::string category() const override { return "Calibration"; };

  void function1D(double *out, const double *xValues, const size_t nData) const override;

  void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) override;

  void init() override;

  /**
   * Creates a new peak, matching the old peak except for a different
   *instrument.
   *
   * The Time of flightis the same except offset by T0. L0 should be the L0
   *for the new instrument.
   * It is added as a parameter in case the instrument will have the initial
   *flight path adjusted later.
   *  NOTE: the wavelength is changed.
   *
   * @param peak_old - The old peak
   * @param instrNew -The new instrument
   * @param T0 :
   * @param L0 :
   * @return The new peak with the new instrument( adjusted with the
   *parameters) and time adjusted.
   */
  static DataObjects::Peak createNewPeak(const DataObjects::Peak &peak_old, const Geometry::Instrument_sptr &instrNew,
                                         double T0, double L0);

  static void cLone(std::shared_ptr<Geometry::ParameterMap> &pmap,
                    const std::shared_ptr<const Geometry::IComponent> &component,
                    std::shared_ptr<const Geometry::ParameterMap> &pmapSv);

  void getRun2MatMap(DataObjects::PeaksWorkspace_sptr &Peaks, const std::string &OptRuns,
                     std::map<int, Mantid::Kernel::Matrix<double>> &Res) const;
  size_t nAttributes() const override { return (size_t)2; }

  static Kernel::Matrix<double> DerivRotationMatrixAboutRegAxis(double theta, char axis);

  static Kernel::Matrix<double> RotationMatrixAboutRegAxis(double theta, char axis);

  std::shared_ptr<Geometry::Instrument> getNewInstrument(const DataObjects::PeaksWorkspace_sptr &peaksWs) const;

  std::vector<std::string> getAttributeNames() const override { return {"OptRuns", "PeakWorkspaceName"}; }

  IFunction::Attribute getAttribute(const std::string &attName) const override {
    if (attName == "OptRuns")
      return IFunction::Attribute(OptRuns);

    if (attName == "PeakWorkspaceName")
      return IFunction::Attribute(PeakWorkspaceName);

    throw std::invalid_argument("Not a valid attribute name");
  }

  void setAttribute(const std::string &attName, const IFunction::Attribute &value) override {
    if (attName == "OptRuns") {
      OptRuns = value.asString();

      if (OptRuns.empty())
        return;

      if (OptRuns.at(0) != '/')
        OptRuns = "/" + OptRuns;

      if (OptRuns.at(OptRuns.size() - 1) != '/')
        OptRuns += "/";

      if (initMode == 1) {
        setUpOptRuns();
        initMode = 2;
      } else if (initMode == 2) // Woops cannot do twice
        throw std::invalid_argument("OptRuns can only be set once");

      return;
    }

    if (attName == "PeakWorkspaceName") {
      PeakWorkspaceName = value.asString();
      return;
    }

    throw std::invalid_argument("Not a valid attribute name");
  }

  bool hasAttribute(const std::string &attName) const override {
    if (attName == "OptRuns")
      return true;

    if (attName == "PeakWorkspaceName")
      return true;

    return false;
  }

private:
  std::string OptRuns;

  std::string PeakWorkspaceName;

  int initMode; // 0 not invoked, 1 invoked but no OptRuns , 2 invoked and
                // OptRuns setup

  void setUpOptRuns();

  mutable std::shared_ptr<Geometry::Instrument> instChange;
  mutable bool hasParameterMap = false;
  mutable Kernel::V3D sampPos;
  mutable std::shared_ptr<const Geometry::ParameterMap> pmapSv;
};
} // namespace Crystal
} // namespace Mantid
