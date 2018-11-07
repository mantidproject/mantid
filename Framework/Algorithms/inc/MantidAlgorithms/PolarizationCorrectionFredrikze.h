// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_H_
#define MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class WorkspaceGroup;
class MatrixWorkspace;
} // namespace API
namespace Algorithms {

/** PolarizationCorrectionFredrikze : Algorithm to perform polarisation
 corrections on
 multi-period group workspaces that implements the Fredrikze (Dutch) method.
 Fredrikze, H, et al. "Calibration of a polarized neutron reflectometer" Physica
 B 297 (2001)
 */
class DLLExport PolarizationCorrectionFredrikze : public API::Algorithm {
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
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  getEfficiencyWorkspace(const std::string &label);
  boost::shared_ptr<Mantid::API::WorkspaceGroup>
  execPA(boost::shared_ptr<Mantid::API::WorkspaceGroup> inWS);
  boost::shared_ptr<Mantid::API::WorkspaceGroup>
  execPNR(boost::shared_ptr<Mantid::API::WorkspaceGroup> inWS);
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  add(boost::shared_ptr<Mantid::API::MatrixWorkspace> &lhsWS,
      const double &rhs);
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  multiply(boost::shared_ptr<Mantid::API::MatrixWorkspace> &lhsWS,
           const double &rhs);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POLARIZATIONCORRECTIONFREDRIKZE_H_ */
