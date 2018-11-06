// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEPOLYNOMIALBACKGROUND_H_
#define MANTID_ALGORITHMS_CALCULATEPOLYNOMIALBACKGROUND_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** CalculatePolynomialBackground : This algorithm fits a polynomial
  background to an input workspace and returns the evaluated background as
  the output workspace.
*/
class MANTID_ALGORITHMS_DLL CalculatePolynomialBackground
    : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"RemoveBackground", "CreateUserDefinedBackground"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALCULATEPOLYNOMIALBACKGROUND_H_ */
