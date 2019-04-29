// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_GENERATEGROUPINGPOWDER_H_
#define MANTID_DATAHANDLING_GENERATEGROUPINGPOWDER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** GenerateGroupingPowder : Generate grouping file and par file, for powder
  scattering.

  @date 2012-07-16
*/
class DLLExport GenerateGroupingPowder : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Generate grouping by angles.";
  }

  int version() const override;
  const std::string category() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadDetectorsGroupingFile", "GroupDetectors"};
  }

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_GENERATEGROUPINGPOWDER_H_ */
