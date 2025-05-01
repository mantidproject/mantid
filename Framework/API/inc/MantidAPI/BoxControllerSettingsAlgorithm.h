// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace API {

/** An abstract algorithm sub-class for algorithms that
 * define properties for BoxController settings.
 *
 * This will be inherited by other algorithms as required.

  @author Janik Zikovsky
  @date 2011-11-02
*/
class DLLExport BoxControllerSettingsAlgorithm : public API::Algorithm {
public:
protected:
  /// Initialise the properties
  void initBoxControllerProps(const std::string &SplitInto = "5", int SplitThreshold = 1000, int MaxRecursionDepth = 5);

  /// Set the settings in the given box controller
  void setBoxController(const Mantid::API::BoxController_sptr &bc,
                        const Mantid::Geometry::Instrument_const_sptr &instrument);

  /// Set the settings in the given box controller
  void setBoxController(const Mantid::API::BoxController_sptr &bc);

  std::string getBoxSettingsGroupName() { return "Box Splitting Settings"; }
  /// Take the defaults for the box splitting from the instrument parameters.
  void takeDefaultsFromInstrument(const Mantid::Geometry::Instrument_const_sptr &instrument, const size_t ndims);

private:
};

} // namespace API
} // namespace Mantid
