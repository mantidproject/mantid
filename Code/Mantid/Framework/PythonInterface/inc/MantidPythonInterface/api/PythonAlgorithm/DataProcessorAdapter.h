#ifndef MANTID_PYTHON_INTERFACE_DATAPROCESSORADAPTER_H
#define MANTID_PYTHON_INTERFACE_DATAPROCESSORADAPTER_H
/**
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmAdapter.h"
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid
{
  namespace PythonInterface
  {
    /**
     * Provides a layer class for boost::python to allow C++ virtual functions
     * to be overridden in a Python object that is derived an DataProcessorAlgorithm.
     *
     * It also provides access to the protected methods on DataProcessorAlgorithm
     * from the type exported to Python
     */
    class DataProcessorAdapter : public AlgorithmAdapter<API::DataProcessorAlgorithm>
    {
      typedef AlgorithmAdapter<API::DataProcessorAlgorithm> SuperClass;

    public:
      /// A constructor that looks like a Python __init__ method
      DataProcessorAdapter(PyObject* self);

      // -------------------- Pass through methods ----------------------------
      // Boost.python needs public access to the base class methods in order to
      // be able to call them. We should just be able to put a using declaration
      // to raise the methods to public but this does not work on MSVC as in
      // the class_ definition it can't seem to figure out that its calling
      // this class not DataProcessorAlgorithm. This means we have to resort to
      // proxy methods :(

      void setLoadAlgProxy(const std::string & alg) { this->setLoadAlg(alg); }

      void setLoadAlgFilePropProxy(const std::string & filePropName) { this->setLoadAlgFileProp(filePropName); }

      void setAccumAlgProxy(const std::string & alg) { this->setAccumAlg(alg); }

      API::ITableWorkspace_sptr determineChunkProxy() { return this->determineChunk(); }

      void loadChunkProxy() { this->loadChunk(); }

      API::Workspace_sptr loadProxy(const std::string &inputData, const bool loadQuiet = false) { return this->load(inputData, loadQuiet); }

      std::vector<std::string> splitInputProxy(const std::string & input) { return this->splitInput(input); }

      void forwardPropertiesProxy() { this->forwardProperties(); }

      boost::shared_ptr<Kernel::PropertyManager> getProcessPropertiesProxy(const std::string &propertyManager)
      {
        return this->getProcessProperties(propertyManager);
      }

      API::Workspace_sptr assembleProxy(const std::string &partialWSName, const std::string &outputWSName)
      {
        return this->assemble(partialWSName, outputWSName);
      }
      void saveNexusProxy(const std::string &outputWSName, const std::string &outputFile)
      {
        this->saveNexus(outputWSName, outputFile);
      }

      bool isMainThreadProxy() { return this->isMainThread(); }

      int getNThreadsProxy() { return this->getNThreads(); }
      // ------------------------------------------------------------------------

    private:
      /// The PyObject must be supplied to construct the object
      DISABLE_DEFAULT_CONSTRUCT(DataProcessorAdapter);
      DISABLE_COPY_AND_ASSIGN(DataProcessorAdapter);
    };

  }
}

#endif // MANTID_PYTHON_INTERFACE_DATAPROCESSORADAPTER_H
