#ifndef MANTID_MDEVENTS_PLUSMDEW_H_
#define MANTID_MDEVENTS_PLUSMDEW_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** PlusMDEW : TODO: DESCRIPTION
    
    @author
    @date 2011-08-12

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
  class DLLExport PlusMDEW  : public API::Algorithm
  {
  public:
    PlusMDEW();
    ~PlusMDEW();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "PlusMDEW";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "General";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    template<typename MDE, size_t nd>
    void doPlus(typename MDEventWorkspace<MDE, nd>::sptr ws);

    /// Workspace into which stuff will get added
    Mantid::API::IMDEventWorkspace_sptr iws1;
    /// Workspace that will be added into ws1
    Mantid::API::IMDEventWorkspace_sptr iws2;

  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_PLUSMDEW_H_ */
