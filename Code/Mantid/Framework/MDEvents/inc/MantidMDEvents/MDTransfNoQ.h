#ifndef  H_CONVERT_TO_MDEVENTS_NOQ_TRANSF
#define  H_CONVERT_TO_MDEVENTS_NOQ_TRANSF
//
#include "MantidMDEvents/MDTransfInterface.h"
#include "MantidMDEvents/ConvToMDEventsBase.h"
#include "MantidMDEvents/MDTransfFactory.h"

//
namespace Mantid
{
namespace MDEvents
{


/** Class responsible for conversion of input workspace 
  * data into proper number of output dimensions in NoQ case, when the data from a ws are just copied to MD WS.
  * 
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


class DLLExport MDTransfNoQ: public MDTransfInterface
{ 
public:

    /// the name, this subalgorithm is known to users (will appear in selection list)
    const std::string transfID()const; // {return "NoQ"; }
// calc target coordinates interface:
    bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd);
    bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i);
    bool calcMatrixCoord(const double& k0,std::vector<coord_t> &Coord)const;
    // constructor;
    MDTransfNoQ();
    //
    void initialize(const MDWSDescription &ConvParams);

    /* clone method allowing to provide the copy of the particular class */
    MDTransfInterface * clone() const{return new MDTransfNoQ(*this);}

//***** output WS definition interface:
    /** return the number of dimensions, calculated by the transformation from the workspace.
        Depending on ws axis units, the numebr here is either 1 or 2* and is independent on emode*/
    unsigned int getNMatrixDimensions(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const;
    /**function returns units ID-s which this transformation prodiuces its ouptut.
       here it is usually input ws units, which are independent on emode */
    std::vector<std::string> outputUnitID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const;
    std::vector<std::string> getDefaultDimID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const;
    const std::string inputUnitID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const;
private:
    unsigned int nMatrixDim;
    // the variables used for exchange data between different specific parts of the generic ND algorithm:
     // pointer to Y axis of MD workspace
     API::NumericAxis *pYAxis;

   // pointer to the class, which contains the information about precprocessed detectors (fake in this case)
     Kernel::V3D const * pDet;
     // min and max values for this conversion
     std::vector<double> dim_min,dim_max;
    /** the vector of the additional coordinates which define additional MD dimensions. 
        For implemented NoQ case, these dimensions do not depend on matrix coordinates and are determined by the WS properties */
    std::vector<coord_t>  addDimCoordinates;
private:
    // internal helper function which extract one or two axis from input matrix workspace;
    static  void getAxes( API::MatrixWorkspace_const_sptr inWS,API::NumericAxis *&pXAxis,API::NumericAxis *&pYAxis);
};

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
