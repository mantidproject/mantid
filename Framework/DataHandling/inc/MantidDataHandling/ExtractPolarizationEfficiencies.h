// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_EXTRACTPOLARIZATIONEFFICIENCIES_H_
#define MANTID_DATAHANDLING_EXTRACTPOLARIZATIONEFFICIENCIES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {
/** Looks for a parameter in the parameter file that stores polarization
   efficiencies to be used with the polarization correction algorithm
   (PolarizationEfficienciesCor).
*/
class DLLExport ExtractPolarizationEfficiencies : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"PolarizationEfficiencyCor"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_EXTRACTPOLARIZATIONEFFICIENCIES_H_ */
