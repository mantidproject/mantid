#ifndef MANTID_PYTHONINTERFACE_PYTHONALGORITHM_H_
#define MANTID_PYTHONINTERFACE_PYTHONALGORITHM_H_
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

#include <boost/python/object.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    /**
     * Provides a class that forms an interface between a Python algorithm
     * and a C++ algorithm.
     *
     * It defines several functions for declaring properties that handle the
     * fact that the type is only known at runtime
     *
     * It works in tandem with the AlgorithmWrapper such that
     * when the AlgorithmWrapper is exported to Python
     * a user sees the PythonAlgorithm class.
     */
    class PythonAlgorithm : public API::Algorithm
    {
    public:
      /// Declare a specialized property
      void declareProperty(Kernel::Property *prop, const std::string &doc="");
      /// Declare a property using the type of the defaultValue with a validator and doc string
      void declareProperty(const std::string & name, const boost::python::object & defaultValue,
                           const boost::python::object & validator = boost::python::object(),
                           const std::string & doc = "", const int direction = Kernel::Direction::Input);

      /// Declare a property with a documentation string
      void declareProperty(const std::string & name, const boost::python::object & defaultValue,
                           const std::string & doc, const int direction = Kernel::Direction::Input);

      /// Declare a property using the type of the defaultValue
      void declareProperty(const std::string & name, const boost::python::object & defaultValue,
                           const int direction);
    private:
      // Hide the base class variants as they are not required on this interface
      using Mantid::API::Algorithm::declareProperty;
    };

  }
}



#endif /* MANTID_PYTHONINTERFACE_PYTHONALGORITHM_H_ */
