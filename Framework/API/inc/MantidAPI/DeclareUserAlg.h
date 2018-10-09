// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DECLAREUSERALG_H_
#define DECLAREUSERALG_H_

#define DECLARE_USER_ALG(x)                                                    \
  Algorithm *x##_create() { return new x; }                                    \
                                                                               \
  void x##_destroy(Algorithm *p) { delete p; }

#endif /*DECLAREUSERALG_H_*/
