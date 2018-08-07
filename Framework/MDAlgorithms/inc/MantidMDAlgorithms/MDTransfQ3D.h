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

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File change history is stored at:
  <https://github.com/mantidproject/mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport MDTransfQ3D : public MDTransfModQ {
public:
  /// the name, this ChildAlgorithm is known to users (will appear in selection
  /// list)
  const std::string transfID() const override; // {return "Q3D"; }
  bool calcYDepCoordinates(std::vector<coord_t> &Coord, size_t i) override;
  bool calcMatrixCoord(const double &x, std::vector<coord_t> &Coord, double &s,
                       double &err) const override;
  // constructor;
  MDTransfQ3D();
  /* clone method allowing to provide the copy of the particular class */
  MDTransfInterface *clone() const override { return new MDTransfQ3D(*this); }
  //
  void initialize(const MDWSDescription &ConvParams) override;

  /** */
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
  inline bool calcMatrixCoord3DInelastic(const double &E_tr,
                                         std::vector<coord_t> &Coord) const;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif
