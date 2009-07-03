//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Regroup.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"

#include <sstream>
#include <numeric>
#include <algorithm>
#include <functional>
#include <math.h>

#include <iostream>

namespace Mantid
{
namespace Algorithms
{

/// Custom validator for the "params" parameter of this algorithm
class RegroupParamsValidator: public Kernel::IValidator<std::vector<double> >
{
public:
  RegroupParamsValidator() {}
  virtual ~RegroupParamsValidator() {}

  const std::string getType() const { return "regroup"; }

  Kernel::IValidator<std::vector<double> >* clone() { return new RegroupParamsValidator(*this); }
private:
  ///Quick check on the inputed bin boundaries and widths
  std::string checkValidity( const std::vector<double> &value ) const;
};

  /** Quick check on the inputed bin boundaries and widths, returns a user level description of problems or "" for no error.  Note, note error doesn't mean that the values will work
   *  @param value The vector of doubles to check
   *  @return A user level description of any problem or "" if there is no problem
   */
std::string RegroupParamsValidator::checkValidity( const std::vector<double> &value ) const
{
  if ( value.empty() ) return "Enter values for this property";
  if ( ( value.size()%2 == 0 ) || ( value.size() == 1 ) )
  {
    return "The number of bin boundaries must be even";
  }

  double previous = value[0];
  for(size_t i=2; i < value.size(); i+=2)
  {
    if (value[i] <= previous)
	{
	  return "Bin boundary values must be given in order of increasing value";
	}
	else previous = value[i];
  }
  return "";
}

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Regroup)

using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace_const_sptr;
using API::MatrixWorkspace;

/// Initialisation method. Declares properties to be used in algorithm.
void Regroup::init()
{
  API::CompositeValidator<> *wsVal = new API::CompositeValidator<>;
  wsVal->add(new API::HistogramValidator<>);
  wsVal->add(new API::CommonBinsValidator<>);
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input, wsVal),
    "Name of the input workspace" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the regrouping");

  declareProperty(
    new ArrayProperty<double>("params", new RegroupParamsValidator),
    "The new approximate bin widths in the form x1, deltax1, x2, deltax2, x3, ..." );
}

/** Executes the regroup algorithm
 *
 *  @throw runtime_error Thrown if
 */
void Regroup::exec()
{
  // retrieve the properties
  std::vector<double> rb_params=getProperty("params");

  // Get the input workspace
  MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");

  // can work only if all histograms have the same boundaries
  if (!API::WorkspaceHelpers::commonBoundaries(inputW))
  {
    g_log.error("Histograms with different boundaries");
    throw std::runtime_error("Histograms with different boundaries");
  }

  bool dist = inputW->isDistribution();

  int histnumber = inputW->getNumberHistograms();
  DataObjects::Histogram1D::RCtype XValues_new;
  const std::vector<double> &XValues_old = inputW->readX(0);
  std::vector<int> xoldIndex;// indeces of new x in XValues_old
  // create new output X axis
  int ntcnew = newAxis(rb_params,XValues_old,XValues_new.access(),xoldIndex);

  // make output Workspace the same type is the input, but with new length of signal array
  API::MatrixWorkspace_sptr outputW = API::WorkspaceFactory::Instance().create(inputW,histnumber,ntcnew,ntcnew-1);
  // Try to cast it to a Workspace2D for use later
  DataObjects::Workspace2D_sptr outputW_2D = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(outputW);

  int progress_step = histnumber / 100;
  if (progress_step == 0) progress_step = 1;
  for (int hist=0; hist <  histnumber;hist++)
  {
    // get const references to input Workspace arrays (no copying)
    const std::vector<double>& XValues = inputW->readX(hist);
    const std::vector<double>& YValues = inputW->readY(hist);
    const std::vector<double>& YErrors = inputW->readE(hist);

    //get references to output workspace data (no copying)
    std::vector<double>& YValues_new=outputW->dataY(hist);
    std::vector<double>& YErrors_new=outputW->dataE(hist);

    // output data arrays are implicitly filled by function
    rebin(XValues,YValues,YErrors,xoldIndex,YValues_new,YErrors_new, dist);

    // Populate the output workspace X values
    if (outputW_2D)
    {
      outputW_2D->setX(hist,XValues_new);
    }
    else
    {
      outputW->dataX(hist)=XValues_new.access();
    }
    if (hist % progress_step == 0)
    {
        progress(double(hist)/histnumber);
        interruption_point();
    }
  }

  outputW->isDistribution(dist);

  // Copy units
  if (outputW->getAxis(0)->unit().get())
    outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
  try
  {
    if (inputW->getAxis(1)->unit().get())
      outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
   }
  catch(Exception::IndexError) {
    // OK, so this isn't a Workspace2D
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputW);

  return;
}

