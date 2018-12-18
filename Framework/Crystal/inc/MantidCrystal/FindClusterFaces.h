// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_FINDCLUSTERFACES_H_
#define MANTID_CRYSTAL_FINDCLUSTERFACES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** FindClusterFaces : Algorithm to find faces of clusters in an
  MDHistoWorkspace (image)
*/
class DLLExport FindClusterFaces : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find faces for clusters in a cluster image.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"IntegratePeaksUsingClusters"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_FINDCLUSTERFACES_H_ */
