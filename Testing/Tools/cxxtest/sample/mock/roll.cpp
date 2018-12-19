// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <stdio.h>
#include "Dice.h"

int main()
{
    Dice dice;
    printf( "First roll: %u\n", dice.roll() );
    printf( "Second roll: %u\n", dice.roll() );
    
    return 0;
}
