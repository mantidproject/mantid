#ifndef MANTID_DATAHANDLING_LOADINSTRUMENT_H_
#define MANTID_DATAHANDLING_LOADINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/DataHandlingCommand.h"

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
	
namespace Geometry
{
	class CompAssembly;
	class Component;
}
namespace API
{
	class Instrument;
}
	
  namespace DataHandling
  {
    /** @class LoadInstrument LoadInstrument.h DataHandling/LoadInstrument.h

    Loads instrument data from a XML instrument description file and adds it 
    to a workspace. 

    LoadInstrument is intended to be used as a child algorithm of 
    other Loadxxx algorithms, rather than being used directly.
    LoadInstrument is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input RAW file </LI>
    <LI> Workspace - The name of the workspace in which to use as a basis for any data to be added.</LI>
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
      virtual const int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Instrument";}

    private:

      /// Overwrites Algorithm method. Does nothing at present
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// Structure for holding detector IDs
      struct IdList
      {
        ///Counted
        int counted;
        ///Vector of the values of the ID list
        std::vector<int> vec;

        ///Constructor
        IdList() : counted(0) {};
      };

      /// Method for populating IdList
      void populateIdList(Poco::XML::Element* pElem, IdList& idList);

      /// Add XML element to parent assuming the element contains other component elements
      void appendAssembly(Geometry::CompAssembly* parent, Poco::XML::Element* pElem, IdList& idList);
      /// Add XML element to parent assuming the element contains other component elements
      void appendAssembly(boost::shared_ptr<Geometry::CompAssembly> parent, Poco::XML::Element* pElem, IdList& idList);

      /// Add XML element to parent assuming the element contains no other component elements
      void appendLeaf(Geometry::CompAssembly* parent, Poco::XML::Element* pElem, IdList& idList);
      /// Add XML element to parent assuming the element contains no other component elements
      void appendLeaf(boost::shared_ptr<Geometry::CompAssembly> parent, Poco::XML::Element* pElem, IdList& idList);

      /// Set location (position) of comp as specified in XML location element
      void setLocation(Geometry::Component* comp, Poco::XML::Element* pElem);

      /// Get parent component element of location element
      Poco::XML::Element* getParentComponent(Poco::XML::Element* pLocElem);

      /// map which holds names of types and whether or not they are catagorised as being 
      /// assembles, which means whether the type element contains component elements
      std::map<std::string,bool> isTypeAssemply;

      /// map which holds names of types and pointers to these type for fast retrievel in code
      std::map<std::string,Poco::XML::Element*> getTypeElement;

      /// The name and path of the input file
      std::string m_filename;

      ///static reference to the logger class
      static Kernel::Logger& g_log;

      /// For convenience added pointer to instrument here
      boost::shared_ptr<API::Instrument> instrument;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADINSTRUMENT_H_*/

