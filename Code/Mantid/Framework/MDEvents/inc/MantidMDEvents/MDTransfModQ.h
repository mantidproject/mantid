#ifndef MANTID_MDEVENTS_MODQ_TRANSF_H
#define MANTID_MDEVENTS_MODQ_TRANSF_H
//
#include "MantidMDEvents/MDTransfInterface.h"
#include "MantidMDEvents/MDTransfFactory.h"
//
namespace Mantid {
namespace MDEvents {

/** Class responsible for conversion of input workspace
  * data into proper number of output dimensions for ModQ case
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.
  *
  * Currently contains Elastic and Inelastic transformations
  *
  * This particular file defines  specializations of generic coordinate
  transformation to the ModQ case
  *
  * @date 16-05-2012

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

class DLLExport MDTransfModQ : public MDTransfInterface {
public:
  /// the name, this ChildAlgorithm is known to users (will appear in selection
  /// list)
  const std::string transfID() const; // {return "ModQ"; }
  /** energy conversion modes supported by this class;
    * The class supports three standard energy conversion modes */
  std::vector<std::string> getEmodes() const;

  bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd);
  bool calcYDepCoordinates(std::vector<coord_t> &Coord, size_t i);
  bool calcMatrixCoord(const double &k0, std::vector<coord_t> &Coord, double &s,
                       double &err) const;
  // constructor;
  MDTransfModQ();
  /* clone method allowing to provide the copy of the particular class */
  MDTransfInterface *clone() const { return new MDTransfModQ(*this); }
  //
  void initialize(const MDWSDescription &ConvParams);

  virtual std::vector<double>
  getExtremumPoints(const double xMin, const double xMax, size_t det_num) const;

  // WARNING!!!! THESE METHODS ARE USED BEFORE INITIALIZE IS EXECUTED SO THEY
  // CAN NOT RELY ON THE CONTENTS OF THE CLASS TO BE DEFINED (THEY ARE VIRTUAL
  // STATIC METHODS)
  /** return the number of dimensions, calculated by the transformation from the
     workspace.
     Depending on EMode, this numebr here is either 1 or 2 and do not depend on
     input workspace*/
  unsigned int
  getNMatrixDimensions(Kernel::DeltaEMode::Type mode,
                       API::MatrixWorkspace_const_sptr Sptr =
                           API::MatrixWorkspace_const_sptr()) const;
  /**function returns units ID-s which this transformation prodiuces its ouptut.
     It is Momentum and Momentum and DelteE in inelastic modes */
  std::vector<std::string>
  outputUnitID(Kernel::DeltaEMode::Type dEmode,
               API::MatrixWorkspace_const_sptr Sptr =
                   API::MatrixWorkspace_const_sptr()) const;
  /**the default dimID-s in ModQ mode are |Q| and dE if necessary */
  std::vector<std::string>
  getDefaultDimID(Kernel::DeltaEMode::Type dEmode,
                  API::MatrixWorkspace_const_sptr Sptr =
                      API::MatrixWorkspace_const_sptr()) const;
  /**  returns the units, the transformation expects for input workspace to be
   * expressed in. */
  const std::string inputUnitID(Kernel::DeltaEMode::Type dEmode,
                                API::MatrixWorkspace_const_sptr Sptr =
                                    API::MatrixWorkspace_const_sptr()) const;

protected:
  //  directions to the detectors
  double m_ex, m_ey, m_ez;
  // the matrix which transforms the neutron momentums from laboratory to
  // crystal coordinate system.
  std::vector<double> m_RotMat;
  // min-max values, some modified to work with squared values:
  std::vector<double> m_DimMin, m_DimMax;
  // pointer to the array of V3D unit vectors, directed from the sample to the
  // positions, where preprocessed detectors are.
  Kernel::V3D const *m_DetDirecton;

  // number of dimensions, calculated from a matrix workspace, which is one in
  // elastic and two in inelastic mode here.
  unsigned int m_NMatrixDim;
  // the variable which describes current conversion mode:
  Kernel::DeltaEMode::Type m_Emode;
  /** the vector of the additional coordinates which define additional MD
     dimensions.
      For implemented ModQ case, these dimensions do not depend on matrix
     coordinates and are determined by WS properties */
  std::vector<coord_t> m_AddDimCoordinates;
  //************* The variables below are relevant to inelastic modes only and
  // will be used in inelastic cases:
  // the wavevector of incident neutrons
  double m_Ki;
  // the energy of the incident neutrons
  double m_Ei;
  // pointer to detector specific input energy (eFixed) defined for indirect
  // instruments; This one is relewant to indirect instruments only;
  float *m_pEfixedArray;
  // pointer to the array which contains detectors masks. TODO: It is here
  // untill Mantid masks spectra by 0 instead of NaNs, when switched to NaN-s --
  // remove
  int *m_pDetMasks;

private:
  /// how to transform workspace data in elastic case
  inline bool calcMatrixCoordElastic(const double &k0,
                                     std::vector<coord_t> &Coored) const;
  /// how to transform workspace data in inelastic case
  inline bool calcMatrixCoordInelastic(const double &DeltaE,
                                       std::vector<coord_t> &Coored) const;
};

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
