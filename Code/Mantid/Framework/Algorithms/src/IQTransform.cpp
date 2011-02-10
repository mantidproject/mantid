//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/IQTransform.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IQTransform)

using namespace Kernel;
using namespace API;

IQTransform::IQTransform() : API::Algorithm(), m_label(new Units::Label)
{
  /* Just for fun, this is implemented as follows....
   * We fill a map below with the transformation name as the key and
   * a function pointer to the method that does the transformation as
   * the value. The 'TransformType' property is filled with the keys
   * and then we search on that to select the correct function in exec.
   */
  m_transforms["Guinier (spheres)"] = &IQTransform::guinierSpheres;
  m_transforms["Guinier (rods)"] = &IQTransform::guinierRods;
  m_transforms["Guinier (sheets)"] = &IQTransform::guinierSheets;
  m_transforms["Zimm"] = &IQTransform::zimm;
  m_transforms["Debye-Bueche"] = &IQTransform::debyeBueche;
  m_transforms["Holtzer"] = &IQTransform::holtzer;
  m_transforms["Kratky"] = &IQTransform::kratky;
  m_transforms["Porod"] = &IQTransform::porod;
  m_transforms["Log-Log"] = &IQTransform::logLog;
  m_transforms["General"] = &IQTransform::general;
}

IQTransform::~IQTransform() {}

void IQTransform::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  // Require the input to be in units of Q and to be a distribution
  // (which the result of a SANS reduction in Mantid will be)
  wsValidator->add(new WorkspaceUnitValidator<>("MomentumTransfer"));
  wsValidator->add(new RawCountValidator<>(false));

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator),
                  "The input workspace must be a distribution with units of Q");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
                  "The name of the output workspace");

  // Extract the keys from the transformations map to pass to the property
  std::set<std::string> plottype;
  for (TransformMap::const_iterator it = m_transforms.begin(); it != m_transforms.end(); ++it)
  {
    plottype.insert(it->first);
  }
  declareProperty("TransformType","",new ListValidator(plottype),
                  "The name of the transformation to be performed on the workspace");

  // A background to be subtracted can be a value or a workspace. Both properties are optional.
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  declareProperty("BackgroundValue",0.0,mustBePositive,
                  "A constant value to subtract from the data prior to its transformation");
  declareProperty(new WorkspaceProperty<>("BackgroundWorkspace","",Direction::Input,true),
                  "A workspace to subtract from the input workspace prior to its transformation."
                  "Must be compatible with the input (as for the Minus algorithm).");

  declareProperty(new ArrayProperty<double>("GeneralFunctionConstants"),
                  "A set of 10 constants to be used (only) with the 'General' transformation");
}

void IQTransform::exec()
{
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  // Print a warning if the input workspace has more than one spectrum
  if ( inputWS->getNumberHistograms() > 1 )
  {
    g_log.warning("This algorithm is intended for use on single-spectrum workspaces.\n"
                  "Only the first spectrum will be transformed.");
  }

  // Do background subtraction from a workspace first because it doesn't like
  // potential conversion to point data that follows. Requires a temporary workspace.
  MatrixWorkspace_sptr tmpWS;
  MatrixWorkspace_sptr backgroundWS = getProperty("BackgroundWorkspace");
  if ( backgroundWS ) tmpWS = subtractBackgroundWS(inputWS,backgroundWS);
  else tmpWS = inputWS;

  // Create the output workspace
  const size_t length = tmpWS->blocksize();
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,1,length,length);
  m_label->setLabel("");
  outputWS->setYUnit("");
  // Copy the data over. Assume single spectrum input (output will be).
  // Take the mid-point of histogram bins
  if ( tmpWS->isHistogramData() )
  {
    VectorHelper::convertToBinCentre(tmpWS->readX(0),outputWS->dataX(0));
  }
  else
  {
    outputWS->setX(0,tmpWS->refX(0));
  }
  MantidVec& Y = outputWS->dataY(0) = tmpWS->dataY(0);
  outputWS->dataE(0) = tmpWS->dataE(0);

  // Subtract a constant background if requested
  const double background = getProperty("BackgroundValue");
  if ( background > 0.0 ) subtractBackgroundValue(Y,background);

  // Select the desired transformation function and call it
  TransformFunc f = m_transforms.find(getProperty("TransformType"))->second;
  (this->*f)(outputWS);

  // Need the generic label unit on this (unless the unit on the X axis hasn't changed)
  if ( ! m_label->caption().empty() ) outputWS->getAxis(0)->unit() = m_label;
  setProperty("OutputWorkspace",outputWS);
}

/** Subtracts a constant from the data values in the given workspace
 *  @param ws    The workspace to perform the subtraction on
 *  @param value The value to subtract from each data point
 */
