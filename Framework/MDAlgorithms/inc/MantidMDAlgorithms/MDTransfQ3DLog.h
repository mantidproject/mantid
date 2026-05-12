// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
//
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
//
namespace Mantid {
namespace MDAlgorithms {

/** Class responsible for conversion of input EventWorkspace
 * data into proper number of output dimensions for Q3D case
 * where the sample rotation changes throughout the scan.
 *
 * We inherit methods from the main MDTransfQ3D class - see that
 * class for more information.
 */

class MANTID_MDALGORITHMS_DLL MDTransfQ3DLog : public MDTransfQ3D {
public:
  const std::string transfID() const override;
  bool calcMatrixCoordTime(const double &deltaEOrK0, std::vector<coord_t> &Coord, const DateAndTime &pulsetime,
                           double &s, double &err) const override;
  void initialize(const MDWSDescription &ConvParams) override;
  bool usesPulseTime() const override { return true; }

private:
  Kernel::DblMatrix m_Wtransf;
  Kernel::V3D m_rotAxis;
  Kernel::TimeSeriesProperty<double> *m_rotLog;
  double m_angZero;
  /// how to transform workspace data in elastic case
  inline bool calcMatrixCoord3DElasticTime(const double &k0, std::vector<coord_t> &Coord, const DateAndTime &pulsetime,
                                           double &signal, double &errSq) const;
  /// how to transform workspace data in inelastic case
  inline bool calcMatrixCoord3DInelasticTime(const double &deltaE, std::vector<coord_t> &Coord,
                                             const DateAndTime &pulsetime) const;
};

} // namespace MDAlgorithms
} // namespace Mantid
