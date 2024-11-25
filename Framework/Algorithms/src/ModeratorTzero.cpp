// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ModeratorTzero.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ModeratorTzero)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

ModeratorTzero::ModeratorTzero()
    : Mantid::API::Algorithm(),
      m_convfactor(0.5e+12 * Mantid::PhysicalConstants::NeutronMass / Mantid::PhysicalConstants::meV), m_niter(1),
      m_tolTOF(0.), m_t1min(200.0) {}

/// set attribute m_formula
void ModeratorTzero::setFormula(const std::string &formula) { m_formula = formula; }

void ModeratorTzero::init() {

  auto wsValidator = std::make_shared<WorkspaceUnitValidator>("TOF");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "The name of the input workspace, containing events and/or "
      "histogram data, in units of time-of-flight");
  // declare the output workspace
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace");

  // declare the instrument scattering mode
  std::vector<std::string> EModeOptions{"Indirect", "Direct", "Elastic"};
  this->declareProperty("EMode", "Indirect", std::make_shared<StringListValidator>(EModeOptions),
                        "The energy mode (default: Indirect)");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<double>>("tolTOF", 0.1, Kernel::Direction::Input),
                  "Tolerance in the calculation of the emission time, in "
                  "microseconds (default:1)");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<size_t>>("Niter", 1, Kernel::Direction::Input),
                  "Number of iterations (default:1)");
} // end of void ModeratorTzero::init()