/** Regroup the data according to new output X array
 *
 * @param xold - old x array of data
 * @param xoldIndex - indeces of new x in XValues_old
 * @param yold - old y array of data
 * @param ynew - new y array of data
 * @param eold - old error array of data
 * @param enew - new error array of data
 * @param distribution - flag defining if distribution data (1) or not (0)
 * @throw runtime_error Thrown if algorithm cannot execute
 * @throw invalid_argument Thrown if input to function is incorrect
 **/
void Regroup::rebin(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
    const std::vector<int>& xoldIndex, std::vector<double>& ynew, std::vector<double>& enew, bool distribution)
{

  for(int i=0;i<int(xoldIndex.size()-1);i++)
  {

    int n = xoldIndex[i];// start the group
    int m = xoldIndex[i+1];// end the group
    double width = xold[m] - xold[n]; // width of the group
    
    if (width == 0.)
    {
      g_log.error("Zero bin width");
      throw std::runtime_error("Zero bin width");
    }
    /*
     *        yold contains counts/unit time, ynew contains counts
     *	       enew contains counts**2
     */
    if(distribution)
    {
      ynew[i] = 0.;
      enew[i] = 0.;
      for(int j=n;j<m;j++)
      {
        double wdt = xold[j+1] - xold[j]; // old bin width
        ynew[i] += yold[j]*wdt;
        enew[i] += eold[j]*eold[j]*wdt*wdt;
      }
      ynew[i] /= width;
      enew[i] = sqrt(enew[i])/width;
    }
    else// yold,eold data is not distribution but counts
    {
      ynew[i] = 0.;
      enew[i] = 0.;
      for(int j=n;j<m;j++)
      {
        ynew[i] += yold[j];
        enew[i] += eold[j]*eold[j];
      }
      enew[i] = sqrt(enew[i]);
    }
  }

  return; //without problems
}

/** Creates a new  output X array  according to specific boundary defnitions
 *
 * @param params    rebin parameters input [x_1, delta_1,x_2, ... ,x_n-1,delta_n-1,x_n)
 * @param xold      the current x array
 * @param xnew      new output workspace x array
 * @param xoldIndex indeces of new x in XValues_old
 **/
int Regroup::newAxis(const std::vector<double>& params,
    const std::vector<double>& xold, std::vector<double>& xnew,std::vector<int> &xoldIndex)
{
  double xcurr, xs;
  int ibound(2), istep(1), inew(0);
  int ibounds=params.size(); //highest index in params array containing a bin boundary
  int isteps=ibounds-1; // highest index in params array containing a step

  xcurr = params[0];
  std::vector<double>::const_iterator iup =
    std::find_if(xold.begin(),xold.end(),std::bind2nd(std::greater_equal<double>(),xcurr));
  if (iup != xold.end())
  {
    xcurr = *iup;
    xnew.push_back(xcurr);
    xoldIndex.push_back(inew);
    inew++;
  }
  else
    return 0;

  while( (ibound <= ibounds) && (istep <= isteps) )
  {
    // if step is negative then it is logarithmic step
    if ( params[istep] >= 0.0)
      xs = params[istep];
    else
      xs = xcurr * fabs(params[istep]);

    //xcurr += xs;

    // find nearest x_i that is >= xcurr
    iup = std::find_if(xold.begin(),xold.end(),std::bind2nd(std::greater_equal<double>(),xcurr+xs));
    if (iup != xold.end())
    {
      if (*iup <= params[ibound])
      {
        xcurr = *iup;
        xnew.push_back(xcurr);
        xoldIndex.push_back(inew);
        inew++;
      }
      else
      {
        ibound += 2;
        istep += 2;
      }
    }
    else
      return inew;
  }
  //returns length of new x array or -1 if failure
  return inew;
  //return( (ibound == ibounds) && (istep == isteps) ? inew : -1 );
}

} // namespace Algorithm
} // namespace Mantid
