#pragma once

#include <initializer_list>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidCurveFitting/Functions/Bk2BkExpConvPV.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::CurveFitting::Functions::Gaussian;

/* define the types of transforms that can be applied to
   a 1D function */
enum class transforms : int { domain_scale, domain_translate, co_domain_scale, co_domain_translate };

/* define a supported transformation with type and scalar amount */
class TransformSpecifier {

public:
  TransformSpecifier(transforms const type, double scalar) : transform_type(type), scalar(scalar) {}

  transforms const transform_type;

  double scalar;
};

// Algorithm to force Gaussian1D to be run by simplex algorithm
class SimplexGaussian : public Gaussian {
public:
  ~SimplexGaussian() override {}
  std::string name() const override { return "Gaussian"; }

protected:
  void functionDerivMW(Jacobian *out, const double *xValues, const size_t nData) {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
    throw Exception::NotImplementedError("No derivative function provided");
  }
};

DECLARE_FUNCTION(SimplexGaussian)
/*
  Create fake data to test cross correlation

  This class creates a matrix workspace with a reference function in the
  first index and subsequent indices containing a transformed version of
  the reference function.

  The transform at index "i" for 1 < i < len( l ) contains
  the reference function transformed according to the specification
  in l[ i - 1 ] where "l" is the initializer list constructor argument.

  use get_workspace() to get the workspace
*/
class CrossCorrelateTestData {
public:
  inline static std::string const gaussian_default = "name=Gaussian,Height=20,Sigma=4";

  /*

  Code to generate parameters for b2bexpconvpv:

    import numpy as np

    diam_d = {"111": 2.05995, "220": 1.26146, "311": 1.07577}

    alp = 0.791431E-01
    beta0 = 0.580874E-01
    beta1 = 0.947427E-01
    sig0 = 0.0E+00
    sig1 = 0.157741E+03
    sig2 = 0.402182E+02
    gamma1 = 0.302644E+01

    print("===============================================")
    print("Back-to-back shape parameters for diamond peaks")
    print("===============================================")
    for key, item in diam_d.items():
      A = alp / item
      B = beta0 + beta1 / item**4
      S = np.sqrt(sig0 + sig1 * item**2 + sig2 * item**4)
      Gamma = gamma1 * item

      print("\n--------------------")
      print("({0:3s})".format(key))
      print("--------------------")
      print("A = {0:<10.5F}".format(A))
      print("B = {0:<10.5F}".format(B))
      print("S = {0:<10.5F}".format(S))
      print("Gamma = {0:<10.5F}".format(Gamma))
      print("===============================================")

    Result:

      ===============================================
      Back-to-back shape parameters for diamond peaks
      ===============================================

      --------------------
      (111)
      --------------------
      A = 0.03842
      B = 0.06335
      S = 37.33017
      Gamma = 6.23432
      ===============================================

      --------------------
      (220)
      --------------------
      A = 0.06274
      B = 0.09550
      S = 18.78430
      Gamma = 3.81773
      ===============================================

      --------------------
      (311)
      --------------------
      A = 0.07357
      B = 0.12883
      S = 15.37579
      Gamma = 3.25575
      ===============================================
  */

  inline static std::string const b2bexp_default_111 =
      "name=Bk2BkExpConvPV,Alpha=0.03842,Beta=0.06335,Sigma2=37.33017,Gamma = "
      "6.23432,Intensity=100";
  inline static std::string const b2bexp_default_220 =
      "name=Bk2BkExpConvPV,Alpha=0.06274,Beta=0.09550,Sigma2=18.78430,Gamma=3."
      "81773,Intensity=100";
  inline static std::string const b2bexp_default_311 =
      "name=Bk2BkExpConvPV,Alpha=0.07357,Beta=0.12883,Sigma2=15.37579,Gamma=3."
      "25575,Intensity=100";

