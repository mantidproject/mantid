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
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/kernel/Environment/Threading.h"

#include <boost/python/object.hpp>
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    ///@cond
    /**
     * A custom deleter to be used with shared_ptr objects that wrap objects
     * instantiated in Python
     */
    template<typename T>
    struct NoDelete
    {
    public:
      void operator()(T const *) // No-op as ptr was not created with new
      {}
    };
    ///@endcond


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
      // A custom deleter is required since calling delete is not what we want
      return boost::shared_ptr<Base>(createUnwrappedInstance());
    }

    /**
     * Creates an instance of the object as raw ptr to the Base type
     * @returns A raw ptr to Base.
     */
    template<typename Base>
    Base * PythonObjectInstantiator<Base>::createUnwrappedInstance() const
    {
      using namespace boost::python;
      Environment::GlobalInterpreterLock gil;

      PyObject *newObj = PyObject_CallObject(m_classObject.ptr(), NULL); // No args
      // borrowed: Do not decrement reference count on destruction
      // we are passing out a new reference for something else to manage
      object wrap(handle<>(borrowed(newObj)));
      try
      {
        // In Python the actual object type is different to that of the main exported
        // type. This is required so that Python we can take over lifetime management of
        // the object when we create it here
        // See http://wiki.python.org/moin/boost.python/HowTo#ownership_of_C.2B-.2B-_object_extended_in_Python
        boost::shared_ptr<Base> newObj = extract<boost::shared_ptr<Base>>(object(wrap));
        Base *barePtr = newObj.get();
        newObj.reset(); // We take ownership
        return barePtr;
      }
      catch(boost::python::error_already_set&)
      {
        throw std::runtime_error("PythonObjectInstantiator::createUnwrapped - Could not extract boost::shared_ptr<> to base type. "
                                 "Please make sure the HeldType in the Python wrapper is boost::shared_ptr<Base>.");
      }
    }

  }
}

#endif /* MANTID_PythonInterface_PYTHONALGORITHMINSTANTIATOR_H_ */
