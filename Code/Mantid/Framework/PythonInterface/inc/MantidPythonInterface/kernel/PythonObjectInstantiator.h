#ifndef MANTID_PYTHONINTERFACE_PYTHONALGORITHMINSTANTIATOR_H_
#define MANTID_PYTHONINTERFACE_PYTHONALGORITHMINSTANTIATOR_H_

/*
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Instantiator.h"
#include "MantidPythonInterface/kernel/Environment/Threading.h"

#include <boost/python/object.hpp>
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {

    template<typename Base>
    class PythonObjectInstantiator : public Kernel::AbstractInstantiator<Base>
    {
    public:
      /// Constructor taking a Python class object wrapped as a boost::python:object
      PythonObjectInstantiator(const boost::python::object & classObject)
        : m_classObject(classObject) {}

      /// Creates an instance of the object as shared_ptr to the Base type
      boost::shared_ptr<Base> createInstance() const;

      /// Creates an instance of the object as raw pointer to the Base type
      Base* createUnwrappedInstance() const;

    private:
      /// The class name
      boost::python::object m_classObject;
    };

    /**
     * Creates an instance of the object as shared_ptr to the Base type
     * @returns A shared_ptr to Base.
     */
    template<typename Base>
    boost::shared_ptr<Base> PythonObjectInstantiator<Base>::createInstance() const
    {
      using namespace boost::python;
      Environment::GlobalInterpreterLock gil;
      
      object instance = m_classObject();
      // The instantiator assumes that the exported type uses a HeldType=boost::shared_ptr<Adapter>,
      // where Adapter inherits from Base,
      // see http://www.boost.org/doc/libs/1_42_0/libs/python/doc/v2/class.html#class_-spec.
      // The advantage is that the memory management is very simple as it is all handled within the
      // boost layer.
      return extract<boost::shared_ptr<Base>>(instance)();
    }

    /**
     * @throws std::runtime_error as we're unable to extract a non-shared ptr from the wrapped object
     */
    template<typename Base>
    Base * PythonObjectInstantiator<Base>::createUnwrappedInstance() const
    {
      throw std::runtime_error("Unable to create unwrapped instance of Python object");
    }

  }
}

#endif /* MANTID_PYTHONINTERFACE_PYTHONALGORITHMINSTANTIATOR_H_ */
