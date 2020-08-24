// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//
// The StdioPrinter is an StdioFilePrinter which defaults to stdout.
// This should have been called StdOutPrinter or something, but the name
// has been historically used.
//

#include <cxxtest/StdioFilePrinter.h>

namespace CxxTest 
{
    class StdioPrinter : public StdioFilePrinter
    {
    public:
        StdioPrinter( FILE *o = stdout, const char *preLine = ":", const char *postLine = "" ) :
            StdioFilePrinter( o, preLine, postLine ) {}
    };
}
