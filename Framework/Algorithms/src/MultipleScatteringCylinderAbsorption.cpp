//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MultipleScatteringCylinderAbsorption.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"

#include <stdexcept>

namespace Mantid {
namespace Algorithms {
DECLARE_ALGORITHM(MultipleScatteringCylinderAbsorption) // Register the class
                                                        // into the algorithm
                                                        // factory

using namespace Kernel;
using namespace API;
using Mantid::DataObjects::EventList;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::DataObjects::WeightedEventNoTime;
using std::vector;
using namespace Mantid::PhysicalConstants;
using namespace Geometry;

// Constants required internally only, so make them static
namespace { // anonymous
static const double C[] = {
    0.730284,  -0.249987, 0.019448,  -0.000006, 0.000249,  -0.000004, 0.848859,
    -0.452690, 0.056557,  -0.000009, 0.000000,  -0.000006, 1.133129,  -0.749962,
    0.118245,  -0.000018, -0.001345, -0.000012, 1.641112,  -1.241639, 0.226247,
    -0.000045, -0.004821, -0.000030, 0.848859,  -0.452690, 0.056557,  -0.000009,
    0.000000,  -0.000006, 1.000006,  -0.821100, 0.166645,  -0.012096, 0.000008,
    -0.000126, 1.358113,  -1.358076, 0.348199,  -0.038817, 0.000022,  -0.000021,
    0.0,       0.0,       0.0,       0.0,       0.0,       0.0,       1.133129,
    -0.749962, 0.118245,  -0.000018, -0.001345, -0.000012, 1.358113,  -1.358076,
    0.348199,  -0.038817, 0.000022,  -0.000021, 0.0,       0.0,       0.0,
    0.0,       0.0,       0.0,       0.0,       0.0,       0.0,       0.0,
    0.0,       0.0,       1.641112,  -1.241639, 0.226247,  -0.000045, -0.004821,
    -0.000030, 0.0,       0.0,       0.0,       0.0,       0.0,       0.0,
    0.0,       0.0,       0.0,       0.0,       0.0,       0.0,       0.0,
    0.0,       0.0,       0.0,       0.0,       0.0};

static const int Z_size = 36; // Caution, this must be updated if the
                              // algorithm is changed to use a different
                              // size Z array.
static const double Z_initial[] = {
    1.0,          0.8488263632, 1.0, 1.358122181, 2.0, 3.104279270,
    0.8488263632, 0.0,          0.0, 0.0,         0.0, 0.0,
    1.0,          0.0,          0.0, 0.0,         0.0, 0.0,
    1.358122181,  0.0,          0.0, 0.0,         0.0, 0.0,
    2.0,          0.0,          0.0, 0.0,         0.0, 0.0,
    3.104279270,  0.0,          0.0, 0.0,         0.0, 0.0};

static const double LAMBDA_REF =
    1.81; ///< Wavelength that the calculations are based on
static const double COEFF4 = 1.1967;
static const double COEFF5 = -0.8667;
} // end of anonymous

MultipleScatteringCylinderAbsorption::MultipleScatteringCylinderAbsorption()
    : API::Algorithm() {}

MultipleScatteringCylinderAbsorption::~MultipleScatteringCylinderAbsorption() {}

const std::string MultipleScatteringCylinderAbsorption::name() const {
  return "MultipleScatteringCylinderAbsorption";
}

int MultipleScatteringCylinderAbsorption::version() const { return 1; }

const std::string MultipleScatteringCylinderAbsorption::category() const {
  return "CorrectionFunctions\\AbsorptionCorrections";
}

/**
 * Initialize the properties to default values
 */
void MultipleScatteringCylinderAbsorption::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace.");

