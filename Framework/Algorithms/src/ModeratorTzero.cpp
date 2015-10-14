//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ModeratorTzero.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/muParser_Silent.h"
#include <boost/lexical_cast.hpp>
#include "MantidDataObjects/EventList.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ModeratorTzero)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

ModeratorTzero::ModeratorTzero()
    : Mantid::API::Algorithm(),
      m_convfactor(0.5e+12 * Mantid::PhysicalConstants::NeutronMass /
                   Mantid::PhysicalConstants::meV),
      m_niter(1), m_tolTOF(0.), m_t1min(200.0) {}

/// set attribute m_formula
void ModeratorTzero::setFormula(const std::string &formula) {
  m_formula = formula;
}

void ModeratorTzero::init() {

  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace, containing events and/or "
                  "histogram data, in units of time-of-flight");
  // declare the output workspace
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The name of the output workspace");

  // declare the instrument scattering mode
  std::vector<std::string> EModeOptions;
  EModeOptions.push_back("Indirect");
  EModeOptions.push_back("Direct");
  EModeOptions.push_back("Elastic");
  this->declareProperty("EMode", "Indirect",
                        boost::make_shared<StringListValidator>(EModeOptions),
                        "The energy mode (default: Indirect)");

  declareProperty(new Kernel::PropertyWithValue<double>(
                      "tolTOF", 0.1, Kernel::Direction::Input),
                  "Tolerance in the calculation of the emission time, in "
                  "microseconds (default:1)");
  declareProperty(new Kernel::PropertyWithValue<size_t>(
                      "Niter", 1, Kernel::Direction::Input),
                  "Number of iterations (default:1)");
} // end of void ModeratorTzero::init()

