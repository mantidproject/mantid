// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//
// This program is used to check if the compiler supports __int64
//
int main()
{
    __int64 ll = 0;
    return (int)ll;
}

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//

