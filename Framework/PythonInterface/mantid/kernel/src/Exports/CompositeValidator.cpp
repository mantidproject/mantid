// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/CompositeValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::CompositeRelation;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::IValidator;
using Mantid::Kernel::IValidator_sptr;
using namespace boost::python;

namespace {
/**
 * Creates a composite validator from a python list of validators
 */
CompositeValidator *createCompositeValidator(const boost::python::list &validators, const CompositeRelation &relation) {
  namespace bpl = boost::python;
  auto composite = new CompositeValidator(relation);
  const bpl::ssize_t nitems = bpl::len(validators);
  for (bpl::ssize_t i = 0; i < nitems; ++i) {
    try {
      composite->add(bpl::extract<IValidator_sptr>(validators[i]));
    } catch (boost::python::error_already_set &) {
      std::stringstream os;
      os << "Cannot extract Validator from element " << i;
      throw std::invalid_argument(os.str());
    }
  }
  return composite;
}
} // namespace

void export_CompositeValidator() {
  enum_<CompositeRelation>("CompositeRelation").value("AND", CompositeRelation::AND).value("OR", CompositeRelation::OR);

  class_<CompositeValidator, bases<IValidator>, boost::noncopyable>("CompositeValidator")
      .def("__init__", make_constructor(&createCompositeValidator, default_call_policies(),
                                        (arg("validators"), arg("relation") = CompositeRelation::AND)))
      .def("add", (void(CompositeValidator::*)(const IValidator_sptr &)) & CompositeValidator::add,
           (arg("self"), arg("other")), "Add another validator to the list");
}
