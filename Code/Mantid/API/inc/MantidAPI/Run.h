#ifndef MANTID_API_RUN_H_
#define MANTID_API_RUN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManager.h"

namespace Mantid
{
  namespace API
  {

    /**
       This class stores information regarding an experimental run as a series
       of log entries

       @author Martyn Gigg, Tessella plc
       @date 26/11/2007
       
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

       File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
       Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport Run : private Kernel::PropertyManager
    {

    public:
      /// Default constructor
      Run();
      /// Virtual destructor
      virtual ~Run();
    
      /// Add data to the object in the form of a property
      void addProperty(Kernel::Property *prop);

      // Expose some of the PropertyManager publicly
      using Kernel::PropertyManager::removeProperty;
      using Kernel::PropertyManager::getProperty;
      using Kernel::PropertyManager::getProperties;

      /** @name Legacy functions */
      //@{
      /// Set the proton charge
      void setProtonCharge( const double charge);
      /// Get the proton charge
      double getProtonCharge() const;
      //@}

    private:
      /// The name of the proton charge property
      const std::string m_protonChargeName;
    };

  }
}

#endif //MANTIDAPI_RUN_H_
