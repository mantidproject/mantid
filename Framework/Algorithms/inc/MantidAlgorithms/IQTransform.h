// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Kernel {
namespace Units {
class Label;
}
} // namespace Kernel

namespace Algorithms {
/** This algorithm permits the linearisation of reduced SANS data by applying a
   chosen transformation
    to the input data. Optionally, a background can be subtracted from the data
   prior to transformation.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace, which must be a
   distribution in units of Q.</LI>
    <LI> OutputWorkspace - The name of the output workspace.</LI>
    <LI> TransformType   - The name of the transformation to be performed on the
   input workspace.</LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> BackgroundValue     - A constant value to be subtracted from the input
   workspace before transformation.</LI>
    <LI> BackgroundWorkspace - A workspace to subtract from the input workspace
   before transformation.</LI>
    <LI> GeneralFunctionConstants - For the 'General' transformation, the 10
   constants to be used.</LI>
    </UL>

    @author Russell Taylor, Tessella
    @date 03/02/2011
*/
class MANTID_ALGORITHMS_DLL IQTransform : public API::Algorithm {
public:
  IQTransform();
  const std::string name() const override { return "IQTransform"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm provides various functions that are sometimes used "
           "to linearise the output of a 'SANS' data reduction prior to "
           "fitting it.";
  }

  int version() const override { return (1); }
  const std::string category() const override { return "SANS"; }

private:
  void init() override;
  void exec() override;

  inline API::MatrixWorkspace_sptr subtractBackgroundWS(const API::MatrixWorkspace_sptr &ws,
                                                        const API::MatrixWorkspace_sptr &background);

  using TransformFunc = void (IQTransform::*)(const API::MatrixWorkspace_sptr &);
  using TransformMap = std::map<std::string, TransformFunc>;
  TransformMap m_transforms; ///< A map of transformation name and function pointers

  std::shared_ptr<Kernel::Units::Label> m_label;

  // A function for each transformation
  void guinierSpheres(const API::MatrixWorkspace_sptr &ws);
  void guinierRods(const API::MatrixWorkspace_sptr &ws);
  void guinierSheets(const API::MatrixWorkspace_sptr &ws);
  void zimm(const API::MatrixWorkspace_sptr &ws);
  void debyeBueche(const API::MatrixWorkspace_sptr &ws);
  void kratky(const API::MatrixWorkspace_sptr &ws);
  void porod(const API::MatrixWorkspace_sptr &ws);
  void holtzer(const API::MatrixWorkspace_sptr &ws);
  void logLog(const API::MatrixWorkspace_sptr &ws);
  void general(const API::MatrixWorkspace_sptr &ws);
};

} // namespace Algorithms
} // namespace Mantid
