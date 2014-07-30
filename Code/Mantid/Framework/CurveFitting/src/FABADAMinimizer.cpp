#include "MantidCurveFitting/FABADAMinimizer.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/BoundaryConstraint.h"

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
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

#include "MantidKernel/Logger.h"

#include <boost/random/normal_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

#include <iostream>
#include <ctime>


/// Probability calculation function
double probability(double x2old, double x2new){
    double pro_change = exp((x2old/2.0)-(x2new/2.0));
    std::cout  << "PROBABILIDAD cambio:   " << pro_change << std::endl;
    return pro_change;
}

/// Generating random number
double frand(size_t c) {
    //unsigned int proba = 123 + int(c);
    //rand(time_t()+proba);
    //double f = double(rand()) / RAND_MAX;
    boost::mt19937 mt;
    mt.seed(time_t()+48*(int(c)+76));
    boost::random::uniform_real_distribution<> distr(0.0,1.0);
    return distr(mt);
}

///Generating Normal_random number
double normal_rand(double mean, double sigma, size_t c) {
    boost::mt19937 mt;
    mt.seed(123*(int(c)+45));
    boost::random::normal_distribution<> distr(mean,sigma);
    return distr(mt);
}

// Changing function
bool cambio(double prob, size_t c) {
   double p = frand(c);
   std::cout << " Random number " << p << std::endl;
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
       jnew = j/10.0;
  }
  else {
       double f = c_c/double(c);
       jnew = j*f/0.6666666666;
  std::cout << f << "   m_counter  "<< c << "   m_changes   " << c_c << std::endl;
  }

  return jnew;
}


