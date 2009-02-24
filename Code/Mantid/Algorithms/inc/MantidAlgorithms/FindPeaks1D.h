#ifndef MANTID_ALGORITHMS_FINDPEAKS1D_H_
#define MANTID_ALGORITHMS_FINDPEAKS1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

class DLLExport FindPeaks1D : public API::Algorithm
{
public:
  /// Constructor
  FindPeaks1D();
  /// Virtual destructor
  virtual ~FindPeaks1D() {}
  /// Algorithm's name
  virtual const std::string name() const { return "FindPeaks1D"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// Sub-algorithm for calculating Smoothed second difference
  void generalisedSecondDifference();
  void retrieveProperties();
  void analyseVector();
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
  /// Parameters
  API::MatrixWorkspace_sptr input;
  API::MatrixWorkspace_sptr second_diff_spec;
  int spec_number;
  int smooth_npts;
  int smooth_iter;
  double threashold;
  boost::shared_ptr<DataObjects::TableWorkspace> peaks; //> Table Workspace to store peaks

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_FINDPEAKS1D_H_*/
