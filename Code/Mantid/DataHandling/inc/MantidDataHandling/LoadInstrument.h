#ifndef MANTID_DATAHANDLING_LOADINSTRUMENT_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/DataHandlingCommand.h"
#include "MantidKernel/Logger.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
	class Element;
}}
/// @endcond

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadInstrument LoadInstrument.h DataHandling/LoadInstrument.h

    Loads instrument data from a file and adds it to a workspace. 
    The current implementation uses a simple data structure that will be replaced later.
    LoadInstrument is intended to be used as a child algorithm of 
    other Loadxxx algorithms, rather than being used directly.
    LoadInstrument is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input RAW file </LI>
    <LI> InputWorkspace - The name of the workspace in which to use as a basis for any data to be added, a blank 2dWorkspace will be used as a default.</LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data </LI>
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 19/11/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 7/3/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
    class DLLExport LoadInstrument : public DataHandlingCommand
    {
    public:
      /// Default constructor
      LoadInstrument();

      /// Destructor
      ~LoadInstrument() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadInstrument";};
      /// Algorithm's version for identification overriding a virtual method
      virtual const std::string version() const { return "1";};

    private:

      /// Overwrites Algorithm method. Does nothing at present
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// Add XML element to parent assuming the element contains other component elements
      void appendAssembly(Geometry::CompAssembly* parent, Poco::XML::Element* pElem, int& runningDetID);

      /// Add XML element to parent assuming the element contains no other component elements
      void appendLeaf(Geometry::CompAssembly* parent, Poco::XML::Element* pElem, int& runningDetID);

      /// Set location (position) of comp as specified in XML location element
      void setLocation(Geometry::Component* comp, Poco::XML::Element* pElem);

      /// map which holds names of types and whether or not they are catagorised as being 
      /// assembles, which means whether the type element contains component elements
      std::map<std::string,bool> isTypeAssemply;

      /// map which holds names of types and pointers to these type for fast retrievel in code
      std::map<std::string,Poco::XML::Element*> getTypeElement;

      /// The name and path of the input file
      std::string m_filename;

      ///static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENT_H_*/

