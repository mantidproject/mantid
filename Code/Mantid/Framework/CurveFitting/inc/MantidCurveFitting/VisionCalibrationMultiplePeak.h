#ifndef MANTID_VISIONCALIBRATIONMULTIPLEPEAK_H_
#define MANTID_VISIONCALIBRATIONMULTIPLEPEAK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace CurveFitting
{
/**This MultiDimensional fitting function fits a series of spectra,
each spectra containing one peak. Each spectra is fit as the sum a linear
background and two Gaussians (Gaussian1 and Gaussian2).
The heights of the Gaussian1 functions (there is one per spectrum) are
tied according to a quadratic expression. Similarly for the set of Gaussian2
functions

@author Jose Borreguero, NScD
@date Aug/17/2012

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
  */


/* Class to fit a single spectrum as a linear background plus two Gaussians.
 * The height of each Gaussian is a quadratic function
 */

class DLLExport VisionCalibrationLBGGs : public API::ParamFunction, public API::IFunction1D
{

public:
  VisionCalibrationLBGGs();
  virtual ~VisionCalibrationLBGGs() {};
  virtual std::string name() const { return "VisionCalibrationLBGGs"; }
  virtual const std::string category() const { return "Vision\\FitFunctions"; }
  /// Set the active parameters in place of the Gaussian standard deviations
  virtual void setActiveParameter( size_t i, double value );
  /// Return the active parameter value in place of the Gaussian standard deviation
  virtual double activeParameter( size_t i ) const;
  virtual size_t nAttributes() const { return 1; }
  virtual void setAttribute( const std::string& attName, const Attribute& );
  virtual Attribute getAttribute( const std::string& attName ) const;
  virtual bool hasAttribute( const std::string& attName ) const { return attName == m_attName; };

protected:
  virtual void function1D( double* out, const double* xValues, const size_t nData ) const;
  virtual void functionDeriv1D( API::Jacobian* out, const double* xValues, const size_t nData );

  private:
  const std::string m_attName; //name of the attribute
  mutable Attribute m_att;    // attribute

}; // class VisionCalibrationLBGGs

/* Class to fit multiple spectra as a linear background plus two Gaussians
 * (gaussian1 and gaussian2). The height of the gaussian1 is a
 * quadratic function whose parameters are the tied for all spectra.
 * Similarly for gaussian2.
 */
class DLLExport VisionCalibrationLBGGm : public API::MultiDomainFunction
{
public:
  VisionCalibrationLBGGm() : m_attName("range"), m_att(0) {}
  virtual ~VisionCalibrationLBGGm() {}
  virtual std::string name() const { return "VisionCalibrationLBGGm"; }
  virtual const std::string category() const { return "Vision\\FitModels"; }
  //Override attribute methods from parent IFunction
  virtual size_t nAttributes() const { return 1; }
  virtual void setAttribute( const std::string& attName,const Attribute& );
  virtual Attribute getAttribute( const std::string& attName ) const;
  virtual bool hasAttribute( const std::string& attName ) const { return attName == m_attName; }
  //Reserve the localAttribute methods of parentMultiDomainFunction for the domains

protected:
  void addFunctions();

private:
  /// number of spectra to fit at the same time. Can be negative
  const std::string m_attName;
  API::IFunction::Attribute m_att;
  std::vector<std::string> m_ties;

}; // VisionCalibrationLBGGm

/* Algorithm to call in order to calibrate multiple spectra
 */
class DLLExport VisionCalibrationMultiplePeak : public API::Algorithm
{
public:
  VisionCalibrationMultiplePeak();
  virtual ~VisionCalibrationMultiplePeak() {}
  virtual int version() const { return 1; }
  virtual const std::string name() const { return "VisionCalibrationMultiplePeak"; }
  virtual const std::string category() const { return "Vision\\FitModels"; }

protected:
  /// Do a sequential unidimensional fitting to estimate initial parameters
  void EstimateInitialParameters();

private:
  /// Initialize input and output algorithm properties
  virtual void init();
  /// Execute the algorithm by performing the fitting
  virtual void exec();

  /// maps a model option in the algorithm menu to a class
  typedef std::map< std::string, std::string, std::less<std::string> > mss;
  mss m_fitModelsMap;
  mss m_modelsDescription;
  API::IFunction_sptr m_fitModel;

}; //class VisionCalibrationMultiplePeak

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_VISIONCALIBRATIONMULTIPLEPEAK_H_*/
