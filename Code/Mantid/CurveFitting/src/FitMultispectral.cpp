//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FitMultispectral.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/TableRow.h"

namespace Mantid
{
namespace CurveFitting
{

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(FitMultispectral)

  using namespace Kernel;
  using namespace API;

  /** Initialisation method
  */
  void FitMultispectral::init()
  {
    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace","",Direction::Input), "Name of the input Workspace");

    declareProperty("StartX",EMPTY_DBL(),"The lowest value on the x axis to include in the fit");
    declareProperty("EndX",EMPTY_DBL(),"The highest value on the x axis to include in the fit");

    declareProperty("StartY",EMPTY_DBL(),"The lowest value on the y axis to include in the fit");
    declareProperty("EndY",EMPTY_DBL(),"The highest value on the y axis to include in the fit");

    declareProperty("Function","","Parameters defining the fitting function and its initial values" );

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("MaxIterations", 500, mustBePositive,
      "Stop after this number of iterations if a good fit is not found" );

    std::vector<std::string> minimizerOptions;
    minimizerOptions.push_back("Levenberg-Marquardt");
    minimizerOptions.push_back("Simplex");
    minimizerOptions.push_back("Conjugate gradient (Fletcher-Reeves imp.)");
    minimizerOptions.push_back("Conjugate gradient (Polak-Ribiere imp.)");
    minimizerOptions.push_back("BFGS");
    declareProperty("Minimizer","Levenberg-Marquardt",new ListValidator(minimizerOptions),
      "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Direction::InOut);

    declareProperty("Output","","Base name for the output workspaces.");

  }


  /** Executes the algorithm
  *
  *  @throw runtime_error Thrown if algorithm cannot execute
  */
  void FitMultispectral::exec()
  {
    // Get the input workspace
    DataObjects::Workspace2D_sptr ws = getProperty("InputWorkspace");

    double startX = getProperty("StartX");
    double endX = getProperty("EndX");

    double startY = getProperty("StartY");
    double endY = getProperty("EndY");

    std::string functionStr = getPropertyValue("Function");
    std::string minimizer = getPropertyValue("Minimizer");
    std::string output = getPropertyValue("Output");

    // Find starting and ending spectra indeces istart and iend
    Axis* yaxis = ws->getAxis(1);
    int istart = 0;
    int iend = ws->getNumberHistograms()-1;
    if (yaxis)
    {
      if (startY == EMPTY_DBL())
      {
        startY = (*yaxis)(0);
      }
      if (endY == EMPTY_DBL())
      {
        endY = (*yaxis)(ws->getNumberHistograms()-1);
      }
    }
    else
    {
      if (startY == EMPTY_DBL())
      {
        startY = 0;
      }
      if (endY == EMPTY_DBL())
      {
        endY = double(ws->getNumberHistograms()-1);
      }
    }

    for(int i=0;i<ws->getNumberHistograms();++i)
    {
      double y;
      if (yaxis)
      {
        y = (*yaxis)(i);
      }
      else
      {
        y = double(i);
      }
      if (y < startY)
      {
        istart = i;
        continue;
      }
      if (y > endY)
      {
        iend = i;
        break;
      }
    }
    int nout = iend-istart+1;
    int ny = ws->blocksize();

    DataObjects::Workspace2D_sptr outputWS;

    if (!output.empty())
    {
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output), 
        "Name of the output Workspace holding resulting simlated spectrum");
      setPropertyValue("OutputWorkspace",output+"_Workspace");
      int histN = ws->isHistogramData() ? 1 : 0;
      outputWS = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
        (
        Mantid::API::WorkspaceFactory::Instance().create(
        ws,
        nout,
        ny + histN,
        ny)
        );
      outputWS->setTitle("");
      outputWS->getAxis(0)->unit() = ws->getAxis(0)->unit();
      if (!yaxis->isSpectra())
      {
        outputWS->replaceAxis(1,new Axis(false,nout));
      }
      outputWS->getAxis(1)->unit() = yaxis->unit();

      // Create table workspace and and proeprty for fitting parameters
      declareProperty( new WorkspaceProperty<API::ITableWorkspace>("OutputParameters","",Direction::Output));
      Mantid::API::ITableWorkspace_sptr params = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      setPropertyValue("OutputParameters",output+"_Parameters");
      setProperty("OutputParameters",params);
    }

    // Start fitting
    Progress progress(this,0,1,istart,nout,1);
    for(int i=istart;i<=iend;++i)
    {
      API::IAlgorithm_sptr fit = createSubAlgorithm("Fit");

      fit->setProperty("InputWorkspace",ws);
      fit->setProperty("WorkspaceIndex",i);

      fit->setProperty("Function",functionStr);

      fit->setProperty("StartX",startX);
      fit->setProperty("EndX",startX);

      fit->setProperty("Minimizer",minimizer);

      fit->setPropertyValue("Output",output);

      int io = i-istart;
      std::ostringstream ostr;
      ostr << "Fitting spectrum " << io+1 << " out of " << nout;
      progress.report(ostr.str());

      try
      {
        fit->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run Fit sub-algorithm");
        throw;
      }

      functionStr = fit->getPropertyValue("Function");

      if (!output.empty())
      {
        DataObjects::Workspace2D_sptr out = fit->getProperty("OutputWorkspace");
        outputWS->dataX(io) = out->readX(0);
        outputWS->dataY(io) = out->readY(1);
        outputWS->dataE(io) = out->readE(1);
        if (ws->getAxis(1))
        {
          outputWS->getAxis(1)->setValue(io,(*ws->getAxis(1))(i));
        }

        API::ITableWorkspace_sptr out_p = fit->getProperty("OutputParameters");
        API::ITableWorkspace_sptr params = getProperty("OutputParameters");
        // Create the output parameter table
        if (params->columnCount() == 0)
        {
          params->addColumn("double","X");
          API::ColumnVector<std::string> names = out_p->getVector("Name");
          for(size_t i=0;i<names.size();i++)
          {
            params->addColumn("double",names[i]);
          }
        }
        API::TableRow row = params->appendRow();
        row <<  (*outputWS->getAxis(1))(io);
        API::ColumnVector<double> values = out_p->getVector("Value");
        for(int i=0;i<values.size();i++)
        {
          row << values[i];
        }
      }
    }

    if (!output.empty())
    {
      setProperty("OutputWorkspace",outputWS);
    }

  }

} // namespace Algorithm
} // namespace Mantid
