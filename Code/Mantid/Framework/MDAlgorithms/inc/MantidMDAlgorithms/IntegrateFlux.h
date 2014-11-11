#ifndef MANTID_MDALGORITHMS_INTEGRATEFLUX_H_
#define MANTID_MDALGORITHMS_INTEGRATEFLUX_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

namespace DataObjects
{
  class EventWorkspace;
}

namespace MDAlgorithms
{

  /** Algorithm IntegrateFlux.

    Calculates indefinite integral of the spectra in the input workspace sampled at a regular grid.
    The input workspace is expected to be an event workspace with weighted-no-time events.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport IntegrateFlux  : public API::Algorithm
  {
  public:

    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;

  private:
    void init();
    void exec();

    boost::shared_ptr<API::MatrixWorkspace> createOutputWorkspace( const DataObjects::EventWorkspace& eventWS, size_t nX ) const;
    void integrateSpectra( const DataObjects::EventWorkspace& eventWS, API::MatrixWorkspace &integrWS );

  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_INTEGRATEFLUX_H_ */