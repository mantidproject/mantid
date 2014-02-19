/*WIKI*

The Sassena application [http://sassena.org] generates intermediate scattering factors from molecular dynamics trajectories. This algorithm reads Sassena output and stores all data in workspaces of type [[Workspace2D]], grouped under a single [[WorkspaceGroup]]. It is implied that the time unit is one '''picosecond'''.

Sassena ouput files are in HDF5 format [http://www.hdfgroup.org/HDF5], and can be made up of the following datasets: ''qvectors'', ''fq'', ''fq0'', ''fq2'', and ''fqt''

The group workspace should contain workspaces '''_fqt.Re''' and '''_fqt.Im''' containing the real and imaginary parts of the intermediate structure factor, respectively. This algorithm will take both and perform [[FFT]], storing the real part of the transform in workspace '''_fqw''' and placing this workspace under the input group workspace. Assuming the time unit to be one picosecond, the resulting energies will be in units of one '''micro-eV'''.

The Schofield correction (P. Schofield, ''Phys. Rev. Letters'' '''4'''(5), 239 (1960)) is optionally applied to the resulting dynamic structure factor to reinstate the detailed balance condition
<math>S(Q,\omega)=e^{\beta \hbar \omega}S(-Q,-\omega)</math>.

== Details ==

=== Parameter FFTonlyRealPart ===

Setting parameter FFTonlyRealPart to true will produce a transform on only the real part of I(Q,t). This is convenient if we know that I(Q,t) should be real but a residual imaginary part was left in a Sassena calculation due to finite orientational average in Q-space.


Below are plots after application of SassenaFFT to <math>I(Q,t) = e^{-t^2/(2\sigma^2)} + i\cdot t \cdot e^{-t^2/(2\sigma^2)}</math> with <math>\sigma=1ps</math>. Real an imaginary parts are shown in panels (a) and (b). Note that <math>I(Q,t)*=I(Q,-t)</math>. If only <math>Re[I(Q,t)]</math> is transformed, the result is another Gaussian: <math>\sqrt{2\pi}\cdot e^{-E^2/(2\sigma'^2)}</math> with <math>\sigma'=4,136/(2\pi \sigma)</math> in units of <math>\mu</math>eV (panel (c)). If I(Q,t) is transformed, the result is a modulated Gaussian: <math>(1+\sigma' E)\sqrt{2\pi}\cdot e^{-E^2/(2\sigma'^2)}</math>(panel (d)).

[[Image:SassenaFFTexample.jpg|center|x800px|alt=Application of SassenaFFT to a I(Q,t)]]

 *WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SassenaFFT.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/FileProperty.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SassenaFFT);

/// Sets Documentation strings for this algorithm
void SassenaFFT::initDocs()
{
  this->setWikiSummary("Performs complex Fast Fourier Transform of intermediate scattering function");
  this->setOptionalMessage("Performs complex Fast Fourier Transform of intermediate scattering function");
}

/// Override Algorithm::checkGroups
bool SassenaFFT::checkGroups() { return false; }

/// processGroups must not be called
bool SassenaFFT::processGroups()
{
  const std::string errMessg = "processGroups must not be called from SassenaFFT";
  this->g_log.error(errMessg);
  throw std::logic_error(errMessg);
  return false;
}


/**
 * Initialise the algorithm. Declare properties which can be set before execution (input) or
 * read from after the execution (output).
 */
void SassenaFFT::init()
{
  this->declareProperty(new API::WorkspaceProperty<API::WorkspaceGroup>("InputWorkspace","",Kernel::Direction::InOut), "The name of the input group workspace");
  // properties for the detailed balance condition
  this->declareProperty(new Kernel::PropertyWithValue<bool>("FFTonlyRealPart", false, Kernel::Direction::Input),"Do we FFT only the real part of I(Q,t)? (optional, default is False)");
  this->declareProperty(new Kernel::PropertyWithValue<bool>("DetailedBalance", false, Kernel::Direction::Input),"Do we apply detailed balance condition? (optional, default is False)");
  this->declareProperty("Temp",300.0,"Multiply structure factor by exp(E/(2*kT)");
  this->setPropertySettings("Temp", new Kernel::EnabledWhenProperty("DetailedBalance", Kernel::IS_EQUAL_TO, "1"));
}

