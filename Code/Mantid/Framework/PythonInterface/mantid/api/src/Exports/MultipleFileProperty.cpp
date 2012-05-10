#include "MantidAPI/MultipleFileProperty.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"
#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/str.hpp>

using Mantid::API::MultipleFileProperty;
using Mantid::Kernel::PropertyWithValue;
using namespace boost::python;

namespace
{
  /// The PropertyWithValue type
  typedef std::vector<std::vector<std::string>> HeldType;

  /**
   * Converts the value from a MultipleFileProperty to a python object rather than using a vector
   * @param self :: A reference to the calling object
   * @returns A string is there is only a single string in the Property's value, and a list if there are multiple ones
   */
  boost::python::object valueAsPyObject(MultipleFileProperty & self)
  {
    const HeldType & propValue = self();
    boost::python::object result;
    if(propValue.size() == 1 && propValue[0].size() == 1) // Special case, unwrap the vector
    {
      result = boost::python::str(propValue[0][0]);
    }
    else
    {
      // Build a list of lists to mimic the behaviour of MultipleFileProperty
      boost::python::list fileList;
      for(auto outerItr = propValue.begin(); outerItr != propValue.end(); ++outerItr)
      {
        const std::vector<std::string> & filenames = *outerItr;
        if(filenames.size() == 1)
        {
          fileList.append(filenames.front());
        }
        else
        {
          boost::python::list groupList;
          for(auto innerItr = filenames.begin(); innerItr != filenames.end(); ++innerItr)
          {
            groupList.append(*innerItr);
          }
          fileList.append(groupList);
        }
      }
      result = fileList;
    }

    return result;
  }
}

void export_MultipleFileProperty()
{
  typedef PropertyWithValue<HeldType> BaseClass;
  EXPORT_PROP_W_VALUE(HeldType,vec_vec_str);

  class_<MultipleFileProperty, bases<BaseClass>, boost::noncopyable>("MultipleFileProperty", no_init)
    // Override the base class one to do something more appropriate
    .add_property("value", &valueAsPyObject)
  ;
}

