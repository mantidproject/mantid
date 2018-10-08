// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef __cxxtest_CrazyRunner_h__
#define __cxxtest_CrazyRunner_h__


/*
 * This is not a proper runner. Just a simple class that looks like one.
 */
namespace CxxTest {
	class CrazyRunner {
		public:
			int run() { return 0; }
            void process_commandline(int argc, char** argv) { }
	};
}

#endif
