#ifndef MANTID_DATAHANDLING_LOADPARAMETERFILE_H_
#define MANTID_DATAHANDLING_LOADPARAMETERFILE_H_
/*WIKI* 

This algorithm allows instrument parameters to be specified in a separate file from the [[InstrumentDefinitionFile|IDF]]. The required format for this file is identical to that used for defining parameters through <component-link>s in an IDF. Below is an example of how to define a parameter named 'test' to be associated with a component named 'bank_90degnew' defined in the IDF of the HRPD instrument:
<div style="border:1pt dashed black; background:#f9f9f9;padding: 1em 0;">
<source lang="xml">
<?xml version="1.0" encoding="UTF-8" ?>
<parameter-file instrument="HRPD" date="blah...">

<component-link name="bank_90degnew" >
  <parameter name="test"> <value val="50.0" /> </parameter>
</component-link>

</parameter-file>
</source></div>


*WIKI*/

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
  namespace Kernel
  {
    class V3D;
  }
  namespace Geometry
  {
    class CompAssembly;
    class Component;
    class Object;
    class ObjComponent;
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

      static void execManually(std::string filename, Mantid::API::ExperimentInfo_sptr localWorkspace);

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      void init();
      void exec();
    };
  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADPARAMETERFILE_H_*/

