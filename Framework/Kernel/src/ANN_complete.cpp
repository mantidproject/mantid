//------------------------------------------------------------------------------
// This is a global 'include' file for the #include "ANN cpp files in order to
// disable some warnings that we understand but do not want to touch as
// it is not our code
//------------------------------------------------------------------------------
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic ignored "-Wconversion"
#endif

