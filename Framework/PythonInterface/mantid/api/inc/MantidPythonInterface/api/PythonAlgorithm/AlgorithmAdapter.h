// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_ALGORITHMADAPTER_H_
#define MANTID_PYTHONINTERFACE_ALGORITHMADAPTER_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

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
  using SuperClass = BaseAlgorithm;

public:
  /// A constructor that looks like a Python __init__ method
  AlgorithmAdapter(PyObject *self);

  /// Disable default constructor - The PyObject must be supplied to construct
  /// the object
  AlgorithmAdapter() = delete;

  /// Disable copy operator
  AlgorithmAdapter(const AlgorithmAdapter &) = delete;

  /// Disable assignment operator
  AlgorithmAdapter &operator=(const AlgorithmAdapter &) = delete;

  /** @name Algorithm virtual methods */
  ///@{
  /// Returns the name of the algorithm
  const std::string name() const override;
  /// Returns a version of the algorithm
  int version() const override;
  /// Returns the summary for the algorithm
  const std::string summary() const override;
  /// Returns a category of the algorithm.
  const std::string category() const override;
  /// Returns seeAlso related algorithms.
  const std::vector<std::string> seeAlso() const override;
  /// Returns optional documentation URL of the algorithm
  const std::string helpURL() const override;
  /// Allow the isRunning method to be overridden
  bool isRunning() const override;
  /// Allow the cancel method to be overridden
  void cancel() override;
  /// A return of false will allow processing workspace groups as a whole
  bool checkGroups() override;
  /// Returns the validateInputs result of the algorithm.
  std::map<std::string, std::string> validateInputs() override;
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
  /// Private init for this algorithm
  void init() override;
  /// Private exec for this algorithm
  void exec() override;

  /// We don't want the base class versions
  using SuperClass::declareProperty;

  /// The Python portion of the object
  PyObject *m_self;
  /// A pointer to an overridden isRunning method
  PyObject *m_isRunningObj;

  /// Here for deprecated setWikiSummary method
  std::string m_wikiSummary;
};
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_ALGORITHMADAPTER_H_ */
