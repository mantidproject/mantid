// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid {
namespace Algorithms {
/**
 Replaces instances of NaN and infinity in the workspace with user defined
 numbers.
 If a replacement value is not provided the check will not occur.

 @author Nicholas Draper, Tessella plc
 @date 18/06/2009
 */
class MANTID_ALGORITHMS_DLL ReplaceSpecialValues : public UnaryOperation {
public:
  /// Default constructor
  ReplaceSpecialValues();
  /// Algorithm's name for identification
  const std::string name() const override { return "ReplaceSpecialValues"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Replaces instances of NaN and infinity in the workspace with user "
           "defined numbers. If a replacement value is not provided the check "
           "will not occur. This algorithm can also be used to replace numbers "
           "whose absolute value is larger than a user-defined threshold.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override { return "CorrectionFunctions\\SpecialCorrections"; }

private:
  // Overridden UnaryOperation methods
  void defineProperties() override;
  void retrieveProperties() override;
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut, double &EOut) override;

  /// returns true if the value is NaN
  bool checkIfNan(const double value) const;
  /// returns true if the value if + or - infinity
  bool checkIfInfinite(const double value) const;
  /// Returns true if the absolute value is larger than the 'big' threshold
  bool checkIfBig(const double value) const;
  /// Returns true is the absolute value is smaller than the 'small' threshold
  bool checkIfSmall(const double value) const;
  /// returns true if the value has been set
  bool checkifPropertyEmpty(const double value) const;

  double m_NaNValue;      ///< The replacement value for NaN
  double m_NaNError;      ///< The replacement error value for NaN
  double m_InfiniteValue; ///< The replacement value for infinity
  double m_InfiniteError; ///< The replacement error value for infinity
  double m_bigThreshold;  ///< The threshold value above which a value is
  /// considered 'big'
  double m_bigValue;       ///< The replacement value for big numbers
  double m_bigError;       ///< The replacement error value for big numbers
  double m_smallThreshold; ///< The threshold value below which a value is 'small'
  double m_smallValue;     ///< The replacement value for small numbers
  double m_smallError;     ///< The replacement error value for small numbers

  bool m_performNaNCheck; ///< Flag to indicate if the NaN check is to be
  /// performed
  bool m_performInfiniteCheck; ///< Flag to indicate if the infinity check is to
  /// be performed
  bool m_performBigCheck; ///< Flag to indicate if the 'big number' check is to
  /// be performed
  bool m_performSmallCheck; ///< Flag to indicate if the 'small number' check is
  /// to
  /// be performed
  bool m_checkErrors;
};

} // namespace Algorithms
} // namespace Mantid
