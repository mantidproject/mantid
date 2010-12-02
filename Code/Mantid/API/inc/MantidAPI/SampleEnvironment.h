#ifndef MANTID_API_SAMPLEENVIRONMENT_H_
#define MANTID_API_SAMPLEENVIRONMENT_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "DllExport.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

namespace Mantid
{
  namespace API
  {
    /** 
      This class stores details regarding the sample environment that was used during
      a specific run. It is implemented as a type of CompAssembly so that enviroment kits
      consisting of objects made from different materials can be constructed easily.
	
      @author Martyn Gigg, Tessella plc
      @date 23/11/2010
      
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
    class EXPORT_OPT_MANTID_API SampleEnvironment : public Geometry::CompAssembly
    {
    public:
      /// Constructor defining the name of the environment
      SampleEnvironment(const std::string & name);
      /// Copy constructor
      SampleEnvironment(const SampleEnvironment& original);
      /// Clone the assembly
      virtual Geometry::IComponent* clone() const;
      /// Type of object
      virtual std::string type() const {return "SampleEnvironment"; }

      /// Override add a component to the assembly so that we can only add components that
      /// have a physical shape
      int add(IComponent* comp);

    private:
      /// Default constructor
      SampleEnvironment();
      /// Assignment operator
      SampleEnvironment& operator=(const SampleEnvironment&);
    };
  }
}

#endif // MANTID_API_SAMPLEENVIRONMENT_H_
