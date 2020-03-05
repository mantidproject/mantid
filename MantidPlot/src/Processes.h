// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROCESSES_H_
#define PROCESSES_H_

/*
 * Access information about the process and others
 */

namespace Processes {

unsigned int numberOfMantids();
long long getProcessID();
} // namespace Processes
#endif // PROCESSES_H_
