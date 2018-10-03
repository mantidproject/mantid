// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATEEPP_H_
#define MANTID_ALGORITHMS_CREATEEPP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** CreateEPP : Calculates the nominal elastic time-of flights for
  each spectrum and creates an elastic peak position table (EPP)
  like the one returned by the FindEPP algorithm.
*/
class MANTID_ALGORITHMS_DLL CreateEPP : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"FindEPP"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs(void) override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATEEPP_H_ */
