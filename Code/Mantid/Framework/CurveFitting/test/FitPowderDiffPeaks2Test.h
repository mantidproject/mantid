#ifndef MANTID_CURVEFITTING_FITPOWDERDIFFPEAKSTEST_H_
#define MANTID_CURVEFITTING_FITPOWDERDIFFPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FitPowderDiffPeaks2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <fstream>

using Mantid::CurveFitting::FitPowderDiffPeaks2;

using namespace std;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;

class FitPowderDiffPeaks2Test : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPowderDiffPeaks2Test *createSuite() { return new FitPowderDiffPeaks2Test(); }
  static void destroySuite( FitPowderDiffPeaks2Test *suite ) { delete suite; }

  /** Test init
    */
  void test_Init()
  {
    CurveFitting::FitPowderDiffPeaks2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    return;
  }

  /** Fit the parameters for PG3's bank 1 with quite-off starting peak parameters.
    */
  void test_RobustFitPG3Bank1()
  {
    // 1. Generate testing workspace
    std::map<std::string, double> newparamvalues;

    API::MatrixWorkspace_sptr dataws = createInputDataWorkspace(2);

    std::string peakfilename("/home/wzz/Mantid/Code/debug/MyTestData/Bank1PeaksParameters.txt");
    std::vector<std::vector<int> > hkls;
    std::vector<std::vector<double> > peakparameters;
    importPeakParametersFile(peakfilename, hkls, peakparameters);
    DataObjects::TableWorkspace_sptr peakparamws = createReflectionWorkspace(hkls, peakparameters);

    std::string insfilename("/home/wzz/Mantid/Code/debug/MyTestData/Bank1InstrumentParameters.txt");
    std::map<std::string, double> instrparameters;
    importInstrumentTxtFile(insfilename, instrparameters);
    DataObjects::TableWorkspace_sptr geomparamws = createInstrumentParameterWorkspace(instrparameters, newparamvalues);

    AnalysisDataService::Instance().addOrReplace("DataWorkspace", dataws);
    AnalysisDataService::Instance().addOrReplace("PeakParameters", peakparamws);
    AnalysisDataService::Instance().addOrReplace("InstrumentParameters", geomparamws);

    // 2. Fit
    FitPowderDiffPeaks2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", dataws);
    alg.setProperty("OutputWorkspace", "FittedPeaks");
    alg.setProperty("BraggPeakParameterWorkspace", peakparamws);
    alg.setProperty("InstrumentParameterWorkspace", geomparamws);
    alg.setProperty("OutputBraggPeakParameterWorkspace", "PeaksParameterTable");
    alg.setProperty("OutputZscoreWorkspace", "ZscoreTable");
    alg.setProperty("WorkspaceIndex", 0);

    alg.setProperty("MinTOF", 19650.0);
    alg.setProperty("MaxTOF", 49000.0);

    vector<int32_t> minhkl(3); // HKL = (331)
    minhkl[0] = 3; minhkl[1] = 3; minhkl[2] = 1;
    alg.setProperty("MinimumHKL", minhkl);
    alg.setProperty("NumberPeaksToFitBelowLowLimit", 2);

    alg.setProperty("FittingMode", "Robust");

    // Right most peak (200)
    vector<int> rightmostpeakhkl(3);
    rightmostpeakhkl[0] = 2; rightmostpeakhkl[1] = 0; rightmostpeakhkl[2] = 0;
    alg.setProperty("RightMostPeakHKL", rightmostpeakhkl);

    alg.setProperty("RightMostPeakLeftBound", 46300.0);
    alg.setProperty("RightMostPeakRightBound", 47903.0);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 3. Check result
    DataObjects::TableWorkspace_sptr newgeomparamws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>
        (AnalysisDataService::Instance().retrieve("InstrumentParameters"));

    std::map<std::string, double> fitparamvalues;
    parseParameterTableWorkspace(newgeomparamws, fitparamvalues);
    double zero = fitparamvalues["Zero"];

    TS_ASSERT_DELTA(zero, 0.0, 1.0);

    return;
  }


  //------------------------------   Diffraction Data [From File] ----------------------------

  /** Create data workspace
   *  Option 1: Old Bank 7 data
   *         2: New Bank 1 data
   */
  API::MatrixWorkspace_sptr createInputDataWorkspace(int option)
  {
    // 1. Import data
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;

    switch (option)
    {
      case 1:
        importDataFromColumnFile("/home/wzz/Mantid/Code/debug/MyTestData/4862b7.inp", vecX, vecY, vecE);
        std::cout << "Option 1:  4862b7.inp.  Vector Size = " << vecX.size() << std::endl;
        break;

      case 2:
        importDataFromColumnFile("/home/wzz/Mantid/Code/debug/MyTestData/PG3_10808-1.dat", vecX, vecY, vecE);
        std::cout << "Option 2:  PG3_10808-1.dat.  Vector Size = " << vecX.size() << std::endl;
        break;

      default:
        // not supported
        std::cout << "LeBailFitTest.createInputDataWorkspace() Option " << option << " is not supported. " << std::endl;
        throw std::invalid_argument("Unsupported option. ");
    }

    // 2. Get workspace
    int64_t nHist = 1;
    int64_t nBins = vecX.size();

    API::MatrixWorkspace_sptr dataws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nHist, nBins, nBins));

    // 3. Input data
    for (size_t i = 0; i < vecX.size(); ++i)
    {
      dataws->dataX(0)[i] = vecX[i];
      dataws->dataY(0)[i] = vecY[i];
      dataws->dataE(0)[i] = vecE[i];
    }

    return dataws;
  }

  /** Import data from a column data file
   */
  void importDataFromColumnFile(std::string filename, std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
  {
    std::ifstream ins;
    ins.open(filename.c_str());

    if (!ins.is_open())
    {
        std::cout << "Data file " << filename << " cannot be opened. " << std::endl;
        throw std::invalid_argument("Unable to open data fiile. ");
    }
    else
    {
        std::cout << "Data file " << filename << " is opened for parsing. " << std::endl;
    }

    char line[256];
    // std::cout << "File " << filename << " isOpen = " << ins.is_open() << std::endl;
    while(ins.getline(line, 256))
    {
      if (line[0] != '#')
      {
        double x, y;
        std::stringstream ss;
        ss.str(line);
        ss >> x >> y;
        vecX.push_back(x);
        vecY.push_back(y);
        double e = 1.0;
        if (y > 1.0E-5)
          e = std::sqrt(y);
        vecE.push_back(e);
      }
    }

    return;
  }


  // ====================  Reflection [From File] ==================== //
  /** Create reflection table workspaces
   */
  DataObjects::TableWorkspace_sptr createReflectionWorkspace(std::vector<std::vector<int> > hkls,
                                                             std::vector<std::vector<double> > peakparams)
  {
    // 1. Crate table workspace
    DataObjects::TableWorkspace* tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr hklws = DataObjects::TableWorkspace_sptr(tablews);

    tablews->addColumn("int", "H");
    tablews->addColumn("int", "K");
    tablews->addColumn("int", "L");
    tablews->addColumn("double", "PeakHeight");
    tablews->addColumn("double", "TOF_h");
    tablews->addColumn("double", "Alpha");
    tablews->addColumn("double", "Beta");
    tablews->addColumn("double", "Sigma2");
    tablews->addColumn("double", "Gamma");

    // 2. Add reflections and heights
    for (size_t ipk = 0; ipk < hkls.size(); ++ipk)
    {
      API::TableRow hkl = hklws->appendRow();
      for (size_t i = 0; i < 3; ++i)
      {
        hkl << hkls[ipk][i];
      }
      for (size_t ipm = 0; ipm < peakparams[ipk].size(); ++ipm)
      {
        hkl << peakparams[ipk][ipm];
      }
    }
    std::cout << "Created Table Workspace with " << hkls.size() << " entries of peaks." << std::endl;

    return hklws;
  }

  /** Import text file containing reflection (HKL) and peak parameters
   * Input:  a text based file
   * Output: a vector for (H, K, L) and a vector for (Height, TOF_H, ALPHA, BETA, ...
   */
  void importPeakParametersFile(std::string filename, std::vector<std::vector<int> >& hkls,
                                std::vector<std::vector<double> >& peakparameters)
  {
    // 1. Open file
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open())
    {
      std::cout << "File " << filename << " cannot be opened. " << std::endl;
      throw std::invalid_argument("Cannot open Reflection-Text-File.");
    }
    else
    {
      std::cout << "Parsing peak parameters file " << filename << std::endl;
    }

    // 2. Parse
    hkls.clear();
    peakparameters.clear();

    char line[256];
    while(ins.getline(line, 256))
    {
      if (line[0] != '#')
      {
        int h, k, l;
        std::vector<int> hkl;
        std::stringstream ss;
        ss.str(line);
        ss >> h >> k >> l;
        hkl.push_back(h);
        hkl.push_back(k);
        hkl.push_back(l);
        hkls.push_back(hkl);

        double height, tof_h, alpha, beta, sigma2, gamma;
        std::vector<double> params;
        ss >> height >> tof_h >> alpha >> beta >> sigma2 >> gamma;
        params.push_back(height);
        params.push_back(tof_h);
        params.push_back(alpha);
        params.push_back(beta);
        params.push_back(sigma2);
        params.push_back(gamma);
        peakparameters.push_back(params);
      }
    }

    // 3. Close up
    ins.close();

    return;
  }


  // ====================  Instrument Parameters [From File] ==================== //

  /** Create instrument geometry parameter/LeBail parameter workspaces
   */
  DataObjects::TableWorkspace_sptr createInstrumentParameterWorkspace(std::map<std::string, double> parameters,
                                                                      std::map<std::string, double> newvalueparameters)
  {
    // 1. Combine 2 inputs
    std::map<std::string, double>::iterator nvit;
    std::stringstream infoss;
    infoss << "Importing instrument related parameters: " << std::endl;
    for (nvit = newvalueparameters.begin(); nvit != newvalueparameters.end(); ++nvit)
    {
      std::map<std::string, double>::iterator fdit;
      fdit = parameters.find(nvit->first);
      if (fdit != parameters.end())
      {
        fdit->second = nvit->second;
        infoss << "Name: " << std::setw(15) << fdit->first << ", Value: " << fdit->second << std::endl;
      }
    }
    std::cout << infoss.str();

    // 2. Crate table workspace
    DataObjects::TableWorkspace* tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr geomws = DataObjects::TableWorkspace_sptr(tablews);

    std::vector<std::string> paramnames;
    paramnames.push_back("Zero");
    paramnames.push_back("Zerot");
    paramnames.push_back("Dtt1");
    paramnames.push_back("Dtt1t");
    paramnames.push_back("Dtt2t");
    paramnames.push_back("Tcross");
    paramnames.push_back("Width");
    paramnames.push_back("LatticeConstant");

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");

    // 2. Add peak parameters' name and values
    for (size_t ipn = 0; ipn < paramnames.size(); ++ipn)
    {
      API::TableRow newrow = geomws->appendRow();
      std::string parname =  paramnames[ipn];
      double parvalue = parameters[paramnames[ipn]];
      newrow << parname << parvalue;
    }

    return geomws;
  }

  /** Import text file containing reflection (HKL)
   *  Input:  a text based file
   *  Output: a map for (parameter name, parameter value)
   */
  void importInstrumentTxtFile(std::string filename, std::map<std::string, double>& parameters)
  {
    // 1. Open file
    std::ifstream ins;
    ins.open(filename.c_str());
    if (!ins.is_open())
    {
      std::cout << "File " << filename << " cannot be opened. " << std::endl;
      throw std::invalid_argument("Cannot open Reflection-Text-File.");
    }
    else
    {
      std::cout << "Importing instrument parameter file " << filename << std::endl;
    }

    // 2. Parse
    parameters.clear();

    char line[256];
    while(ins.getline(line, 256))
    {
      if (line[0] != '#')
      {
        std::string parname;
        double parvalue;

        std::stringstream ss;
        ss.str(line);
        ss >> parname >> parvalue;

        parameters.insert(std::make_pair(parname, parvalue));
      }
    }

    ins.close();

    return;
  }


  // ==============================  Check Output ========================= //

  /** Parse parameter table workspace : Name, Value to a map
    */
  void parseParameterTableWorkspace(Mantid::DataObjects::TableWorkspace_sptr paramws,
                                    std::map<std::string, double>& paramvalues)
  {
    for (size_t irow = 0; irow < paramws->rowCount(); ++irow)
    {
      Mantid::API::TableRow row = paramws->getRow(irow);
      std::string parname;
      double parvalue;
      row >> parname >> parvalue;

      paramvalues.insert(std::make_pair(parname, parvalue));
    }

    return;
  }

};


#endif /* MANTID_CURVEFITTING_FITPOWDERDIFFPEAKSTEST_H_ */
