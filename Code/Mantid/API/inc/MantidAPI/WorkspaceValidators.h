#ifndef MANTID_API_WORKSPACEVALIDATORS_H_
#define MANTID_API_WORKSPACEVALIDATORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IValidator.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <numeric>

namespace Mantid
{
namespace API
{

//===============================================================================================
/** A validator for workspaces which can contain a number of individual validators,
    all of which must pass for the overall validator to do so.
Workspace
    @author Russell Taylor, Tessella Support Services plc
    @date 16/09/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <typename TYPE = MatrixWorkspace>
class DLLExport CompositeValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  CompositeValidator() {}

  virtual ~CompositeValidator()
  {
    for (unsigned int i=0; i < m_children.size(); ++i)
    {
      delete m_children[i];
    }
    m_children.clear();
  }

  // IValidator methods
  ///Gets the type of the validator
  std::string getType() const { return "composite"; }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone()
  {
    CompositeValidator<TYPE>* copy = new CompositeValidator<TYPE>();
    for (unsigned int i=0; i < m_children.size(); ++i)
    {
      copy->add( m_children[i]->clone() );
    }
    return copy;
  }

  /** Adds a validator to the group of validators to check
   *  @param child A pointer to the validator to add
   */
  void add(Kernel::IValidator<boost::shared_ptr<TYPE> >* child)
  {
    m_children.push_back(child);
  }

private:
  /// Private Copy constructor: NO DIRECT COPY ALLOWED
  CompositeValidator(const CompositeValidator&);

  /** Checks the value of all child validators. Fails if any child fails.
   *  @param value The workspace to test
   *  @return A user level description of the first problem it finds otherwise ""
   */
  std::string checkValidity( const boost::shared_ptr<TYPE>& value ) const
  {
    //Go though all the validators
    for (unsigned int i=0; i < m_children.size(); ++i)
    {
      std::string error = m_children[i]->isValid(value);
      //exit on the first error, to avoid passing doing more tests on invalid objects that could fail
      if (error != "") return error;
    }
    //there were no errors
    return "";
  }

  /// A container for the child validators
  std::vector<Kernel::IValidator<boost::shared_ptr<TYPE> >*> m_children;
};


//===============================================================================================
/** A validator which checks that the unit of the workspace referred to
 *  by a WorkspaceProperty is the expected one.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
template <typename TYPE = MatrixWorkspace>
class DLLExport WorkspaceUnitValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  /** Constructor
   *  @param unitID The name of the unit that the workspace must have. If left empty,
   *                the validator will simply check that the workspace is not unitless.
   */
  explicit WorkspaceUnitValidator(const std::string& unitID = "") : m_unitID(unitID) {}

  virtual ~WorkspaceUnitValidator() {}

  ///Gets the type of the validator
  std::string getType() const { return "workspaceunit"; }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new WorkspaceUnitValidator(*this); }

private:
  /** Checks that the units of the workspace data are declared match any required units
   *
   *  @param value The workspace to test
   *  @return A user level description of the error or "" for no error
   */
  std::string checkValidity( const boost::shared_ptr<TYPE>& value ) const
  {
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
template <typename TYPE = MatrixWorkspace>
class DLLExport HistogramValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  /** Constructor
   *  @param mustBeHistogram Flag indicating whether the check is that a workspace should
   *                         contain histogram data (true, default) or shouldn't (false).
   */
  explicit HistogramValidator(const bool& mustBeHistogram = true) :
    m_mustBeHistogram(mustBeHistogram) {}

  virtual ~HistogramValidator() {}

  ///Gets the type of the validator
  std::string getType() const { return "histogram"; }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new HistogramValidator(*this); }

private:
  /** Checks if the workspace contains a histogram when it shouldn't and vice-versa
   *  @param value The workspace to test
   *  @return A user level description if a problem exists or ""
   */
  std::string checkValidity( const boost::shared_ptr<TYPE>& value ) const
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
/** A validator which checks that a workspace is an EventWorkspace.
 *
 *  @author Janik Zikovsky, SNS
 *  @date 13/08/2010
 */
template <typename TYPE = MatrixWorkspace>
class DLLExport EventWorkspaceValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  /** Constructor
   *  @param mustBeEvent Flag indicating whether the check is that a workspace should
   *                     be EventWorkspace (true, default) or shouldn't (false).
   */
  explicit EventWorkspaceValidator(const bool& mustBeEvent = true) :
    m_mustBeEvent(mustBeEvent) {}

  virtual ~EventWorkspaceValidator() {}

  ///Gets the type of the validator
  std::string getType() const { return "eventworkspace"; }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new EventWorkspaceValidator(*this); }

