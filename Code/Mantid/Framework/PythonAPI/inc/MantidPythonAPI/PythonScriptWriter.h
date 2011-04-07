#ifndef MANTID_PYTHONAPI_PYTHONSCRIPTWRITER_H_
#define MANTID_PYTHONAPI_PYTHONSCRIPTWRITER_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/ScriptWriter.h"

namespace Mantid
{
  namespace PythonAPI
  {
    
    /**
      Defines a PythonScriptWriter class that serves to write
      a given workspace history out as a Python script
      
      @author Martyn Gigg, Tessella Plc
      @data 07/04/2011
      
      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
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
    class DLLExport PythonScriptWriter : public API::ScriptWriter
    {
    public:
      /// Generate the script from the history
      std::string write(const API::WorkspaceHistory & history) const;
    };
    
    
  } // namespace Mantid
} // namespace PythonAPI

#endif  /* MANTID_PYTHONAPI_PYTHONSCRIPTWRITER_H_ */
