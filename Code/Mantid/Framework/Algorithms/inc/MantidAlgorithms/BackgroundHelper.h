#ifndef MANTID_ALGORITHM_BACKGROUNDHELPER_H_
#define MANTID_ALGORITHM_BACKGROUNDHELPER_H_


#include "MantidKernel/Unit.h"
#include "MantidKernel/cow_ptr.h"

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <list>

namespace Mantid
{
namespace Algorithms
{
/** Class provides removal of constant (and possibly non-constant after simple modification) background calculated in TOF units
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
   ~BackgroundHelper();
    void initialize(const API::MatrixWorkspace_const_sptr &bkgWS,const API::MatrixWorkspace_sptr &sourceWS,int emode,int nTreads=1);

    void removeBackground(int hist,const MantidVec &XValues,MantidVec &y_data,MantidVec &e_data,int tread_num=0)const;

    //returns the list of the failing detectors
    std::list<int> & getFailingSpectrsList()const{return FailingSpectraList;}
  private:
    //vector of pointers to the units conversion class for the working workspace;
    std::vector<Kernel::Unit *> m_WSUnit;

    // shared pointer to the workspace containing background
    API::MatrixWorkspace_const_sptr m_bgWs;
    // shared pointer to the workspace where background should be removed
    API::MatrixWorkspace_const_sptr m_wkWS;

    // if the background workspace is single value workspace
    bool m_singleValueBackground;
    // average number of counts at background for first spectra of a background workspace
    double m_NBg;
    // time interval for measuring the background
    double m_dtBg;
    // Squared error of the background for first spectra of a background workspace
    //double m_ErrSq;
    // energy conversion mode
    int m_Emode;
    // source-sample distance
    double m_L1;
    // incident for direct or analysis for indirect energy for units conversion
    double m_Efix;
    // shared pointer to the sample
    Geometry::IComponent_const_sptr m_Sample;

    // list of the spectra numbers for which detectors retrieval has been unsuccessful
    mutable std::list<int> FailingSpectraList;

    // get Ei attached to direct or indirect instrument workspace
    double getEi(const API::MatrixWorkspace_const_sptr &inputWS)const;
    // the procedure user to delete existing unit converter pointers
    void deleteUnitsConverters();
  };

}
}
#endif