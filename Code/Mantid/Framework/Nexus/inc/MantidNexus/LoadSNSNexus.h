#ifndef MANTID_DATAHANDLING_LOADSNSNEXUS_H_
#define MANTID_DATAHANDLING_LOADSNSNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/IDataFileChecker.h"
#include <napi.h>
#include <climits>

#include <boost/shared_array.hpp>


//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
namespace NeXus
{

    /** @class LoadSNSNexus LoadSNSNexus.h NeXus/LoadSNSNexus.h

    Loads a file in a Nexus format and stores it in a 2D workspace 
    (Workspace2D class). LoadSNSNexus is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input Nexus file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data 
    (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)
    [ not yet implemented for Nexus ]</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multiperiod file)
    <UL>
    <LI> spectrum_min  - The spectrum to start loading from</LI>
    <LI> spectrum_max  - The spectrum to load to</LI>
    <LI> spectrum_list - An ArrayProperty of spectra to load</LI>
    </UL>

    @author Roman Tolchenov, Tessella plc
    @date 09/07/09

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadSNSNexus : public API::IDataFileChecker
    {
    public:
        /// Default constructor
        LoadSNSNexus();
        /// Destructor
        ~LoadSNSNexus() {}
        /// Algorithm's name for identification overriding a virtual method
        virtual const std::string name() const { return "LoadSNSNexus"; }
        /// Algorithm's version for identification overriding a virtual method
        virtual int version() const { return 1; }
        /// Algorithm's category for identification overriding a virtual method
        virtual const std::string category() const { return "DataHandling"; }
       /// do a quick check that this file can be loaded 
      virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
      /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
      virtual int fileCheck(const std::string& filePath);

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
        /// Overwrites Algorithm method.
        void init();
        /// Overwrites Algorithm method
        void exec();

        /// The l1
        double m_L1;

        /// Class for comparing bank names in format "bank123" according to the numeric part of the name
        class CompareBanks
        {
        public:
            /** Compare operator
            *  @param s1 :: First argument
            *  @param s2 :: Second argument
            *  @return true if i1<i2
            */
            bool operator()(const std::string& s1, const std::string& s2)
            {
                int i1 = atoi( s1.substr(4,s1.size()-4).c_str() );
                int i2 = atoi( s2.substr(4,s2.size()-4).c_str() );
                return i1 < i2;
            }
        };

        /// User input spectra selection
        std::vector<int> getSpectraSelection();

        /// Read in an entry (period).
        API::Workspace_sptr loadEntry(NXEntry entry,int period, double progress_start, double progress_end);

        /// Personal wrapper for sqrt to allow msvs to compile
        static double dblSqrt(double in);

    };

} // namespace NeXus
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADSNSNEXUS_H_*/
