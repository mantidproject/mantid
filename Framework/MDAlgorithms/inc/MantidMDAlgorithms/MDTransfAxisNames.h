// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {
/** The class defines default axis names/dimension ID-s for MD workspace which
  can be obtained/used
  * in the MD transformations converting matrix workspace to Q or Q-dE space
  *
  * @date 29-05-2012
*/

/** describes default dimensions ID currently used by multidimensional workspace
 *
 *  DimensionID is the short name which used to retrieve this dimesnion from MD
 *workspace.
 *  The names themself are defined in constructor  */
namespace CnvrtToMD {
enum defaultDimID {
  ModQ_ID,   //< the defauld |Q| id for mod Q or powder mode
  Q1_ID,     //< 1 of 3 dimID in Q3D mode
  Q2_ID,     //< 2 of 3 dimID in Q3D mode
  Q3_ID,     //< 3 of 3 dimID in Q3D mode
  dE_ID,     //< energy transfer ID
  nDefaultID //< ID conunter
};
}

class MANTID_MDALGORITHMS_DLL MDTransfAxisNames {
public:
  /// function returns default dimension id-s for different Q and dE modes,
  /// defined by this class
  std::vector<std::string> getDefaultDimIDQ3D(Kernel::DeltaEMode::Type dEMode) const;
  std::vector<std::string> getDefaultDimIDModQ(Kernel::DeltaEMode::Type dEMode) const;

  // constructor
  MDTransfAxisNames();

private:
  /// the vector describes default dimension names, specified along the axis if
  /// no names are explicitly requested;
  std::vector<std::string> m_DefaultDimID;
};
/** function to build mslice-like axis name from the vector, which describes
 * crystallographic direction along this axis*/
std::string DLLExport makeAxisName(const Kernel::V3D &Dir, const std::vector<std::string> &QNames);
/**creates string representation of the number with accuracy, cpecified by eps*/
std::string DLLExport sprintfd(const double data, const double eps);

} // namespace MDAlgorithms
} // namespace Mantid
