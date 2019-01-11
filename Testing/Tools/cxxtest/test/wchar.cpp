// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//
// This program is used to check if the compiler supports basic_string<wchar_t>
//
#include <string>

int main()
{
    std::basic_string<wchar_t> s(L"s");
    return 0;
}

//
// Local Variables:
// compile-command: "perl test.pl"
// End:
//

