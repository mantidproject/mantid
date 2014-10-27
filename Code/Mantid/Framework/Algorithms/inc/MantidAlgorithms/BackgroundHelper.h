#ifndef MANTID_ALGORITHM_BACKGROUNDHELPER_H_
#define MANTID_ALGORITHM_BACKGROUNDHELPER_H_


#include "MantidKernel/Unit.h"
#include "MantidKernel/cow_ptr.h"

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Class helping to remove constant (and possibly non-constant after simple modification) background calculated in TOF units
    from a matrix workspace, expressed in units, different from TOF.

    @date 26/10/2014

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

  class DLLExport BackgroundHelper
  {
  public:
    BackgroundHelper();
    void initialize(const API::MatrixWorkspace_const_sptr &bkgWS,const API::MatrixWorkspace_sptr &sourceWS,int emode);

    void removeBackground(int hist,const MantidVec &XValues,MantidVec &y_data,MantidVec &e_data);
  private:
    // pointer to the units conversion class for the working workspace;
    Kernel::Unit_sptr m_WSUnit;

    // shared pointer to the workspace containing background
    API::MatrixWorkspace_const_sptr m_bgWs;
    // shared pointer to the workspace where background should be removed
    API::MatrixWorkspace_const_sptr m_wkWS;

    // if the background workspace is single value workspace
    bool m_singleValueBackground;
    // the intensity of the background for a given spectra of a background workspace
    double m_Jack05;
    // energy conversion mode
    int m_Emode;
    // source-sample distance
    double m_L1;
    // incident for direct or analysis for indirect energy for units conversion
    double m_Efix;
    Geometry::IComponent_const_sptr m_Sample;

  };

}
}
#endif