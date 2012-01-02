#ifndef MANTID_ALGORITHMS_VULCANREBIN_H_
#define MANTID_ALGORITHMS_VULCANREBIN_H_
/*WIKI*
TODO: Enter wiki description here.
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid
{
namespace Algorithms
{

  /** IDLRebin : TODO: DESCRIPTION
    
    @author
    @date 2011-10-06

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
  class DLLExport IDLRebin : public API::Algorithm, public API::DeprecatedAlgorithm
  {
  public:
    IDLRebin();
    ~IDLRebin();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "IDLRebin";}
    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return (1);}
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction;Transforms\\Rebin";}

  private:
    void init();
    void exec();

    void rebinIDL(const std::vector<double> tofs, const std::vector<double> events, std::vector<double>& rebindata);
    size_t locateEventInBin(double eventtof, std::vector<double> tofs);

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_VULCANREBIN_H_ */
