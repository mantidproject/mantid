#ifndef MANTID_API_WORKSPACEVALIDATORS_H_
#define MANTID_API_WORKSPACEVALIDATORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IValidator.h"
#include "MantidAPI/Workspace.h"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace Mantid
{
namespace API
{

/** A validator for workspaces which can contain a number individual validators,
    all of which must pass for the overall validator to do so.

    @author Russell Taylor, Tessella Support Services plc
    @date 16/09/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class CompositeValidator: public Kernel::IValidator<boost::shared_ptr<Workspace> >
{
public:
  CompositeValidator();
  virtual ~CompositeValidator();

  // IValidator methods
  const std::string getType() const { return "composite"; }
  const bool isValid( const boost::shared_ptr<Workspace> &value ) const;
  Kernel::IValidator<boost::shared_ptr<Workspace> >* clone();

  void add(Kernel::IValidator<boost::shared_ptr<Workspace> >* child);

private:
  /// Private Copy constructor: NO DIRECT COPY ALLOWED
  CompositeValidator(const CompositeValidator&);

  /// A container for the child validators
  std::vector<Kernel::IValidator<boost::shared_ptr<Workspace> >*> m_children;
};


/** A validator which checks that the unit of the workspace referred to
 *  by a WorkspaceProperty is the expected one.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
class WorkspaceUnitValidator: public Kernel::IValidator<boost::shared_ptr<Workspace> >
{
public:
  explicit WorkspaceUnitValidator(const std::string& unitID = "");
  virtual ~WorkspaceUnitValidator() {}

  const std::string getType() const { return "workspaceunit"; }
  const bool isValid( const boost::shared_ptr<Workspace> &value ) const;
  Kernel::IValidator<boost::shared_ptr<Workspace> >* clone() { return new WorkspaceUnitValidator(*this); }

private:
  /// The name of the required unit
  const std::string m_unitID;
};


/** A validator which checks that a workspace contains histogram data (the default)
 *  or point data as required.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
class HistogramValidator: public Kernel::IValidator<boost::shared_ptr<Workspace> >
{
public:
  explicit HistogramValidator(const bool& mustBeHistogram = true);
  virtual ~HistogramValidator() {}

  const std::string getType() const { return "histogram"; }
  const bool isValid( const boost::shared_ptr<Workspace> &value ) const;
  Kernel::IValidator<boost::shared_ptr<Workspace> >* clone() { return new HistogramValidator(*this); }

private:
  /// A flag indicating whether this validator requires that the workspace be a histogram (true) or not
  const bool m_mustBeHistogram;
};

/** A validator which checks that a workspace contains raw counts in its bins
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
class RawCountValidator: public Kernel::IValidator<boost::shared_ptr<Workspace> >
{
public:
  RawCountValidator() {}
  virtual ~RawCountValidator() {}

  const std::string getType() const { return "rawcount"; }
  const bool isValid( const boost::shared_ptr<Workspace> &value ) const;
  Kernel::IValidator<boost::shared_ptr<Workspace> >* clone() { return new RawCountValidator(*this); }
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACEVALIDATORS_H_ */
