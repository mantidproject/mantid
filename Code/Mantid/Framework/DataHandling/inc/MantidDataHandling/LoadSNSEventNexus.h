#ifndef MANTID_DATAHANDLING_LOADSNSEVENTNEXUS_H_
#define MANTID_DATAHANDLING_LOADSNSEVENTNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDataFileChecker.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"


namespace Mantid
{

  namespace DataHandling
  {
    /** @class LoadSNSEventNexus LoadSNSEventNexus.h DataHandling/LoadSNSEventNexus.h

    Load SNS EventNexus files.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NeXus file </LI>
    <LI> Workspace - The name of the workspace to output</LI>
    </UL>

    @author Janik Zikovsky, SNS
    @date Sep 27, 2010

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
    class DLLExport LoadSNSEventNexus : public LoadEventNexus, public API::DeprecatedAlgorithm
    {
    public:
      /// Default constructor
      LoadSNSEventNexus();

      /// Destructor
      virtual ~LoadSNSEventNexus()
      { }

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const;

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const;

      int fileCheck(const std::string& filePath);

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADSNSEVENTNEXUS_H_*/

