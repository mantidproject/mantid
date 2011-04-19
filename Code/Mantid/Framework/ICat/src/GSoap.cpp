//------------------------------------------------------------------------------
// This is a global 'include' file for the GSoap cpp files in order to 
// disable some warnings that we understand but do not want to touch as 
// it is 3rd party code
//------------------------------------------------------------------------------

#if defined(__GNUC__)
  #pragma GCC diagnostic ignored "-Wcast-qual"
  #pragma GCC diagnostic ignored "-Wconversion"
  #pragma GCC diagnostic ignored "-Wunused-parameter"
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
  #pragma GCC diagnostic ignored "-Wformat"
#elif defined(_WIN32)
  #pragma warning( disable: 4100 )
#endif

#include "GSoap/stdsoap2.cpp"
