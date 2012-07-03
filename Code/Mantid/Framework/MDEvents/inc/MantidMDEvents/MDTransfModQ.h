#ifndef  H_CONVERT_TO_MDEVENTS_MODQ_TRANSF
#define  H_CONVERT_TO_MDEVENTS_MODQ_TRANSF
//
#include "MantidMDEvents/MDTransfInterface.h"
//#include "MantidMDEvents/ConvToMDBase.h"
#include "MantidMDEvents/MDTransfFactory.h"
#include "MantidMDEvents/MDTransfDEHelper.h"
//
namespace Mantid
{
namespace MDEvents
{
    
/** Class responsible for conversion of input workspace 
  * data into proper number of output dimensions for ModQ case
  * 
  * Currently contains Elastic and Inelastic transformations
  *
  * This particular file defines  specializations of generic coordinate transformation to the ModQ case
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


class DLLExport MDTransfModQ: public MDTransfInterface
{ 
public:
    /// the name, this subalgorithm is known to users (will appear in selection list)
    const std::string transfID()const; // {return "ModQ"; }
    /** energy conversion modes supported by this class; 
      * The class supports three standard energy conversion modes */
    std::vector<std::string> getEmodes()const{MDTransfDEHelper dEModes;  return dEModes.getEmodes();}

    bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd);
    bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i);
    bool calcMatrixCoord(const double& k0,std::vector<coord_t> &Coord)const;
    // constructor;
    MDTransfModQ();
    /* clone method allowing to provide the copy of the particular class */
    MDTransfInterface * clone() const{return new MDTransfModQ(*this);}
    //
    void initialize(const MDWSDescription &ConvParams);

// WARNING!!!! THESE METHODS ARE USED BEFORE INITIALIZE IS EXECUTED SO THEY CAN NOT RELY ON THE CONTENTS OF THE CLASS TO BE DEFINED (THEY ARE VIRTUAL STATIC METHODS)
    /** return the number of dimensions, calculated by the transformation from the workspace.
       Depending on EMode, this numebr here is either 1 or 2 and do not depend on input workspace*/
    unsigned int getNMatrixDimensions(CnvrtToMD::EModes mode,
        API::MatrixWorkspace_const_sptr Sptr = API::MatrixWorkspace_const_sptr())const;
    /**function returns units ID-s which this transformation prodiuces its ouptut.
       It is Momentum and Momentum and DelteE in inelastic modes */
    std::vector<std::string> outputUnitID(CnvrtToMD::EModes dEmode,
        API::MatrixWorkspace_const_sptr Sptr = API::MatrixWorkspace_const_sptr())const;
    /**the default dimID-s in ModQ mode are |Q| and dE if necessary */ 
    std::vector<std::string> getDefaultDimID(CnvrtToMD::EModes dEmode,
        API::MatrixWorkspace_const_sptr Sptr = API::MatrixWorkspace_const_sptr())const;
   /**  returns the units, the transformation expects for input workspace to be expressed in. */
    const std::string inputUnitID(CnvrtToMD::EModes dEmode,
        API::MatrixWorkspace_const_sptr Sptr = API::MatrixWorkspace_const_sptr())const;


 
protected:
    //  directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    // min-max values, some modified to work with squared values:
    std::vector<double> dim_min,dim_max;
    // pointer to the class, which contains the information about precprocessed detectors
    Kernel::V3D const * pDet;

    // number of dimensions, calculated from a matrix workspace, which is one in elastic and two in inelastic mode here. 
    unsigned int nMatrixDim;
    // the variable which describes current conversion mode:
    CnvrtToMD::EModes emode;
    /** the vector of the additional coordinates which define additional MD dimensions. 
        For implemented ModQ case, these dimensions do not depend on matrix coordinates and are determined by WS properties */
    std::vector<coord_t>  addDimCoordinates;
    //************* These two variables are relevant to inelastic modes only and will be used in inelastic cases:
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;  
private:
     /// how to transform workspace data in elastic case
    inline bool calcMatrixCoordElastic(const double &k0,std::vector<coord_t> &Coored)const;
    /// how to transform workspace data in inelastic case
    inline bool calcMatrixCoordInelastic(const double &DeltaE,std::vector<coord_t> &Coored)const;
    
};

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