void ModeratorTzero::exec() {
  m_tolTOF = getProperty("tolTOF"); // Tolerance in the calculation of the
                                    // emission time, in microseconds
  m_niter = getProperty("Niter");   // number of iterations
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string emode = getProperty("EMode");

  // Check if Ei stored in workspace
  if (emode == "Direct") {
    if (!inputWS->run().hasProperty("Ei")) {
      g_log.error("No incident energy value (Ei) has been set or stored.");
      return;
    }
  }

  // extract formula from instrument parameters
  auto t0_formula = inputWS->getInstrument()->getStringParameter("t0_formula");
  if (t0_formula.empty()) {
    g_log.error("Unable to retrieve t0_formula among instrument parameters.");
    return;
  }
  m_formula = t0_formula[0];

  // Run execEvent if eventWorkSpace
  EventWorkspace_const_sptr eventWS = std::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventWS != nullptr) {
    execEvent(emode);
    return;
  }

  // Run exec for matrix workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // Check whether input == output to see whether a new workspace is required.
  if (outputWS != inputWS) {
    // Create new workspace for output from old
    outputWS = create<MatrixWorkspace>(*inputWS);
  }

  // calculate tof shift once for all neutrons if emode==Direct
  double t0_direct(-1);
  if (emode == "Direct") {
    auto Ei = inputWS->run().getPropertyValueAsType<double>("Ei");
    mu::Parser parser;
    parser.DefineVar("incidentEnergy", &Ei); // associate E1 to this parser
    parser.SetExpr(m_formula);
    t0_direct = parser.Eval();
  }

  const auto &spectrumInfo = inputWS->spectrumInfo();
  const double Lss = spectrumInfo.l1();

  const auto numHists = static_cast<size_t>(inputWS->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  // iterate over the spectra
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    outputWS->setHistogram(i, inputWS->histogram(i));

    // One parser for each parallel processor needed (except Edirect mode)
    double E1;
    mu::Parser parser;
    parser.DefineVar("incidentEnergy", &E1); // associate E1 to this parser
    parser.SetExpr(m_formula);

    double L1(Lss); // distance from source to sample
    double L2(-1);  // distance from sample to detector
    if (spectrumInfo.hasDetectors(i)) {
      if (spectrumInfo.isMonitor(i)) {
        // redefine the sample as the monitor
        L1 = Lss + spectrumInfo.l2(i); // L2 in SpectrumInfo defined negative
        L2 = 0;
      } else {
        L2 = spectrumInfo.l2(i);
      }
    } else {
      g_log.error() << "Unable to calculate distances to/from detector" << i << '\n';
    }

    if (L2 >= 0) {
      // fast neutrons are shifted by min_t0_next, irrespective of tof
      double v1_max = L1 / m_t1min;
      E1 = m_convfactor * v1_max * v1_max;
      double min_t0_next = parser.Eval();

      if (emode == "Indirect") {
        double t2(-1.0); // time from sample to detector. (-1) signals error
        if (spectrumInfo.isMonitor(i)) {
          t2 = 0.0;
        } else {
          static const double convFact = 1.0e-6 * sqrt(2 * PhysicalConstants::meV / PhysicalConstants::NeutronMass);
          std::vector<double> wsProp = spectrumInfo.detector(i).getNumberParameter("Efixed");
          if (!wsProp.empty()) {
            double E2 = wsProp.at(0);        //[E2]=meV
            double v2 = convFact * sqrt(E2); //[v2]=meter/microsec
            t2 = L2 / v2;
          } else {
            // t2 is kept to -1 if no Efixed is found
            g_log.debug() << "Efixed not found for detector " << i << '\n';
          }
        }
        // shift the time of flights by the emission time from the moderator
        if (t2 >= 0) // t2 < 0 when no detector info is available
        {
          auto &outbins = outputWS->mutableX(i);
          for (auto &tof : outbins) {
            if (tof < m_t1min + t2)
              tof -= min_t0_next;
            else
              tof -= CalculateT0indirect(tof, L1, t2, E1, parser);
          }
        }
      } // end of if(emode=="Indirect")
      else if (emode == "Elastic") {
        auto &outbins = outputWS->mutableX(i);
        for (auto &tof : outbins) {
          if (tof < m_t1min * (L1 + L2) / L1)
            tof -= min_t0_next;
          else
            tof -= CalculateT0elastic(tof, L1 + L2, E1, parser);
        }
      } // end of else if(emode=="Elastic")
      else if (emode == "Direct") {
        outputWS->mutableX(i) += -t0_direct;
      } // end of else if(emode="Direct")
    } // end of if(L2 >= 0)

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  } // end of for (int i = 0; i < static_cast<int>(numHists); ++i)
  PARALLEL_CHECK_INTERRUPT_REGION

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

  const MatrixWorkspace_const_sptr matrixInputWS = getProperty("InputWorkspace");

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  if (matrixOutputWS != matrixInputWS) {
    matrixOutputWS = matrixInputWS->clone();
    setProperty("OutputWorkspace", matrixOutputWS);
  }
  auto outputWS = std::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);

  // calculate tof shift once for all neutrons if emode==Direct
  double t0_direct(-1);
  if (emode == "Direct") {
    auto Ei = outputWS->run().getPropertyValueAsType<double>("Ei");
    mu::Parser parser;
    parser.DefineVar("incidentEnergy", &Ei); // associate E1 to this parser
    parser.SetExpr(m_formula);
    t0_direct = parser.Eval();
  }

  const auto &spectrumInfo = outputWS->spectrumInfo();
  const double Lss = spectrumInfo.l1();

  // Loop over the spectra
  const auto numHists = static_cast<size_t>(outputWS->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, numHists); // report progress of algorithm
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int i = 0; i < static_cast<int>(numHists); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    auto wsIndex = static_cast<size_t>(i);
    EventList &evlist = outputWS->getSpectrum(wsIndex);
    if (evlist.getNumberEvents() > 0) // don't bother with empty lists
    {
      double L1(Lss); // distance from source to sample
      double L2(-1);  // distance from sample to detector

      if (spectrumInfo.hasDetectors(i)) {
        if (spectrumInfo.isMonitor(i)) {
          // redefine the sample as the monitor
          L1 = Lss + spectrumInfo.l2(i); // L2 in SpectrumInfo defined negative
          L2 = 0;
        } else {
          L2 = spectrumInfo.l2(i);
        }
      } else {
        g_log.error() << "Unable to calculate distances to/from detector" << i << '\n';
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
          if (spectrumInfo.isMonitor(i)) {
            t2 = 0.0;
          } else {
            static const double convFact = 1.0e-6 * sqrt(2 * PhysicalConstants::meV / PhysicalConstants::NeutronMass);
            std::vector<double> wsProp = spectrumInfo.detector(i).getNumberParameter("Efixed");
            if (!wsProp.empty()) {
              double E2 = wsProp.at(0);        //[E2]=meV
              double v2 = convFact * sqrt(E2); //[v2]=meter/microsec
              t2 = L2 / v2;
            } else {
              // t2 is kept to -1 if no Efixed is found
              g_log.debug() << "Efixed not found for detector " << i << '\n';
            }
          }
          if (t2 >= 0) // t2 < 0 when no detector info is available
          {
            // fix the histogram bins
            auto &x = evlist.mutableX();
            for (double &tof : x) {
              if (tof < m_t1min + t2)
                tof -= min_t0_next;
              else
                tof -= CalculateT0indirect(tof, L1, t2, E1, parser);
            }

            MantidVec tofs = evlist.getTofs();
            for (double &tof : tofs) {
              if (tof < m_t1min + t2)
                tof -= min_t0_next;
              else
                tof -= CalculateT0indirect(tof, L1, t2, E1, parser);
            }
            evlist.setTofs(tofs);
            evlist.setSortOrder(Mantid::DataObjects::EventSortType::UNSORTED);
          } // end of if( t2>= 0)
        } // end of if(emode=="Indirect")
        else if (emode == "Elastic") {
          // Apply t0 correction to histogram bins
          auto &x = evlist.mutableX();
          for (double &tof : x) {
            if (tof < m_t1min * (L1 + L2) / L1)
              tof -= min_t0_next;
            else
              tof -= CalculateT0elastic(tof, L1 + L2, E1, parser);
          }

          MantidVec tofs = evlist.getTofs();
          for (double &tof : tofs) {
            // add a [-0.1,0.1] microsecond noise to avoid artifacts
            // resulting from original tof data
            if (tof < m_t1min * (L1 + L2) / L1)
              tof -= min_t0_next;
            else
              tof -= CalculateT0elastic(tof, L1 + L2, E1, parser);
          }
          evlist.setTofs(tofs);
          evlist.setSortOrder(Mantid::DataObjects::EventSortType::UNSORTED);
        } // end of else if(emode=="Elastic")
        else if (emode == "Direct") {
          // fix the histogram bins
          evlist.mutableX() -= t0_direct;

          MantidVec tofs = evlist.getTofs();

          std::transform(tofs.begin(), tofs.end(), tofs.begin(), [&t0_direct](double tof) { return tof - t0_direct; });
          evlist.setTofs(tofs);
          evlist.setSortOrder(Mantid::DataObjects::EventSortType::UNSORTED);
        } // end of else if(emode=="Direct")
      } // end of if(L2 >= 0)
    } // end of if (evlist.getNumberEvents() > 0)
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  } // end of for (int i = 0; i < static_cast<int>(numHists); ++i)
  PARALLEL_CHECK_INTERRUPT_REGION
  outputWS->clearMRU(); // Clears the Most Recent Used lists */
} // end of void ModeratorTzero::execEvent()

/// Calculate emission time for a given detector (L1, t2)
/// and TOF when Emode==Inelastic
double ModeratorTzero::CalculateT0indirect(const double &tof, const double &L1, const double &t2, double &E1,
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
double ModeratorTzero::CalculateT0elastic(const double &tof, const double &L12, double &E1, mu::Parser &parser) {
  double t0_curr, t0_next;
  t0_curr = m_tolTOF; // current iteration emission time
  t0_next = 0.0;      // next iteration emission time, initialized to zero
  size_t iiter(0);    // current iteration number
  // iterate until convergence in t0 reached
  while (std::fabs(t0_curr - t0_next) >= m_tolTOF && iiter < m_niter) {
    t0_curr = t0_next;
    double v1 = L12 / (tof - t0_curr); // v1 = v2 = v since emode is elastic
    E1 = m_convfactor * v1 * v1;       // Energy in meV if v1 in meter/microsecond
    t0_next = parser.Eval();
    iiter++;
  }
  return t0_next;
}
double ModeratorTzero::gett1min() { return m_t1min; }

} // namespace Mantid::Algorithms
