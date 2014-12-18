#ifndef MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_
#define MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/Detector.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{

namespace API
{
  /// Forward declaration for MatrixWorkspace.
  class MatrixWorkspace;
  class Progress;
}
namespace Algorithms
{
  /** NormaliseByDetector : Normalises a workspace with respect to the detector efficiency function stored against components in the instrument parameters. See wiki for more details.
    Detector efficiency functions are calculated using the wavelengths in the input workspace.
    
    @date 2012-07-17

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport NormaliseByDetector  : public API::Algorithm
  {
  public:
    NormaliseByDetector(bool parallelExecution = true);
    virtual ~NormaliseByDetector();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Normalise the input workspace by the detector efficiency.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:
    /// Flag to indicate that the histograms should be processed in parallel.
    const bool m_parallelExecution;
    /// Try to parse a function parameter and extract the correctly typed parameter.
    const Mantid::Geometry::FitParameter tryParseFunctionParameter(Mantid::Geometry::Parameter_sptr parameter, Geometry::IDetector_const_sptr det);
    /// Block to process histograms.
    boost::shared_ptr<Mantid::API::MatrixWorkspace>  processHistograms(boost::shared_ptr<Mantid::API::MatrixWorkspace> inWS);
    /// Process indivdual histogram.
    void processHistogram(size_t wsIndex, boost::shared_ptr<Mantid::API::MatrixWorkspace> denominatorWS, boost::shared_ptr<const Mantid::API::MatrixWorkspace> inWS, Mantid::API::Progress& prog);

    void init();
    void exec();


  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_ */