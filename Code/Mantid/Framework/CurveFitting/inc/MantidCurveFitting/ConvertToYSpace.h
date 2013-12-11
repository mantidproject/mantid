#ifndef MANTID_CURVEFITTING_CONVERTTOYSPACE_H_
#define MANTID_CURVEFITTING_CONVERTTOYSPACE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace CurveFitting
{
  //---------------------------------------------------------------------------
  // Forward declarations
  //---------------------------------------------------------------------------
  struct DetectorParams;

  /**
    Takes a workspace with X axis in TOF and converts it to Y-space where the transformation is defined
    by equation (7) in http://link.aip.org/link/doi/10.1063/1.3561493?ver=pdfcov

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ConvertToYSpace  : public API::Algorithm
  {
  public:
    /// Constructor
    ConvertToYSpace();
    
    const std::string name() const;
    int version() const;
    const std::string category() const;

    /// Convert single time value to Y,Q & Ei values
    static void calculateY(double & yspace, double & qspace, double &ei,
                           const double mass, const double tmicro,
                           const double k1, const double v1,
                           const DetectorParams & detpar);

  private:
    virtual void initDocs();
    void init();
    void exec();

    /// Check and store appropriate input data
    void retrieveInputs();
    /// Create the output workspace
    void createOutputWorkspace();
    /// Compute & store the parameters that are fixed during the correction
    void cacheInstrumentGeometry();

    /// Input workspace
    API::MatrixWorkspace_sptr m_inputWS;
    /// Source-sample distance
    double m_l1;
    /// Sample position
    Kernel::V3D m_samplePos;

    /// Output workspace
    API::MatrixWorkspace_sptr m_outputWS;
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_CONVERTTOYSPACE_H_ */
