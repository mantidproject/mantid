// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SAVEPARAMETERFILE_H_
#define MANTID_ALGORITHMS_SAVEPARAMETERFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/System.h"
namespace Mantid {

namespace DataHandling {

/** SaveParameterFile : Save a workspace's parameters to an instrument parameter
  file.

  @author Harry Jeffery, ISIS, RAL
  @date 17/7/2014
*/
class DLLExport SaveParameterFile : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save an instrument's parameters to an instrument parameter file.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadParameterFile"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SAVEPARAMETERFILE_H_ */
