// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_CHANGEBINOFFSET_H_
#define MANTID_ALGORITHM_CHANGEBINOFFSET_H_

#include "MantidAlgorithms/SpectrumAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/**Takes a workspace and adjusts all the time bin values by the same amount.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Offset - The number by which to change the time bins by</LI>
</UL>

@author
@date 11/07/2008
*/
class DLLExport ChangeBinOffset : public SpectrumAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ChangeBinOffset"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Adjusts all the time bin values in a workspace by a specified "
           "amount.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ScaleX"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Axes"; }
  /// Algorithm's Alternate Name
  const std::string alias() const override { return "OffsetX"; }

private:
  /// Initialisation method. Declares properties to be used in algorithm.
  void init() override;
  /// Executes the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_CHANGEBINOFFSET_H_*/
