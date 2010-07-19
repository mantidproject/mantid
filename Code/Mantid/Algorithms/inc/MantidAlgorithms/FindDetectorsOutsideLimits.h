#ifndef MANTID_ALGORITHM_FINDDETECTORSOUTSIDELIMITS_H_
#define MANTID_ALGORITHM_FINDDETECTORSOUTSIDELIMITS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <set>
#include <string>

namespace Mantid
{
  namespace Algorithms
  {
    /**
    Takes a workspace as input and identifies all spectra were the sum of the 
    counts in all bins is outside a range. This is then used to mark all 'bad'
    detectors with a 'bad' marker value, while all spectra from live detectors
    are given a value of 'good' marker value.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    <LI> HighThreshold - Spectra whose total number of counts are above or equal to this value will be marked dead</LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> LowThreshold - Spectra whose total number of counts are below or equal to this value will be marked dead (default 0)</LI>
    <LI> LiveValue - The value to assign to an integrated spectrum flagged as 'live' (default 0.0)</LI>
    <LI> DeadValue - The value to assign to an integrated spectrum flagged as 'dead' (default 100.0)</LI>
    <LI> StartX - Start the integration at the above bin above the one that this value is in (default: the start of each histogram)</LI>
    <LI> EndX - Stop the integration at the bin before the one that contains this x value (default: the end of each histogram)</LI>
    <LI> OutputFile - (Optional) A filename to which to write the list of dead detector UDETs </LI>
    </UL>

    @author Steve D Williams, ISIS Facility Rutherford Appleton Laboratory
    @date 07/07/2009

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    class DLLExport FindDetectorsOutsideLimits : public API::Algorithm
    {
    public:
      /// Default constructor
      FindDetectorsOutsideLimits() : API::Algorithm()
      {};
      /// Destructor
      virtual ~FindDetectorsOutsideLimits() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "FindDetectorsOutsideLimits";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diagnostics";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();

      API::MatrixWorkspace_sptr integrateWorkspace();
      void writeFile(const std::string &fname, const std::set<int> &lowList, const std::set<int> &highList) const;
      void writeList(std::ostream & outputStream, const std::set<int> & specList, const std::string & prefix, const std::string & suffix) const;
      
    };

  } // namespace Algorithm
} // namespace Mantid


#endif /*MANTID_ALGORITHM_FINDDETECTORSOUTSIDELIMITS_H_*/
