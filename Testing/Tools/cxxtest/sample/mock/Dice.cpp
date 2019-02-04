// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <T/stdlib.h>
#include "Dice.h"

Dice::Dice()
{
    T::srand( T::time( 0 ) );
}

unsigned Dice::roll()
{
    return (T::rand() % 6) + 1;
}