void IQTransform::subtractBackgroundValue(MantidVec& Y, const double value)
{
  g_log.debug() << "Subtracting the background value " << value << " from the input workspace.\n";
  std::transform(Y.begin(),Y.end(),Y.begin(),std::bind2nd(std::minus<double>(),value));
}

/** Uses the Minus algorithm to subtract the background workspace from the given workspace.
 *  If the subalgorithm fails (e.g. if the background workspace is the wrong size), then this
 *  entire algorithm will.
 *  @param ws         The workspace to perform the subtraction on
 *  @param background The workspace containing the background values
 */
API::MatrixWorkspace_sptr IQTransform::subtractBackgroundWS(API::MatrixWorkspace_sptr ws, API::MatrixWorkspace_sptr background)
{
  g_log.debug() << "Subtracting the workspace " << background->getName() << " from the input workspace.\n";
  return ws - background;
}

/** @name Available transformation functions */
//@{

/** Performs the Guinier (spheres) transformation: Ln(I) v Q^2
 *  @param ws The workspace to be transformed
 *  @throw std::range_error if an attempt is made to take log of a negative number
 */
void IQTransform::guinierSpheres(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  std::transform(X.begin(),X.end(),X.begin(),VectorHelper::Squares<double>());
  std::transform(E.begin(),E.end(),Y.begin(),E.begin(),std::divides<double>());
  std::transform(Y.begin(),Y.end(),Y.begin(),VectorHelper::Log<double>());

  ws->setYUnitLabel("Ln(I)");
  m_label->setLabel("Q^2");
}

/** Performs the Guinier (rods) transformation: Ln(IQ) v Q^2
 *  @param ws The workspace to be transformed
 *  @throw std::range_error if an attempt is made to take log of a negative number
 */
void IQTransform::guinierRods(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  std::transform(E.begin(),E.end(),Y.begin(),E.begin(),std::divides<double>());
  std::transform(Y.begin(),Y.end(),X.begin(),Y.begin(),std::multiplies<double>());
  std::transform(Y.begin(),Y.end(),Y.begin(),VectorHelper::Log<double>());
  std::transform(X.begin(),X.end(),X.begin(),VectorHelper::Squares<double>());

  ws->setYUnitLabel("Ln(I x Q)");
  m_label->setLabel("Q^2");
}

/** Performs the Guinier (sheets) transformation: Ln(IQ^2) v Q^2
 *  @param ws The workspace to be transformed
 *  @throw std::range_error if an attempt is made to take log of a negative number
 */
void IQTransform::guinierSheets(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  std::transform(E.begin(),E.end(),Y.begin(),E.begin(),std::divides<double>());
  std::transform(X.begin(),X.end(),X.begin(),VectorHelper::Squares<double>());
  std::transform(Y.begin(),Y.end(),X.begin(),Y.begin(),std::multiplies<double>());
  std::transform(Y.begin(),Y.end(),Y.begin(),VectorHelper::Log<double>());

  ws->setYUnitLabel("Ln(I x Q^2)");
  m_label->setLabel("Q^2");
}

/** Performs the Zimm transformation: 1/I v Q^2
 *  The output is set to zero for negative input Y values
 *  @param ws The workspace to be transformed
 */
void IQTransform::zimm(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  std::transform(X.begin(),X.end(),X.begin(),VectorHelper::Squares<double>());
  for(size_t i = 0; i < Y.size(); ++i)
  {
    if ( Y[i] > 0.0 )
    {
      Y[i] = 1.0 / Y[i];
      E[i] *= std::pow(Y[i],2);
    }
    else
    {
      Y[i] = 0.0;
      E[i] = 0.0;
    }
  }

  ws->setYUnitLabel("1/I");
  m_label->setLabel("Q^2");
}

/** Performs the Debye-Bueche transformation: 1/sqrt(I) v Q^2
 *  The output is set to zero for negative input Y values
 *  @param ws The workspace to be transformed
 */
void IQTransform::debyeBueche(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  std::transform(X.begin(),X.end(),X.begin(),VectorHelper::Squares<double>());
  for(size_t i = 0; i < Y.size(); ++i)
  {
    if ( Y[i] > 0.0 )
    {
      Y[i] = 1.0 / std::sqrt(Y[i]);
      E[i] *= std::pow(Y[i],3);
    }
    else
    {
      Y[i] = 0.0;
      E[i] = 0.0;
    }
  }

  ws->setYUnitLabel("1/sqrt(I)");
  m_label->setLabel("Q^2");
}

/** Performs the Holtzer transformation: IQ v Q
 *  @param ws The workspace to be transformed
 */
