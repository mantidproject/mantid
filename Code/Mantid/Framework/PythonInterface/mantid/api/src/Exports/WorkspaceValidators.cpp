#include "MantidAPI/WorkspaceValidators.h" // They are all defined in one file so export them in one
#include "MantidPythonInterface/kernel/TypedValidatorExportMacro.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::TypedValidator;
using namespace boost::python;

/// This is the base TypedValidator for most of the WorkspaceValidators
void export_MatrixWorkspaceValidator()
{
  using Mantid::API::MatrixWorkspace_sptr;
  using Mantid::API::MatrixWorkspaceValidator;
  EXPORT_TYPEDVALIDATOR(MatrixWorkspace_sptr);
  class_<MatrixWorkspaceValidator, 
         bases<TypedValidator<MatrixWorkspace_sptr>>, 
         boost::noncopyable>("MatrixWorkspaceValidator", no_init)
    ;
}
namespace
{
/// Export a validator derived from a MatrixWorkspaceValidator that has a no-arg constructor
#define EXPORT_WKSP_VALIDATOR_NO_ARG(ValidatorType, DocString) \
  class_<ValidatorType, \
          bases<MatrixWorkspaceValidator>, \
          boost::noncopyable\
         >(#ValidatorType, \
          init<>(DocString))\
   ;
/// Export a validator derived from a MatrixWorkspaceValidator that has a single-arg constructor
#define EXPORT_WKSP_VALIDATOR_ARG(ValidatorType, ArgType, ArgName, DocString) \
   class_<ValidatorType, \
          bases<MatrixWorkspaceValidator>, \
          boost::noncopyable\
         >(#ValidatorType, \
          init<ArgType>(arg(ArgName), DocString))\
   ;
/// Export a validator derived from a MatrixWorkspaceValidator that has a single-arg constructor 
/// with a default argument
#define EXPORT_WKSP_VALIDATOR_DEFAULT_ARG(ValidatorType, ArgType, ArgName, DefaultValue, DocString) \
   class_<ValidatorType, \
          bases<MatrixWorkspaceValidator>, \
          boost::noncopyable\
         >(#ValidatorType, \
          init<ArgType>(arg(ArgName)=DefaultValue, DocString))\
   ;

}

void export_WorkspaceValidators()
{
  using namespace Mantid::API;
  
  EXPORT_WKSP_VALIDATOR_ARG(WorkspaceUnitValidator, std::string, "unit", "Checks the workspace has the given unit along the X-axis");
  EXPORT_WKSP_VALIDATOR_DEFAULT_ARG(HistogramValidator, bool, "mustBeHistogram", true,
                                    "If mustBeHistogram=True then the workspace must be a histogram otherwise it must be point data.");
  EXPORT_WKSP_VALIDATOR_DEFAULT_ARG(RawCountValidator, bool, "mustNotBeDistribution", true,
                                    "If mustNotBeDistribution=True then the workspace must not have been divided by the bin-width");
  EXPORT_WKSP_VALIDATOR_NO_ARG(CommonBinsValidator, "A tentative check that the bins are common across the workspace");
  EXPORT_WKSP_VALIDATOR_DEFAULT_ARG(SpectraAxisValidator, int, "axisNumber", 1,
                                    "Checks whether the axis specified by axisNumber is a SpectraAxis");
  EXPORT_WKSP_VALIDATOR_DEFAULT_ARG(NumericAxisValidator, int, "axisNumber", 1,
                                    "Checks whether the axis specified by axisNumber is a NumericAxis");

}

