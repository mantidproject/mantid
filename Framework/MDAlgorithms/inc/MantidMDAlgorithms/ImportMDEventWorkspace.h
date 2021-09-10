// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include <deque>

namespace Mantid {
namespace MDAlgorithms {

/** ImportMDEventWorkspace : Loads a file containing dimensionality and data for
  an MDEventWorkspace. Handles either full mdevents for mdleanevents as input
  data types.

  @date 2012-07-11
*/
class DLLExport ImportMDEventWorkspace : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Reads an ASCII file containing MDEvent data and constructs an "
           "MDEventWorkspace.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ImportMDHistoWorkspace"}; }
  const std::string category() const override;

  /// Flag used to indicate the dimension block in the file
  static const std::string DimensionBlockFlag();
  /// Flag used to indicate the mdevent block in the file
  static const std::string MDEventBlockFlag();
  /// Flag used to indicate a comment line.
  static const std::string CommentLineStartFlag();

private:
  /// Typdef for the white-space separated file data type.
  using DataCollectionType = std::deque<std::string>;
  /// All read-in data.
  DataCollectionType m_file_data;
  /// Iterator for the dimensionality start position.
  DataCollectionType::iterator m_posDimStart;
  /// Iterator for the mdevent data start position.
  DataCollectionType::iterator m_posMDEventStart;
  /// Possible Event Types
  enum MDEventType { Lean, Full, NotSpecified };
  /// Flag indicating whether full md events for lean events will be generated.
  bool m_IsFullDataObjects = false;
  /// Actual number of dimensions specified
  size_t m_nDimensions = 0;
  /// Actual number of md events provided.
  size_t m_nDataObjects = 0;
  /// call back to add event data
  template <typename MDE, size_t nd> void addEventsData(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
  /// Quick check of the structure, so we can abort if passed junk.
  void quickFileCheck();
  ///  Check that the a flag exists in the file.
  bool fileDoesContain(const std::string &flag);

  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
