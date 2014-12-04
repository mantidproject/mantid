#ifndef MANTID_API_WORKSPACEVALIDATORS_H_
#define MANTID_API_WORKSPACEVALIDATORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include <numeric>
#include <vector>

namespace Mantid
{
namespace API
{

//===============================================================================================
/** A validator for workspaces which can contain a number of individual validators,
    all of which must pass for the overall validator to do so.

    @author Russell Taylor, Tessella Support Services plc
    @date 16/09/2008

    Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

//==============================================================================================
/**
 * An interface for those validators that require the MatrixWorkspace interface
 */
class MatrixWorkspaceValidator : public Kernel::TypedValidator<MatrixWorkspace_sptr>
{
};

//===============================================================================================
/** A validator which checks that the unit of the workspace referred to
 *  by a WorkspaceProperty is the expected one.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
class DLLExport WorkspaceUnitValidator : public MatrixWorkspaceValidator
{
public:
  /** Constructor
   *  @param unitID :: The name of the unit that the workspace must have. If left empty,
   *                the validator will simply check that the workspace is not unitless.
   */
  explicit WorkspaceUnitValidator(const std::string& unitID = "") 
    : MatrixWorkspaceValidator(), m_unitID(unitID) {}

  ///Gets the type of the validator
  std::string getType() const { return "workspaceunit"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const { return boost::make_shared<WorkspaceUnitValidator>(*this); }

private:
  /** Checks that the units of the workspace data are declared match any required units
   *
   *  @param value :: The workspace to test
   *  @return A user level description of the error or "" for no error
   */
  std::string checkValidity( const MatrixWorkspace_sptr & value ) const
  {
    // This effectively checks for single-valued workspaces
    if ( value->axes() == 0 ) return "A single valued workspace has no unit, which is required for this algorithm";

    Kernel::Unit_const_sptr unit = value->getAxis(0)->unit();
    // If m_unitID is empty it means that the workspace must have units, which can be anything
    if ( m_unitID.empty() )
    {
      return ( unit && (!boost::dynamic_pointer_cast<const Kernel::Units::Empty>(unit)) ? "" : "The workspace must have units" );
    }
    //now check if the units of the workspace is correct
    else
    {
      if ( (!unit) || ( unit->unitID().compare(m_unitID) ) )
      {
        return "The workspace must have units of " + m_unitID; //+ "; its unit is: " + unit->caption();
      }
      else return "";
    }
  }

  /// The name of the required unit
  const std::string m_unitID;
};


//===============================================================================================
/** A validator which checks that a workspace contains histogram data (the default)
 *  or point data as required.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
class DLLExport HistogramValidator : public MatrixWorkspaceValidator
{
public:
  /** Constructor
   *  @param mustBeHistogram :: Flag indicating whether the check is that a workspace should
   *                         contain histogram data (true, default) or shouldn't (false).
   */
  explicit HistogramValidator(const bool& mustBeHistogram = true) 
    : MatrixWorkspaceValidator(), m_mustBeHistogram(mustBeHistogram) {}

  ///Gets the type of the validator
  std::string getType() const { return "histogram"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const { return boost::make_shared<HistogramValidator>(*this); }

private:
  /** Checks if the workspace contains a histogram when it shouldn't and vice-versa
   *  @param value :: The workspace to test
   *  @return A user level description if a problem exists or ""
   */
  std::string checkValidity( const MatrixWorkspace_sptr& value ) const
  {
    if (m_mustBeHistogram)
    {
      if ( value->isHistogramData() ) return "";
      else return "The workspace must contain histogram data";
    }
    else
    {
      if ( !value->isHistogramData() ) return "";
      else return "The workspace must not contain histogram data";
    }
  }

  /// A flag indicating whether this validator requires that the workspace be a histogram (true) or not
  const bool m_mustBeHistogram;
};


//===============================================================================================
/** A validator which checks that a workspace contains raw counts in its bins
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
class DLLExport RawCountValidator : public MatrixWorkspaceValidator
{
public:
  /** Constructor
   *  @param mustNotBeDistribution :: Flag indicating whether the check is that a workspace should
   *                               not be a distribution (true, default) or should be (false).
   */
  RawCountValidator(const bool& mustNotBeDistribution = true) :
    m_mustNotBeDistribution(mustNotBeDistribution) {}

