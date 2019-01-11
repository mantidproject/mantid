// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_REPLICATEMD_H_
#define MANTID_MDALGORITHMS_REPLICATEMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/DllConfig.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}
namespace DataObjects {
class MDHistoWorkspace;
}
namespace MDAlgorithms {

/** ReplicateMD : Algorithm header for ReplicateMD. An algorithm to create a
  higher dimensionality MDWorkspace from a
  lower dimensionality one.
*/
class MANTID_MDALGORITHMS_DLL ReplicateMD : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateMDWorkspace", "MergeMD"};
  }
  const std::string category() const override;
  const std::string summary() const override;
  /// Valdiate the algorithm inputs
  std::map<std::string, std::string> validateInputs() override;

private:
  boost::shared_ptr<const Mantid::DataObjects::MDHistoWorkspace> transposeMD(
      boost::shared_ptr<Mantid::DataObjects::MDHistoWorkspace> &toTranspose,
      const std::vector<int> &axes);
  boost::shared_ptr<Mantid::DataObjects::MDHistoWorkspace>
  getDataWorkspace() const;
  boost::shared_ptr<Mantid::DataObjects::MDHistoWorkspace>
  getShapeWorkspace() const;
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_REPLICATEMD_H_ */
