#ifndef MANTID_DATAHANDLING_SAVEDIFFFITTINGASCII_H_
#define MANTID_DATAHANDLING_SAVEDIFFFITTINGASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace Mantid {
namespace DataHandling {

class DLLExport SaveDiffFittingAscii : public Mantid::API::Algorithm,
                                       public API::DeprecatedAlgorithm {
public:
  /// (Empty) Constructor
  SaveDiffFittingAscii();

  /// Algorithm's name
  const std::string name() const override { return "SaveDiffFittingAscii"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves the results to ASCII file after carrying out single peak "
           "fitting process "
           "or running "
           "EnggFitPeaks algorithm.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"EnggFitPeaks", "SaveAscii"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  /// Process two groups and ensure the Result string is set properly on the
  /// final algorithm
  bool processGroups() override;

  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

  /// Main exec routine, called for group or individual workspace processing.
  void processAll(const std::vector<API::ITableWorkspace_sptr> input_ws);

  std::vector<std::string> splitList(std::string strList);

  void writeInfo(const std::string &runNumber, const std::string &bank,
                 std::ofstream &file);

  void writeHeader(const std::vector<std::string> &columnHeadings,
                   std::ofstream &file);

  void writeData(const API::ITableWorkspace_sptr workspace, std::ofstream &file,
                 const size_t columnSize);

  void writeVal(const std::string &val, std::ofstream &file,
                const bool endline);

  /// the separator
  const char m_sep;

  /// table_counter
  int m_counter;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SAVEDIFFFITTINGASCII_H_  */
