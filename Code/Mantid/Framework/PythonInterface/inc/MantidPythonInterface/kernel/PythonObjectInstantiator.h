#ifndef MANTID_PYTHONINTERFACE_PYTHONALGORITHMINSTANTIATOR_H_
#define MANTID_PYTHONINTERFACE_PYTHONALGORITHMINSTANTIATOR_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Instantiator.h"
#include "MantidAPI/IAlgorithm.h"

#include <boost/python/object.hpp>
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
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

    template<typename Base>
    class PythonObjectInstantiator : public Kernel::AbstractInstantiator<Base>
    {
    public:
      /// Constructor taking a Python class object wrapped as a boost::python:object
      PythonObjectInstantiator(boost::python::object classObject)
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
      return boost::shared_ptr<Base>(createUnwrappedInstance(), NoDelete<Base>());
    }

    /**
     * Creates an instance of the object as raw ptr to the Base type
     * @returns A raw ptr to Base.
     */
    template<typename Base>
    Base * PythonObjectInstantiator<Base>::createUnwrappedInstance() const
    {
      using namespace boost::python;
      PyObject *alg = PyObject_CallObject(m_classObject.ptr(), NULL); // No args
      object wrap(handle<>(borrowed(alg))); // borrowed: Do not decrement reference count on destruction
      Base *ptr = extract<Base*>(wrap);
      return ptr;
    }

  }
}

#endif /* MANTID_PythonInterface_PYTHONALGORITHMINSTANTIATOR_H_ */
