// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>

namespace Mantid {
namespace Algorithms {

/** CreateTransmissionWorkspaceAuto : Creates a transmission run workspace in
Wavelength from input TOF workspaces.
*/
class DLLExport CreateTransmissionWorkspaceAuto
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

  template <typename T> boost::optional<T> isSet(std::string propName) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO_H_ */
