// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once


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
