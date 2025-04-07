// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace Algorithms {

enum OperandType { eEventList = 0, eHistogram = 1, eNumber = 2 };

/**
BinaryOperation supports the implementation of a binary operation on two input
workspaces.
It inherits from the Algorithm class, and overrides the init() & exec() methods.

Required Properties:
<UL>
<LI> InputWorkspace1 - The name of the workspace forming the left hand
operand</LI>
<LI> InputWorkspace2 - The name of the workspace forming the right hand operand
</LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
</UL>

@author Nick Draper
@date 14/12/2007
*/
class MANTID_ALGORITHMS_DLL BinaryOperation : public API::Algorithm {
public:
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic"; }

  /** BinaryOperationTable: a list of ints.
   * Index into vector: workspace index in the lhs;
   * Value at that index: workspace index of the rhs to apply to the WI in the
   * lhs. -1 if not found.
   */
  using BinaryOperationTable = std::vector<int64_t>;
  using BinaryOperationTable_sptr = std::shared_ptr<BinaryOperationTable>;

  static BinaryOperationTable_sptr buildBinaryOperationTable(const API::MatrixWorkspace_const_sptr &lhs,
                                                             const API::MatrixWorkspace_const_sptr &rhs);

protected:
  // Overridden Algorithm methods
  void exec() override;
  void init() override;

  bool handleSpecialDivideMinus();

  /// Execution method for event workspaces, to be overridden as needed.
  virtual void execEvent(DataObjects::EventWorkspace_const_sptr lhs, DataObjects::EventWorkspace_const_sptr rhs);

  /// The name of the first input workspace property
  virtual std::string inputPropName1() const { return "LHSWorkspace"; }
  /// The name of the second input workspace property
  virtual std::string inputPropName2() const { return "RHSWorkspace"; }
  /// The name of the output workspace property
  virtual std::string outputPropName() const { return "OutputWorkspace"; }

