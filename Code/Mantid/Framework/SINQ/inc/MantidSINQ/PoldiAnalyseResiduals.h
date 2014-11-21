#ifndef MANTID_SINQ_POLDIANALYSERESIDUALS_H_
#define MANTID_SINQ_POLDIANALYSERESIDUALS_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"

namespace Mantid
{
namespace Poldi
{

/** PoldiAnalyseResiduals : TODO: DESCRIPTION

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
class MANTID_SINQ_DLL PoldiAnalyseResiduals  : public API::Algorithm
{
public:
    PoldiAnalyseResiduals();
    virtual ~PoldiAnalyseResiduals();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;


protected:
    double sumCounts(const DataObjects::Workspace2D_sptr &workspace, const std::vector<int> &workspaceIndices) const;
    size_t numberOfPoints(const DataObjects::Workspace2D_sptr &workspace, const std::vector<int> &workspaceIndices) const;
    void addValue(DataObjects::Workspace2D_sptr &workspace, double value, const std::vector<int> &workspaceIndices) const;

private:
    void init();
    void exec();
};


} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIANALYSERESIDUALS_H_ */
