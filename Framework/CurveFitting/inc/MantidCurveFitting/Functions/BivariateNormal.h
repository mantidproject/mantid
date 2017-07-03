#ifndef MANTID_CURVEFITTING_BIVARIATENORMAL_H_
#define MANTID_CURVEFITTING_BIVARIATENORMAL_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/Functions/UserFunction.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace HistogramData {
class HistogramY;
}
namespace CurveFitting {
namespace Functions {

/**
  * Provide peak shape function interface a Peak shape on one time slice of a
  * RectangularDetector.
  * i.e. the function: Background +Intensity*
  *        NormalDist( col,row,col_mean,row_mean,col_sigma,row_sigma,
  covariance)
  *
  * Where NormalDist is the bivariate normal distribution whose total "area" is
  * 1. So Intensity should be the integrated intensity.
  *
  * BivariateNormal parameters:
  *<UL>
  * <LI> Background - The background of the peak</LI>
  *  <LI> Intensity - The intensity of data for the peak on this time slice
  </LI>
  *  <LI> Mcol -  The col of the center of the peak </LI>
  *  <LI> Mrow - The row of the center of the peak on this slice</LI>
  *  <LI> SScol -The variance of the column values in the peak for this time
  slice </LI>
  *  <LI> SSrow - The variance of the row values in the peak for this time slice
  </LI>
  *  <LI> SSrc - The covariance of the row and column values in the peak for
  this time slice </LI>
  * </UL>
  * There is one attribute, This must be specified.
  * <UL>
  *   <LI> CalcVariances -
  *                           If true,calculates SScol, SSrow, and SSrc from the
  experimental data
  *                           given Background, Mcol, and Mrow,if the parameter
  has not been tied,
  *                           If false, the parameters SScol, SSrow, and SSrc
  will be fit, unless
  *                            tied.
  *  </LI>
  * </UL>
  *
  * This is a bivariate function.  The workspace must have three histograms of
  equal length.
  * Histogram 0: Contains the experimental values for each x and y, along with
  their errors.
  * Histogram 1: Contains the corresponding x value for the data in Histogram 0
  * Histogram 2: contains the corresponding y values for the data in Histogram 0
  *
  * @author Ruth Mikkelson, SNS ORNL
  * @date 11/4/2011
  *
   Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
   along with this program.  If not, see <http://www.gnu.org/licenses>.

   File change history is stored at: <https://github.com/mantidproject/mantid>
   Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport BivariateNormal : public API::ParamFunction,
                                  public API::IFunction1D,
                                  public API::IFunctionMW {
public:
  BivariateNormal();
  /// Destructor
  ~BivariateNormal() override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "BivariateNormal"; }

  const std::string category() const override { return "Peak"; }

  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;

  size_t nAttributes() const override { return (size_t)1; }

  std::vector<std::string> getAttributeNames() const override {
    std::vector<std::string> V;
    V.emplace_back("CalcVariances");
    return V;
  }

  Attribute getAttribute(const std::string &attName) const override {
    if (!hasAttribute(attName))
      throw std::invalid_argument("Not a valid attribute name");

    // if( CalcVariances)
    //  return Attribute( 1);

    return Attribute(CalcVariances);
  }

  void setAttribute(const std::string &attName,
                    const Attribute &value) override {

    if (!hasAttribute(attName))
      throw std::invalid_argument("Not a valid attribute name");

    CalcVariances = value.asBool();

    if (CalcVariances) {
      CalcVxx = CalcVyy = CalcVxy = true;
    } else {
      declareParameter("SScol", 0.00, "Variance of the column(x) values");
      declareParameter("SSrow", 0.00, "Variance of the row(y) values");
      declareParameter("SSrc", 0.00,
                       "Covariance of the column(x) and row(y) values");
      CalcVxx = CalcVyy = CalcVxy = false;
    }
  }

  bool hasAttribute(const std::string &attName) const override {
    return attName == "CalcVariances";
  }

  bool CalcVxx, CalcVyy, CalcVxy;

protected:
  void init() override;

  int NCells;

  bool CalcVariances; ///< from experimental data versus fit the (Co)Variances

  double initCommon(); ///<Check for changes in parameters, etc. Calculates
  /// common values

  // Returns penalty.
  double initCoeff(const HistogramData::HistogramY &D,
                   const HistogramData::HistogramY &X,
                   const HistogramData::HistogramY &Y, double &coefNorm,
                   double &expCoeffx2, double &expCoeffy2, double &expCoeffxy,
                   int &NCells, double &Varxx, double &Varxy,
                   double &Varyy) const;

  double mIx, mx, mIy, my;                //< For calculating variances
  double SIxx, SIyy, SIxy, Sxx, Syy, Sxy; //< For calculating variances
  double TotI, TotN;                      //< For calculating variances

  double Varx0, Vary0; // Crude estimate of the variances for bounds on
                       // variances

  double LastParams[9]; ///< Saves previous/this set of parameters

  double *expVals; ///< Save common exponential values for each cell

  double uu, coefNorm, expCoeffx2, expCoeffy2,
      expCoeffxy; //<Other common values used in calculating values and
                  //<derivatives
};
} // namespace Functions
}
}

#endif
