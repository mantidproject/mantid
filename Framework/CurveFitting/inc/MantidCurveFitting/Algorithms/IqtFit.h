// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_IQTFITSEQUENTIAL_H_
#define MANTID_ALGORITHMS_IQTFITSEQUENTIAL_H_

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {


template <typename Base> class DLLExport IqtFit : public Base {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

protected:
  std::vector<API::MatrixWorkspace_sptr> getWorkspaces() const override;

private:
  double getStartX(std::size_t index) const;
  double getEndX(std::size_t index) const;

  std::map<std::string, std::string> validateInputs() override;
  bool isFitParameter(const std::string &name) const override;
  bool throwIfElasticQConversionFails() const override;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif
