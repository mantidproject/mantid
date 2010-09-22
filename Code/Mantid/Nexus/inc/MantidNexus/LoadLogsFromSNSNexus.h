#ifndef MANTID_NEXUS_LOADLOGSFROMSNSNEXUS_H_
#define MANTID_NEXUS_LOADLOGSFROMSNSNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"

//Copy of the NexusCpp API was placed in MantidNexus
#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/NeXusException.hpp"


namespace Mantid
{

namespace Geometry
{
	class CompAssembly;
	class Component;
}
namespace API
{
	class Instrument;
}

  namespace NeXus
  {
    /** @class LoadLogsFromSNSNexus LoadLogsFromSNSNexus.h Nexus/LoadLogsFromSNSNexus.h

    Load sample logs (single values and time series data) from a SNS nexus format file.
    This is meant to be used as a sub-algorithm to other algorithms.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NEXUS file </LI>
    <LI> Workspace - The name of the workspace in which to use as a basis for any data to be added.</LI>
    </UL>

    @author Janik Zikovsky, SNS
    @date Sep 17, 2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    */
    class DLLExport LoadLogsFromSNSNexus : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadLogsFromSNSNexus();

      /// Destructor
      ~LoadLogsFromSNSNexus() {}

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadLogsFromSNSNexus";};

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Nexus\\Logs";}

    private:

      void init();

      void exec();

      void loadSampleLog(NeXusAPI::File& file, std::string entry_name, std::string entry_class);

      /// The name and path of the input file
      std::string m_filename;

      /// The workspace being filled out
      API::MatrixWorkspace_sptr WS;

    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_NEXUS_LOADLOGSFROMSNSNEXUS_H_*/

