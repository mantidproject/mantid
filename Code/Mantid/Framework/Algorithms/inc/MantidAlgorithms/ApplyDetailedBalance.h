#ifndef MANTID_ALGORITHMS_APPLYDETAILEDBALANCE_H_
#define MANTID_ALGORITHMS_APPLYDETAILEDBALANCE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Algorithms
{

  /** ApplyDetailedBalance : The algorithm calculates the imaginary part of the dynamic susceptibility chi''=Pi*(1-exp(-E/T))*S
    
    @author Andrei Savici, ORNL
    @date 2011-09-01

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport ApplyDetailedBalance  : public API::Algorithm
  {
  public:
    ApplyDetailedBalance();
    ~ApplyDetailedBalance();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ApplyDetailedBalance";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Transform scattering intensity to dynamic susceptibility.";}

    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Inelastic";}
    
  private:
    
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();


  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_APPLYDETAILEDBALANCE_H_ */
