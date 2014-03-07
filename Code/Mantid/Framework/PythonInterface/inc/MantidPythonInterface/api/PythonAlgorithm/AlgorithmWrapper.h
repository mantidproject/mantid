#ifndef MANTID_PYTHONINTERFACE_ALGORITHMWRAPPER_H_
#define MANTID_PYTHONINTERFACE_ALGORITHMWRAPPER_H_
/**
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/PythonAlgorithm.h"
#include "MantidKernel/ClassMacros.h"
#include <boost/python/wrapper.hpp>
#include <map>

namespace Mantid
{
  namespace PythonInterface
  {

    /**
     * Provides a layer class for boost::python to allow C++ virtual functions
     * to be overridden in a Python object that is derived from PythonAlgorithm.
     *
     * It works in tandem with the PythonAlgorithm class. This is essentially
     * a transparent layer that handles the function calls up into Python.
     */
    class AlgorithmWrapper : public PythonAlgorithm
    {
    public:
      /// A constructor that looks like a Python __init__ method
      AlgorithmWrapper(PyObject* self);

      /** @name Algorithm virtual methods */
      ///@{
      /// Returns the name of the algorithm
      const std::string name() const;
      /// Returns a version of the algorithm
      virtual int version() const;
      /// A default version, chosen if there is no override
      int defaultVersion() const;
      /// Returns a category of the algorithm.
      virtual const std::string category() const;
      /// A default category, chosen if there is no override
      std::string defaultCategory() const;
      /// Returns the validateInputs result of the algorithm.
      std::map<std::string, std::string> validateInputs();
      ///@}

    private:
      /// The PyObject must be supplied to construct the object
      DISABLE_DEFAULT_CONSTRUCT(AlgorithmWrapper);
      DISABLE_COPY_AND_ASSIGN(AlgorithmWrapper);

      /// Private init for this algorithm
      virtual void init();
      /// Private exec for this algorithm
      virtual void exec();

      /**
       *  Returns the PyObject that owns this wrapper, i.e. self
       * @returns A pointer to self
       */
      inline PyObject * getSelf() const { return m_self; }

      /// The Python portion of the object
      PyObject *m_self;
    };
  }
}


#endif /* MANTID_PYTHONINTERFACE_ALGORITHMWRAPPER_H_ */
