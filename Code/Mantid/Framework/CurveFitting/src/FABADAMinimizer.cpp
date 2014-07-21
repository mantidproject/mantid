#include "MantidCurveFitting/FABADAMinimizer.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"

#include <stdio.h>
#include<stdlib.h>
#include<time.h>

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidKernel/Logger.h"

 /// Probability calculation function
double probability(double x2old, double x2new){
  double pro_old = exp(-x2old/2);
  double pro_new = exp(-x2new/2);
  double pro_change = pro_new/pro_old;
  return pro_change;
}

/// Changing function
bool cambio(double prob) {
   ///default_random_engine generator(time(0));
   ///uniform_real_distribution<double> distribution(0.0,1.0);
   /// double p = distribution(generator);
   srand(time_t());
   double p = rand()%1;
   if (p <= prob){
     return true;
   }
   else{
     return false;
   }
}

/// Jump updating
double jump_update(double j, size_t c, double c_c) {
  double jnew;
  if (c_c == 0.0) {
    jnew = j/50.0;
  }
  else {
  double f = c_c/double(c);
  jnew = j*f/0.66666666;
  }
  return jnew;
}

namespace Mantid
{
namespace CurveFitting
{


namespace
{
  /// static logger object
  Kernel::Logger g_log("FABADAMinimizer");
}


DECLARE_FUNCMINIMIZER(FABADAMinimizer, FABADA)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FABADAMinimizer::FABADAMinimizer()
  {
    declareProperty("NumberIterations",static_cast<size_t>(100),"Number of iterations to run.");
    declareProperty(
      new API::WorkspaceProperty<>("OutputWorkspace","abc",Kernel::Direction::Output),
      "The name to give the output workspace");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FABADAMinimizer::~FABADAMinimizer()
  {
  }

  void FABADAMinimizer::initialize(API::ICostFunction_sptr function)
  {
    f_leastSquares = boost::dynamic_pointer_cast<CostFuncLeastSquares>(function);
    if ( !f_leastSquares )
      {
        throw std::invalid_argument("FABADA works only with least squares. Different function was given.");
      }

    size_t n =  f_leastSquares->nParams();
    counter = 0;
    f_leastSquares->getParameters(parameters);
    for (unsigned int i ; i<n ; ++i) {
      changes.push_back(0);
      jump.push_back(parameters.get(i)/10);
    }
    chi2 = f_leastSquares -> val();
    numberIterations = getProperty("NumberIterations");
  }

  bool FABADAMinimizer::iterate()
  {
    if ( !f_leastSquares )
      {
        throw std::runtime_error("Cost function isn't set up.");
      }




    size_t n = f_leastSquares->nParams();

    for (unsigned int i; i<n ; i++) {
      GSLVector new_parameters = parameters;
      double new_value = parameters.get(i) + jump[i];
      new_parameters.set(i,new_value);
      f_leastSquares -> setParameter(i,new_value);
      double chi2_new = f_leastSquares -> val();
      if (chi2_new < chi2) {
        chain.push_back(new_parameters);
        parameters = new_parameters;
        chi2 = chi2_new;
        changes[i] += 1;
      }
      else {
        double prob = probability(chi2, chi2_new);
        bool chg = cambio(prob);
        if (chg) {
            chain.push_back(new_parameters);
            parameters = new_parameters;
            chi2 = chi2_new;
            changes[i] += 1;
        }
        else {
            chain.push_back(parameters);
            jump[i] = -jump[i];
        }
      }
      if (counter%20 == 19) {
          jump[i] = jump_update(jump[i], counter, changes[i]);
      }

    }
    counter += 1;
    if (counter<numberIterations){
      return true;
    }
    else {
        API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create("Workspace2D",n, numberIterations, numberIterations - 1);
        for(size_t i = 0; i < ws->getNumberHistograms(); ++i)
        {
            MantidVec & X = ws->dataX(i);
            MantidVec & Y = ws->dataY(i);
            MantidVec & E = ws->dataE(i);
            for(size_t j = 0; j < X.size(); ++j)
            {
                X[j] = chain[j].get(i);
                Y[j] = 5;
                E[j] = 1.0;
            }
        }
        setProperty("OutputWorkspace",ws);
        API::AnalysisDataService::Instance().addOrReplace("MyOutputProperty",ws);
        return false;
    }
  }

  double FABADAMinimizer::costFunctionVal()
  {
      return 0.0;
  }
  

} // namespace CurveFitting
} // namespace Mantid
