#ifndef MANTID_DATAHANDLING_SAVENEXUS_H_
#define MANTID_DATAHANDLING_SAVENEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <climits>

namespace Mantid
{
  namespace DataHandling
  {
    /**
    Saves a file in NeXus format and from a 2D workspace
    (Workspace2D class). SaveNeXus is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the output NeXus file </LI>
    <LI> OutputWorkspace - The name of the workspace from which to write the exported data </LI>
    </UL>

    @author Freddie Akeroyd, STFC ISIS Facility, GB
    @date 24/01/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport SaveNexus : public API::Algorithm
    {
    public:
      /// Default constructor
      SaveNexus();

      /// Destructor
      ~SaveNexus() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveNexus";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "The SaveNexus algorithm will write the given Mantid workspace to a NeXus file. SaveNexus currently just invokes SaveNexusProcessed.";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Nexus";}

    private:
      

      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// The name and path of the input file
      std::string m_filename;
      /// The name and path of the input file
      std::string m_entryname;
      /// The prefix to be used to name spectra (_n added for nth spectra)
      std::string m_dataname;
      /// The file type to save, currently only one type possible
      std::string m_filetype;
      /// Pointer to the local workspace
      API::Workspace_sptr m_inputWorkspace;
      /// Method to execute SNP Child Algorithm
      void runSaveNexusProcessed();
      /// sets non workspace properties for the algorithm
      void setOtherProperties(IAlgorithm* alg,const std::string & propertyName,const std::string &propertyValue,int perioidNum);

    protected:

      /// Override process groups
      virtual bool processGroups();

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SAVENEXUS_H_*/
