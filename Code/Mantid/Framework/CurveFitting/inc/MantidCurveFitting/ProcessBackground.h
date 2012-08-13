#ifndef MANTID_CURVEFITTING_PROCESSBACKGROUND_H_
#define MANTID_CURVEFITTING_PROCESSBACKGROUND_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

namespace Mantid
{
namespace CurveFitting
{

  /** ProcessBackground : Process background obtained from LeBailFit
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport ProcessBackground : public API::Algorithm
  {
  public:
    ProcessBackground();
    virtual ~ProcessBackground();

    virtual void initDocs();

    virtual void init();

    virtual void exec();

    virtual const std::string category() const {return "Diffraction\\Utility";}

    virtual const std::string name() const {return "ProcessBackground";}

    virtual int version() const {return 1;}

private:
    DataObjects::Workspace2D_const_sptr inpWS;
    DataObjects::Workspace2D_sptr outWS;

    double mLowerBound;
    double mUpperBound;

    double mTolerance;

    /// Remove peaks in a certain region
    void removePeaks();

    /// Remove a certain region from input workspace
    void deleteRegion();

    /// Add a certain region from a reference workspace
    void addRegion();

    /// Select background points automatically
    void autoBackgroundSelection();
    
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_PROCESSBACKGROUND_H_ */
