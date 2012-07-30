#ifndef MDFITTINGTESTHELPERS_H_
#define MDFITTINGTESTHELPERS_H_

#include "MantidMDAlgorithms/Quantification/ForegroundModelFactory.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/ParamFunction.h"

using Mantid::API::IFunction;

class FakeForegroundModel : public Mantid::MDAlgorithms::ForegroundModel
{
public:
  FakeForegroundModel() : start1(0.2), start2(0.8), a0(1.0), a1(4.6), att0(1.5), att1(10.5)
  {
  }

  void init()
  {
    declareParameter("FgA0", start1, "Parameter 1");
    declareParameter("FgA1", start2, "Parameter 2");

    declareAttribute("FgAtt0", IFunction::Attribute(att0));
    declareAttribute("FgAtt1", IFunction::Attribute(att1));
  }
  std::string name() const { return "FakeSharpModel"; }
  void function(const Mantid::API::FunctionDomain&, Mantid::API::FunctionValues&) const {}
  ModelType modelType() const { return ForegroundModel::Sharp; }
  double scatteringIntensity(const Mantid::API::ExperimentInfo &, const std::vector<double> &) const
  {
    return 1.0;
  }
public:
  double start1, start2;
  double a0, a1, att0, att1;
};

class Fake1DFunction
  : public virtual Mantid::API::ParamFunction, public virtual Mantid::API::IFunction1D
{
public:
    Fake1DFunction() :
    Mantid::API::ParamFunction(),
    Mantid::API::IFunction1D(),
    a0(1.5), a1(3.4), fgModel()
  {
    declareParameter("A0", 0.1);
    declareParameter("A1", 0.2);
    fgModel.initialize();
    fgModel.setFunctionUnderMinimization(*this);
    const size_t nparams = fgModel.nParams();
    for(size_t i = 0; i < nparams; ++i)
    {
      this->declareParameter(fgModel.parameterName(i), fgModel.getInitialParameterValue(i), fgModel.parameterDescription(i));
    }

  }
  std::string name() const { return "FakeFittingFunction"; }
  // Just sets the parameters
  virtual void function1D(double*, const double*, const size_t) const
  {
    // Simulate fitting
    Fake1DFunction * me = const_cast<Fake1DFunction*>(this);
    me->setParameter("A0", a0);
    me->setParameter("A1", a1);

    me->setParameter("FgA0", fgModel.a0);
    me->setParameter("FgA1", fgModel.a1);

  }

  const double a0, a1;
  FakeForegroundModel fgModel;
};

/// Sets all of the parameters to 1
class FakeMDFunction
  : public virtual Mantid::API::IFunctionMD, public virtual Mantid::API::ParamFunction
{
  std::string name() const { return "Fake"; }
  double functionMD(const Mantid::API::IMDIterator&) const
  {
    return 0.0;
  }
};

class FakeMDResolutionConvolution : public Mantid::MDAlgorithms::MDResolutionConvolution
{
public:
  FakeMDResolutionConvolution() : initialAtt0(1.5), initialAtt1(9.8)
  {
  }
  std::string name() const { return "FakeMDResolutionConvolution"; }

  void declareAttributes()
  {
    declareAttribute("ConvAtt0", IFunction::Attribute(initialAtt0));
    declareAttribute("ConvAtt1", IFunction::Attribute(initialAtt1));
  }

  double signal(const Mantid::API::IMDIterator & , const size_t ,
                Mantid::API::ExperimentInfo_const_sptr ) const
  {
    IFunction::Attribute att0 = getAttribute("ConvAtt0");
    if(att0.asDouble() == initialAtt0)
    {
      throw std::runtime_error("ConvAtt0 still at initial value.");
    }

    return 10.0;
  }

  const double initialAtt0, initialAtt1;
};

#endif // MDFITTINGTESTHELPERS_H_
