#ifndef MANTID_ALGORITHMS_IQTRANSFORM_H_
#define MANTID_ALGORITHMS_IQTRANSFORM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** This algorithm permits the linearisation of reduced SANS data by applying a chosen transformation
    to the input data. Optionally, a background can be subtracted from the data prior to transformation.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace, which must be a distribution in units of Q.</LI>
    <LI> OutputWorkspace - The name of the output workspace.</LI>
    <LI> TransformType   - The name of the transformation to be performed on the input workspace.</LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> BackgroundValue     - A constant value to be subtracted from the input workspace before transformation.</LI>
    <LI> BackgroundWorkspace - A workspace to subtract from the input workspace before transformation.</LI>
    <LI> GeneralFunctionConstants - For the 'General' transformation, the 10 constants to be used.</LI>
    </UL>

    @author Russell Taylor, Tessella
    @date 03/02/2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IQTransform : public API::Algorithm
{
public:
  IQTransform();
  virtual ~IQTransform();
  virtual const std::string name() const { return "IQTransform"; }
  virtual int version() const { return (1); }
  virtual const std::string category() const { return "SANS"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  void init();
  void exec();

  inline void subtractBackgroundValue(MantidVec& Y, const double value);
  inline API::MatrixWorkspace_sptr subtractBackgroundWS(API::MatrixWorkspace_sptr ws, API::MatrixWorkspace_sptr background);

  typedef void (IQTransform::*TransformFunc)(API::MatrixWorkspace_sptr);
  typedef std::map<std::string,TransformFunc> TransformMap;
  TransformMap m_transforms;   ///< A map of transformation name and function pointers

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
