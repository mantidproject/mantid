#ifndef MANTID_ALGORITHMS_IQTRANSFORM_H_
#define MANTID_ALGORITHMS_IQTRANSFORM_H_
/*WIKI* 


This algorithm is intended to take the output of a SANS reduction and apply a transformation to the data in an attempt to linearise the curve. Optionally, a background can be subtracted from the input data prior to transformation. This can be either a constant value, another workspace or both. Note that this expects a single spectrum input; if the input workspace contains multiple spectra, only the first will be transformed and appear in the output workspace.

A SANS reduction results in data in the form I(Q) vs Q, where Q is Momentum Transfer and I denotes intensity (the actual unit on the Y axis is 1/cm). These abbreviations are used in the descriptions of the transformations which follow. If the input is a histogram, the mid-point of the X (i.e. Q) bins will be taken. The output of this algorithm is always point data.

{| border="1" cellpadding="5" cellspacing="0"
!Transformation Name
!Y
!X
|-
| Guinier (spheres) || align="center"|<math>\ln (I)</math>            || align="center"|<math>Q^2</math>
|-
| Guinier (rods)    || align="center"|<math>\ln (IQ)</math>           || align="center"|<math>Q^2</math>
|-
| Guinier (sheets)  || align="center"|<math>\ln (IQ^2)</math>         || align="center"|<math>Q^2</math>
|-
| Zimm              || align="center"|<math>\frac{1}{I}</math>        || align="center"|<math>Q^2</math>
|-
| Debye-Bueche      || align="center"|<math>\frac{1}{\sqrt{I}}</math> || align="center"|<math>Q^2</math>
|-
| Holtzer           || align="center"|<math>I \times Q</math>         || align="center"|<math>Q</math>
|-
| Kratky            || align="center"|<math>I \times Q^2</math>       || align="center"|<math>Q</math>
|-
| Porod             || align="center"|<math>I \times Q^4</math>       || align="center"|<math>Q</math>
|-
| Log-Log           || align="center"|<math>\ln(I)</math>             || align="center"|<math>\ln(Q)</math>
|-
|General *
|<math>Q^{C_1} \times I^{C_2} \times \ln{\left( Q^{C_3} \times I^{C_4} \times C_5 \right)}</math>
|<math>Q^{C_6} \times I^{C_7} \times \ln{\left( Q^{C_8} \times I^{C_9} \times C_{10} \right)}</math>
|}
<nowiki>*</nowiki> The constants <math>C_1 - C_{10} </math> are, in subscript order, the ten constants passed to the GeneralFunctionConstants property.



*WIKI*/

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