  /* Specify default values */
  CrossCorrelateTestData(std::string function_specifier = gaussian_default, int domain_radius = 20,
                         std::initializer_list<class TransformSpecifier> l = {
                             // TransformSpecifier(transforms::domain_translate, 10),
                             // TransformSpecifier(transforms::domain_scale, 10),
                             // TransformSpecifier(transforms::co_domain_translate, 10),
                             // TransformSpecifier(transforms::co_domain_scale, 10)
                         });

  MatrixWorkspace_sptr get_workspace() { return workspace; }

  /* debugging function - print to stdout */
  void print_workspace();

  /* methods */
private:
  /* generates the co-domain given the domain */
  std::vector<double> apply_function(std::vector<double> &domain);

  /* offset domain by a constant before applying a function */
  std::vector<double> translate_domain(std::vector<double> const &domain, double translation);

  /* scale domain by a constant factor before applying a function */
  std::vector<double> scale_domain(std::vector<double> const &domain, double scale_factor);

  /* scaling and translation of the co-domain can occur after the function has
     already
     been evaluated by manipulating the entries in the workspace: */
  void translate_co_domain(int reference_index, int target_index, double translation);

  void scale_co_domain(int reference_index, int target_index, double scale_factor);

  /* assign at given index */
  void assign_to_workspace(int workspace_index, std::vector<double> const &domain,
                           std::vector<double> const &co_domain);

  /* print a vector */
  void dump_vector(std::vector<double> const &v, std::string header_line);

  /* variables */
private:
  /* this object's deliverable */
  MatrixWorkspace_sptr workspace;

  /* function to map domain to co-domain */
  std::shared_ptr<CompositeFunction> function;

  /* function extends this far from the origin in either direction */
  int const domain_radius;

  /* number of functions in the workspace */
  size_t const num_functions;
};

/* it will move and unmove this vector */
std::vector<double> CrossCorrelateTestData::apply_function(std::vector<double> &domain) {
  FunctionDomain1DVector domain_oop(std::move(domain));
  /* move after clear */
  domain.clear();

  /* populate range */
  FunctionValues co_domain_oop(domain_oop);
  function->function(domain_oop, co_domain_oop);

  /* invalidate FunctionDomain1DVector and get underlying data primitive */
  domain = domain_oop.getVector();

  /* invalidate FunctionValues object and get underlying data primitive */
  auto co_domain = co_domain_oop.toVector();

  return co_domain;
}

std::vector<double> CrossCorrelateTestData::translate_domain(std::vector<double> const &domain, double translation) {
  std::vector<double> translated_domain;

  std::transform(domain.begin(), domain.end(), std::back_inserter(translated_domain),
                 [translation](double d) -> double { return d + translation; });

  return translated_domain;
}

std::vector<double> CrossCorrelateTestData::scale_domain(std::vector<double> const &domain, double scale_factor) {
  std::vector<double> scaled_domain;

  std::transform(domain.begin(), domain.end(), std::back_inserter(scaled_domain),
                 [scale_factor](double d) -> double { return scale_factor * d; });

  return scaled_domain;
}

/* scaling and translation of the co-domain can occur after the function has
   already
   been evaluated by manipulating the entries in the workspace */
void CrossCorrelateTestData::translate_co_domain(int reference_index, int target_index, double translation) {
  workspace->mutableX(target_index) = workspace->mutableX(reference_index);
  auto reference_y = workspace->mutableY(reference_index);
  reference_y += translation;
  workspace->mutableY(target_index) = reference_y;
}

void CrossCorrelateTestData::scale_co_domain(int reference_index, int target_index, double scale_factor) {
  workspace->mutableX(target_index) = workspace->mutableX(reference_index);
  workspace->mutableY(target_index) = scale_factor * workspace->mutableY(reference_index);
}

void CrossCorrelateTestData::assign_to_workspace(int workspace_index, std::vector<double> const &domain,
                                                 std::vector<double> const &co_domain) {
  auto &x_data = workspace->mutableX(workspace_index);
  x_data.assign(domain.cbegin(), domain.cend());

  auto &y_data = workspace->mutableY(workspace_index);
  y_data.assign(co_domain.cbegin(), co_domain.cend());
}

