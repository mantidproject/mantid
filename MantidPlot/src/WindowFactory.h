// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef WINDOWFACTORY_H
#define WINDOWFACTORY_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/CaseInsensitiveMap.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/SingletonHolder.h"

#include "MantidQtWidgets/Common/IProjectSerialisable.h"

#include <cstring>
#include <iterator>
#include <map>
#include <vector>

#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class ApplicationWindow;

namespace Mantid {
namespace API {

/** Abstract base instantiator object.
 *
 * This is required to hold the templated base type for dynamic instantiation
 * after subscription.
 */
template <class Base> class AbstractProjectInstantiator {
public:
  /// Creates the AbstractInstantiator.
  AbstractProjectInstantiator() = default;
  /// Destroys the AbstractProjectInstantiator.
  virtual ~AbstractProjectInstantiator() = default;
  /// Load a class of type Base from a serialised form
  virtual Base *loadFromProject(const std::string &lines,
                                ApplicationWindow *app,
                                const int fileVersion = -1) const = 0;

private:
  /// Private copy constructor
  AbstractProjectInstantiator(const AbstractProjectInstantiator &);
  /// Private assignment operator
  AbstractProjectInstantiator &operator=(const AbstractProjectInstantiator &);
};

// A template class for the easy instantiation of
// instantiators with non default constructors.
//
// Ideally this would just be implemented within the Instantiator class
// however due both methods being virtual they cannot be templated.
template <class C, class Base>
class ProjectWindowInstantiator : public AbstractProjectInstantiator<Base> {
public:
  /// Creates the Instantiator.
  ProjectWindowInstantiator() = default;
  /** Creates an instance of a concrete subclass of Base from its serialised
   * form.
   */
  Base *loadFromProject(const std::string &lines, ApplicationWindow *app,
                        const int fileVersion = -1) const override {
    return C::loadFromProject(lines, app, fileVersion);
  }
};

/** The WindowFactory class is in charge of the creation of concrete
    instances of MantidQt::API::IProjectSerialisable.

    It is implemented as a singleton class.

    @author Samuel Jackson, ISIS Rutherford Appleton Laboratory
    @date 11/07/2016
*/

class WindowFactoryImpl final {
private:
  using AbstractFactory =
      AbstractProjectInstantiator<MantidQt::API::IProjectSerialisable>;

public:
  WindowFactoryImpl();

  /** Load a window from its serialised form.
   *
   * @param className :: the name of the class to instantiate
   * @param lines :: the string containing the serialised form of the window
   * @param app :: handle to the ApplicationWindow instance
   * @param fileVersion :: the file version number
   * @return an instance of MantidQt::API::IProjectSerialisable
   */
  MantidQt::API::IProjectSerialisable *
  loadFromProject(const std::string &className, const std::string &lines,
                  ApplicationWindow *app, const int fileVersion = -1) {
    auto it = _map.find(className);
    if (it != _map.end())
      return it->second->loadFromProject(lines, app, fileVersion);
    else
      throw Kernel::Exception::NotFoundError(
          "WindowFactory: " + className + " is not registered.\n", className);
  }

  /** Subscribe a class to the factory
   *
   * @param className :: the name of the class to subscribe
   */
  template <class C> void subscribe(const std::string &className) {
    auto instantiator = std::make_unique<
        ProjectWindowInstantiator<C, MantidQt::API::IProjectSerialisable>>();
    subscribe(className, std::move(instantiator));
  }

  /** Get all of the keys for the classes subscribed to the factory
   *
   * @return a vector of strings for classes subscribed to the factory.
   */
  virtual const std::vector<std::string> getKeys() const {
    std::vector<std::string> names;
    names.reserve(_map.size());
    std::transform(
        _map.cbegin(), _map.cend(), std::back_inserter(names),
        [](const std::pair<const std::string, std::unique_ptr<AbstractFactory>>
               &mapPair) { return mapPair.first; });
    return names;
  }

private:
  /** Subscribe a class to the factory.
   *
   * @param className :: the name of the class to subscribe
   * @param instantiator :: the instantiator object to used to create the object
   */
  void subscribe(const std::string &className,
                 std::unique_ptr<AbstractFactory> instantiator) {
    if (className.empty()) {
      throw std::invalid_argument("Cannot register empty class name");
    }

    auto it = _map.find(className);
    if (it == _map.end()) {
      _map[className] = std::move(instantiator);
    } else {
      throw std::runtime_error(className + " is already registered.\n");
    }
  }

  /// A typedef for the map of registered classes
  using FactoryMap =
      Mantid::Kernel::CaseInsensitiveMap<std::unique_ptr<AbstractFactory>>;
  /// The map holding the registered class names and their instantiators
  FactoryMap _map;
};

#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class Mantid::Kernel::SingletonHolder<WindowFactoryImpl>;
#endif /* _WIN32 */

using WindowFactory = Mantid::Kernel::SingletonHolder<WindowFactoryImpl>;
} // namespace API
} // namespace Mantid

#endif // WINDOWFACTORY_H
