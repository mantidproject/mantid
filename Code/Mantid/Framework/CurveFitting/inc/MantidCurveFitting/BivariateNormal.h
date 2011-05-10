#ifndef MANTID_CURVEFITTING_BIVARIATENORMAL_H_
#define MANTID_CURVEFITTING_BIVARIATENORMAL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMW.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
using namespace Mantid::CurveFitting;
using namespace Mantid; 
  
    /**
    Provide peak shape function interface a Peak shape on one time slice of a
    RectangularDetector.
    I.e. the function: Background +Intensity* 
             NormalDist( col,row,col_mean,row_mean,col_sigma,row_sigma, covariance)
             
     Where NormalDist is the bivariate normal distribution whose total area is
     1. So Intensity should be the integrated intensity.

    BivariateNormal parameters:
    <UL>
     <LI> Background - The background of the peak</LI>
     <LI> Intensity - The intensity of data for the peak on this time slice </LI>
     <LI> Mcol -  The col of the center of the peak </LI>
     <LI> Mrow - The row of the center of the peak on this slice</LI>
     <LI> SScol -The variance of the column values in the peak for this time slice </LI>
     <LI> SSrow - The variance of the row values in the peak for this time slice </LI>
     <LI> SSrc- The covariance of the row and column values in the peak for this time slice </LI>
    </UL>
    
    BivariateNormal attributes. All are double values 
    <UL> 
       <LI>StartRow- The start row in the Rectangular Detector</LI>
       <LI>StartCol-The start col in the Rectangular Detector</LI>
       <LI> NRows -The number of rows of data that will be fit. NOTE: The index, i,
             of a cell at row,col is i= (row-StartRow)*NCols+(col_StartCol)</LI>
       <LI> NCols- The number of columns of data that will be fit.NOTE: The index, i,
             of a cell at row,col is i= (row-StartRow)*NCols+(col_StartCol)</LI>
       <LI>SSIxx - Sum of squares of col values times intensity for each cell</LI>
       <LI>SSIyy - Sum of squares of row values times intensity for each cell</LI>
       <LI>SSIxy - Sum of row values times col values times intensity for each cell</LI>
       <LI>SSxx - Sum of squares of col values for each cell</LI>
       <LI>SSyy - Sum of squares of row values for each cell</LI>
       <LI>SSxy - Sum of row times col values for each cell</LI>
       <LI> SSIx -Sum of col values times intensity for each cell</LI>
       <LI> SSIy -Sum of row values times intensity for each cell</LI>
       <LI> SSx -Sum of col values for each cell</LI>
       <LI> SSy -Sum of row values for each cell</LI>
       <LI> Intensities  - Sum of the intensities for all the cells </LI>
      
    </UL>

    @author Ruth Mikkelson, SNS ORNL
    @date 11/4/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    
    
   class  DLLExport  BivariateNormal: public CurveFitting::UserFunction
    {
    
    public:
    
      
      BivariateNormal();
      
      /// Destructor
      virtual ~BivariateNormal ();

    

      /// overwrite IFunction base class methods
      std::string name()const
          {return "BivariateNormal";}
          
      void 	function  (double *out, const double *xValues, const int &nData) ; 
      
      void 	functionDeriv  (API::Jacobian  *out, const double *xValues, const int &nData);
      
      int 	nAttributes  () const
           { return 16;}
           
      std::vector< std::string > getAttributeNames () const;
    
 	  
      Mantid::API::IFitFunction::Attribute getAttribute(const std::string &attName)const;
      
 	  void 	setAttribute (const std::string &attName, const  Mantid::API::IFitFunction::Attribute &att);
 	  
      bool   hasAttribute (const std::string &attName) const;
 	 
    protected:
   
      void init();
      
      void initCommon();//Check for changes in parameters, etc. Calculates common values
      
      double* Attrib;//Saves Attribute values
      
      double* LastParams;//Saves previous/this set of parameters
      
      std::vector<std::string> AttNames;
      
      double SIxx,
             SIyy,
             SIxy,
             Sxx,
             Syy,
             Sxy; //Var and CoVar calc for 0 background and means sample means
             
      double mIx,
             mx,
             mIy,
             my; //Means corresp to background = 0
             
      double* expVals;// Save common exponential values for each cell
      
      double uu,
             coefNorm , 
             expCoeffx2 , 
             expCoeffy2 , 
             expCoeffxy; //Other common values used in calculating values and
                         //derivatives
      
      BoundaryConstraint *BackConstraint;
      
      BoundaryConstraint *MeanxConstraint;
      
      BoundaryConstraint *MeanyConstraint;

      BoundaryConstraint *IntensityConstraint;

    };



#endif