  ///Gets the type of the validator
  std::string getType() const { return "rawcount"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const { return boost::make_shared<RawCountValidator>(*this); }

private:
  /** Checks if the workspace must be a distribution but isn't and vice-versa
   *  @param value :: The workspace to test
   *  @return A user level description of any problem that exists or "" no problem
   */
  std::string checkValidity( const MatrixWorkspace_sptr & value ) const
  {
    if (m_mustNotBeDistribution)
    {
      if ( !value->isDistribution() ) return "";
      else return "A workspace containing numbers of counts is required here";
    }
    else
    {
      if ( value->isDistribution() ) return "";
      else return "A workspace of numbers of counts is not allowed here";
    }
  }

  /// A flag indicating whether this validator requires that the workspace must be a distribution (false) or not (true, the default)
  const bool m_mustNotBeDistribution;
};

//===============================================================================================
/** A validator which provides a <I>TENTATIVE</I> check that a workspace contains
 *  common bins in each spectrum.
 *  For efficiency reasons, it only checks that the first and last spectra have
 *  common bins, so it is important to carry out a full check within the algorithm
 *  itself.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 18/09/2008
 */
class DLLExport CommonBinsValidator : public MatrixWorkspaceValidator
{
public:
  ///Gets the type of the validator
  std::string getType() const { return "commonbins"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const { return boost::make_shared<CommonBinsValidator>(*this); }

private:
  /** Checks that the bin boundaries of each histogram in the workspace are the same
   *  @param value :: The workspace to test
   *  @return A message for users saying that bins are different, otherwise ""
   */
  std::string checkValidity( const MatrixWorkspace_sptr& value ) const
  {
    if ( !value ) return "Enter an existing workspace"; 
    if ( value->isCommonBins() ) return "";
    else return "The workspace must have common bin boundaries for all histograms";
  }

};
//===============================================================================================
/** A validator which checks whether the input workspace has the Spectra number in the axis.
 *  @author Michael Whitty, STFC
 *  @date 15/09/2010
 */
class DLLExport SpectraAxisValidator : public MatrixWorkspaceValidator
{
public:
  /** Class constructor with parameter.
   * @param axisNumber :: set the axis number to validate
   */
  SpectraAxisValidator(const int& axisNumber = 1) : m_axisNumber(axisNumber) {}

  ///Gets the type of the validator
  std::string getType() const { return "spectraaxis"; }
  /// Clone the current validator
  Kernel::IValidator_sptr clone() const { return boost::make_shared<SpectraAxisValidator>(*this); }

private:
  /** Checks that the axis stated 
  *  @param value :: The workspace to test
  *  @return A message for users with negative results, otherwise ""
  */
  std::string checkValidity( const MatrixWorkspace_sptr& value ) const
  {
    Mantid::API::Axis* axis = value->getAxis(m_axisNumber);
    if ( axis->isSpectra() ) return "";
    else return "A workspace with axis being Spectra Number is required here.";
  }
  const int m_axisNumber; ///< axis number to check on, defaults to 1

};
//===============================================================================================
/** A validator which checks whether the input workspace has the Numeric data in the axis.
 *  @author Michael Whitty, STFC
 *  @date 15/09/2010
 */
class DLLExport NumericAxisValidator : public MatrixWorkspaceValidator
{
public:
  /** Class constructor with parameter.
   * @param axisNumber :: set the axis number to validate
   */
  NumericAxisValidator(const int& axisNumber = 1) : m_axisNumber(axisNumber) {}

  ///Gets the type of the validator
  std::string getType() const { return "numericaaxis"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const { return boost::make_shared<NumericAxisValidator>(*this); }

private:
  /** Checks that the axis stated 
  *  @param value :: The workspace to test
  *  @return A message for users with negative results, otherwise ""
  */
  std::string checkValidity( const MatrixWorkspace_sptr& value ) const
  {
    Mantid::API::Axis* axis = value->getAxis(m_axisNumber);
    if ( axis->isNumeric() ) return "";
    else return "A workspace with axis being a Numeric Axis is required here.";
  }
  const int m_axisNumber; ///< axis number to check on, defaults to 1

};

//===============================================================================================
/** A validator which checks that a workspace has a valid instrument
 *  or point data as required.
 *
 *  @author Russell Taylor, Tessella
 *  @date 17/12/2010
 */
class DLLExport InstrumentValidator : public Kernel::TypedValidator<boost::shared_ptr<ExperimentInfo> >
{
public:
  ///Gets the type of the validator
  std::string getType() const { return "Instrument"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const { return boost::make_shared<InstrumentValidator>(*this); }

private:
  /** Checks that the workspace has an instrument defined
   *  @param value :: The workspace to test
   *  @return A user level description if a problem exists or ""
   */
  std::string checkValidity( const boost::shared_ptr<ExperimentInfo>& value ) const
  {
    // Just checks that an instrument has a sample position.
    // Could be extended for more detailed checks if needed.
    if ( ! value->getInstrument()->getSample() )
    {
      return "The workspace must have an instrument defined";
    }
    else
    {
      return "";
    }
  }
};

//==============================================================================
/** 
    @class IncreasingAxisValidator

    A validator which checks that the X axis of a workspace is increasing from
    left to right.

    @author Arturs Bekasovs [arturs.bekasovs@stfc.ac.uk]
    @date 08/07/2013

    TODO: Move out to the different file.
 */
class DLLExport IncreasingAxisValidator : public MatrixWorkspaceValidator
{
public:
  /// Get the type of the validator
  std::string getType() const { return "IncreasingAxis"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const { return boost::make_shared<IncreasingAxisValidator>(*this); }
  
private:
  /** 
   * Checks that X axis is in the right direction.
   *
   * @param value The workspace to check
   * @return "" if is valid, otherwise a user level description of a problem
   */
  std::string checkValidity( const MatrixWorkspace_sptr& value ) const
  {
    // 0 for X axis
    Axis* xAxis = value->getAxis(0);
    
    // Left-most axis value should be less than the right-most, if ws has
    // more than one X axis value
    if(xAxis->length() > 1 && xAxis->getValue(0) >= xAxis->getValue(xAxis->length() - 1))
      return "X axis of the workspace should be increasing from left to right";
    else
      return "";
      
  }
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACEVALIDATORS_H_ */
