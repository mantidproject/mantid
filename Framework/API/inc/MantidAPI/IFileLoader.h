// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IFILELOADER_H_
#define MANTID_API_IFILELOADER_H_

#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/NexusDescriptor.h"

namespace Mantid {
namespace API {

/**
Defines an interface to an algorithm that loads a file so that it can take part
in
the automatic selection procedure provided by the FileLoaderRegistry.
 */
template <typename DescriptorType>
class MANTID_API_DLL IFileLoader : public ParallelAlgorithm {
public:
  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(DescriptorType &descriptor) const = 0;
  /// Returns a value indicating whether or not loader wants to load multiple
  /// files into a single workspace
  virtual bool loadMutipleAsOne() { return false; }
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_IFILELOADER_H_ */
