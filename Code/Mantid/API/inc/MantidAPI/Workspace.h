#ifndef MANTID_API_WORKSPACE_H_
#define MANTID_API_WORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/IErrorHelper.h"
#include "MantidAPI/GaussianErrorHelper.h"
#include "MantidAPI/Axis.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"
#include "MantidGeometry/ParameterMap.h"
#include "boost/shared_ptr.hpp"

namespace Mantid
{

//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace API
{
/** Base Workspace Abstract Class.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
class DLLExport Workspace
{
public:
  // The Workspace Factory create-from-parent method needs direct access to the axes.
  friend class WorkspaceFactoryImpl;
  /// Typedef for the workspace_iterator to use with a Workspace
  typedef workspace_iterator<LocatedDataRef, Workspace> iterator;
  /// Typedef for the const workspace_iterator to use with a Workspace
  typedef workspace_iterator<const LocatedDataRef, const Workspace> const_iterator;

  /// Return the workspace typeID
  virtual const std::string id() const = 0;

  void initialize(const int &NVectors, const int &XLength, const int &YLength);
  virtual ~Workspace();

  void setTitle(const std::string&);
  void setComment(const std::string&);
  void setInstrument(const boost::shared_ptr<IInstrument>&);
  void setSpectraMap(const boost::shared_ptr<SpectraDetectorMap>& map);
  void copySpectraMap(const boost::shared_ptr<SpectraDetectorMap>& map);
  void setSample(const boost::shared_ptr<Sample>& sample);
  const std::string& getTitle() const;
  const std::string& getComment() const;
  boost::shared_ptr<IInstrument> getInstrument()const;
  boost::shared_ptr<Instrument> getBaseInstrument()const;
  boost::shared_ptr<SpectraDetectorMap> getSpectraMap() const;
  boost::shared_ptr<Sample> getSample() const;

  boost::shared_ptr<Geometry::ParameterMap> InstrumentParameters(){return sptr_parmap;}

  /// Get the footprint in memory in KB.
  virtual long int getMemorySize() const;

  // Section required for iteration
  /// Returns the number of single indexable items in the workspace
  virtual int size() const = 0;
  /// Returns the size of each block of data returned by the dataX accessors
  virtual int blocksize() const = 0;
  /// Returns the number of histograms in the workspace
  virtual const int getNumberHistograms() const = 0;

  //----------------------------------------------------------------------
  // DATA ACCESSORS
  //----------------------------------------------------------------------
  // Methods for getting read-only access to the data. Created for the python interface.
  // Just passes through to the virtual dataX/Y/E function (const version)
  /// Returns a read-only (i.e. const) reference to the specified X array
  const std::vector<double>& readX(int const index) const { return dataX(index); }
  /// Returns a read-only (i.e. const) reference to the specified Y array
  const std::vector<double>& readY(int const index) const { return dataY(index); }
  /// Returns a read-only (i.e. const) reference to the specified E array
  const std::vector<double>& readE(int const index) const { return dataE(index); }

  /// Returns the x data
  virtual std::vector<double>& dataX(int const index) = 0;
  /// Returns the y data
  virtual std::vector<double>& dataY(int const index) = 0;
  /// Returns the error data
  virtual std::vector<double>& dataE(int const index) = 0;
  /// Returns the x data const
  virtual const std::vector<double>& dataX(int const index) const = 0;
  /// Returns the y data const
  virtual const std::vector<double>& dataY(int const index) const = 0;
  /// Returns the error const
  virtual const std::vector<double>& dataE(int const index) const = 0;
  //----------------------------------------------------------------------

  /// Returns the ErrorHelper applicable for this spectra
  virtual const IErrorHelper* errorHelper(int const index) const = 0;
  /// Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,API::IErrorHelper* errorHelper) = 0;
  /// Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,const API::IErrorHelper* errorHelper) = 0;

  /// Returns a reference to the WorkspaceHistory
  WorkspaceHistory& history() { return m_history; }
  /// Returns a reference to the WorkspaceHistory const
  const WorkspaceHistory& getHistory() const { return m_history; }

  const int axes() const;
  Axis* const getAxis(const int axisIndex) const;

  /// Returns true if the workspace contains data in histogram form (as opposed to point-like)
  const bool isHistogramData() const;

  std::string YUnit() const;
  void setYUnit(const std::string& newUnit);

  /// Are the Y-values dimensioned?
  const bool& isDistribution() const;
  bool& isDistribution(bool newValue);

protected:
  Workspace();

  /// Initialises the workspace. Sets the size and lengths of the arrays. Must be overloaded.
  virtual void init(const int &NVectors, const int &XLength, const int &YLength) = 0;

  /// A vector of pointers to the axes for this workspace
  std::vector<Axis*> m_axes;

private:
  /// Private copy constructor. NO COPY ALLOWED
  Workspace(const Workspace&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  Workspace& operator=(const Workspace&);

  /// Has this workspace been initialised?
  bool m_isInitialized;

  /// The title of the workspace
  std::string m_title;
  /// A user-provided comment that is attached to the workspace
  std::string m_comment;

  /// The instrument used for this experiment
  boost::shared_ptr<Instrument> sptr_instrument;
  /// The SpectraDetector table used for this experiment
  boost::shared_ptr<SpectraDetectorMap> sptr_spectramap;
  /// The information on the sample environment
  boost::shared_ptr<Sample> sptr_sample;

  /// The history of the workspace, algorithm and environment
  WorkspaceHistory m_history;

  /// The unit for the data values (e.g. Counts)
  std::string m_YUnit;
  /// Flag indicating whether the Y-values are dimensioned. False by default
  bool m_isDistribution;

  /// Parameters modifying the base instrument
  boost::shared_ptr<Geometry::ParameterMap> sptr_parmap;

	/// Static reference to the logger class
	static Kernel::Logger& g_log;
};

///shared pointer to the workspace base class
typedef boost::shared_ptr<Workspace> Workspace_sptr;
///shared pointer to the workspace base class (const version)
typedef boost::shared_ptr<const Workspace> Workspace_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACE_H_*/
