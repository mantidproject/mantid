#ifndef MANTID_ALGORITHMS_COPYSAMPLE_H_
#define MANTID_ALGORITHMS_COPYSAMPLE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/Sample.h"

namespace Mantid
{
namespace Algorithms
{

  /** CopySample : The algorithm copies some/all the sample information from one workspace to another.
  For MD workspaces, if no input sample number is specified, or not found, it will copy the first sample.
  For MD workspaces, if no output sample number is specified (or negative), it will copy to all samples. The following information can be copied:

    Name
    Material
    Sample environment
    Shape
    Oriented lattice
    
    @author Andrei Savici, ORNL
    @date 2011-08-11

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
  class DLLExport CopySample  : public API::Algorithm
  {
  public:
    CopySample();
    ~CopySample();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "CopySample";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Copy some/all the sample information from one workspace to another.";}

    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Sample;Utility\\Workspaces;DataHandling";}
    virtual std::map<std::string, std::string> validateInputs();
  private:
    
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();
    /// Function to copy information from one sample to another
    void copyParameters(API::Sample& from, API::Sample& to, bool nameFlag, bool materialFlag, bool environmentFlag, bool shapeFlag, bool latticeFlag, bool orientationOnlyFlag);

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_COPYSAMPLE_H_ */