void ModeratorTzero::exec() {
  m_tolTOF = getProperty("tolTOF"); // Tolerance in the calculation of the
                                    // emission time, in microseconds
  m_niter = getProperty("Niter");   // number of iterations
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  m_instrument = inputWS->getInstrument(); // pointer to the instrument
  const std::string emode = getProperty("EMode");

  // Check if Ei stored in workspace
  if (emode == "Direct") {
    if (!inputWS->run().hasProperty("Ei")) {
      g_log.error("No incident energy value (Ei) has been set or stored.");
      return;
    }
  }

  // extract formula from instrument parameters
  std::vector<std::string> t0_formula =
      m_instrument->getStringParameter("t0_formula");
  if (t0_formula.empty()) {
    g_log.error("Unable to retrieve t0_formula among instrument parameters.");
    return;
  }
  m_formula = t0_formula[0];
  // Are there source and sample?
  IComponent_const_sptr source;
  IComponent_const_sptr sample;
  double Lss(0); // distance from source to sample
  try {
    source = m_instrument->getSource();
    sample = m_instrument->getSample();
    Lss = source->getDistance(*sample);
  } catch (Exception::NotFoundError &) {
    g_log.error("Unable to calculate source-sample distance.");
    return;
  }

  // Run execEvent if eventWorkSpace
  EventWorkspace_const_sptr eventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventWS != NULL) {
    execEvent(emode);
    return;
  }

  // Run exec for matrix workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // Check whether input == output to see whether a new workspace is required.
  if (outputWS != inputWS) {
    // Create new workspace for output from old
    outputWS = WorkspaceFactory::Instance().create(inputWS);
  }

  // calculate tof shift once for all neutrons if emode==Direct
  double t0_direct(-1);
  if (emode == "Direct") {
    Kernel::Property *eiprop = inputWS->run().getProperty("Ei");
    double Ei = boost::lexical_cast<double>(eiprop->value());
    mu::Parser parser;
    parser.DefineVar("incidentEnergy", &Ei); // associate E1 to this parser
    parser.SetExpr(m_formula);
    t0_direct = parser.Eval();
  }

  const size_t numHists = static_cast<size_t>(inputWS->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR2(inputWS, outputWS)
  // iterate over the spectra
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    MantidVec &inbins = inputWS->dataX(i);
    MantidVec &outbins = outputWS->dataX(i);

    // One parser for each parallel processor needed (except Edirect mode)
    double E1;
    mu::Parser parser;
    parser.DefineVar("incidentEnergy", &E1); // associate E1 to this parser
    parser.SetExpr(m_formula);

    IDetector_const_sptr det;
    double L1(Lss); // distance from source to sample
    double L2(-1);  // distance from sample to detector
    try {
      det = inputWS->getDetector(i);
      if (det->isMonitor()) {
        // redefine the sample as the monitor
        L1 = source->getDistance(*det);
        L2 = 0;
      } else {
        L2 = sample->getDistance(*det);
      }
    } // end of try
    catch (Exception::NotFoundError &) {
      g_log.error() << "Unable to calculate distances to/from detector" << i
                    << std::endl;
      outbins = inbins;
    }
    if (L2 >= 0) {
      // fast neutrons are shifted by min_t0_next, irrespective of tof
      double v1_max = L1 / m_t1min;
      E1 = m_convfactor * v1_max * v1_max;
      double min_t0_next = parser.Eval();

      if (emode == "Indirect") {
        double t2(-1.0); // time from sample to detector. (-1) signals error
        if (det->isMonitor()) {
          t2 = 0.0;
        } else {
          static const double convFact =
              1.0e-6 *
              sqrt(2 * PhysicalConstants::meV / PhysicalConstants::NeutronMass);
          std::vector<double> wsProp = det->getNumberParameter("Efixed");
          if (!wsProp.empty()) {
            double E2 = wsProp.at(0);        //[E2]=meV
            double v2 = convFact * sqrt(E2); //[v2]=meter/microsec
            t2 = L2 / v2;
          } else {
            // t2 is kept to -1 if no Efixed is found
            g_log.debug() << "Efixed not found for detector " << i << std::endl;
          }
        }
        // shift the time of flights by the emission time from the moderator
        if (t2 >= 0) // t2 < 0 when no detector info is available
        {
          // iterate over the time-of-flight values
          for (unsigned int ibin = 0; ibin < inbins.size(); ibin++) {
            double tof = inbins[ibin]; // recorded time-of-flight
            if (tof < m_t1min + t2)
              tof -= min_t0_next;
            else
              tof -= CalculateT0indirect(tof, L1, t2, E1, parser);
            outbins[ibin] = tof;
          }
        } else {
          outbins = inbins;
        }
      } // end of if(emode=="Indirect")
      else if (emode == "Elastic") {
        for (unsigned int ibin = 0; ibin < inbins.size(); ibin++) {
          double tof = inbins[ibin]; // recorded time-of-flight;
          if (tof < m_t1min * (L1 + L2) / L1)
            tof -= min_t0_next;
          else
            tof -= CalculateT0elastic(tof, L1 + L2, E1, parser);
          outbins[ibin] = tof;
        }
      } // end of else if(emode=="Elastic")
      else if (emode == "Direct") {
        for (unsigned int ibin = 0; ibin < inbins.size(); ibin++) {
          outbins[ibin] = inbins[ibin] - t0_direct;
        }
      } // end of else if(emode="Direct")
    }   // end of if(L2 >= 0)

    // Copy y and e data
    outputWS->dataY(i) = inputWS->dataY(i);
    outputWS->dataE(i) = inputWS->dataE(i);
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // end of for (int i = 0; i < static_cast<int>(numHists); ++i)
  PARALLEL_CHECK_INTERUPT_REGION

  // Copy units
  if (inputWS->getAxis(0)->unit().get()) {
    outputWS->getAxis(0)->unit() = inputWS->getAxis(0)->unit();
  }
  try {
    if (inputWS->getAxis(1)->unit().get()) {
      outputWS->getAxis(1)->unit() = inputWS->getAxis(1)->unit();
    }
  } catch (Exception::IndexError &) {
    // OK, so this isn't a Workspace2D
  }

  // Assign it to the output workspace property
} // end of void ModeratorTzero::exec

