#include "MantidKernel/PropertyWithValue.h"

namespace Mantid
{
namespace Kernel
{

/// @cond

template DLLExport class PropertyWithValue<int>;
template DLLExport class PropertyWithValue<bool>;
template DLLExport class PropertyWithValue<double>;
template DLLExport class PropertyWithValue<std::string>;

//template DLLExport class PropertyWithValue<std::vector<int> >;
template DLLExport class PropertyWithValue<std::vector<double> >;
template DLLExport class PropertyWithValue<std::vector<std::string> >;

/// @endcond

/// Takes a comma-separated string of values and stores them as the vector of values
template <>
std::string PropertyWithValue<std::vector<int> >::setValue(const std::string& value)
{
  try{
    // Split up comma-separated properties
    typedef Poco::StringTokenizer tokenizer;
    tokenizer values(value, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

    std::vector<int> vec;
    vec.reserve(values.count());

    for (tokenizer::Iterator it = values.begin(); it != values.end(); ++it)
    {
      if (it->find('-'))
      {
        tokenizer range(*it, "-", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
        if (range.count() == 1)
          vec.push_back( boost::lexical_cast<int>( *it ) );
        else
        {
          int iStart = boost::lexical_cast<int>( *range.begin() );
          int iEnd = boost::lexical_cast<int>( *(range.begin()+1) );
          for(int i=iStart;i<=iEnd;i++)
            vec.push_back(i);
        }
      }
      else
        vec.push_back( boost::lexical_cast<int>( *it ) );
    }
    if (vec.size() != m_value.size())
    {
      g_log.information("New property value has different number of elements");
    }
    m_value = vec;
    return "";
  }
  catch ( boost::bad_lexical_cast )
  {
    std::string error = "Could not set property " + name() +
      ". Can not convert \"" + value + "\" to " + type();
    g_log.debug() << error;
    return error;
  }
  catch ( std::invalid_argument& except)
  {
    g_log.debug() << "Could not set property " << name() << ": " << except.what();
    return except.what();
  }
}

} // namespace Kernel
} // namespace Mantid
