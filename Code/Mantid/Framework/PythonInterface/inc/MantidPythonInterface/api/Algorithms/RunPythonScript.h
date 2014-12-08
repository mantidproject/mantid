#ifndef MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPT_H_
#define MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPT_H_

#include "MantidAPI/Algorithm.h"

#include <boost/python/dict.hpp>

namespace Mantid
{
  namespace PythonInterface
  {

    /**

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class DLLExport RunPythonScript  : public API::Algorithm
    {
    public:

      const std::string name() const;
      int version() const;
      const std::string category() const;
      const std::string summary() const;

    private:
      virtual bool checkGroups();
      void init();
      void exec();

      /// Return the code string to execute
      std::string scriptCode() const;
      /// Sets up the code context & executes it
      boost::shared_ptr<API::Workspace> executeScript(const std::string & script) const;
      /// Execute the code in the given local context
      boost::python::dict doExecuteScript(const std::string & script) const;
      /// Builds the local dictionary that defines part of the execution context of the script
      boost::python::dict buildLocals() const;
      /// Extracts any output workspace pointer that was created
      boost::shared_ptr<API::Workspace> extractOutputWorkspace(const boost::python::dict & locals) const;
    };


  } // namespace PythonInterface
} // namespace Mantid

#endif  /* MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPT_H_ */
