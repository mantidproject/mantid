#ifndef MANTID_ALGORITHMS_REPLACESPECIALVALUES_H_
#define MANTID_ALGORITHMS_REPLACESPECIALVALUES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid
{
namespace Algorithms
{
/** 
 Replaces instances of NaN and infinity in the workspace with user defined numbers.
 If a replacement value is not provided the check will not occur.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the workspace to correct</LI>
 <LI> OutputWorkspace - The name of the corrected workspace (can be the same as the input one)</LI>
 <LI> NaNValue        - The value used to replace occurances of NaN (default do not check)</LI>
 <LI> NaNError        - The error value used when replacing a value of NaN (default 0)</LI>
 <LI> InfinityValue   - The value used to replace occurances of positive or negative infinity (default do not check)</LI>
 <LI> InfinityError   - The error value used when replacing a value of infinity (default 0)</LI>
 </UL>

 @author Nicholas Draper, Tessella plc
 @date 18/06/2009

 Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

 File change history is stored at: <https://github.com/mantidproject/mantid>    
 */
class DLLExport ReplaceSpecialValues : public UnaryOperation
{
public:
  /// Default constructor
  ReplaceSpecialValues() : UnaryOperation() {}
  /// Destructor
  virtual ~ReplaceSpecialValues() {}
  /// Algorithm's name for identification
  virtual const std::string name() const { return "ReplaceSpecialValues"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Replaces instances of NaN and infinity in the workspace with user defined numbers. If a replacement value is not provided the check will not occur. This algorithm can also be used to replace numbers whose absolute value is larger than a user-defined threshold.";}

  /// Algorithm's version for identification
  virtual int version() const { return 1; }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility;CorrectionFunctions\\SpecialCorrections"; }

private:
  
  // Overridden UnaryOperation methods
  void defineProperties();
  void retrieveProperties();
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut,
      double& EOut);

  /// returns true if the value is NaN
  bool checkIfNan(const double& value) const;
  /// returns true if the value if + or - infinity
  bool checkIfInfinite(const double& value) const;
  /// Returns true if the absolute value is larger than the 'big' threshold
  bool checkIfBig(const double& value) const;
  ///returns true if the value has been set
  bool checkifPropertyEmpty(const double& value) const;

  double m_NaNValue;       ///< The replacement value for NaN
  double m_NaNError;       ///< The replacement error value for NaN
  double m_InfiniteValue;  ///< The replacement value for infinity
  double m_InfiniteError;  ///< The replacement error value for infinity
  double m_bigThreshold;   ///< The threshold value above which a value is considered 'big'
  double m_bigValue;       ///< The replacement value for big numbers
  double m_bigError;       ///< The replacement error value for big numbers

  bool m_performNaNCheck;       ///< Flag to indicate if the NaN check is to be performed
  bool m_performInfiniteCheck;  ///< Flag to indicate if the infinity check is to be performed
  bool m_performBigCheck;       ///< Flag to indicate if the 'big number' check is to be performed
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_REPLACESPECIALVALUES_H_*/
