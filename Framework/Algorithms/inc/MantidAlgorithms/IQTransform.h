#ifndef MANTID_ALGORITHMS_IQTRANSFORM_H_
#define MANTID_ALGORITHMS_IQTRANSFORM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
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

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IQTransform : public API::Algorithm {
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

  inline API::MatrixWorkspace_sptr
  subtractBackgroundWS(API::MatrixWorkspace_sptr ws,
                       API::MatrixWorkspace_sptr background);

  using TransformFunc = void (IQTransform::*)(API::MatrixWorkspace_sptr);
  using TransformMap = std::map<std::string, TransformFunc>;
  TransformMap
      m_transforms; ///< A map of transformation name and function pointers

  boost::shared_ptr<Kernel::Units::Label> m_label;

  // A function for each transformation
  void guinierSpheres(API::MatrixWorkspace_sptr ws);
  void guinierRods(API::MatrixWorkspace_sptr ws);
  void guinierSheets(API::MatrixWorkspace_sptr ws);
  void zimm(API::MatrixWorkspace_sptr ws);
  void debyeBueche(API::MatrixWorkspace_sptr ws);
  void kratky(API::MatrixWorkspace_sptr ws);
  void porod(API::MatrixWorkspace_sptr ws);
  void holtzer(API::MatrixWorkspace_sptr ws);
  void logLog(API::MatrixWorkspace_sptr ws);
  void general(API::MatrixWorkspace_sptr ws);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_IQTRANSFORM_H_*/
