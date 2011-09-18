#ifndef MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_
#define MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_
/*WIKI* 


*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/Run.h"

namespace Mantid
{
namespace Algorithms
{

  /** RemovePromptPulse : TODO: DESCRIPTION
    
    @author
    @date 2011-07-18

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
  class DLLExport RemovePromptPulse  : public API::Algorithm
  {
  public:
    RemovePromptPulse();
    ~RemovePromptPulse();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const;

    /// Algorithm's version for identification 
    virtual int version() const;

    /// Algorithm's category for identification
    virtual const std::string category() const;
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();
    /// Try to get the frequency from a given name.
    double getFrequency(const API::Run& run);
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_REMOVEPROMPTPULSE_H_ */
