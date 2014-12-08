#ifndef MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_
#define MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid
{

namespace DataObjects
{
/** Concrete workspace implementation. Data is a single double value
    @author Nicholas Draper
    @date 19/05/2008

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport WorkspaceSingleValue : public API::MatrixWorkspace
{
public:

  /**	Gets the name of the workspace type
   * @return Standard string name  */
  virtual const std::string id() const {return "WorkspaceSingleValue";}

  WorkspaceSingleValue(double value=0.0,double error=0.0);

  virtual ~WorkspaceSingleValue();

  ///Returns the number of single indexable items in the workspace
  virtual std::size_t size() const
  { return 1; }

  ///Returns the size of each block of data returned by the dataX accessors
  virtual std::size_t blocksize() const
  { return 1; }

  /// @return the number of histograms (spectra)
  std::size_t getNumberHistograms() const
  { return 1; }

  //------------------------------------------------------------
  // Return the underlying ISpectrum ptr at the given workspace index.
  virtual Mantid::API::ISpectrum * getSpectrum(const size_t index);

  // Return the underlying ISpectrum ptr (const version) at the given workspace index.
  virtual const Mantid::API::ISpectrum * getSpectrum(const size_t index) const;

  void generateHistogram(const std::size_t index, const MantidVec& X, MantidVec& Y, MantidVec& E, bool skipError = false) const;

private:
  /// Private copy constructor. NO COPY ALLOWED
  WorkspaceSingleValue(const WorkspaceSingleValue&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  WorkspaceSingleValue& operator=(const WorkspaceSingleValue&);

  // allocates space in a new workspace - does nothing in this case
  virtual void init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength);

  /// Instance of Histogram1D that holds the "spectrum" (AKA the single value);
  Histogram1D data;
};

  ///shared pointer to the WorkspaceSingleValue class
typedef boost::shared_ptr<WorkspaceSingleValue> WorkspaceSingleValue_sptr;
typedef boost::shared_ptr<const WorkspaceSingleValue> WorkspaceSingleValue_const_sptr;

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_*/
