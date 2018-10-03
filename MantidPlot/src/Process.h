// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROCESS_H_
#define PROCESS_H_


/*
 * Access information about the process and others
 */

namespace Process {

unsigned int numberOfMantids();
long long getProcessID();
} // namespace Process
#endif // PROCESS_H_
