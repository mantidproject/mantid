#ifndef MANTID_ALGORITHM_REBUNCH_H_
#define MANTID_ALGORITHM_REBUNCH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 

    Takes a workspace as input and rebunches the data according to the input rebunch parameters.
	The process is exactly that defined by the mgenie code of T. G. Perring for both point and histogram data,
	in that the the distribution flag is ignored for point data, and the contributing point data points are averaged.
    
	Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>
    <LI> n_bunch - number of data points to bunch together for each new point, must be >= 1 </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

   
    @author Dickon Champion, STFC
    @date 24/06/2008

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport Rebunch : public API::Algorithm
    {
    public:
      /// Default constructor
      Rebunch() : API::Algorithm() {};
      /// Destructor
      virtual ~Rebunch() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Rebunch";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Rebin";}
    
    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden Algorithm methods
      void init();
      void exec();
      void rebunch_hist(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
			std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, const size_t n_bunch, const bool distribution);
      void rebunch_point(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
			std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, const int n_bunch);

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_REBUNCH_H_*/
