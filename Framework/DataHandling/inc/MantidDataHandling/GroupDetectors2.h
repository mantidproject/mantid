#ifndef MANTID_DATAHANDLING_GROUPDETECTORS2_H_
#define MANTID_DATAHANDLING_GROUPDETECTORS2_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/StringTokenizer.h"

#include <map>

namespace Mantid {
namespace DataHandling {
/** An algorithm for grouping detectors and their associated spectra into
  single spectra and DetectorGroups.
    This algorithm can only be used on a workspace that has common X bins.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
  result </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> MapFile - A file that containing lists of spectra to group, its format
  is given below</LI>
    <LI> SpectraList - An array containing a list of the indexes of the spectra
  to combine</LI>
    <LI> DetectorList - An array of detector ID's</LI>
    <LI> WorkspaceIndexList - An array of workspace indices to combine</LI>
    <LI> KeepUngroupedSpectra -  If true ungrouped spectra will be copied to the
  output workspace</LI>
    </UL>

    Any input file must have the following format:

  |      The format of the grouping file each phrase in "" is replaced by
  | a single integer (ignore all | and my insersions in []). Extra space
  | and comments starting with # are both allowed

  | "unused number"                                         |[in some
  implementations this is the number of groups in the file but here all groups
  in the file are read regardless]
  | "unused number"                                         |[a positive integer
  must be here but the group's spectra number is the spectra number of the first
  spectra that went into the group and its index number is the number of groups
  in the list before it]
  | "number_of_input_spectra1"                              |[this number must
  equal the number of spectra numbers on the next lines]
  | "input spec1" "input spec2" "input spec3" "input spec4" |[these spectra
  numbers can be listed over any number of lines]
  | "input spec5 input spec6"                               |
  ------------------------------------------------------------------------
  | "unused number"                                         |[this section of
  the file is repeated once for each group to form]
  | "number_of_input_spectra2"                              |[not all the input
  sectra have to be included in a group
  | "input spec1" "input spec2" "input spec3" "input spec4" |
  | "input spec5 input spec6"                               |
  ------------------------------------------------------------------------

  An example of an input file follows:
  2
  1
  64
  1 2 3 4 5 6 7 8 9 10
  11 12 13 14 15 16 17 18 19 20
  21 22 23 24 25 26 27 28 29 30
  31 32 33 34 35 36 37 38 39 40
  41 42 43 44 45 46 47 48 49 50
  51 52 53 54 55 56 57 58 59 60
  61 62 63 64
  2
  64
  65 66 67 68 69 70 71 72 73 74
  75 76 77 78 79 80 81 82 83 84
  85 86 87 88 89 90 91 92 93 94
  95 96 97 98 99 100 101 102 103 104
  105 106 107 108 109 110 111 112 113 114
  115 116 117 118 119 120 121 122 123 124
  125 126 127 128

    @author Steve Williams and Russell Taylor (Tessella Support Services plc)
    @date 27/07/2009

    Copyright &copy; 2008-11 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport GroupDetectors2 : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GroupDetectors"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Sums spectra bin-by-bin, equivalent to grouping the data from a "
           "set of detectors.  Individual groups can be specified by passing "
           "the algorithm a list of spectrum numbers, detector IDs or "
           "workspace indices. Many spectra groups can be created in one "
           "execution via an input file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; };
  const std::vector<std::string> seeAlso() const override {
    return {"SpatialGrouping"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Grouping"; }

  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// provides a function that expands pairs of integers separated with a hyphen
  /// into a list of all the integers between those values
  class RangeHelper {
  public:
    static void getList(const std::string &line, std::vector<size_t> &outList);

  private:
    /// this class can't be constructed it is just a holder for some static
    /// things
    RangeHelper() = default;
    /// give an enum from poco a better name here
    enum {
      IGNORE_SPACES =
          Mantid::Kernel::StringTokenizer::TOK_TRIM |
          Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY ///< equal to
      /// Mantid::Kernel::StringTokenizer::TOK_TRIM but
      /// saves some typing
    };
  };

  /// used to store the lists of WORKSPACE INDICES that will be grouped, the
  /// keys are not used
  using storage_map = std::map<specnum_t, std::vector<size_t>>;

  /// An estimate of the percentage of the algorithm runtimes that has been
  /// completed
  double m_FracCompl = 0.0;
  /// stores lists of spectra indexes to group, although we never do an index
  /// search on it
  storage_map m_GroupWsInds;

  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
  void execEvent();

  /// read in the input parameters and see what findout what will be to grouped
  void getGroups(API::MatrixWorkspace_const_sptr workspace,
                 std::vector<int64_t> &unUsedSpec);
  /// gets the list of spectra _index_ _numbers_ from a file of _spectra_
  /// _numbers_
  void processFile(const std::string &fname,
                   API::MatrixWorkspace_const_sptr workspace,
                   std::vector<int64_t> &unUsedSpec);
  /// gets groupings from XML file
  void processXMLFile(const std::string &fname,
                      API::MatrixWorkspace_const_sptr workspace,
                      std::vector<int64_t> &unUsedSpec);
  void
  processGroupingWorkspace(DataObjects::GroupingWorkspace_const_sptr groupWS,
                           API::MatrixWorkspace_const_sptr workspace,
                           std::vector<int64_t> &unUsedSpec);
  void processMatrixWorkspace(API::MatrixWorkspace_const_sptr groupWS,
                              API::MatrixWorkspace_const_sptr workspace,
                              std::vector<int64_t> &unUsedSpec);
  /// used while reading the file turns the string into an integer number (if
  /// possible), white space and # comments ignored
  int readInt(const std::string &line);

  void readFile(const spec2index_map &specs2index, std::istream &File,
                size_t &lineNum, std::vector<int64_t> &unUsedSpec,
                const bool ignoreGroupNumber);

  /// used while reading the file reads reads spectra numbers from the string
  /// and returns spectra indexes
  void readSpectraIndexes(const std::string &line,
                          const spec2index_map &specs2index,
                          std::vector<size_t> &output,
                          std::vector<int64_t> &unUsedSpec,
                          const std::string &seperator = "#");

  /// Estimate how much what has been read from the input file constitutes
  /// progress for the algorithm
  double fileReadProg(
      DataHandling::GroupDetectors2::storage_map::size_type numGroupsRead,
      DataHandling::GroupDetectors2::storage_map::size_type numInHists);

  /// Copy the and combine the histograms that the user requested from the input
  /// into the output workspace
  size_t formGroups(API::MatrixWorkspace_const_sptr inputWS,
                    API::MatrixWorkspace_sptr outputWS, const double prog4Copy,
                    const bool keepAll, const std::set<int64_t> &unGroupedSet,
                    Indexing::IndexInfo &indexInfo);
  /// Copy the and combine the event lists that the user requested from the
  /// input into the output workspace
  size_t formGroupsEvent(DataObjects::EventWorkspace_const_sptr inputWS,
                         DataObjects::EventWorkspace_sptr outputWS,
                         const double prog4Copy);

  /// Returns true if detectors exists and is masked
  bool isMaskedDetector(const API::SpectrumInfo &detector,
                        const size_t index) const;

  /// Copy the ungrouped spectra from the input workspace to the output
  template <class TIn, class TOut>
  void moveOthers(const std::set<int64_t> &unGroupedSet, const TIn &inputWS,
                  TOut &outputWS, size_t outIndex);

  /// flag values
  enum {
    USED = 1000 - INT_MAX, ///< goes in the unGrouped spectra list to say that a
    /// spectrum will be included in a group, any other
    /// value and it isn't. Spectra numbers should always
    /// be positive so we shouldn't accidientally set a
    /// spectrum number to this
    EMPTY_LINE = 1001 - INT_MAX, ///< when reading from the input file this
    /// value means that we found any empty line
    IGNORE_SPACES = Mantid::Kernel::StringTokenizer::TOK_TRIM ///< equal to
    /// Mantid::Kernel::StringTokenizer::TOK_TRIM but
    /// saves some typing
  };

  static const double CHECKBINS; ///< a (worse case) estimate of the time
  /// required to check that the X bin boundaries
  /// are the same as a percentage of total
  /// algorithm run time
  static const double
      OPENINGFILE; ///< gives the progress bar a nudge when the file opens
  static const double READFILE; ///< if a file must be read in estimate that
  /// reading it will take this percentage of the
  /// algorithm execution time
  static const int INTERVAL = 128; ///< copy this many histograms and then check
                                   /// for an algorithm notification and update
                                   /// the progress bar
};

/**
 *  Only to be used if the KeepUnGrouped property is true, moves the spectra
 * that were not selected to be in a group to the end of the output spectrum
 *  @param unGroupedSet :: list of WORKSPACE indexes that were included in a
 * group
 *  @param inputWS :: user selected input workspace for the algorithm
 *  @param outputWS :: user selected output workspace for the algorithm
 *  @param outIndex :: the next spectra index available after the grouped
 * spectra
 */
template <class TIn, class TOut>
void GroupDetectors2::moveOthers(const std::set<int64_t> &unGroupedSet,
                                 const TIn &inputWS, TOut &outputWS,
                                 size_t outIndex) {
  g_log.debug() << "Starting to copy the ungrouped spectra\n";
  double prog4Copy = (1. - 1. * static_cast<double>(m_FracCompl)) /
                     static_cast<double>(unGroupedSet.size());

  // go thorugh all the spectra in the input workspace
  for (auto copyFrIt : unGroupedSet) {
    if (copyFrIt == USED)
      continue; // Marked as not to be used
    size_t sourceIndex = static_cast<size_t>(copyFrIt);

    outputWS.getSpectrum(outIndex) = inputWS.getSpectrum(sourceIndex);

    // go to the next free index in the output workspace
    outIndex++;
    // make regular progress reports and check for cancelling the algorithm
    if (outIndex % INTERVAL == 0) {
      m_FracCompl += INTERVAL * prog4Copy;
      if (m_FracCompl > 1.0) {
        m_FracCompl = 1.0;
      }
      progress(m_FracCompl);
      interruption_point();
    }
  }

  g_log.debug() << name() << " copied " << unGroupedSet.size() - 1
                << " ungrouped spectra\n";
}

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_GROUPDETECTORS2_H_*/
