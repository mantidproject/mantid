// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxControllerSettingsAlgorithm.h"
#include "MantidMDAlgorithms/ConvToMDBase.h"
#include "MantidMDAlgorithms/MDWSDescription.h"

#include "MantidKernel/DeltaEMode.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToMDParent :
   *  Main part of two algorithms which use ConvertToMD factory to transform
   points from instrument space to physical MD space
   *
   * The description of the algorithm is available at:
   <http://www.mantidproject.org/ConvertToMD>
   * The detailed description of the algorithm is provided at:
   <http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation>

   * @date 11-10-2011
*/

/// Convert to MD Events class itself:
class DLLExport ConvertToMDParent : public API::BoxControllerSettingsAlgorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override = 0;
  /// Algorithm's version for identification
  int version() const override = 0;
  /// Algorithm's category for identification
  const std::string category() const override;

protected:
  void init() override;
  //
  DataObjects::TableWorkspace_const_sptr preprocessDetectorsPositions(
      const Mantid::API::MatrixWorkspace_const_sptr &InWS2D,
      const std::string &dEModeRequested, bool updateMasks,
      const std::string &OutWSName);
  DataObjects::TableWorkspace_sptr runPreprocessDetectorsToMDChildUpdatingMasks(
      const Mantid::API::MatrixWorkspace_const_sptr &InWS2D,
      const std::string &OutWSName, const std::string &dEModeRequested,
      Kernel::DeltaEMode::Type &Emode);

  /// logger -> to provide logging, for MD dataset file operations
  static Mantid::Kernel::Logger &g_Log;

  /// pointer to the class, which does the particular conversion
  std::shared_ptr<MDAlgorithms::ConvToMDBase> m_Convertor;

  /// Template to check if a variable equal to NaN
  template <class T> inline bool isNaN(T val) {
    volatile T buf = val;
    return (val != buf);
  }
};

} // namespace MDAlgorithms
} // namespace Mantid