  /// Checks the compatibility of the two workspaces
  virtual bool checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                  const API::MatrixWorkspace_const_sptr rhs) const;

  /// Checks the compatibility of event-based processing of the two workspaces
  virtual bool checkEventCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                       const API::MatrixWorkspace_const_sptr rhs);

  /// Checks the overall size compatibility of two workspaces
  virtual std::string checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                             const API::MatrixWorkspace_const_sptr rhs) const;

  virtual bool propagateSpectraMask(const API::SpectrumInfo &lhsSpectrumInfo, const API::SpectrumInfo &rhsSpectrumInfo,
                                    const int64_t index, API::MatrixWorkspace &out, API::SpectrumInfo &outSpectrumInfo);

  /** Carries out the binary operation on a single spectrum, with another
   *spectrum as the right-hand operand.
   *
   *  @param lhs :: Lhs histogram data
   *  @param rhs :: Rhs histogram data
   *  @param YOut :: Data values resulting from the operation
   *  @param EOut :: Drror values resulting from the operation
   */
  virtual void performBinaryOperation(const HistogramData::Histogram &lhs, const HistogramData::Histogram &rhs,
                                      HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) = 0;

  /** Carries out the binary operation when the right hand operand is a single
   *number.
   *
   *  @param lhs :: Lhs histogram data
   *  @param rhsY :: The rhs data value
   *  @param rhsE :: The lhs data value
   *  @param YOut :: Data values resulting from the operation
   *  @param EOut :: Error values resulting from the operation
   */
  virtual void performBinaryOperation(const HistogramData::Histogram &lhs, const double rhsY, const double rhsE,
                                      HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) = 0;

  // ===================================== EVENT LIST BINARY OPERATIONS
  // ==========================================

  /** Carries out the binary operation IN-PLACE on a single EventList,
   * with another EventList as the right-hand operand.
   * The event lists simply get appended.
   *
   *  @param lhs :: Reference to the EventList that will be modified in place.
   *  @param rhs :: Const reference to the EventList on the right hand side.
   */
  virtual void performEventBinaryOperation(DataObjects::EventList &lhs, const DataObjects::EventList &rhs);

  /// Carries out the binary operation IN-PLACE on a single EventList, with another (histogrammed) spectrum as the
  /// right-hand operand.
  virtual void performEventBinaryOperation(DataObjects::EventList &lhs, const MantidVec &rhsX, const MantidVec &rhsY,
                                           const MantidVec &rhsE);

  /// Carries out the binary operation IN-PLACE on a single EventList, with a single (double) value as the right-hand
  /// operand
  virtual void performEventBinaryOperation(DataObjects::EventList &lhs, const double &rhsY, const double &rhsE);

  /** Should be overridden by operations that need to manipulate the units of
   * the output workspace.
   *  Does nothing by default.
   *  @param lhs :: The first input workspace
   *  @param rhs :: The second input workspace
   *  @param out :: The output workspace
   */
  virtual void setOutputUnits(const API::MatrixWorkspace_const_sptr lhs, const API::MatrixWorkspace_const_sptr rhs,
                              API::MatrixWorkspace_sptr out) {
    (void)lhs; // Avoid compiler warning
    (void)rhs;
    (void)out;
  }

  /** Only overridden by operations that affect the properties of the run (e.g.
   * Plus
   *  where the proton currents (charges) are added). Otherwise it does nothing.
   *  @param lhs :: One of the workspaces to operate on
   *  @param rhs :: The other workspace
   *  @param ans :: The output workspace
   */

  virtual void operateOnRun(const API::Run &lhs, const API::Run &rhs, API::Run &ans) const {
    (void)lhs; // Avoid compiler warning
    (void)rhs;
    (void)ans;
  };

  OperandType getOperandType(const API::MatrixWorkspace_const_sptr &ws);

  virtual void checkRequirements();

  // ------- Workspaces being worked on --------
  /// Left-hand side workspace
  API::MatrixWorkspace_const_sptr m_lhs;
  /// Left-hand side EventWorkspace
  DataObjects::EventWorkspace_const_sptr m_elhs;

  /// Right-hand side workspace
  API::MatrixWorkspace_const_sptr m_rhs;
  /// Right-hand side EventWorkspace
  DataObjects::EventWorkspace_const_sptr m_erhs;

  /// Output workspace
  API::MatrixWorkspace_sptr m_out;
  /// Output EventWorkspace
  DataObjects::EventWorkspace_sptr m_eout;

  /// The property value
  bool m_AllowDifferentNumberSpectra{false};
  /// Flag to clear RHS workspace in binary operation
  bool m_ClearRHSWorkspace{false};
  /// Cache for LHS workspace's blocksize
  size_t m_lhsBlocksize;
  /// Cache for RHS workspace's blocksize
  size_t m_rhsBlocksize;

  /// Cache for if LHS workspace's is ragged
  bool m_lhsRagged{false};
  /// Cache for if RHS workspace's is ragged
  bool m_rhsRagged{false};
  //------ Requirements -----------

  /// matchXSize set to true if the X sizes of histograms must match.
  bool m_matchXSize{false};

  /// flipSides set to true if the rhs and lhs operands should be flipped - for
  /// commutative binary operations, normally.
  bool m_flipSides{false};

  /// Variable set to true if the operation allows the output to stay as an
  /// EventWorkspace. If this returns false, any EventWorkspace will be
  /// converted to Workspace2D. This is ignored if the lhs operand is not an
  /// EventWorkspace.
  bool m_keepEventWorkspace{false};

  /** Are we going to use the histogram representation of the RHS event list
   * when performing the operation?
   * e.g. divide and multiply? Plus and Minus will set this to false (default).
   */
  bool m_useHistogramForRhsEventWorkspace{false};

  /** Special case for plus/minus: if there is only one bin on the RHS, use the
   * 2D method (appending event lists)
   * so that the single bin is not treated as a scalar
   */
  bool m_do2D_even_for_SingleColumn_on_rhs{false};

private:
  void doSingleValue();
  void doSingleSpectrum();
  void doSingleColumn();
  void do2D(bool mismatchedSpectra);

  void propagateBinMasks(const API::MatrixWorkspace_const_sptr &rhs, const API::MatrixWorkspace_sptr &out);
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress = nullptr;
};

} // namespace Algorithms
} // namespace Mantid
