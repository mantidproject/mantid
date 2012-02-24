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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/PythonAlgorithm/PythonAlgorithm.h"
#include <boost/python/wrapper.hpp>

namespace Mantid
{
  namespace PythonInterface
  {

    /**
     * Provides a wrapper class for boost::python to allow virtual functions
     * to be overridden in Python.
     *
     * It works in tandem with the PythonAlgorithm class. This is essentially
     * a transparent layer that handles the function calls up into Python.
     *
     * When exported the user sees and item of type PythonAlgorithm
     */
    class AlgorithmWrapper : public PythonAlgorithm, public boost::python::wrapper<PythonAlgorithm>
    {
    public:
      /// Returns the name of the algorithm
      virtual const std::string name() const;
      /// A default version, chosen if no override exists
      const std::string defaultName() const;

      /// Returns a version of the algorithm
      virtual int version() const;
      /// A default version, chosen if there is no override
      int defaultVersion() const;

      /// Returns a category of the algorithm.
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
