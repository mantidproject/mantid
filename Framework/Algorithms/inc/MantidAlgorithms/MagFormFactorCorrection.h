#ifndef MANTID_ALGORITHMS_MAGFORMFACTORCORRECTION_H_
#define MANTID_ALGORITHMS_MAGFORMFACTORCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** MagneticFormFactors scales the input workspace by the 1/F(Q) where F(Q)
    is the magnetic form factor for the given magnetic ion.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> MagneticIon     - The name of the magnetic ion (e.g. Fe2 for Fe2+)
   </LI>
    </UL>

    @author Manh Duc Le, STFC
    @date 08/09/2016

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MagFormFactorCorrection : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MagFormFactorCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "MagFormFactorCorrection corrects a workspace for the magnetic form "
           "factor F(Q) by dividing S(Q,w) by F(Q)^2.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SofQW"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MAGFORMFACTORCORRECTION_H_*/
