#include "MantidPythonInterface/kernel/PropertyWithValue.h"

void export_BasicPropertyWithValueTypes()
{
  #define EXPORT_BASIC_INTEGER_TYPE(ctype, export_name)\
    EXPORT_PROP_W_VALUE(ctype, export_name);\
    EXPORT_PROP_W_VALUE(std::vector<ctype>, export_name);\
    EXPORT_PROP_W_VALUE(unsigned ctype, unsigned ## _export_name);\
    EXPORT_PROP_W_VALUE(std::vector<unsigned ctype>, vector_ ## export_name);

  #define EXPORT_BASIC_TYPE(ctype, export_name)\
    EXPORT_PROP_W_VALUE(ctype, export_name);\
    EXPORT_PROP_W_VALUE(std::vector<ctype>, vector_ ## export_name);

  // -- PropertyWithValueTypes
  EXPORT_BASIC_INTEGER_TYPE(int, int);
  EXPORT_BASIC_INTEGER_TYPE(long, long);
  EXPORT_BASIC_INTEGER_TYPE(long long, long_long);

  EXPORT_BASIC_TYPE(double, double);
  EXPORT_BASIC_TYPE(bool, bool);
  EXPORT_BASIC_TYPE(std::string, string);
}


