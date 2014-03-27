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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
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
     * fact that the type is only known at runtime. The exact base class is
     * specified as a template to allow flexiblity as to what Algorthm type
     * is being exported. The type specified by AlgorithmBase should be
     * API::Algorithm or inherit from the API::Algorithm class
     */
    template<typename BaseAlgorithm>
    class PythonAlgorithm : public BaseAlgorithm
    {
      /// Convenience typedef to access the base class
      typedef BaseAlgorithm SuperClass;

    public:
      /** @name Property declarations
       * The first function matches the base-classes signature so a different
       * name is used consistently to avoid accidentally calling the wrong function internally
       * From Python they will still be called declareProperty
       */
      ///@{
      /// Declare a specialized property
      void declarePyAlgProperty(Kernel::Property *prop, const std::string &doc="");
      /// Declare a property using the type of the defaultValue with a validator and doc string
      void declarePyAlgProperty(const std::string & name, const boost::python::object & defaultValue,
                                 const boost::python::object & validator = boost::python::object(),
                                 const std::string & doc = "", const int direction = Kernel::Direction::Input);

      /// Declare a property with a documentation string
      void declarePyAlgProperty(const std::string & name, const boost::python::object & defaultValue,
                                const std::string & doc, const int direction = Kernel::Direction::Input);

      /// Declare a property using the type of the defaultValue
      void declarePyAlgProperty(const std::string & name, const boost::python::object & defaultValue,
                                const int direction);
    ///@}
    private:
      // Hide the base class variants as they are not required on this interface
      using SuperClass::declareProperty;
    };

//    /**
//     * Declare a preconstructed property.
//     * @param prop :: A pointer to a property
//     * @param doc :: An optional doc string
//     */
//    template<typename BaseAlgorithm>
//    void PythonAlgorithm<BaseAlgorithm>::declarePyAlgProperty(Kernel::Property *prop, const std::string & doc)
//    {
//      // We need to clone the property so that python doesn't own the object that gets inserted
//      // into the manager
//      this->declareProperty(prop->clone(), doc);
//    }

//    /**
//     * Declare a property using the type of the defaultValue, a documentation string and validator
//     * @param name :: The name of the new property
//     * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
//     * @param validator :: A validator object
//     * @param doc :: The documentation string
//     * @param direction :: The direction of the property
//     */
//    template<typename BaseAlgorithm>
//    void PythonAlgorithm<BaseAlgorithm>::declarePyAlgProperty(const std::string & name, const boost::python::object & defaultValue,
//                                                              const boost::python::object & validator,
//                                                              const std::string & doc, const int direction)
//    {
//      this->declareProperty(Registry::PropertyWithValueFactory::create(name, defaultValue, validator, direction), doc);
//    }

//    /**
//     * Declare a property using the type of the defaultValue and a documentation string
//     * @param name :: The name of the new property
//     * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
//     * @param doc :: The documentation string
//     * @param direction :: The direction of the property
//     */
//    template<typename BaseAlgorithm>
//    void PythonAlgorithm<BaseAlgorithm>::declarePyAlgProperty(const std::string & name, const boost::python::object & defaultValue,
//                                                              const std::string & doc, const int direction)
//    {
//      this->declareProperty(Registry::PropertyWithValueFactory::create(name, defaultValue, direction), doc);
//    }

//    /**
//    * Declare a property using the type of the defaultValue
//    * @param name :: The name of the new property
//    * @param defaultValue :: A default value for the property. The type is mapped to a C++ type
//    * @param direction :: The direction of the property
//    */
//    template<typename BaseAlgorithm>
//    void PythonAlgorithm<BaseAlgorithm>::declarePyAlgProperty(const std::string & name, const boost::python::object & defaultValue,
//                                                              const int direction)
//    {
//      declarePyAlgProperty(name, defaultValue, "", direction);
//    }

  }
}



#endif /* MANTID_PYTHONINTERFACE_PYTHONALGORITHM_H_ */
