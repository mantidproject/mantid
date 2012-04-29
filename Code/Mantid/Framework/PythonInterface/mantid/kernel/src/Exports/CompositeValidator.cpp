#include "MantidKernel/CompositeValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/default_call_policies.hpp>

using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::IValidator;
using Mantid::Kernel::IValidator_sptr;
using namespace boost::python;

namespace
{
  /**
   * Creates a composite validator from a python list of validators
   */
  CompositeValidator * createCompositeValidator(const boost::python::list & validators)
  {
    namespace bpl = boost::python;
    CompositeValidator *composite = new CompositeValidator;
    const bpl::ssize_t nitems = bpl::len(validators);
    for(bpl::ssize_t i = 0; i < nitems; ++i)
    {
      try
      {
        composite->add(bpl::extract<IValidator_sptr>(validators[i]));
      }
      catch(boost::python::error_already_set &)
      {
        std::stringstream os;
        os << "Cannot extract Validator from element " << i;
        throw std::invalid_argument(os.str());
      }
    }
    return composite;
  }
}

void export_CompositeValidator()
{
  class_<CompositeValidator, bases<IValidator>, boost::noncopyable>("CompositeValidator")
    .def("__init__", make_constructor(&createCompositeValidator, default_call_policies(),
                                      (arg("validators"))
                                      ))

    .def("add", (void (CompositeValidator::*)(IValidator_sptr))&CompositeValidator::add, "Add another validator to the list")
  ;
}