void ModeratorTzero::execEvent(const std::string &emode) {
  g_log.information("Processing event workspace");

  const MatrixWorkspace_const_sptr matrixInputWS =
      getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  const size_t numHists = static_cast<size_t>(inputWS->getNumberHistograms());
  Mantid::API::MatrixWorkspace_sptr matrixOutputWS =
      getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS) {
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  } else {
    // Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        WorkspaceFactory::Instance().create("EventWorkspace", numHists, 2, 1));
    // Copy geometry over.
    WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    // You need to copy over the data as well.
    outputWS->copyDataFrom((*inputWS));
    // Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    setProperty("OutputWorkspace", matrixOutputWS);
  }

  // Get pointers to sample and source
  IComponent_const_sptr source = m_instrument->getSource();
  IComponent_const_sptr sample = m_instrument->getSample();
  double Lss = source->getDistance(*sample); // distance from source to sample

  // calculate tof shift once for all neutrons if emode==Direct
  double t0_direct(-1);
  if (emode == "Direct") {
    Kernel::Property *eiprop = inputWS->run().getProperty("Ei");
    double Ei = boost::lexical_cast<double>(eiprop->value());
    mu::Parser parser;
    parser.DefineVar("incidentEnergy", &Ei); // associate E1 to this parser
    parser.SetExpr(m_formula);
    t0_direct = parser.Eval();
  }

  // Loop over the spectra
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR1(outputWS)
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    size_t wsIndex = static_cast<size_t>(i);
    EventList &evlist = outputWS->getEventList(wsIndex);
    if (evlist.getNumberEvents() > 0) // don't bother with empty lists
    {
      IDetector_const_sptr det;
      double L1(Lss); // distance from source to sample
      double L2(-1);  // distance from sample to detector

      try {
        det = inputWS->getDetector(i);
        if (det->isMonitor()) {
          // redefine the sample as the monitor
          L1 = source->getDistance(*det);
          L2 = 0;
        } else {
          L2 = sample->getDistance(*det);
        }
      } catch (Exception::NotFoundError &) {
        g_log.error() << "Unable to calculate distances to/from detector" << i
                      << std::endl;
      }

      if (L2 >= 0) {
        // One parser for each parallel processor needed (except Edirect mode)
        double E1;
        mu::Parser parser;
        parser.DefineVar("incidentEnergy", &E1); // associate E1 to this parser
        parser.SetExpr(m_formula);

        // fast neutrons are shifted by min_t0_next, irrespective of tof
        double v1_max = L1 / m_t1min;
        E1 = m_convfactor * v1_max * v1_max;
        double min_t0_next = parser.Eval();

        if (emode == "Indirect") {
          double t2(-1.0); // time from sample to detector. (-1) signals error
          if (det->isMonitor()) {
            t2 = 0.0;
          } else {
            static const double convFact =
                1.0e-6 * sqrt(2 * PhysicalConstants::meV /
                              PhysicalConstants::NeutronMass);
            std::vector<double> wsProp = det->getNumberParameter("Efixed");
            if (!wsProp.empty()) {
              double E2 = wsProp.at(0);        //[E2]=meV
              double v2 = convFact * sqrt(E2); //[v2]=meter/microsec
              t2 = L2 / v2;
            } else {
              // t2 is kept to -1 if no Efixed is found
              g_log.debug() << "Efixed not found for detector " << i
                            << std::endl;
            }
          }
          if (t2 >= 0) // t2 < 0 when no detector info is available
          {
            double tof;
            // fix the histogram bins
            MantidVec &x = evlist.dataX();
            for (MantidVec::iterator iter = x.begin(); iter != x.end();
                 ++iter) {
              tof = *iter;
              if (tof < m_t1min + t2)
                tof -= min_t0_next;
              else
                tof -= CalculateT0indirect(tof, L1, t2, E1, parser);
              *iter = tof;
            }

            MantidVec tofs = evlist.getTofs();
            for (unsigned int itof = 0; itof < tofs.size(); itof++) {
              tof = tofs[itof];
              if (tof < m_t1min + t2)
                tof -= min_t0_next;
              else
                tof -= CalculateT0indirect(tof, L1, t2, E1, parser);
              tofs[itof] = tof;
            }
            evlist.setTofs(tofs);
            evlist.setSortOrder(Mantid::DataObjects::EventSortType::UNSORTED);
          } // end of if( t2>= 0)
        }   // end of if(emode=="Indirect")
        else if (emode == "Elastic") {
          double tof;
          // Apply t0 correction to histogram bins
          MantidVec &x = evlist.dataX();
          for (MantidVec::iterator iter = x.begin(); iter != x.end(); ++iter) {
            tof = *iter;
            if (tof < m_t1min * (L1 + L2) / L1)
              tof -= min_t0_next;
            else
              tof -= CalculateT0elastic(tof, L1 + L2, E1, parser);
            *iter = tof;
          }

          MantidVec tofs = evlist.getTofs();
          for (unsigned int itof = 0; itof < tofs.size(); itof++) {
            // add a [-0.1,0.1] microsecond noise to avoid artifacts
            // resulting from original tof data
            tof = tofs[itof];
            if (tof < m_t1min * (L1 + L2) / L1)
              tof -= min_t0_next;
            else
              tof -= CalculateT0elastic(tof, L1 + L2, E1, parser);
            tofs[itof] = tof;
          }
          evlist.setTofs(tofs);
          evlist.setSortOrder(Mantid::DataObjects::EventSortType::UNSORTED);

          MantidVec tofs_b = evlist.getTofs();
          MantidVec xarray = evlist.readX();
        } // end of else if(emode=="Elastic")
        else if (emode == "Direct") {
          // fix the histogram bins
          MantidVec &x = evlist.dataX();
          for (MantidVec::iterator iter = x.begin(); iter != x.end(); ++iter) {
            *iter -= t0_direct;
          }

          MantidVec tofs = evlist.getTofs();
          for (unsigned int itof = 0; itof < tofs.size(); itof++) {
            tofs[itof] -= t0_direct;
          }
          evlist.setTofs(tofs);
          evlist.setSortOrder(Mantid::DataObjects::EventSortType::UNSORTED);
        } // end of else if(emode=="Direct")
      }   // end of if(L2 >= 0)
    }     // end of if (evlist.getNumberEvents() > 0)
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // end of for (int i = 0; i < static_cast<int>(numHists); ++i)
  PARALLEL_CHECK_INTERUPT_REGION
  outputWS->clearMRU(); // Clears the Most Recent Used lists */
} // end of void ModeratorTzero::execEvent()

