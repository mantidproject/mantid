#ifndef MANTID_PYTHONINTERFACE_ALGORITHMWRAPPER_H_
#define MANTID_PYTHONINTERFACE_ALGORITHMWRAPPER_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <boost/python/wrapper.hpp>


namespace Mantid
{
  namespace PythonInterface
  {
    /**
      @author Martyn Gigg, Tessella plc

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

    /**
     * This class simply acts as a marker for now.
     * It is required so that the export functions
     * can properly mark Algorithm as a base class
     * of a PythonAlgorithm.
     *
     * Trying to do this with the wrapper below
     * causes a compilation error
     */
    class PythonAlgorithm : public API::Algorithm
    {};

    /**
      This class wraps the PythonAlgorithm class and allows classes in Python
      to inherit from it.

      This class is treated by Boost Python as if it were of type PythonAlgorithm.
     */

    class AlgorithmWrapper : public PythonAlgorithm, public boost::python::wrapper<PythonAlgorithm>
    {
    public:
      /// Returns the name of the algorithm
      virtual const std::string name() const;
      /// Returns a version of the algorithm, default = 1
      virtual int version() const;
      /// Returns a category of the algorithm. A default implementation is provided
      virtual const std::string category() const;

    private:
      /// Private init for this algorithm
      virtual void init();
      /// Private exec for this algorithm
      virtual void exec();
    };
  }
}


#endif /* MANTID_PYTHONINTERFACE_ALGORITHMWRAPPER_H_ */
