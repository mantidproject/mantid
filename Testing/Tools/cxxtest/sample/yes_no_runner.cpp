// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//
// A sample program that uses class YesNoRunner to run all the tests
// and find out if all pass.
//

#include <cxxtest/YesNoRunner.h>

int main()
{
    return CxxTest::YesNoRunner().run();
}
