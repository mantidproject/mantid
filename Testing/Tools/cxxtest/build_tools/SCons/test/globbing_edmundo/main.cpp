// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
 * \file
 * Main function comes here.
 */
/****************************************************
 * Author: Edmundo LOPEZ
 * email:  lopezed5@etu.unige.ch
 *
 * **************************************************/

#include <hello.hh>
#include <iostream>

int main (int argc, char *argv[])
  {
    Hello h;
    std::cout << h.foo(2,3) << std::endl;
  }
