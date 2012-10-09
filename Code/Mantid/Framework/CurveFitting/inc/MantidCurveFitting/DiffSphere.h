#ifndef MANTID_DIFFSPHERE_H_
#define MANTID_DIFFSPHERE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
#include "DeltaFunction.h"

namespace Mantid
{
namespace CurveFitting
{
  /**
  @author Jose Borreguero, NScD
  @date 11/14/2011

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

/// structure to hold info on Volino's coefficients
struct xnlc{
  double x;
  unsigned int l;
  unsigned int n;
};

/// simple structure to hold a linear interpolation of factor J around its numerical divergence point
struct linearJ{
  double slope;
  double intercept;
};

class DLLExport ElasticDiffSphere : public DeltaFunction
{
public:

  /// Constructor
  ElasticDiffSphere();

  /// Destructor
  virtual ~ElasticDiffSphere() {};

  /// overwrite IFunction base class methods
  virtual std::string name()const{return "ElasticDiffSphere";}

  /// A rescaling of the peak intensity
  double HeightPrefactor() const;

  /// Returns the number of attributes associated with the function
  virtual size_t nAttributes()const{return 1;}

  /// Returns a list of attribute names
  virtual std::vector<std::string> getAttributeNames()const;

  /// Return a value of attribute attName
  virtual Attribute getAttribute(const std::string& attName)const;

  /// Set a value to attribute attName
  virtual void setAttribute(const std::string& attName,const Attribute& );

  /// Check if attribute attName exists
  virtual bool hasAttribute(const std::string& attName)const;

private:

  /// Q-value, modulus of the momentum transfer
  double m_Q;

};

/* Class representing the inelastic portion of the DiffSphere algorithm.
 * Contains the 98 Lorentzians.
 */
class DLLExport InelasticDiffSphere : public API::ParamFunction, public API::IFunction1D
{
public:

  /// Constructor
  InelasticDiffSphere();

  /// Destructor
  virtual ~InelasticDiffSphere() {}

  /// overwrite IFunction base class methods
  virtual std::string name()const{return "InelasticDiffSphere";}

  virtual const std::string category() const { return "QuasiElastic";}

  /// Returns the number of attributes associated with the function
  virtual size_t nAttributes()const{return 1;}

  /// Returns a list of attribute names
  virtual std::vector<std::string> getAttributeNames()const;

  /// Return a value of attribute attName
  virtual Attribute getAttribute(const std::string& attName)const;

  /// Set a value to attribute attName
  virtual void setAttribute(const std::string& attName,const Attribute& );

  /// Check if attribute attName exists
  virtual bool hasAttribute(const std::string& attName)const;

protected:
  void function1D(double* out, const double* xValues, const size_t nData)const;
  std::vector<double> LorentzianCoefficients(double a) const;

private:

  /// initialize the Xnl coefficients
  void initXnlCoeff();

  /// initialize the alpha coefficients
  void initAlphaCoeff();

  /// initialize the list of Linearized J values
  void initLinJlist();

  /// xnl coefficients
  std::vector<xnlc> xnl;

  /// certain coefficients invariant during fitting
  std::vector<double> alpha;

  /// maximum value of l in xnlist
  unsigned int lmax;

  /// number of coefficients
  unsigned int ncoeff;

  /// linear interpolation zone around the numerical divergence of factor J
  double divZone;

  /// list of linearized J values
  std::vector<linearJ> linearJlist;

  /// Q-value, modulus of the momentum transfer
  double m_Q;
}; // end of class InelasticDiffSphere

class DLLExport DiffSphere : public API::ImmutableCompositeFunction
{
public:
  /// Constructor
  DiffSphere();
  /// Destructor
  ~DiffSphere() {};

  /// overwrite IFunction base class methods
  std::string name()const{return "DiffSphere";}
  virtual const std::string category() const { return "QuasiElastic";}
  virtual int version() const { return 1;}

private:
  //API::IFunctionMW* m_elastic;    //elastic intensity of the DiffSphere structure factor
  boost::shared_ptr<ElasticDiffSphere> m_elastic;
  boost::shared_ptr<InelasticDiffSphere> m_inelastic;  //inelastic intensity of the DiffSphere structure factor
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_DIFFSPHERE_H_*/
