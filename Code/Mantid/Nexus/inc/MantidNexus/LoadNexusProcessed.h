#ifndef MANTID_NEXUS_LOADNEXUSPROCESSED_H_
#define MANTID_NEXUS_LOADNEXUSPROCESSED_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusFileIO.h"
#include <climits>
//class NexusFileIO;


namespace Mantid
{
  namespace NeXus
  {
    /** @class LoadNexusProcessed LoadNexusProcessed.h NeXus/LoadNexusProcessed.h

    Loads a workspace from a Nexus Processed entry in a Nexus file. 
    LoadNexusProcessed is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of the input Nexus file (must exist) </LI>
    <LI> InputWorkspace - The name of the workspace to put the data </LI>
    </UL>

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadNexusProcessed : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadNexusProcessed();

      /// Destructor
      ~LoadNexusProcessed();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadNexusProcessed";};
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling";}

    private:

      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();
      /// NexusFileIO instance to do IO operations
      NexusFileIO *nexusFile;

      /// Load algorithm history (or process)
      void loadAlgorithmHistory(DataObjects::Workspace2D_sptr localWorkspace);
      std::time_t createTime_t_FromString(const std::string &str);
      /// check optional params
      void checkOptionalProperties();
      /// Object to han
      /// The name and path of the input file
      std::string m_filename;
      /// The number of the input entry
      int m_entrynumber;
      /// The title of the processed data section
      std::string m_title;
      /// Pointer to the local workspace
      API::MatrixWorkspace_sptr m_outputWorkspace;
      /// Flag set if list of spectra to save is specifed
      //bool m_list;
      ///// Flag set if interval of spectra to write is set
      //bool m_interval;
      ///// The value of the spectrum_list property
      //std::vector<int> m_spec_list;
      ///// The value of the spectrum_min property
      //int m_spec_min;
      ///// The value of the spectrum_max property
      //int m_spec_max;
      /// The value of the workspace number property
      int m_numberofspectra;
      /// total channels
      int m_numberofchannels;
      /// total x points (m_numberofchannels or m_numberofchannels+1)
      int m_xpoints;
      /// flag if bounds are same for all spectra
      bool m_uniformbounds;
      /// axes names
      std::string m_axes;
      /// Y units from Nexus file
      std::string m_yunits;
      /// The name of the instrument
      std::string m_instrumentName;
//      /// The instrument XML file name read from the Nexus file
//      std::string m_instrumentxml;
//      /// Instrument xml file date
//      std::string m_instrumentdate;
//      /// Instrument xml file version
//      std::string m_instrumentversion;
      /// Function to try and load instrument data from XML
      void runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace);
      ///a flag int value that indicates a value that wasn't set
      static const int unSetInt = INT_MAX-15;

    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_NEXUS_LOADNEXUSPROCESSED_H_*/
