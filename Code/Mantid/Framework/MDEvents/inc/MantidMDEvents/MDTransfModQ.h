#ifndef  H_CONVERT_TO_MDEVENTS_MODQ_TRANSF
#define  H_CONVERT_TO_MDEVENTS_MODQ_TRANSF
//
#include "MantidMDEvents/MDTransfInterface.h"
#include "MantidMDEvents/ConvToMDEventsBase.h"
//
namespace Mantid
{
namespace MDEvents
{


/** Set of internal class used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into proper number of output dimensions for ModQ transformation
  * 
  * Currently contains Elastic and Inelastic transformations
  *
  * This particular file defines  specializations of generic coordinate transformation templated to the ModQ case
   *
   * @date 16-05-2012

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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




// ModQ,Elastic 
class DLLExport MDTransfModQElastic: public MDTransfInterface
{ 
public:
    bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd);
    bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i);
    bool calcMatrixCoord(const double& k0,std::vector<coord_t> &Coord)const;
    // constructor;
    MDTransfModQElastic():pDet(NULL),pHost(NULL),nMatrixDim(1){}
    void initialize(const ConvToMDEventsBase &Conv);

protected:
    //  directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    // min-max values, some modified to work with squared values:
    std::vector<double> dim_min,dim_max;
    //
    Kernel::V3D const * pDet;
    // The pointer to the class, which drives this conversion and provides all necessary values for variables
    ConvToMDEventsBase const* pHost;
    // number of dimensions, calculated from matrix workspace
    int nMatrixDim;
    
 
};

class DLLExport MDTransfModQInelastic: public MDTransfModQElastic
{ 
public:
     bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd);
     bool calcMatrixCoord(const double& k0,std::vector<coord_t> &Coord)const;
     void initialize(const ConvToMDEventsBase &Conv);
private:
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    // energy conversion mode
    ConvertToMD::EModes emode;
};

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
