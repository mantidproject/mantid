// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#ifdef __cplusplus
  extern "C" {
#endif

  typedef struct stack_t {
    int size;
    int* vals;
    int capacity;
  } stack_t;

  stack_t* stack_create();
  void     stack_free(stack_t* stack);
  int      stack_size(stack_t* stack);
  void     stack_push(stack_t* stack, int val);
  int      stack_pop(stack_t* stack);
  int      stack_peak(stack_t* stack);
  int      stack_capacity(stack_t* stack);

#ifdef __cplusplus
  }
#endif
