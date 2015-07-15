#ifndef MANTID_DATAHANDLING_DECORATORWORKSPACE_H_
#define MANTID_DATAHANDLING_DECORATORWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/DllConfig.h"

#include <vector>
#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace DataHandling {

/** DecoratorWorkspace : Decorator pattern around a collection of EventWorspaces to give backward-forward compatibility
 around performing operations on groups. Behave like an EventWorkspace with some overriden functionality and some additional new functionality.
Original purpose to support LoadEventNexus for the MultiPeriod cases.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport DecoratorWorkspace : public Mantid::DataObjects::EventWorkspace {

private:

  /// Vector of EventWorkspaces
  std::vector<DataObjects::EventWorkspace_sptr> m_WsVec;
  /// Create Empty EventWorkspaces
  DataObjects::EventWorkspace_sptr createEmptyEventWorkspace() const;
  /// private copy constructor. Not implemented.
  DecoratorWorkspace(const DecoratorWorkspace &other);
  /// Private copy assignment operator. Assignment not implemented.
  DecoratorWorkspace &operator=(const DecoratorWorkspace &other);

public:

  DecoratorWorkspace();
  virtual ~DecoratorWorkspace();

  /*-------------------------------------------------------
   * DecoratorWorkspace specific methods
   *-------------------------------------------------------*/
  void setNPeriods(size_t nPeriods,  std::unique_ptr<const Kernel::TimeSeriesProperty<int> >& periodLog);
  void reserveEventListAt(size_t wi, size_t size);
  size_t nPeriods() const;
  DataObjects::EventWorkspace_sptr getSingleHeldWorkspace();
  API::Workspace_sptr combinedWorkspace();
  const DataObjects::EventList& getEventList(const size_t workspace_index, const size_t periodNumber) const;
  virtual DataObjects::EventList& getEventList(const size_t workspace_index, const size_t periodNumber);
  void setGeometryFlag(const int flag);
  void setThickness(const float flag);
  void setHeight(const float flag);
  void setWidth(const float flag);

  /*-------------------------------------------------------
   * EventWorkspace overriden methods
   *-------------------------------------------------------*/
  Geometry::Instrument_const_sptr getInstrument() const;
  const API::Run &run() const;
  API::Run &mutableRun();
  Mantid::API::ISpectrum* getSpectrum(const size_t index);
  virtual const Mantid::API::ISpectrum *getSpectrum(const size_t index) const;
  virtual Mantid::API::Axis* getAxis(const size_t& i) const;
  size_t getNumberHistograms() const;
  virtual const DataObjects::EventList& getEventList(const size_t workspace_index) const;

  virtual DataObjects::EventList &getEventList(const std::size_t workspace_index);
  void getSpectrumToWorkspaceIndexVector(std::vector<size_t>&out, Mantid::specid_t& offset) const;

  void getDetectorIDToWorkspaceIndexVector(std::vector<size_t>&out, Mantid::specid_t& offset, bool dothrow) const;
  Kernel::DateAndTime getFirstPulseTime() const;
  void setAllX(Kernel::cow_ptr<MantidVec>& x);
  size_t getNumberEvents() const;
  void resizeTo(const size_t size);
  void padSpectra(const std::vector<int32_t>& padding);
  void setInstrument(const Geometry::Instrument_const_sptr& inst);
  void setMonitorWorkspace(const boost::shared_ptr<API::MatrixWorkspace>& monitorWS);
  void updateSpectraUsing(const API::SpectrumDetectorMapping& map);
  DataObjects::EventList* getEventListPtr(size_t i);
  void populateInstrumentParameters();
};

typedef boost::shared_ptr<DecoratorWorkspace> DecoratorWorkspace_sptr;
typedef std::unique_ptr<DecoratorWorkspace>DecoratorWorkspace_uptr;

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_DECORATORWORKSPACE_H_ */