private:
  /** Checks if the workspace contains a histogram when it shouldn't and vice-versa
   *  @param value The workspace to test
   *  @return A user level description if a problem exists or ""
   */
  std::string checkValidity( const boost::shared_ptr<TYPE>& value ) const
  {
    //Try to cast to EventWorkspace
    IEventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<IEventWorkspace>(value);

    if (m_mustBeEvent)
    {
      if ( eventWS ) return "";
      else return "The workspace must be an EventWorkspace";
    }
    else
    {
      if ( !eventWS ) return "";
      else return "The workspace must be not be an EventWorkspace";
    }
  }

  /// A flag indicating whether this validator requires that the workspace be a histogram (true) or not
  const bool m_mustBeEvent;
};



//===============================================================================================
/** A validator which checks that a workspace contains raw counts in its bins
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
template <typename TYPE = MatrixWorkspace>
class DLLExport RawCountValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  /** Constructor
   *  @param mustNotBeDistribution Flag indicating whether the check is that a workspace should
   *                               not be a distribution (true, default) or should be (false).
   */
  RawCountValidator(const bool& mustNotBeDistribution = true) :
    m_mustNotBeDistribution(mustNotBeDistribution) {}

  virtual ~RawCountValidator() {}

  ///Gets the type of the validator
  std::string getType() const { return "rawcount"; }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new RawCountValidator(*this); }

private:
  /** Checks if the workspace must be a distribution but isn't and vice-versa
   *  @param value The workspace to test
   *  @return A user level description of any problem that exists or "" no problem
   */
  std::string checkValidity( const boost::shared_ptr<TYPE>& value ) const
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
template <typename TYPE = MatrixWorkspace>
class DLLExport CommonBinsValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  CommonBinsValidator() {}
  virtual ~CommonBinsValidator() {}

  ///Gets the type of the validator
  std::string getType() const { return "commonbins"; }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new CommonBinsValidator(*this); }

private:
  /** Checks that the bin boundaries of each histogram in the workspace are the same
   *  @param value The workspace to test
   *  @return A message for users saying that bins are different, otherwise ""
   */
  std::string checkValidity( const boost::shared_ptr<TYPE>& value ) const
  {
    if ( !value ) return "Enter an existing workspace"; 
    //there being only one or zero histograms is accepted as not being an error
    if ( !value->blocksize() || value->getNumberHistograms() < 2) return "";
    //otherwise will compare some of the data, to save time just check two the first and the last
    const int lastSpec = value->getNumberHistograms() - 1;
    // Quickest check is to see if they are actually the same vector
    if ( &(value->readX(0)[0]) == &(value->readX(lastSpec)[0]) ) return "";
    // Now check numerically
    const double first = std::accumulate(value->readX(0).begin(),value->readX(0).end(),0.);
    const double last = std::accumulate(value->readX(lastSpec).begin(),value->readX(lastSpec).end(),0.);
    if ( std::abs(first-last)/std::abs(first+last) > 1.0E-9 )
    {
      return "The workspace must have common bin boundaries for all histograms";
    }
    return "";
  }

};
//===============================================================================================
/** A validator which checks whether the input workspace has the Spectra number in the axis.
 *  @author Michael Whitty, STFC
 *  @date 15/09/2010
 */
template <typename TYPE = MatrixWorkspace>
class DLLExport SpectraAxisValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  SpectraAxisValidator(const int& axisNumber = 1) : m_axisNumber(axisNumber) {}
  virtual ~SpectraAxisValidator() {}

  ///Gets the type of the validator
  std::string getType() const { return "spectraaxis"; }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new SpectraAxisValidator(*this); }

private:
  /** Checks that the axis stated 
  *  @param value The workspace to test
  *  @return A message for users with negative results, otherwise ""
  */
  std::string checkValidity( const boost::shared_ptr<TYPE>& value ) const
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
template <typename TYPE = MatrixWorkspace>
class DLLExport NumericAxisValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  NumericAxisValidator(const int& axisNumber = 1) : m_axisNumber(axisNumber) {}
  virtual ~NumericAxisValidator() {}

  ///Gets the type of the validator
  std::string getType() const { return "numericaaxis"; }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new NumericAxisValidator(*this); }

private:
  /** Checks that the axis stated 
  *  @param value The workspace to test
  *  @return A message for users with negative results, otherwise ""
  */
  std::string checkValidity( const boost::shared_ptr<TYPE>& value ) const
  {
    Mantid::API::Axis* axis = value->getAxis(m_axisNumber);
    if ( axis->isNumeric() ) return "";
    else return "A workspace with axis being a Numeric Axis is required here.";
  }
  const int m_axisNumber; ///< axis number to check on, defaults to 1

};
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACEVALIDATORS_H_ */
