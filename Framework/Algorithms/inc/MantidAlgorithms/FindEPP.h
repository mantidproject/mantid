// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_FINDEPP_H_
#define MANTID_ALGORITHMS_FINDEPP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Performs Gaussian fits over each spectrum to find the Elastic Peak
 Position (EPP).
*/
class MANTID_ALGORITHMS_DLL FindEPP : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void fitGaussian(int64_t);
  void initWorkspace();

  Mantid::API::MatrixWorkspace_sptr m_inWS;
  Mantid::API::ITableWorkspace_sptr m_outWS;
  std::unique_ptr<Mantid::API::Progress> m_progress;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FINDEPP_H_ */
