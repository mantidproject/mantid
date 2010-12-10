#ifndef MANTID_DATAHANDLING_LOADMUONNEXUS2_H_
#define MANTID_DATAHANDLING_LOADMUONNEXUS2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
  namespace NeXus
  {
    // Forward declarations
    template<class T>
    class NXDataSetTyped;
    typedef NXDataSetTyped<int> NXInt;
    class NXEntry;

    /**
    Loads an file in Nexus Muon format and stores it in a 2D workspace 
    (Workspace2D class). LoadMuonNexus is an algorithm and as such inherits
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadMuonNexus2 : public LoadMuonNexus
    {
    public:
      /// Default constructor
      LoadMuonNexus2();
      /// Destructor
      ~LoadMuonNexus2() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadMuonNexus"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 2; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

      /// do a quick check that this file can be loaded 
      virtual bool quickFileCheck(const std::string& filePath,int nread,unsigned char* header_buffer);
      /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
      virtual int fileCheck(const std::string& filePath);
    private:
      /// Overwrites Algorithm method
      void exec();
      
      void loadData(const NXInt& counts,const std::vector<double>& timeBins,int wsIndex,
                    int period,int spec,API::MatrixWorkspace_sptr localWorkspace);
      //void runLoadInstrument(API::MatrixWorkspace_sptr localWorkspace);
      void loadLogs(API::MatrixWorkspace_sptr ws, NXEntry & entry,int period);
    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADMUONNEXUS2_H_*/
