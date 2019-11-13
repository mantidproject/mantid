// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_Q3D_TRANSF_H
#define MANTID_MDALGORITHMS_Q3D_TRANSF_H
//
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"
#include "MantidMDAlgorithms/MDTransfModQ.h"
//
namespace Mantid {
namespace MDAlgorithms {

/** Class responsible for conversion of input workspace
  * data into proper number of output dimensions for Q3D case
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.
  *
  * Currently contains Elastic and Inelastic transformations
  *
  * Some methods here are the same as in ModQ case, so the class difectly
  inherigs from ModQ to utilize this.
  *
  * @date 31-05-2012
*/

class DLLExport MDTransfQ3D : public MDTransfModQ {
public:
  /// the name, this ChildAlgorithm is known to users (will appear in selection
  /// list)
  const std::string transfID() const override;
  bool calcYDepCoordinates(std::vector<coord_t> &Coord, size_t i) override;
  bool calcMatrixCoord(const double &deltaEOrK0, std::vector<coord_t> &Coord,
                       double &s, double &err) const override;
  // constructor;
  MDTransfQ3D();
  /* clone method allowing to provide the copy of the particular class */
  MDTransfInterface *clone() const override { return new MDTransfQ3D(*this); }
  //
  void initialize(const MDWSDescription &ConvParams) override;

  std::vector<double> getExtremumPoints(const double xMin, const double xMax,
                                        size_t det_num) const override;

  // WARNING!!!! THESE METHODS ARE USED BEFORE INITIALIZE IS EXECUTED SO THEY
  // CAN NOT RELY ON THE CONTENTS OF THE CLASS (THEY ARE VIRTUAL STATIC METHODS)
  /** return the number of dimensions, calculated by the transformation from the
     workspace.
     Depending on EMode, this numebr here is either 3 or 4 and do not depend on
     input workspace*/
  unsigned int
  getNMatrixDimensions(Kernel::DeltaEMode::Type mode,
                       API::MatrixWorkspace_const_sptr inWS =
                           API::MatrixWorkspace_const_sptr()) const override;
  /**function returns units ID-s which this transformation prodiuces its ouptut.
     It is Momentum and Momentum and DelteE in inelastic modes */
  std::vector<std::string>
  outputUnitID(Kernel::DeltaEMode::Type dEmode,
               API::MatrixWorkspace_const_sptr inWS =
                   API::MatrixWorkspace_const_sptr()) const override;
  /**the default dimID-s in Q3D mode are Q1,Q2,Q3 and dE if necessary */
  std::vector<std::string>
  getDefaultDimID(Kernel::DeltaEMode::Type dEmode,
                  API::MatrixWorkspace_const_sptr inWS =
                      API::MatrixWorkspace_const_sptr()) const override;

protected:
  // the variable which verifies if Lorentz corrections have to be calculated in
  // Elastic mode;
  bool m_isLorentzCorrected;
  // pointer to the array of precalculated sin^2(Theta) values for all
  // detectors, used if Lorentz corrections calculations are requested
  double const *m_SinThetaSqArray;
  // the vector containing precaluclated sin^2(theta) values
  std::vector<double> SinThetaSq;
  // current value of Sin(Theta)^2 corresponding to the current detector value
  // and used to calculate Lorentz corrections
  double m_SinThetaSq;
  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string convention;
  // all other variables are the same as in ModQ
  // hole near origin of Q
  double m_AbsMin;

private:
  /// how to transform workspace data in elastic case
  inline bool calcMatrixCoord3DElastic(const double &k0,
                                       std::vector<coord_t> &Coord,
                                       double &signal, double &errSq) const;
  /// how to transform workspace data in inelastic case
  inline bool calcMatrixCoord3DInelastic(const double &deltaE,
                                         std::vector<coord_t> &Coord) const;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif
