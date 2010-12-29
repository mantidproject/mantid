#ifndef MANTID_DATAHANDLING_LOADPARAMETERFILE_H_
#define MANTID_DATAHANDLING_LOADPARAMETERFILE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

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
    class Object;
    class ObjComponent;
    class V3D;
    class Instrument;
  }

  namespace DataHandling
  {
    /** @class LoadParameterFile LoadParameterFile.h DataHandling/LoadParameterFile.h

    Loads instrument parameter data from a XML instrument parameter file and adds it
    to a workspace.

    LoadParameterFile is an algorithm and as such inherits
    from the Algorithm class and overrides the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the workspace </LI>
    <LI> Filename - The name of the parameter file </LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 19/4/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadParameterFile : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadParameterFile();

      /// Destructor
      ~LoadParameterFile() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadParameterFile";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Instrument";}

    private:
      void init();
      void exec();
    };
  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADPARAMETERFILE_H_*/

