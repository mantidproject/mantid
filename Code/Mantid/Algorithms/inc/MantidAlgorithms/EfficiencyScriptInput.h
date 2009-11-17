#ifndef MANTID_ALGORITHM_EFFICIENCYSCRIPTINPUT_H_
#define MANTID_ALGORITHM_EFFICIENCYSCRIPTINPUT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{
  using DataObjects::Workspace2D_const_sptr;
  using namespace API;
  using namespace Geometry;
/**????

    @author Steve Williams based on code by T.G.Perring
    @date 6/10/2009

    Copyright &copy; 2008-9 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/              
class DLLExport EfficiencyScriptInput : public API::Algorithm
{
public:
  EfficiencyScriptInput(){}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "EfficiencyScriptInput"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const{return "CorrectionFunctions";}
private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
  
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_EFFICIENCYSCRIPTINPUT_H_*/
