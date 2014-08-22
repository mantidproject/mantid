#ifndef MANTID_ALGORITHMS_REBINBYTIMEATSAMPLE_H_
#define MANTID_ALGORITHMS_REBINBYTIMEATSAMPLE_H_

#include "MantidKernel/System.h"
#include "MantidAlgorithms/RebinByTimeBase.h"

namespace Mantid
{
  namespace Algorithms
  {

    /** RebinByTimeAtSample : Rebins an event workspace to a histogram workspace with time at sample along the x-axis.

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
    class DLLExport RebinByTimeAtSample: public RebinByTimeBase
    {
    public:
      RebinByTimeAtSample();
      virtual ~RebinByTimeAtSample();

      virtual const std::string name() const;
      virtual int version() const;
      virtual const std::string category() const;
      virtual const std::string summary() const;

    private:

      void doHistogramming(Mantid::API::IEventWorkspace_sptr inWS,
          Mantid::API::MatrixWorkspace_sptr outputWS, Mantid::MantidVecPtr& XValues_new,
          Mantid::MantidVec& OutXValues_scaled, Mantid::API::Progress& prog);

      /// Get the minimum x across all spectra in workspace
      virtual uint64_t getMaxX(Mantid::API::IEventWorkspace_sptr ws) const;
      /// Get the maximum x across all spectra in workspace
      virtual uint64_t getMinX(Mantid::API::IEventWorkspace_sptr ws) const;

    };

  } // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_REBINBYTIMEATSAMPLE_H_ */