void IQTransform::holtzer(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  std::transform(Y.begin(),Y.end(),X.begin(),Y.begin(),std::multiplies<double>());
  std::transform(E.begin(),E.end(),X.begin(),E.begin(),std::multiplies<double>());

  ws->setYUnitLabel("I x Q");
}

/** Performs the Kratky transformation: IQ^2 v Q
 *  @param ws The workspace to be transformed
 */
void IQTransform::kratky(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  MantidVec Q2(X.size());
  std::transform(X.begin(),X.end(),Q2.begin(),VectorHelper::Squares<double>());
  std::transform(Y.begin(),Y.end(),Q2.begin(),Y.begin(),std::multiplies<double>());
  std::transform(E.begin(),E.end(),Q2.begin(),E.begin(),std::multiplies<double>());

  ws->setYUnitLabel("I x Q^2");
}

/** Performs the Porod transformation: IQ^4 v Q
 *  @param ws The workspace to be transformed
 */
void IQTransform::porod(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  MantidVec Q4(X.size());
  std::transform(X.begin(),X.end(),X.begin(),Q4.begin(),VectorHelper::TimesSquares<double>());
  std::transform(Y.begin(),Y.end(),Q4.begin(),Y.begin(),std::multiplies<double>());
  std::transform(E.begin(),E.end(),Q4.begin(),E.begin(),std::multiplies<double>());

  ws->setYUnitLabel("I x Q^4");
}

/** Performs a log-log transformation: Ln(I) v Ln(Q)
 *  @param ws The workspace to be transformed
 *  @throw std::range_error if an attempt is made to take log of a negative number
 */
void IQTransform::logLog(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  std::transform(X.begin(),X.end(),X.begin(),VectorHelper::Log<double>());
  std::transform(E.begin(),E.end(),Y.begin(),E.begin(),std::divides<double>());
  std::transform(Y.begin(),Y.end(),Y.begin(),VectorHelper::Log<double>());

  ws->setYUnitLabel("Ln(I)");
  m_label->setLabel("Ln(Q)");
}

/** Performs a transformation of the form: Q^A x I^B x Ln(Q^C x I^D x E) v Q^F x I^G x Ln(Q^H x I^I x J).
 *  Uses the 'GeneralFunctionConstants' property where A-J are the 10 (ordered) input constants.
 *  @param ws The workspace to be transformed
 *  @throw std::range_error if an attempt is made to take log of a negative number
 */
void IQTransform::general(API::MatrixWorkspace_sptr ws)
{
  MantidVec& X = ws->dataX(0);
  MantidVec& Y = ws->dataY(0);
  MantidVec& E = ws->dataE(0);
  const std::vector<double> C = getProperty("GeneralFunctionConstants");
  // Check for the correct number of elements
  if ( C.size() != 10 )
  {
    std::string mess("The General transformation requires 10 values to be provided.");
    g_log.error(mess);
    throw std::invalid_argument(mess);
  }

  for(size_t i = 0; i < Y.size(); ++i)
  {
    double tmpX = std::pow(X[i],C[7]) * std::pow(Y[i],C[8]) * C[9];
    if ( tmpX <= 0.0 ) throw std::range_error("Attempt to take log of a zero or negative number.");
    tmpX = std::pow(X[i],C[5]) * std::pow(Y[i],C[6]) * std::log(tmpX);
    const double tmpY = std::pow(X[i],C[2]) * std::pow(Y[i],C[3]) * C[4];
    if ( tmpY <= 0.0 ) throw std::range_error("Attempt to take log of a zero or negative number.");
    const double newY = std::pow(X[i],C[0]) * std::pow(Y[i],C[1]) * std::log(tmpY);

    E[i] *= std::pow(X[i],C[0]) * ( C[1]*std::pow(Y[i],C[1]-1) * std::log(tmpY)
                    + (( std::pow(Y[i],C[1]) * std::pow(X[i],C[2]) * C[4] * C[3]*std::pow(Y[i],C[3]-1) )
                       / tmpY ) );
    X[i] = tmpX;
    Y[i] = newY;
  }

  std::stringstream ylabel;
  ylabel << "Q^" << C[0] << " x I^" << C[1] << " x Ln( Q^" << C[2] << " x I^" << C[3] << " x " << C[4] << ")";
  ws->setYUnitLabel(ylabel.str());
  std::stringstream xlabel;
  xlabel << "Q^" << C[5] << " x I^" << C[6] << " x Ln( Q^" << C[7] << " x I^" << C[8] << " x " << C[9] << ")";
  m_label->setLabel(xlabel.str());
}

//@}

} // namespace Algorithms
} // namespace Mantid
