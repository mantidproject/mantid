#ifndef MANTID_DATAHANDLING_LOADMUONNEXUS1_H_
#define MANTID_DATAHANDLING_LOADMUONNEXUS1_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MuonNexusReader;

namespace Mantid
{
  namespace DataHandling
  {
    using namespace DataObjects;

    /** @class LoadMuonNexus LoadMuonNexus.h DataHandling/LoadMuonNexus.h

    Loads an file in Nexus Muon format version 1 and stores it in a 2D workspace 
    (Workspace2D class). LoadMuonNexus is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NeXus file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data 
         (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)
         [ not yet implemented for Nexus ]</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multiperiod file)
    <UL>
    <LI> spectrum_min  - The spectrum to start loading from</LI>
    <LI> spectrum_max  - The spectrum to load to</LI>
    <LI> spectrum_list - An ArrayProperty of spectra to load</LI>
    <LI> auto_group - Determines whether the spectra are automatically grouped together based on the groupings in the NeXus file. </LI>
    </UL>
    
    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadMuonNexus1 : public LoadMuonNexus
    {
    public:
      /// Default constructor
      LoadMuonNexus1();
      /// Destructor
      virtual ~LoadMuonNexus1() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadMuonNexus"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Nexus;Muon"; }
      
      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::NexusDescriptor & descriptor) const;

    protected:
      /// Overwrites Algorithm method
      void exec();
      /// Implement the base class method
      void runLoadInstrumentFromNexus(DataObjects::Workspace2D_sptr);
      
    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      
      void loadData(const MantidVecPtr::ptr_type& tcbs,size_t hist, specid_t& i, 
        MuonNexusReader& nxload, const int64_t lengthIn, Workspace2D_sptr localWorkspace);
      void runLoadMappingTable(Workspace2D_sptr);
      void runLoadLog(Workspace2D_sptr);
      void loadRunDetails(Workspace2D_sptr localWorkspace);

      /// Loads dead time table for the detector
      void loadDeadTimes(Mantid::NeXus::NXRoot& root);

      /// Creates Dead Time Table using all the data between begin and end
      TableWorkspace_sptr createDeadTimeTable(std::vector<double>::const_iterator begin, 
        std::vector<double>::const_iterator end);
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADMUONNEXUS1_H_*/
