// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#if defined(_MSC_FULL_VER) && _MSC_FULL_VER > 190023918 && _MSC_FULL_VER < 191125506
// Visual Studio Update 3 refuses to link boost python exports that use
// register_ptr_to_python with a virtual base. This is a work around
#define GET_POINTER_SPECIALIZATION(TYPE)                                                                               \
  namespace boost {                                                                                                    \
  template <> const volatile TYPE *get_pointer(const volatile TYPE *p) { return p; }                                   \
  }
#else
#define GET_POINTER_SPECIALIZATION(TYPE)
#endif