/// Execute the algorithm
void SassenaFFT::exec()
{
  const std::string gwsName = this->getPropertyValue("InputWorkspace");
  API::WorkspaceGroup_sptr gws = this->getProperty("InputWorkspace");

  const std::string ftqReName = gwsName + "_fqt.Re";
  const std::string ftqImName = gwsName + "_fqt.Im";

  // Make sure the intermediate structure factor is there
  if(!gws->contains(ftqReName) )
  {
    const std::string errMessg = "workspace "+gwsName+" does not contain an intermediate structure factor";
    this->g_log.error(errMessg);
    throw Kernel::Exception::NotFoundError("group workspace does not contain",ftqReName);
  }

  // Retrieve the real and imaginary parts of the intermediate scattering function
  DataObjects::Workspace2D_sptr fqtRe = boost::dynamic_pointer_cast<DataObjects::Workspace2D>( gws->getItem( ftqReName ) );
  DataObjects::Workspace2D_sptr fqtIm = boost::dynamic_pointer_cast<DataObjects::Workspace2D>( gws->getItem( ftqImName ) );

  // Calculate the FFT for all spectra, retaining only the real part since F(q,-t) = F*(q,t)
  int part=3; // extract the real part of the transform, assuming I(Q,t) is real
  const std::string sqwName = gwsName + "_sqw";
  API::IAlgorithm_sptr fft = this->createChildAlgorithm("ExtractFFTSpectrum");
  fft->setProperty<DataObjects::Workspace2D_sptr>("InputWorkspace", fqtRe);
  if( !this->getProperty("FFTonlyRealPart") )
  {
    part=0; // extract the real part of the transform, assuming I(Q,t) is complex
    fft->setProperty<DataObjects::Workspace2D_sptr>("InputImagWorkspace", fqtIm);
  }
  fft->setPropertyValue("OutputWorkspace", sqwName );
  fft->setProperty<int>("FFTPart",part); // extract the real part
  fft->executeAsChildAlg();
  API::MatrixWorkspace_sptr sqw0 = fft->getProperty("OutputWorkspace");
  DataObjects::Workspace2D_sptr sqw = boost::dynamic_pointer_cast<DataObjects::Workspace2D>( sqw0 );
  API::AnalysisDataService::Instance().addOrReplace( sqwName, sqw );

  // Transform the X-axis to appropriate dimensions
  // We assume the units of the intermediate scattering function are in picoseconds
  // The resulting frequency unit is in mili-eV, thus use m_ps2meV
  API::IAlgorithm_sptr scaleX = this->createChildAlgorithm("ScaleX");
  scaleX->setProperty<DataObjects::Workspace2D_sptr>("InputWorkspace",sqw);
  scaleX->setProperty<double>("Factor", m_ps2meV);
  scaleX->setProperty<DataObjects::Workspace2D_sptr>("OutputWorkspace", sqw);
  scaleX->executeAsChildAlg();

  //Do we apply the detailed balance condition exp(E/(2*kT)) ?
  if( this->getProperty("DetailedBalance") )
  {
    double T = this->getProperty("Temp");
    // The ExponentialCorrection algorithm assumes the form C0*exp(-C1*x). Note the explicit minus in the exponent
    API::IAlgorithm_sptr ec = this->createChildAlgorithm("ExponentialCorrection");
    ec->setProperty<DataObjects::Workspace2D_sptr>("InputWorkspace", sqw);
    ec->setProperty<DataObjects::Workspace2D_sptr>("OutputWorkspace", sqw);
    ec->setProperty<double>("C0",1.0);
    ec->setProperty<double>("C1",-1.0/(2.0*T*m_T2ueV)); // Temperature in units of ueV
    ec->setPropertyValue("Operation","Multiply");
    ec->executeAsChildAlg();
  }

  // Set the Energy unit for the X-axis
  sqw->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("DeltaE");

  // Add to group workspace, except if we are replacing the workspace. In this case, the group workspace
  // is already notified of the changes by the analysis data service.
  if(!gws->contains(sqwName))
  {
    gws->add( sqwName );
  }
  else
  {
    this->g_log.information("Workspace "+sqwName+" replaced with new contents");
  }

}

} // namespacce Mantid
} // namespace Algorithms
