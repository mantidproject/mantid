// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_METADATAEXTRACTORUTILS_H_
#define MANTID_VATES_METADATAEXTRACTORUTILS_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include <string>

/**
 * Class with utility methdos to extract meta data information from a
 *IMDWorkspace.
 *
 * @date November 21, 2014
 *
 *
 */
namespace Mantid {
namespace VATES {

class DLLExport MetaDataExtractorUtils {
public:
  MetaDataExtractorUtils();

  ~MetaDataExtractorUtils();

  /**
   * Extracts the instrument from the workspace.
   * @param workspace A pointer to a workspace.
   * @returns The instrument.
   */
  std::string extractInstrument(const Mantid::API::IMDWorkspace *workspace);
};
} // namespace VATES
} // namespace Mantid
#endif
