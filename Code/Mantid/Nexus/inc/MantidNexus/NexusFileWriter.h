#ifndef NEXUSFILEWRITER_H
#define NEXUSFILEWRITER_H
#include <napi.h>
#include "MantidDataObjects/Workspace2D.h"
namespace Mantid
{
  namespace NeXus
  {
    /** @class NexusFileWriter NexusFileWriter.h NeXus/NexusFileWriter.h

    Utility method for saving NeXus format of Mantid Workspace

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
    class DLLExport NexusFileWriter
    {
    public:
      /// Default constructor
      NexusFileWriter();

      /// Destructor
      ~NexusFileWriter() {}

      /// open the nexus file
      int openNexusWrite(const std::string& fileName, const std::string& entryName);
      /// write the header ifon for the Mantid workspace format
      int writeNexusProcessedHeader( const std::string& entryName, const std::string& title);
      /// write sample related data
      int writeNexusProcessedSample( const std::string& entryName, const std::string& title,
							  const boost::shared_ptr<Mantid::API::Sample> sample);
      /// write the workspace data
      int writeNexusProcessedData( const std::string& entryName,
							const boost::shared_ptr<Mantid::DataObjects::Workspace2D> localworkspace,
							const bool uniformSpectra, const int fromY, const int toY);
      /// write the algorithm and environment information
      int writeNexusProcessedProcess(const boost::shared_ptr<Mantid::DataObjects::Workspace2D> localworkspace);
      /// write the source XML file used, if it exists
      bool writeNexusInstrumentXmlName(const std::string& instrumentXml,const std::string& date,
                            const std::string& version);
      /// write an instrument section - currently only the name
      bool writeNexusInstrument(const boost::shared_ptr<API::Instrument>& instrument);
      /// close the nexus file
      int closeNexusFile();

    private:
      /// Nexus file handle
      NXhandle fileID;
      /// write a text value plus possible attribute
      bool writeNxText(NXhandle fileID, std::string name, std::string value, std::vector<std::string> attributes,
				 std::vector<std::string> avalues);
      /// write an NXnote with standard fields (but NX_CHAR rather than NX_BINARY data)
      bool writeNxNote(const std::string& noteName, const std::string& author, const std::string& date,
                         const std::string& description, const std::string& pairValues);
      /// write a flost value plus possible attributes
      bool writeNxFloat(const std::string name, const double& value, const std::vector<std::string> attributes,
	                           const std::vector<std::string> avalues);
      /// write test field
      int writeNexusTextField( const NXhandle& h, const std::string& name, const std::string& value);
      /// search for exisiting MantidWorkpace_n entries in opened file
      int findMantidWSEntries();
      /// nexus file name
      std::string m_filename;
      ///static reference to the logger class
      static Kernel::Logger& g_log;

    };

  } // namespace NeXus
} // namespace Mantid

#endif /* NEXUSFILEWRITER_H */
