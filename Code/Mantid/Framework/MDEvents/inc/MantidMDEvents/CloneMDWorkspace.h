#ifndef MANTID_MDEVENTS_CLONEMDWORKSPACE_H_
#define MANTID_MDEVENTS_CLONEMDWORKSPACE_H_
/*WIKI* 

This algorithm will clones an existing MDEventWorkspace into a new one.

If the InputWorkspace is a file-backed MDEventWorkspace, then the algorithm will copy the original file into a new one with the suffix '_clone' added to its filename, in the same directory.
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** Algorithm to clone a MDEventWorkspace to a new one.
   * Can also handle file-backed MDEventWorkspace's
    
    @author Janik Zikovsky
    @date 2011-08-15

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
  class DLLExport CloneMDWorkspace  : public API::Algorithm
  {
  public:
    CloneMDWorkspace();
    ~CloneMDWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "CloneMDWorkspace";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDEvents";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    template<typename MDE, size_t nd>
    void doClone(typename MDEventWorkspace<MDE, nd>::sptr ws);

  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_CLONEMDWORKSPACE_H_ */
