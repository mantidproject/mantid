#ifndef MANTID_CUSTOMINTERFACES_REFLVECTORSTRING_H
#define MANTID_CUSTOMINTERFACES_REFLVECTORSTRING_H

namespace MantidQt {
  namespace CustomInterfaces {

    /**
    Create string of comma separated list of values from a vector
    @param param_vec : vector of values
    @return string of comma separated list of values
    */
    template<typename T, typename A>
    std::string vectorString(const std::vector<T,A> &param_vec)
    {
      std::ostringstream vector_string;
      const char* separator = "";
      for(auto paramIt = param_vec.begin(); paramIt != param_vec.end(); ++paramIt)
      {
        vector_string << separator << *paramIt;
        separator = ", ";
      }

      return vector_string.str();
    }

    /**
    Create string of comma separated list of parameter values from a vector
    @param param_name : name of the parameter we are creating a list of
    @param param_vec : vector of parameter values
    @return string of comma separated list of parameter values
    */
    template<typename T, typename A>
    std::string vectorParamString(const std::string & param_name, const std::vector<T,A> &param_vec)
    {
      std::ostringstream param_vector_string;

      param_vector_string << param_name << " = '";
      param_vector_string << vectorString(param_vec);
      param_vector_string << "'";

      return param_vector_string.str();
    }

  }
}


#endif //MANTID_CUSTOMINTERFACES_REFLVECTORSTRING_H
