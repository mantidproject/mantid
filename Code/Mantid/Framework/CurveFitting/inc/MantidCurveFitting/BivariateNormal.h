#ifndef MANTID_CURVEFITTING_BIVARIATENORMAL_H_
#define MANTID_CURVEFITTING_BIVARIATENORMAL_H_
 
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMW.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidKernel/Logger.h"
#include "MantidCurveFitting/BoundaryConstraint.h"

namespace Mantid
{
  namespace CurveFitting
  {
   
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
        <LI> NRows -The number of rows of data that will be fit. NOTE: The index, i in Data[0] of the workspace,
             of a cell at row,col is i= (row-StartRow)*NCols+(col - StartCol)</LI>
       <LI> NCols- The number of columns of data that will be fit.NOTE: The index, i in Data[0] of the workspace,
             of a cell at row,col is i= (row-StartRow)*NCols+(col - StartCol)</LI>
       <LI>SSIxx  = &Sigma<sub>cell</sub> (Intensity<sub>cell</sub>*column<sub>cell</sub><sup>2</sup>)</LI>
       <LI>SSIyy = &Sigma<sub>cell</sub> (Intensity<sub>cell</sub>*row<sub>cell</sub><sup>2</sup>)</LI>
       <LI>SSIxy = &Sigma<sub>cell</sub> (Intensity<sub>cell</sub>*column<sub>cell</sub>*row<sub>cell</sub>)</LI></LI>
       <LI>SSxx  = &Sigma<sub>cell</sub> (column<sub>cell</sub><sup>2</sup>)
       <LI>SSyy  = &Sigma<sub>cell</sub> (row<sub>cell</sub><sup>2</sup>)</LI>
       <LI>SSxy  = &Sigma<sub>cell</sub> (column<sub>cell</sub>*row<sub>cell</sub>)</LI>
       <LI> SSIx = &Sigma<sub>cell</sub> (Intensity<sub>cell</sub>*column<sub>cell</sub>)</LI>
       <LI> SSIy = &Sigma<sub>cell</sub> (Intensity<sub>cell</sub>*row<sub>cell</sub>)</LI>
       <LI> SSx= &Sigma<sub>cell</sub> (column<sub>cell</sub>)</LI>
       <LI> SSy = &Sigma<sub>cell</sub> (row<sub>cell</sub>)</LI>
       <LI> Intensities = &Sigma<sub>cell</sub> (Intensity<sub>cell</sub>)</LI>
      
      
    </UL>

    Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class  DLLExport  BivariateNormal:public BackgroundFunction
    {
    public:
      BivariateNormal();
      /// Destructor
      virtual ~BivariateNormal ();

      /// overwrite IFunction base class methods
      std::string name()const {return "BivariateNormal";}

      virtual const std::string category() const { return "Peak";}

      void functionMW  (double *out, const double *xValues, const size_t nData)const ;

      void functionDerivMW  (API::Jacobian  *out, const double *xValues, const size_t nData);

      size_t nAttributes  () const
      { return 16;}

      std::vector< std::string > getAttributeNames () const;


      Mantid::API::IFitFunction::Attribute getAttribute(const std::string &attName)const;

      void 	setAttribute (const std::string &attName, const  Mantid::API::IFitFunction::Attribute &att);

      bool   hasAttribute (const std::string &attName) const;

      void  fit(const std::vector<double>& ,const std::vector<double>&);
    protected:

      void init();

      /// Check for changes in parameters, etc. Calculates common values
      void initCommon();

      /// Check for changes in given parameters, etc. Calculates common values
      void initCommon(  double* LastParams,double* expVals,double &uu,
          double &coefNorm,double &expCoeffx2,double  &expCoeffy2,double  &expCoeffxy,
          bool &isNaNs) const;

      double* Attrib; ///< Saves Attribute values

      double* LastParams; ///< Saves previous/this set of parameters

      std::vector<std::string> AttNames; ///< List of attribute names

      double mIx,///< =&Sigma<sub>cell</sub>(Intensity<sub>cell</sub>*column<sub>cell</sub>)/TotalIntensity.
      mx, ///< =&Sigma<sub>cell</sub>(column<sub>cell</sub>)/Ncells.
      mIy,///< =&Sigma<sub>cell</sub>(Intensity<sub>cell</sub>*row<sub>cell</sub>)/TotalIntensity.
      my; ///< =&Sigma<sub>cell</sub>(row<sub>cell</sub>)/Ncells

      double SIxx,///<=&Sigma<sub>cell</sub> Intensity<sub>cell</sub>*(column<sub>cell</sub>-mIx)<sup>2</sup>
      SIyy,///<=&Sigma<sub>cell</sub> Intensity<sub>cell</sub>*(row<sub>cell</sub>-mIy)<sup>2</sup>
      SIxy,///<=&Sigma<sub>cell</sub> Intensity<sub>cell</sub>*(column<sub>cell</sub>-mIx)*(row<sub>cell</sub>-mIy)
      Sxx,///<=&Sigma<sub>cell</sub> (column<sub>cell</sub>-mI)<sup>2</sup>
      Syy,///<=&Sigma<sub>cell</sub> (row<sub>cell</sub>-my)<sup>2</sup>
      Sxy;///<=&Sigma<sub>cell</sub> (column<sub>cell</sub>-mx)*(row<sub>cell</sub>-my)



      double* expVals; ///< Save common exponential values for each cell

      double uu,
      coefNorm ,
      expCoeffx2 ,
      expCoeffy2 ,
      expCoeffxy; //Other common values used in calculating values and
      //derivatives

      BoundaryConstraint *BackConstraint; ///< Constraint for background

      BoundaryConstraint *MeanxConstraint; ///< Constraint using mean x

      BoundaryConstraint *MeanyConstraint; ///< Constraint using mean y

      BoundaryConstraint *IntensityConstraint; ///< Constraint using intensity

      static Kernel::Logger & g_log;

    };
  }
}

#endif