// Probability Density Function creator
std::vector<double> PDF(std::vector<double> v, std::vector<double>& x, double val_par, std::vector<double>& error, double& mostP) {
  std::vector<double> y(100,0);
  size_t length = v.size();
  std::sort(v.begin(),v.end());
  std::vector<double>::const_iterator pos_par = std::find(v.begin(),v.end(),val_par);
  int sigma = int(0.34*double(length));
  std::vector<double>::const_iterator pos_left = pos_par - sigma;
  std::vector<double>::const_iterator pos_right = pos_par + sigma;
  error.push_back(*pos_left - *pos_par);
  error.push_back(*pos_right - *pos_par);
  double start = v[0];
  double bin = (v[length-1] - start)/100;
  size_t step = 0;
  x[0] = start;
  for (int i = 1; i<101; i++) {
      double bin_end = start + i*bin;
      x[i] = bin_end;
      while(step<length && v[step] <= bin_end) {
        y[i-1] += 1;
        ++step;
      }
  }
  std::vector<double>::iterator pos_MP = std::max_element(y.begin(),y.end());
  mostP = x[pos_MP-y.begin()]+(bin/2.0);
  return y;
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
    ////static_cast<size_t>
    declareProperty("NumberIterations",10000,"Number of iterations to run.");
    declareProperty(
      new API::WorkspaceProperty<>("OutputWorkspacePDF","pdf",Kernel::Direction::Output),
      "The name to give the output workspace");
    declareProperty(
      new API::WorkspaceProperty<>("OutputWorkspaceChain","chain",Kernel::Direction::Output),
      "The name to give the output workspace");
    declareProperty(
      new API::WorkspaceProperty<>("OutputWorkspaceConverged","conv",Kernel::Direction::Output),
      "The name to give the output workspace");
    declareProperty(
          new API::WorkspaceProperty<API::ITableWorkspace>("ChiSquareTable","chi2",Kernel::Direction::Output),
      "The name to give the output workspace");
    declareProperty(
          new API::WorkspaceProperty<API::ITableWorkspace>("PdfError","pdfE",Kernel::Direction::Output),
      "The name to give the output workspace");
    //declareProperty("Converged chain", true, "Show the coverged part of the chain separately");


  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FABADAMinimizer::~FABADAMinimizer()
  {
  }

  void FABADAMinimizer::initialize(API::ICostFunction_sptr function)
  {
    m_leastSquares = boost::dynamic_pointer_cast<CostFuncLeastSquares>(function);
    if ( !m_leastSquares )
      {
        throw std::invalid_argument("FABADA works only with least squares. Different function was given.");
      }

    size_t n =  m_leastSquares->nParams();
    m_counter = 0;
    m_leastSquares->getParameters(m_parameters);
    API::IFunction_sptr fun = m_leastSquares->getFittingFunction();
    for (size_t i=0 ; i<n ; ++i) {
      std::vector<double> v;
      double p = m_parameters.get(i);
      m_bound.push_back(false);
      API::IConstraint *iconstr = fun->getConstraint(i);
      if ( iconstr )
      {
          BoundaryConstraint *bcon = dynamic_cast<BoundaryConstraint*>(iconstr);
          if ( bcon )
          {
             m_bound[i] = true;
             if ( bcon -> hasLower()) {
                 m_lower.push_back(bcon->lower());
             }
             else {
                 m_lower.push_back(-10e50);
             }
             if ( bcon -> hasUpper()){
                 m_upper.push_back(bcon -> upper());
             }
             else {
                 m_upper.push_back(10e50);
             }
             if (p<m_lower[i]) {
                 p = m_lower[i];
                 m_parameters.set(i,p);
             }
             if (p>m_upper[i]) {
                 p = m_upper[i];
                 m_parameters.set(i,p);
             }

          }

      }
      v.push_back(p);
      m_chain.push_back(v);
      m_changes.push_back(0);
      m_par_converged.push_back(false);
      if (p != 0.0) {
        m_jump.push_back(std::abs(p/10));
      }
      else {
        m_jump.push_back(0.1);
      }
    }
    m_chi2 = m_leastSquares -> val();
    std::vector<double> v;
    v.push_back(m_chi2);
    m_chain.push_back(v);
    m_numberIterations = getProperty("NumberIterations");
    m_converged = false;
  }
    


  bool FABADAMinimizer::iterate()
  {
    if ( !m_leastSquares )
      {
        throw std::runtime_error("Cost function isn't set up.");
      }


    size_t n = m_leastSquares->nParams();
    srand(time_t());

    for(size_t i = 0; i<n ; i++) {
      GSLVector new_parameters = m_parameters;
      double step;
      if (m_converged) {
        step = normal_rand(0,std::abs(m_jump[i]), m_counter);
        if (step == 0) {
            step = 0.00000001;
        }
      }
      else {
          step = m_jump[i];
          //step = std::abs(normal_rand(0,std::abstd::s(m_jump[i]), m_counter));
          if (step == 0) {
            step = 0.00000001;
          }
      }
      std::cout << m_counter << "." << i << std::endl;
      std::cout << "Real step:  " << step << "      Due to the m_jump:  " << m_jump[i] << std::endl;
      double new_value = m_parameters.get(i) + step;
      //std::cout<< "constrictions?????    " << m_bound[i];
      if (m_bound[i]) {
          if (new_value < m_lower[i]) {
              new_value = m_lower[i] + (m_lower[i]-new_value)/2;
          }
          if (new_value > m_upper[i]) {
              new_value = m_upper[i] - (new_value-m_upper[i])/2;
          }
      }

      new_parameters.set(i,new_value);
      m_leastSquares -> setParameter(i,new_value);
      double chi2_new = m_leastSquares -> val();
      std::cout << "OLD Chi2: " << m_chi2 << "      NEW Chi2:  " << chi2_new << std::endl;
      if (chi2_new < m_chi2) {
        for (size_t j=0; j<n ; j++) {
            m_chain[j].push_back(new_parameters.get(j));
        }
        m_chain[n].push_back(chi2_new);
        m_parameters = new_parameters;
        m_chi2 = chi2_new;
        m_changes[i] += 1;
        std::cout << "Salta directamente!!" << std::endl;
      }
      else {

        double prob = probability(m_chi2, chi2_new);
        bool chg = cambio(prob,m_counter+3*i);
        if (chg) {
            for (size_t j = 0; j<n ; j++) {
                m_chain[j].push_back(new_parameters.get(j));
            }
            m_chain[n].push_back(chi2_new);
            m_parameters = new_parameters;
            m_chi2 = chi2_new;
            m_changes[i] += 1;
            std::cout << "SI hay cambio" << std::endl;
        }
        else {
            for (size_t j = 0; j<n ; j++) {
                m_chain[j].push_back(m_parameters.get(j));
            }
            m_chain[n].push_back(m_chi2);
            m_leastSquares -> setParameter(i,new_value-m_jump[i]);
            m_jump[i] = -m_jump[i];
            std::cout << "NO hay cambio" << std::endl;
        }
        std::cout << std::endl << std::endl << std::endl;
      }
      if (m_counter%200 == 150) {
          //std::cout << std::endl << m_counter << "." << i << std::endl;
          m_jump[i] = jump_update(m_jump[i], m_counter, m_changes[i]);
          if (std::abs(m_jump[i])<0.0000000000000000000001) {
              API::IFunction_sptr fun = m_leastSquares->getFittingFunction();
              throw std::runtime_error("Wrong convergence for parameter " + fun -> parameterName(i) +". Try to set a proper initial value this parameter" );
              return false;
          }
      }
      if (!m_par_converged[i] && m_counter > 500) {
          if (m_chain[n][n*m_counter+i+1] != m_chain[n][n*m_counter+i]) {
            double chi2_quotient = std::abs(m_chain[n][n*m_counter+i+1]-m_chain[n][n*m_counter+i])/m_chain[n][n*m_counter+i];
            //std::cout << std::endl << "cociente par el parametro  " << i << "   "<< chi2_quotient << std::endl;
            if (chi2_quotient < 0.00001){
                  m_par_converged[i] = true;
            }
          }
      }
    }
    m_counter += 1;
    if (m_counter > 1000 && !m_converged){
        size_t t = 0;
        for(size_t i = 0; i<n ; i++) {
            if (m_par_converged[i]) {
                t += 1;
            }
        }
        if (t == n) {
                m_converged = true;
                m_conv_point = m_counter*n+1;
                m_counter = 0;
                for (size_t i=0 ; i<n ; ++i) {
                    m_changes[i] = 0;
                }

            }
        }

    ///cuando solucione el problema con el parametro maxIter la condicon ha de ser:
    /// m_counter<m_conv_point + m_numberIterations
    /// Esto si y solo si el limitador de iteraciones esta fuera y no aqui dentro
    if (!m_converged && m_counter <= 50000) {
        return true;
    }

    if (!m_converged && m_counter > 50000) {
        API::IFunction_sptr fun = m_leastSquares->getFittingFunction();
        std::string failed = "";
        for(int i=0; i<n; ++i) {
            if(!m_par_converged[i]){
                failed = failed + fun -> parameterName(i) + ", ";
            }
        }
        failed.replace(failed.end()-2,failed.end(),".");
        throw std::length_error("Convegence NOT reached after 100000 iterations.\n   Try to set proper initial values for parameters: " + failed);

        return false;
    }

    if (m_converged && m_counter < m_numberIterations){
        return true;
    }
    else {

        //const bool con = getProperty("Converged chain");
        bool con = true;
        size_t pdf_length = 100;
        API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create("Workspace2D",n, pdf_length + 1, pdf_length);
        API::ITableWorkspace_sptr wsPdfE = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
        wsPdfE -> addColumn("str","Name");
        wsPdfE -> addColumn("double","Value");
        wsPdfE -> addColumn("double","Left's error");
        wsPdfE -> addColumn("double","Rigth's error");


        std::vector<double>::iterator pos_min = std::min_element(m_chain[n].begin(),m_chain[n].end());
        std::vector<double> par_def(n);
        //std::vector<std::vector<double>> all_errors;
        API::IFunction_sptr fun = m_leastSquares->getFittingFunction();



        for (size_t j =0; j < ws->getNumberHistograms(); ++j){
            par_def[j]=m_chain[j][pos_min-m_chain[n].begin()];
            std::vector<double> par_error;
            double par_MP;
            API::TableRow row = wsPdfE -> appendRow();

            std::vector<double>::const_iterator first = m_chain[j].begin()+m_conv_point;
            std::vector<double>::const_iterator last = m_chain[j].end();
            std::vector<double> conv_chain(first, last);
            size_t conv_length = conv_chain.size();
            std::vector<double> pdf_x(101,0);
            std::vector<double> pdf_y;
            pdf_y = PDF(conv_chain, pdf_x, par_def[j],par_error,par_MP);
            row << fun->parameterName(j) << par_def[j] << par_error[0] << par_error[1];
            //all_errors.push_back(par_error);
            MantidVec & X = ws->dataX(j);
            MantidVec & Y = ws->dataY(j);
            //MantidVec & E = ws->dataE(i);
            double bin = pdf_x[1]-pdf_x[0];
            for(size_t k=0; k < Y.size(); ++k) {
                X[k] = pdf_x[k];
                Y[k] = pdf_y[k]/(double(conv_length)*bin);
                //E[k] = ???
            }
            X[pdf_length] = pdf_x[pdf_length];
            m_leastSquares -> setParameter(j,par_MP);

        }
        double Chi2MP = m_leastSquares -> val();



        for (size_t j =0; j<n; ++j){
            m_leastSquares -> setParameter(j,par_def[j]);
        }


        setProperty("OutputWorkspacePDF",ws);
        API::AnalysisDataService::Instance().addOrReplace("Parameters PDF",ws);
        setProperty("PdfError",wsPdfE);
        API::AnalysisDataService::Instance().addOrReplace("PDF Errors",wsPdfE);
        API::MatrixWorkspace_sptr wsC = API::WorkspaceFactory::Instance().create("Workspace2D",n+1, m_chain[0].size(), m_chain[0].size());

        for (size_t j =0; j < wsC->getNumberHistograms(); ++j){
            MantidVec & X = wsC->dataX(j);
            MantidVec & Y = wsC->dataY(j);
            //MantidVec & E = wsC->dataE(i);
            for(size_t k=0; k < X.size(); ++k) {
                X[k] = double(k);
                Y[k] = m_chain[j][k];
                //E[k] = ???
            }

        }

        setProperty("OutputWorkspaceChain",wsC);
        API::AnalysisDataService::Instance().addOrReplace("Parameters Chain",wsC);

        if (con){
        API::MatrixWorkspace_sptr wsConv = API::WorkspaceFactory::Instance().create("Workspace2D",n+1, m_counter*n, m_counter*n);
        for (size_t j =0; j < wsConv->getNumberHistograms(); ++j){
            std::vector<double>::const_iterator first = m_chain[j].begin()+m_conv_point;
            std::vector<double>::const_iterator last = m_chain[j].end();
            std::vector<double> conv_chain(first, last);
            MantidVec & X = wsConv->dataX(j);
            MantidVec & Y = wsConv->dataY(j);
            //MantidVec & E = wsC->dataE(i);
            for(size_t k=0; k < X.size(); ++k) {
                X[k] = double(k);
                Y[k] = conv_chain[k];
                //E[k] = ???
            }

        }

        setProperty("OutputWorkspaceConverged",wsConv);
        API::AnalysisDataService::Instance().addOrReplace("Converged Chain",wsConv);
        }
        API::FunctionValues_sptr values = m_leastSquares -> getValues();
        size_t length = values->size();

        API::ITableWorkspace_sptr wsChi2 = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
        wsChi2 -> addColumn("double","Chi2min");
        wsChi2 -> addColumn("double","Chi2MP");
        wsChi2 -> addColumn("double","Chi2min_red");
        wsChi2 -> addColumn("double","Chi2MP_red");

        double Chi2min_red = *pos_min/(double(length-n));
        double Chi2MP_red = Chi2MP/(double(length-n));
        API::TableRow row = wsChi2 -> appendRow();
        row << *pos_min << Chi2MP << Chi2min_red << Chi2MP_red;

        setProperty("ChiSquareTable",wsChi2);
        API::AnalysisDataService::Instance().addOrReplace("Chi Square Values",wsChi2);


/*
        API::IFunction_sptr fun = m_leastSquares->getFittingFunction();
        for (size_t i=0 ; i<n ; ++i) {
          fun->setError(i,12.345);
        } */
        ////std::cout << std::endl <<std::endl <<std::endl <<" -----> NUMBER ITERATIONS <-----" << m_numberIterations << std::endl;
        return false;
    }
  }
  double FABADAMinimizer::costFunctionVal() {
      return 0.0;
  }
  

} // namespace CurveFitting
} // namespace Mantid
