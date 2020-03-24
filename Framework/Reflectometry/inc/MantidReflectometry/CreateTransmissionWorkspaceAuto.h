// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidReflectometry/DllConfig.h"
#include <boost/optional.hpp>

namespace Mantid {
namespace Reflectometry {

/** CreateTransmissionWorkspaceAuto : Creates a transmission run workspace in
Wavelength from input TOF workspaces.
*/
class MANTID_REFLECTOMETRY_DLL CreateTransmissionWorkspaceAuto
    : public API::DataProcessorAlgorithm {
public:
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override {
    return "CreateTransmissionWorkspaceAuto";
  }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry\\ISIS"; }
  /// Algorithm's summary for documentation
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  template <typename T>
  boost::optional<T> isSet(const std::string &propName) const;
};

} // namespace Reflectometry
} // namespace Mantid
