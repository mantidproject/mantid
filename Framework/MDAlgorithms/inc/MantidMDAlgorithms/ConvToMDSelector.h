// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/ConvToMDBase.h"

namespace Mantid {
namespace MDAlgorithms {
/** small class to select proper solver as function of the workspace kind and
  (possibly, in a future) other workspace parameters.
  * may be replaced by usual mantid factory in a future;
  *
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.
  *
  * @date 25-05-2012
*/

class DLLExport ConvToMDSelector {
public:
  enum ConverterType { DEFAULT, INDEXED };
  /**
   *
   * @param tp :: type of converter (indexed or default)
   */
  ConvToMDSelector(ConverterType tp = DEFAULT);
  /// function which selects the convertor depending on workspace type and
  /// (possibly, in a future) some workspace properties
  std::shared_ptr<ConvToMDBase> convSelector(const API::MatrixWorkspace_sptr &inputWS,
                                             std::shared_ptr<ConvToMDBase> &currentSolver) const;

private:
  ConverterType converterType;
};
} // namespace MDAlgorithms
} // namespace Mantid
