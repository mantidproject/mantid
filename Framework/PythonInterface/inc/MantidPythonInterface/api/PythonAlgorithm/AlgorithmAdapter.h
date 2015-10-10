#ifndef MANTID_PYTHONINTERFACE_ALGORITHMADAPTER_H_
#define MANTID_PYTHONINTERFACE_ALGORITHMADAPTER_H_
/**
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

#include "MantidKernel/ClassMacros.h"
#include <boost/python/wrapper.hpp>
#include <map>

namespace Mantid {
namespace PythonInterface {

/**
 * Provides a layer class for boost::python to allow C++ virtual functions
 * to be overridden in a Python object that is derived an Algorithm.
 *
 * The templated-base class provides a mechanism to reuse the same adapter
 * class for other classes that inherit from a different Algorithm sub class
 */
template <typename BaseAlgorithm>
class AlgorithmAdapter : public BaseAlgorithm {
  typedef BaseAlgorithm SuperClass;

public:
  /// A constructor that looks like a Python __init__ method
  AlgorithmAdapter(PyObject *self);

  /** @name Algorithm virtual methods */
  ///@{
  /// Returns the name of the algorithm
  const std::string name() const;
  /// Returns a version of the algorithm
  virtual int version() const;
  /// A default version, chosen if there is no override
  int defaultVersion() const;
  /// Returns the summary for the algorithm
  virtual const std::string summary() const;
  /// Returns the summary for the algorithm
  std::string defaultSummary() const;
  /// Returns a category of the algorithm.
  virtual const std::string category() const;
  /// A default category, chosen if there is no override
  std::string defaultCategory() const;
  /// Allow the isRunning method to be overridden
  virtual bool isRunning() const;
  /// Allow the cancel method to be overridden
  virtual void cancel();
  /// A return of false will allow processing workspace groups as a whole
  virtual bool checkGroups();
  /// A default value for checkGroups, chosen if there is no override
  bool checkGroupsDefault();
  /// Returns the validateInputs result of the algorithm.
  std::map<std::string, std::string> validateInputs();
  ///@}

  // -- Deprecated methods --
  /// Set the summary text
  void setWikiSummary(const std::string &summary);

  /** @name Property declarations
   * The first function matches the base-classes signature so a different
   * name is used consistently to avoid accidentally calling the wrong function
   * internally
   * From Python they will still be called declareProperty
   */
  ///@{
  /// Declare a specialized property
  static void declarePyAlgProperty(boost::python::object &self,
                                   Kernel::Property *prop,
                                   const std::string &doc = "");
  /// Declare a property using the type of the defaultValue with a validator and
  /// doc string
  static void declarePyAlgProperty(
      boost::python::object &self, const std::string &name,
      const boost::python::object &defaultValue,
      const boost::python::object &validator = boost::python::object(),
      const std::string &doc = "",
      const int direction = Kernel::Direction::Input);

  /// Declare a property with a documentation string
  static void
  declarePyAlgProperty(boost::python::object &self, const std::string &name,
                       const boost::python::object &defaultValue,
                       const std::string &doc,
                       const int direction = Kernel::Direction::Input);

  /// Declare a property using the type of the defaultValue
  static void declarePyAlgProperty(boost::python::object &self,
                                   const std::string &name,
                                   const boost::python::object &defaultValue,
                                   const int direction);

protected:
  /**
   *  Returns the PyObject that owns this wrapper, i.e. self
   * @returns A pointer to self
   */
  inline PyObject *getSelf() const { return m_self; }

private:
  /// The PyObject must be supplied to construct the object
  DISABLE_DEFAULT_CONSTRUCT(AlgorithmAdapter)
  DISABLE_COPY_AND_ASSIGN(AlgorithmAdapter)

  /// Private init for this algorithm
  virtual void init();
  /// Private exec for this algorithm
  virtual void exec();

  /// We don't want the base class versions
  using SuperClass::declareProperty;

  /// The Python portion of the object
  PyObject *m_self;
  /// A pointer to an overridden isRunning method
  PyObject *m_isRunningObj;

  /// Here for deprecated setWikiSummary method
  std::string m_wikiSummary;
};
}
}

#endif /* MANTID_PYTHONINTERFACE_ALGORITHMADAPTER_H_ */
