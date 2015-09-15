#ifndef MANTID_MDALGORITHMS_MD_TRANSFORMATION_FACTORYIMPL_H
#define MANTID_MDALGORITHMS_MD_TRANSFORMATION_FACTORYIMPL_H

/* Used to register unit classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 *

 * The second operation that this macro performs is to provide the definition
 * of the unitID method for the concrete unit.

 * Second macro does the same thing as the first one, but allow to register the
 transformation
 * with a name, different form the class name and specified by the
 transformation ID.
 */
#define DECLARE_MD_TRANSF(classname)                                           \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper                                           \
      register_alg_##classname(((Mantid::MDAlgorithms::MDTransfFactory::Instance() \
                                     .subscribe<classname>(#classname)),       \
                                0));                                           \
  }                                                                            \
  const std::string Mantid::MDAlgorithms::classname::transfID() const {            \
    return #classname;                                                         \
  }

#define DECLARE_MD_TRANSFID(classname, regID)                                  \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper                                           \
      register_alg_##classname(((Mantid::MDAlgorithms::MDTransfFactory::Instance() \
                                     .subscribe<classname>(#regID)),           \
                                0));                                           \
  }                                                                            \
  const std::string Mantid::MDAlgorithms::classname::transfID() const {            \
    return #regID;                                                             \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/DllConfig.h"

#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

#include "MantidMDAlgorithms/MDTransfInterface.h"

namespace Mantid {
namespace MDAlgorithms {

/** Creates instances of concrete transformations into multidimensional (MD)
  coordinates.
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.
  *

    The factory is a singleton that hands out shared pointers to the base
  MDTransfornation class.
    It overrides the base class DynamicFactory::create method so that only a
  single
    instance of a given transformation is ever created, and a pointer to that
  same instance
    is passed out each time the transformation is requested.


    @date 17/05/2012

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class MANTID_MDALGORITHMS_DLL MDTransfFactoryImpl
    : public Kernel::DynamicFactory<MDTransfInterface> {
public:
  virtual boost::shared_ptr<MDTransfInterface>
  create(const std::string &className) const;

private:
  friend struct Kernel::CreateUsingNew<MDTransfFactoryImpl>;

  /// Private Constructor for singleton class
  MDTransfFactoryImpl();
  /// Private copy constructor - NO COPY ALLOWED
  MDTransfFactoryImpl(const MDTransfFactoryImpl &);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  MDTransfFactoryImpl &operator=(const MDTransfFactoryImpl &);
  /// Private Destructor
  virtual ~MDTransfFactoryImpl();

  /// Stores pointers to already created unit instances, with their name as the
  /// key
  mutable std::map<std::string, boost::shared_ptr<MDTransfInterface>>
      m_createdTransf;
};

/// Forward declaration of a specialization of SingletonHolder for
/// AlgorithmFactoryImpl (needed for dllexport/dllimport) .
#ifdef _WIN32
// this breaks new namespace declaration rules; need to find a better fix
template class MANTID_MDALGORITHMS_DLL
    Mantid::Kernel::SingletonHolder<MDTransfFactoryImpl>;
#endif /* _WIN32 */
/// The specialization of the SingletonHolder class that holds the
/// MDTransformations Factory
typedef Kernel::SingletonHolder<MDTransfFactoryImpl> MDTransfFactory;

} // namespace MDAlgorithms
} // namespace Mantid

#endif
