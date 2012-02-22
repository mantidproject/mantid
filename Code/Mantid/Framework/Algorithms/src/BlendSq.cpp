/*WIKI* 


*WIKI*/
#include "MantidAlgorithms/BlendSq.h"
#include <vector>
#include <math.h>
#include <sstream>
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/UnitFactory.h"

#define HUGEVALUE 1.0E10

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(BlendSq)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BlendSq::BlendSq()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BlendSq::~BlendSq()
  {
    // TODO Auto-generated destructor stub
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void BlendSq::initDocs()
  {
    this->setWikiSummary("Blended S(Q) from multiple banks");
    this->setOptionalMessage("Total scattering S(Q) from multiple banks will be rebinned and blended");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void BlendSq::init()
  {
    declareProperty(new ArrayProperty<std::string>("InputWorkspaces", new MandatoryValidator<std::vector<std::string> >), "The names of the input workspaces as a comma-separated list" ); 
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace for blended S(Q).");
    declareProperty(new ArrayProperty<double>("RangeOfLowerBounds", new MandatoryValidator<std::vector<double> >), "The lower bounds of each bank.");
    declareProperty(new ArrayProperty<double>("RangeOfUpperBounds", new MandatoryValidator<std::vector<double> >), "The upper bounds of each bank.");
    declareProperty("DeltaQ", 0.02);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BlendSq::exec()
  {
    // 1. Get input data and check validity (soft)
    const std::vector<std::string> inputWorkspaceNames = getProperty("InputWorkspaces");
    const std::vector<double> spectralowerbounds = getProperty("RangeOfLowerBounds");
    const std::vector<double> spectraupperbounds = getProperty("RangeOfUpperBounds");
    const double dq = getProperty("DeltaQ");

    size_t numspaces = inputWorkspaceNames.size();
    size_t numlowerbounds = spectralowerbounds.size();
    size_t numupperbounds = spectraupperbounds.size();
    if (numspaces != numlowerbounds || numlowerbounds != numupperbounds){
      g_log.error() << "Input number of Workspaces and upper/lower boundaries are not matched!" << std::endl;
      throw std::invalid_argument("Input argments not matched");
    }

    // 2. Configuration and check
    std::vector<MatrixWorkspace_const_sptr> inputWorkspaces;
    for (size_t i = 0; i < numspaces; i ++){
      MatrixWorkspace_sptr tws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWorkspaceNames[i]);
      inputWorkspaces.push_back(tws);
    }

    for (size_t i = 0; i < numspaces; i ++){
      const MantidVec& ix = inputWorkspaces[i]->readX(0);
      const MantidVec& iy = inputWorkspaces[i]->readY(0);
      if (ix.size() != iy.size()){
        if (ix.size() == iy.size()+1){
          g_log.error() << "Input Workspace (Indexed " << i << ") for BlendSq() must be Point Data, but not Histogram" << std::endl;
        } else {
          g_log.error() << "Invalid Workspace due to size of X and Y" << std::endl;
        }
        throw std::invalid_argument("Input Workspace not valid");
      }
    }

    // 3. Rebin, fill-array with empty bin.
    std::vector<MatrixWorkspace_sptr> constbinWorkspaces;
    for (size_t i=0; i <numspaces; i++){
      int newsize = int((spectraupperbounds[i]-spectralowerbounds[i])/dq);
      std::stringstream namestream;
      namestream << "constbin_" << i;
      std::string wsname = namestream.str();
      MatrixWorkspace_sptr constbinws = API::WorkspaceFactory::Instance().create(wsname, 1, newsize, newsize);
      constbinws->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
      rebinData(inputWorkspaces[i], constbinws, spectralowerbounds[i], spectraupperbounds[i], dq);
      fillArray(constbinws);
      constbinWorkspaces.push_back(constbinws);
    }

    // 4. Extend range such that all banks have S(Q) on same grids
    int newsize = int((100.0-0.0)/dq);
    std::vector<MatrixWorkspace_sptr> extendWorkspaces;
    for (size_t i=0; i <numspaces; i++){
      MatrixWorkspace_sptr newws = API::WorkspaceFactory::Instance().create(inputWorkspaces[0], 1, newsize, newsize);
      extendWorkspaceRange(constbinWorkspaces[i], newws, 0.0, dq);
      extendWorkspaces.push_back(newws);
    }

    // 5. Create new S(Q) workspace
    MatrixWorkspace_sptr blendsqws = API::WorkspaceFactory::Instance().create("SoQ", 1, newsize, newsize);

    // 6. Blend
    blendBanks(extendWorkspaces, blendsqws, spectralowerbounds, spectraupperbounds);

    // 7. Extend smoothly to Q=0
    double qmin = HUGEVALUE;
    for (size_t bi = 0; bi < spectralowerbounds.size(); bi++)
      if (spectralowerbounds[bi] < qmin)
        qmin = spectralowerbounds[bi];

    extendToZeroQ(blendsqws, qmin);

    setProperty("OutputWorkspace", blendsqws);

    return;
  }

  void BlendSq::extendWorkspaceRange(API::MatrixWorkspace_sptr sourcews, API::MatrixWorkspace_sptr targetws, double qmin, double dq){
    MantidVec& sx = sourcews->dataX(0);
    MantidVec& sy = sourcews->dataY(0);
    MantidVec& se = sourcews->dataE(0);

    MantidVec& tx = targetws->dataX(0);
    MantidVec& ty = targetws->dataY(0);
    MantidVec& te = targetws->dataE(0);

    // 1. Set target workspace
    size_t tsize = tx.size();
    for (size_t i=0; i<tsize; i++){
      tx[i] = qmin+(double)i*dq;
      ty[i] = 0.0;
      te[i] = -0.0;
    }

    // 2. Find start start and end indices
    size_t istart = (size_t)((sx[0]-qmin)/dq);
    size_t iend = (size_t)((sx[sx.size()-1]-qmin)/dq);
    g_log.debug() << "BlendSq-extendWorkspaceRange: start/end: " << istart << iend << std::endl;

    // 3. Transport
    for (size_t i=istart; i<iend+1; i++){
      tx[i] = sx[i-istart];
      ty[i] = sy[i-istart];
      te[i] = se[i-istart];
    }

    return;
  }


  void BlendSq::rebinData(API::MatrixWorkspace_const_sptr sourcews, API::MatrixWorkspace_sptr targetws, double qmin, double qmax, double dq){
    const MantidVec& sx = sourcews->readX(0);
    const MantidVec& sy = sourcews->readY(0);
    const MantidVec& se = sourcews->readE(0);

    MantidVec& tx = targetws->dataX(0);
    MantidVec& ty = targetws->dataY(0);
    MantidVec& te = targetws->dataE(0);

    // 1. Initialize target
    int* nbin = new int[tx.size()];
    for (size_t i=0; i <tx.size(); i++){
      tx[i] = qmin+(double)i*dq;
      ty[i] = 0.0;
      te[i] = -0.0;
      nbin[i] = 0;
    }

    // 2. Rebin
    for (size_t i=0; i<sx.size(); i++){
      double q = sx[i];
      if (q>=qmin && q<qmax){
        // a) determine individual-Q's index in new bin
        int32_t newqindex = (int)((q-qmin+0.5*dq)/dq);
        g_log.debug() << "Q = " << q << " binned to Bin: " << newqindex << std::endl;

        if (newqindex < 0 || newqindex >= (int32_t)tx.size()){
          // b) in case of out of range
          g_log.error() << "Impossible!" << std::endl;
        } else {
          // c) put to new bin
          nbin[newqindex] += 1;
          ty[newqindex] += sy[i];
          te[newqindex] += se[i]*se[i];
        } // if-else
      } else {
        g_log.information() << "Q = " << q << " is out of range" << std::endl;
      }
    } // for

    // 3. Clean new bin
    for (size_t i=0; i < tx.size(); i ++){
      if (nbin[i] > 0){
        ty[i] /= (double)nbin[i];
        te[i] = sqrt(te[i])/(double)nbin[i];
        if (fabs(te[i]) < 1.0E-15){
          te[i] = -0.0;
        }
      } else {
        te[i] = -0.0;
      }
    }

    // 4. Finalize
    delete[] nbin;

    return;
  }

  void BlendSq::fillArray(API::MatrixWorkspace_sptr ws){
    MantidVec& sx = ws->dataX(0);
    MantidVec& sy = ws->dataY(0);
    MantidVec& se = ws->dataE(0);

    // 1. Locate x0 as min(x) with e > 0
    int32_t ix0 = -1;
    for (int32_t i = 0; i < (int32_t)sx.size(); i ++){
      if (se[i] > 0){
        ix0 = i;
        break;
      }
    }
    if (ix0 < 0){
      throw std::invalid_argument("Impossible such that all Ei < 0");
    }

    double xlow = sx[ix0];
    double ylow = sy[ix0];
    double elow = se[ix0];

    // 2. Locate xf as max(x) with e > 0
    int32_t ixf = -1;
    for (int32_t i =0; i < (int32_t)sx.size(); i ++){
      if (se[sx.size()-1-i] > 0){
        ixf = (int32_t)sx.size()-1-i;
        break;
      }
    }
    if (ixf < 0){
      throw std::invalid_argument("Impossible such that all Ei < 0");
    }

    double xhigh = sx[ixf];
    double yhigh = sy[ixf];
    double ehigh = se[ixf];

    // 3. Fill points with E[i] < 0 with linear interpolation
    int32_t numfill = 0;
    bool inzero = false;
    int32_t iendzero = 0;
    int32_t istartzero = 0;
    for (int32_t i=0; i < (int32_t)se.size(); i ++){
      if (se[i] <= 0.0){
        // Record points with E < 0
        if (inzero){
          iendzero = i;
        } else {
          inzero = true;
          istartzero = i;
          iendzero = i;
        }
        numfill ++;
      } else if (inzero) {
        // treat previous E[i] <= 0 data points
        inzero = false;
        if (istartzero == 0){

          // E[i] < 0 from i=0.  Extrapolate is required
          for (int32_t j = istartzero; j <= iendzero; j ++){
            double fraction = sx[j]/xlow;
            sy[j] = fraction*ylow;
            se[j] = fraction*elow;
          } // j

        } else if (iendzero == (int32_t)sx.size()-1){

          // E[i] < 0 from i=-1.  Extrapolate is required
          double xmax = sx[sx.size()-1];
          for (int32_t j = istartzero; j <= iendzero; j ++){
            double fraction = (xmax-sx[j])/(xmax-xhigh);
            sy[j] = fraction*yhigh;
            se[j] = fraction*ehigh;
          } // j

        } else {

          // Interpolation case
          int32_t ix0 = istartzero-1;
          int32_t ixf = iendzero+1;
          for (int32_t j = istartzero; j <= iendzero; j ++){
            double fraction = (sx[j]-sx[ix0])/(sx[ixf]-sx[ix0]);
            sy[j] = sy[ix0]+fraction*(sy[ixf]-sy[ix0]);
            se[j] = se[ix0]+fraction*(se[ixf]-se[ix0]);
          } // j
        } // if-else...
      } // if - else
    } // for: i

    g_log.information() << "Number of Filled Points = " << numfill << std::endl;

    return;
  }

  void BlendSq::blendBanks(std::vector<API::MatrixWorkspace_sptr> sqwspaces, API::MatrixWorkspace_sptr blendworkspace, std::vector<double> lowerbounds, std::vector<double> upperbounds){

    // 1. Check validity: all input and output Workspaces should have same range
    size_t wssize = blendworkspace->dataX(0).size();
    for (size_t i = 0; i < sqwspaces.size(); i ++){
      if (wssize != sqwspaces[i]->dataX(0).size()){
        throw std::invalid_argument("blendBanks() Input Workspace sizes are different from output workspace's");
      }
    }

    // 2. Blend
    for (size_t qi = 0; qi < wssize; qi ++){
      // For each pixel
      double temps = 0.0;
      double tempe = 0.0;

      for (size_t bi = 0; bi < sqwspaces.size(); bi ++){
        // For each bank
        double q = sqwspaces[bi]->dataX(0)[qi];
        double s = sqwspaces[bi]->dataY(0)[qi];
        double e = sqwspaces[bi]->dataE(0)[qi];

        // a) Filter out data points outside of user-specified range by set E[i] to HUGE
        if (q < lowerbounds[bi] || q > upperbounds[bi]){
          s = 1.0;
          e = HUGEVALUE;
        }

        // b) E[i] too small... Impossible
        if (fabs(e) < 1.0E-8){
          g_log.error() << "Impossible! For index = " << qi << "Bank = " << bi << "E = " << e << std::endl;
          s = 0;
          e = HUGEVALUE;
        }

        // c) Add up
        temps += s/(e*e);
        tempe += 1/(e*e);
      } // for-bi: all banks per pixel

      // d) combine all banks's data
      double blende, blends;
      if (tempe < 1.0E-8){
        g_log.error() << "Impossible! For index = " << qi << "Point Q =" << blendworkspace->dataX(0)[qi] << "summed E <= 0" << std::endl;
        blends = 1.0;
        blende = 1.0;
      } else {
        blende = 1/sqrt(tempe);
        blends = temps/tempe;
      }

      // e) put to workspaces
      blendworkspace->dataY(0)[qi] = blends;
      blendworkspace->dataE(0)[qi] = blende;

    } // for-qi:  all the data points

	  return;
  }

  void BlendSq::extendToZeroQ(API::MatrixWorkspace_sptr ws, double qmin){
    MantidVec& sx = ws->dataX(0);
    MantidVec& sy = ws->dataY(0);
    MantidVec& se = ws->dataE(0);

    // 1. Find start point
    size_t iq0 = sx.size();
    for (size_t i = 0; i < sx.size(); i ++){
      if (sx[i] >= qmin && se[i] < HUGEVALUE){
        iq0 = i;
        break;
      }
    }
    if (iq0 == sx.size()){
      throw std::invalid_argument("Workspace E[i] list Error!");
    }

    // 2. Determine coefficient of y = a x^2
    double cof = sy[iq0]/(sx[iq0]*sx[iq0]);

    // 3. Set value
    for (size_t i = 0; i < iq0; i ++){
      sy[i] = cof*sx[i]*sx[i];
      se[i] = sqrt(fabs(sy[i]));
    }

    return;
  }


} // namespace Mantid
} // namespace Algorithms

