// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/NumericAxis.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"

namespace Mantid {
namespace MDAlgorithms {

/** Class responsible for conversion of input workspace
  * data into proper number of output dimensions in NoQ case, when the data from
  a ws are just copied to MD WS.
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.

  *
  * @date 16-05-2012
*/

class DLLExport MDTransfNoQ : public MDTransfInterface {
public:
  /// the name, this ChildAlgorithm is known to users (will appear in selection
  /// list)
  const std::string transfID() const override; // {return "NoQ"; }
  // calc target coordinates interface:
  bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd) override;
  bool calcYDepCoordinates(std::vector<coord_t> &Coord, size_t i) override;
  bool calcMatrixCoord(const double &X, std::vector<coord_t> &Coord, double &s, double &err) const override;
  // constructor;
  MDTransfNoQ();
  /* clone method allowing to provide the copy of the particular class */
  MDTransfInterface *clone() const override { return new MDTransfNoQ(*this); }
  // initializes the contents of the class
  void initialize(const MDWSDescription &ConvParams) override;

  /**This transformation dos nothing with the workspace ranges, so extremum
   * points for this transformation coordinates are the
   * coordinates themselves */
  std::vector<double> getExtremumPoints(const double xMin, const double xMax, size_t det_num) const override;

  // WARNING!!!! THESE METHODS ARE USED BEFORE INITIALIZE IS EXECUTED SO THEY
  // CAN NOT RELY ON THE CONTENTS OF THE CLASS TO BE DEFINED (THEY ARE VIRTUAL
  // STATIC METHODS)
  //***** output WS definition interface:
  /** return the number of dimensions, calculated by the transformation from the
     workspace.
      Depending on ws axis units, the numebr here is either 1 or 2* and is
     independent on emode*/
  unsigned int getNMatrixDimensions(Kernel::DeltaEMode::Type mode, API::MatrixWorkspace_const_sptr inWS) const override;
  /**function returns units ID-s which this transformation prodiuces its ouptut.
     here it is usually input ws units, which are independent on emode */
  std::vector<std::string> outputUnitID(Kernel::DeltaEMode::Type mode,
                                        API::MatrixWorkspace_const_sptr inWS) const override;
  std::vector<std::string> getDefaultDimID(Kernel::DeltaEMode::Type mode,
                                           API::MatrixWorkspace_const_sptr inWS) const override;
  const std::string inputUnitID(Kernel::DeltaEMode::Type mode, API::MatrixWorkspace_const_sptr inWS) const override;

  void setDisplayNormalization(Mantid::API::IMDWorkspace_sptr mdWorkspace,
                               Mantid::API::MatrixWorkspace_sptr underlyingWorkspace) const override;

private:
  unsigned int m_NMatrixDim;
  // the variables used for exchange data between different specific parts of
  // the generic ND algorithm:
  // pointer to Y axis of MD workspace
  API::NumericAxis *m_YAxis;

  // pointer to the class, which contains the information about precprocessed
  // detectors (fake in this case)
  Kernel::V3D const *m_Det;
  // min and max values for this conversion
  std::vector<double> m_DimMin, m_DimMax;
  /** the vector of the additional coordinates which define additional MD
     dimensions.
      For implemented NoQ case, these dimensions do not depend on matrix
     coordinates and are determined by the WS properties */
  std::vector<coord_t> m_AddDimCoordinates;

private:
  // internal helper function which extract one or two axis from input matrix
  // workspace;
  static void getAxes(const API::MatrixWorkspace_const_sptr &inWS, API::NumericAxis *&pXAxis,
                      API::NumericAxis *&pYAxis);
};

} // namespace MDAlgorithms
} // namespace Mantid