void CrossCorrelateTestData::dump_vector(std::vector<double> const &v, std::string header_line) {
  std::cout << header_line << std::endl;
  for (double i : v) {
    std::cout << " " << i;
  }
  std::cout << std::endl;
}

void CrossCorrelateTestData::print_workspace() {
  for (int i = 0; i < static_cast<int>(num_functions); ++i) {
    std::cout << "workspace: " << i << std::endl;
    std::vector<double> const &domain = workspace->mutableX(i).rawData();
    std::vector<double> const &co_domain = workspace->mutableY(i).rawData();
    dump_vector(domain, "domain:");
    dump_vector(co_domain, "co_domain:");
  }
}

MatrixWorkspace_sptr convertToHistogram(const MatrixWorkspace_sptr inWS) {
  ConvertToHistogram alg;

  // setup alg
  alg->initialize();
  alg->setRethrows(true);
  alg->setProperty("InputWorkspace", inWS);
  alg->setProperty("OutputWorkspace", inWS);

  // run alg
  alg->execute();
  inWS = alg->getProperty("OutputWorkspace");
  return inWS;
}

CrossCorrelateTestData::CrossCorrelateTestData(std::string function_specifier, int domain_radius,
                                               std::initializer_list<class TransformSpecifier> l)
    : domain_radius(domain_radius), num_functions(l.size() + 1) {
  /* allocate the workspace but do not populate yet */
  int domain_size = this->domain_radius * 2 + 1;

  /* domain will represent points not bins, so the domain and co-domain are
     of identical size (as opposed to domain being co_domain_size + 1 ) */
  workspace = createWorkspace<Workspace2D>(l.size() + 1, domain_size, domain_size);

  /* create the function that will operate on a discrete domain */
  function = std::make_shared<API::CompositeFunction>();
  const auto functionInstance = API::FunctionFactory::Instance().createInitialized(function_specifier);
  function->addFunction(std::dynamic_pointer_cast<IPeakFunction>(functionInstance));

  /* create discrete domain */
  std::vector<double> symmetric_domain(domain_size);
  std::iota(symmetric_domain.begin(), symmetric_domain.end(), -1 * domain_radius);

  /* create the reference spectrum at the first index */
  std::vector<double> co_domain = apply_function(symmetric_domain);
  assign_to_workspace(0, symmetric_domain, co_domain);

  std::vector<double> errorDomain(co_domain.size(), 0.0);
  workspace->mutableE(0).assign(errorDomain.begin(), errorDomain.end());

  workspace = convertToHistogram(workspace);

  /* iterate through the initializer list and handle all the objects */
  int workspace_index = 1;
  for (auto &s : l) {
    /* scale domain, recompute co-domain, assign to workspace index */
    if (s.transform_type == transforms::domain_scale) {
      std::vector<double> scaled_domain = scale_domain(symmetric_domain, s.scalar);
      std::vector<double> scaled_co_domain = apply_function(scaled_domain);
      assign_to_workspace(workspace_index, scaled_domain, scaled_co_domain);
    }

    /* translate domain, recompute co-domain, assign to workspace index */
    else if (s.transform_type == transforms::domain_translate) {
      std::vector<double> translated_domain = translate_domain(symmetric_domain, s.scalar);
      std::vector<double> translated_co_domain = apply_function(translated_domain);
      assign_to_workspace(workspace_index, translated_domain, translated_co_domain);
    } else if (s.transform_type == transforms::co_domain_scale) {
      scale_co_domain(0, workspace_index, s.scalar);
    } else if (s.transform_type == transforms::co_domain_translate) {
      translate_co_domain(0, workspace_index, s.scalar);
    } else {
      std::cout << "unrecognized transform type" << std::endl;
      exit(0);
    }

    ++workspace_index;
  }

  workspace->getAxis(0)->setUnit("dSpacing");
}