  declareProperty("AttenuationXSection", 2.8, "Coefficient 1, absorption cross "
                                              "section / 1.81 if not set with "
                                              "SetSampleMaterial");
  declareProperty("ScatteringXSection", 5.1, "Coefficient 3, total scattering "
                                             "cross section if not set with "
                                             "SetSampleMaterial");
  declareProperty("SampleNumberDensity", 0.0721,
                  "Coefficient 2, density if not set with SetSampleMaterial");
  declareProperty("CylinderSampleRadius", 0.3175, "Sample radius, in cm");
}

/**
 * Execute the algorithm
 */
void MultipleScatteringCylinderAbsorption::exec() {
  // common information
  API::MatrixWorkspace_sptr in_WS = getProperty("InputWorkspace");
  double radius = getProperty("CylinderSampleRadius");
  double coeff1 = getProperty("AttenuationXSection");
  double coeff2 = getProperty("SampleNumberDensity");
  double coeff3 = getProperty("ScatteringXSection");
  const Material &sampleMaterial = in_WS->sample().getMaterial();
  if (sampleMaterial.totalScatterXSection(LAMBDA_REF) != 0.0) {
    g_log.information() << "Using material \"" << sampleMaterial.name()
                        << "\" from workspace\n";
    if (std::abs(coeff1 - 2.8) < std::numeric_limits<double>::epsilon())
      coeff1 = sampleMaterial.absorbXSection(LAMBDA_REF) / LAMBDA_REF;
    if ((std::abs(coeff2 - 0.0721) < std::numeric_limits<double>::epsilon()) &&
        (!isEmpty(sampleMaterial.numberDensity())))
      coeff2 = sampleMaterial.numberDensity();
    if (std::abs(coeff3 - 5.1) < std::numeric_limits<double>::epsilon())
      coeff3 = sampleMaterial.totalScatterXSection(LAMBDA_REF);
  } else // Save input in Sample with wrong atomic number and name
  {
    NeutronAtom neutron(static_cast<uint16_t>(EMPTY_DBL()),
                        static_cast<uint16_t>(0), 0.0, 0.0, coeff3, 0.0, coeff3,
                        coeff1);
    Object shape = in_WS->sample().getShape(); // copy
    shape.setMaterial(Material("SetInMultipleScattering", neutron, coeff2));
    in_WS->mutableSample().setShape(shape);
  }
  g_log.debug() << "radius=" << radius << " coeff1=" << coeff1
                << " coeff2=" << coeff2 << " coeff3=" << coeff3 << "\n";

  // geometry stuff
  const int64_t NUM_HIST = static_cast<int64_t>(in_WS->getNumberHistograms());
  Instrument_const_sptr instrument = in_WS->getInstrument();
  if (instrument == NULL)
    throw std::runtime_error(
        "Failed to find instrument attached to InputWorkspace");
  IComponent_const_sptr source = instrument->getSource();
  IComponent_const_sptr sample = instrument->getSample();
  if (source == NULL)
    throw std::runtime_error(
        "Failed to find source in the instrument for InputWorkspace");
  if (sample == NULL)
    throw std::runtime_error(
        "Failed to find sample in the instrument for InputWorkspace");

  // Initialize progress reporting.
  Progress prog(this, 0.0, 1.0, NUM_HIST);

  EventWorkspace_sptr in_WSevent =
      boost::dynamic_pointer_cast<EventWorkspace>(in_WS);
  if (in_WSevent) {
    MatrixWorkspace_sptr out_WS = getProperty("OutputWorkspace");
    EventWorkspace_sptr out_WSevent =
        boost::dynamic_pointer_cast<EventWorkspace>(out_WS);

    // not in-place so create a new copy
    if (in_WSevent != out_WSevent) {
      out_WSevent = boost::dynamic_pointer_cast<EventWorkspace>(
          API::WorkspaceFactory::Instance().create(
              "EventWorkspace", in_WSevent->getNumberHistograms(), 2, 1));
      // Copy geometry over.
      API::WorkspaceFactory::Instance().initializeFromParent(
          in_WSevent, out_WSevent, false);
      // You need to copy over the data as well.
      out_WSevent->copyDataFrom((*in_WSevent));
    }

    // convert to weighted events
    out_WSevent->switchEventType(API::WEIGHTED_NOTIME);

    // now do the correction
    PARALLEL_FOR1(out_WSevent)
    for (int64_t index = 0; index < NUM_HIST; ++index) {
      PARALLEL_START_INTERUPT_REGION
      IDetector_const_sptr det = out_WSevent->getDetector(index);
      if (det == NULL)
        throw std::runtime_error("Failed to find detector");
      if (det->isMasked())
        continue;
      const double tth_rad = out_WSevent->detectorTwoTheta(det);

      EventList &eventList = out_WSevent->getEventList(index);
      vector<double> tof_vec, y_vec, err_vec;
      eventList.getTofs(tof_vec);
      eventList.getWeights(y_vec);
      eventList.getWeightErrors(err_vec);

      apply_msa_correction(tth_rad, radius, coeff1, coeff2, coeff3, tof_vec,
                           y_vec, err_vec);

      std::vector<WeightedEventNoTime> &events =
          eventList.getWeightedEventsNoTime();
      for (size_t i = 0; i < events.size(); ++i) {
        events[i] = WeightedEventNoTime(tof_vec[i], y_vec[i], err_vec[i]);
      }
      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // set the output workspace
    this->setProperty(
        "OutputWorkspace",
        boost::dynamic_pointer_cast<MatrixWorkspace>(out_WSevent));
  } else // histogram case
  {
    // Create the new workspace
    MatrixWorkspace_sptr out_WS = WorkspaceFactory::Instance().create(
        in_WS, NUM_HIST, in_WS->readX(0).size(), in_WS->readY(0).size());

    for (int64_t index = 0; index < NUM_HIST; ++index) {
      IDetector_const_sptr det = in_WS->getDetector(index);
      if (det == NULL)
        throw std::runtime_error("Failed to find detector");
      if (det->isMasked())
        continue;
      const double tth_rad = in_WS->detectorTwoTheta(det);

      MantidVec tof_vec = in_WS->readX(index);
      MantidVec y_vec = in_WS->readY(index);
      MantidVec err_vec = in_WS->readE(index);

      apply_msa_correction(tth_rad, radius, coeff1, coeff2, coeff3, tof_vec,
                           y_vec, err_vec);

      out_WS->dataX(index).assign(tof_vec.begin(), tof_vec.end());
      out_WS->dataY(index).assign(y_vec.begin(), y_vec.end());
      out_WS->dataE(index).assign(err_vec.begin(), err_vec.end());
      prog.report();
    }
    setProperty("OutputWorkspace", out_WS);
  }
}

namespace { // anonymous namespace
            /**
             * Set up the Z table for the specified two theta angle (in degrees).
             */
vector<double> createZ(const double angle_rad) {
  vector<double> Z(Z_initial, Z_initial + Z_size);

  const double theta_rad = angle_rad * .5;
  int l, J;
  double sum;

  for (int i = 1; i <= 4; i++) {
    for (int j = 1; j <= 4; j++) {
      int iplusj = i + j;
      if (iplusj <= 5) {
        l = 0;
        J = 1 + l + 6 * (i - 1) + 6 * 4 * (j - 1);
        sum = C[J - 1];

        for (l = 1; l <= 5; l++) {
          J = 1 + l + 6 * (i - 1) + 6 * 4 * (j - 1);
          sum = sum + C[J - 1] * cos(l * theta_rad);
        }
        J = 1 + i + 6 * j;
        Z[J - 1] = sum;
      }
    }
  }
  return Z;
}

/**
 * Evaluate the AttFac function for a given sigir and sigsr.
 */
double AttFac(const double sigir, const double sigsr, const vector<double> &Z) {
  double facti = 1.0;
  double att = 0.0;

  for (size_t i = 0; i <= 5; i++) {
    double facts = 1.0;
    for (size_t j = 0; j <= 5; j++) {
      if (i + j <= 5) {
        size_t J = 1 + i + 6 * j; // TODO J defined in terms of j?
        att = att + Z[J - 1] * facts * facti;
        facts = -facts * sigsr / static_cast<double>(j + 1);
      }
    }
    facti = -facti * sigir / static_cast<double>(i + 1);
  }
  return att;
}

double calculate_msa_factor(const double radius, const double Q2,
                            const double sigsct, const vector<double> &Z,
                            const double wavelength) {

  const double sigabs = Q2 * wavelength;
  const double sigir = (sigabs + sigsct) * radius;
  const double sigsr = sigir;

  const double delta = COEFF4 * sigir + COEFF5 * sigir * sigir;
  const double deltp = (delta * sigsct) / (sigsct + sigabs);

  double temp = AttFac(sigir, sigsr, Z);
  return ((1.0 - deltp) / temp);
}
} // namespace

/**
 *  This method will change the values in the y_val array to correct for
 *  multiple scattering absorption. Parameter total_path is in meters, and
 *  the sample radius is in cm.
 *
 *  @param angle_deg ::   The scattering angle (two theta) in degrees
 *  @param radius ::      The sample rod radius in cm
 *  @param coeff1 ::      The absorption cross section / 1.81
 *  @param coeff2 ::      The density
 *  @param coeff3 ::      The total scattering cross section
 *  @param wavelength ::          Array of wavelengths at bin boundaries
    *                     (or bin centers) for the spectrum, in Angstroms
 *  @param y_val ::       The spectrum values
 *  @param errors ::      The spectrum errors
 */
void MultipleScatteringCylinderAbsorption::apply_msa_correction(
    const double angle_deg, const double radius, const double coeff1,
    const double coeff2, const double coeff3, const vector<double> &wavelength,
    vector<double> &y_val, std::vector<double> &errors) {

  const size_t NUM_Y = y_val.size();
  bool is_histogram = false;
  if (wavelength.size() == NUM_Y + 1)
    is_histogram = true;
  else if (wavelength.size() == NUM_Y)
    is_histogram = false;
  else
    throw std::runtime_error("Data is neither historgram or density");

  // initialize Z array for this angle
  vector<double> Z = createZ(angle_deg);

  const double Q2 = coeff1 * coeff2;
  const double sigsct = coeff2 * coeff3;

  for (size_t j = 0; j < NUM_Y; j++) {
    double wl_val = wavelength[j];
    if (is_histogram) // average with next value
      wl_val = .5 * (wl_val + wavelength[j + 1]);

    const double temp = calculate_msa_factor(radius, Q2, sigsct, Z, wl_val);

    y_val[j] *= temp;
    errors[j] *= temp;
  }
}

} // namespace Algorithm
} // namespace Mantid
