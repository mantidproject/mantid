// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_
#define MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** EditInstrumentGeometry : TODO: DESCRIPTION

  @author
  @date 2011-08-22
*/
class DLLExport EditInstrumentGeometry : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Adds a new instrument or edits an existing one; "
           "currently works in an overwrite mode only.";
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  /// Validate the inputs that must be parallel
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EDITINSTRUMENTGEOMETRY_H_ */