/// Calculate emission time for a given detector (L1, t2)
/// and TOF when Emode==Inelastic
double ModeratorTzero::CalculateT0indirect(const double &tof, const double &L1,
                                           const double &t2, double &E1,
                                           mu::Parser &parser) {

  double t0_curr, t0_next;
  t0_curr = m_tolTOF; // current iteration emission time
  t0_next = 0.0;      // next iteration emission time, initialized to zero
  size_t iiter(0);    // current iteration number
  // iterate until convergence in t0 reached
  while (std::fabs(t0_curr - t0_next) >= m_tolTOF && iiter < m_niter) {
    t0_curr = t0_next;
    double t1 = tof - t0_curr - t2;
    double v1 = L1 / t1;
    E1 = m_convfactor * v1 * v1; // Energy in meV if v1 in meter/microsecond
    t0_next = parser.Eval();
    iiter++;
  }
  return t0_next;
}

/// Calculate emission time for a given detector (L1, t2)
/// and TOF when Emode==Elastic
double ModeratorTzero::CalculateT0elastic(const double &tof, const double &L12,
                                          double &E1, mu::Parser &parser) {
  double t0_curr, t0_next;
  t0_curr = m_tolTOF; // current iteration emission time
  t0_next = 0.0;      // next iteration emission time, initialized to zero
  size_t iiter(0);    // current iteration number
  // iterate until convergence in t0 reached
  while (std::fabs(t0_curr - t0_next) >= m_tolTOF && iiter < m_niter) {
    t0_curr = t0_next;
    double v1 = L12 / (tof - t0_curr); // v1 = v2 = v since emode is elastic
    E1 = m_convfactor * v1 * v1; // Energy in meV if v1 in meter/microsecond
    t0_next = parser.Eval();
    iiter++;
  }
  return t0_next;
}
double ModeratorTzero::gett1min() { return m_t1min; }

} // namespace Algorithms
} // namespace Mantid
