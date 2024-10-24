// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"
#include <algorithm>
#include <gmock/gmock.h>

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"
#include "MantidSINQ/PoldiUtilities/PoldiHeliumDetector.h"

#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using ::testing::Return;

namespace Mantid {
namespace Poldi {

using namespace Geometry;

using DoublePair = std::pair<double, double>;

class MockDetector : public PoldiAbstractDetector {
protected:
  std::vector<int> m_availableElements;

public:
  MockDetector() : PoldiAbstractDetector() {
    m_availableElements.resize(400);
    for (int i = 0; i < static_cast<int>(m_availableElements.size()); ++i) {
      m_availableElements[i] = i;
    }
  }

  ~MockDetector() override = default;

  void loadConfiguration(Instrument_const_sptr poldiInstrument) override { UNUSED_ARG(poldiInstrument); }
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD0(efficiency, double());
  MOCK_METHOD1(twoTheta, double(int elementIndex));
  MOCK_METHOD1(distanceFromSample, double(int elementIndex));
  MOCK_METHOD0(elementCount, size_t());
  MOCK_METHOD0(centralElement, size_t());
  MOCK_METHOD2(qLimits, DoublePair(double lambdaMin, double lambdaMax));
  GNU_DIAG_ON_SUGGEST_OVERRIDE

  const std::vector<int> &availableElements() override { return m_availableElements; }
};

class ConfiguredHeliumDetector : public PoldiHeliumDetector {
public:
  ConfiguredHeliumDetector() : PoldiHeliumDetector() { loadConfiguration(Instrument_const_sptr()); }

  void loadConfiguration(Instrument_const_sptr poldiInstrument) override {
    UNUSED_ARG(poldiInstrument);

    initializeFixedParameters(3000.0, static_cast<size_t>(400), 2.5, 0.88);
    initializeCalibratedParameters(Kernel::V2D(-931.47, -860.0), Conversions::degToRad(90.41));
  }
};

class MockChopper : public PoldiAbstractChopper {
protected:
  std::vector<double> m_slitPositions;
  std::vector<double> m_slitTimes;

public:
  MockChopper() : PoldiAbstractChopper() {
    double slits[] = {0.000000, 0.162156, 0.250867, 0.3704, 0.439811, 0.588455, 0.761389, 0.895667};
    m_slitPositions = std::vector<double>(slits, slits + sizeof(slits) / sizeof(slits[0]));

    double times[] = {0.000000, 243.234, 376.3, 555.6, 659.716, 882.682, 1142.08, 1343.5};
    m_slitTimes = std::vector<double>(times, times + sizeof(times) / sizeof(times[0]));
  }

  ~MockChopper() override = default;

  void loadConfiguration(Instrument_const_sptr poldiInstrument) override{
      UNUSED_ARG(poldiInstrument)} GNU_DIAG_OFF_SUGGEST_OVERRIDE MOCK_METHOD0(rotationSpeed, double());
  MOCK_METHOD0(cycleTime, double());
  MOCK_METHOD0(zeroOffset, double());
  MOCK_METHOD0(distanceFromSample, double());

  MOCK_METHOD1(setRotationSpeed, void(double rotationSpeed));
  GNU_DIAG_ON_SUGGEST_OVERRIDE

  const std::vector<double> &slitPositions() override { return m_slitPositions; }
  const std::vector<double> &slitTimes() override { return m_slitTimes; }
};
class PoldiFakeSourceComponent : public ObjComponent {
public:
  PoldiFakeSourceComponent() : ObjComponent("FakePoldiSource", nullptr) {}
};

class PoldiAbstractFakeInstrument : public Instrument {
public:
  PoldiAbstractFakeInstrument() {}

  std::shared_ptr<const IComponent> getComponentByName(const std::string &cname, int nlevels = 0) const override {
    UNUSED_ARG(cname);
    UNUSED_ARG(nlevels);

    return getComponentByNameFake();
  }

  virtual std::shared_ptr<const IComponent> getComponentByNameFake() const = 0;
};

class PoldiValidSourceFakeInstrument : public PoldiAbstractFakeInstrument {
public:
  PoldiValidSourceFakeInstrument() : PoldiAbstractFakeInstrument() {}

  std::shared_ptr<const IComponent> getComponentByNameFake() const override {
    return std::make_shared<PoldiFakeSourceComponent>();
  }
};

class PoldiInvalidSourceFakeInstrument : public PoldiAbstractFakeInstrument {
public:
  PoldiInvalidSourceFakeInstrument() : PoldiAbstractFakeInstrument() {}

  std::shared_ptr<const IComponent> getComponentByNameFake() const override {
    return std::shared_ptr<const IComponent>();
  }
};

class PoldiValidFakeParameterMap : public ParameterMap {
public:
  PoldiValidFakeParameterMap(const IComponent *component) : ParameterMap() {
    add("fitting", component, "WavelengthDistribution", FitParameter());
  }
};

class PoldiInvalidFakeParameterMap : public ParameterMap {
public:
  PoldiInvalidFakeParameterMap() : ParameterMap() {}
};

class ConfiguredSpectrum : public PoldiSourceSpectrum {
public:
  ConfiguredSpectrum() : PoldiSourceSpectrum(Kernel::Interpolation()) {
    m_spectrum.addPoint(0.986430049, 0.0379200461685);
    m_spectrum.addPoint(1.01097, 0.0647900877564);
    m_spectrum.addPoint(1.03552008, 0.158472779233);
    m_spectrum.addPoint(1.06007004, 0.519912166374);
    m_spectrum.addPoint(1.08460999, 1.61236820184);
    m_spectrum.addPoint(1.10916007, 2.92556206166);
    m_spectrum.addPoint(1.13371003, 3.01527151822);
    m_spectrum.addPoint(1.15825999, 3.08617045167);
    m_spectrum.addPoint(1.18280005, 3.3797870393);
    m_spectrum.addPoint(1.20735002, 3.54669731732);
    m_spectrum.addPoint(1.23189998, 3.6408488786);
    m_spectrum.addPoint(1.25644004, 3.61670774172);
    m_spectrum.addPoint(1.28099, 3.74104456034);
    m_spectrum.addPoint(1.30553997, 3.85943944836);
    m_spectrum.addPoint(1.33009005, 3.75758537926);
    m_spectrum.addPoint(1.35462999, 3.72106056606);
    m_spectrum.addPoint(1.37918007, 3.73501609979);
    m_spectrum.addPoint(1.40373003, 3.80737158413);
    m_spectrum.addPoint(1.42826998, 3.74757696126);
    m_spectrum.addPoint(1.45282006, 3.79171629046);
    m_spectrum.addPoint(1.47737002, 3.70519039935);
    m_spectrum.addPoint(1.50190997, 3.64835854136);
    m_spectrum.addPoint(1.52646005, 3.58509014427);
    m_spectrum.addPoint(1.55101001, 3.52528432718);
    m_spectrum.addPoint(1.57554996, 3.58574587753);
    m_spectrum.addPoint(1.60010004, 3.52296612136);
    m_spectrum.addPoint(1.62465, 3.48146692706);
    m_spectrum.addPoint(1.64919996, 3.29247486904);
    m_spectrum.addPoint(1.67374003, 3.18613505977);
    m_spectrum.addPoint(1.69828999, 3.11493402243);
    m_spectrum.addPoint(1.72284007, 3.019974154);
    m_spectrum.addPoint(1.74738002, 2.95223419475);
    m_spectrum.addPoint(1.77192998, 2.87013941439);
    m_spectrum.addPoint(1.79648006, 2.8518896839);
    m_spectrum.addPoint(1.82102001, 2.81334596521);
    m_spectrum.addPoint(1.84556997, 2.79063867035);
    m_spectrum.addPoint(1.87012005, 2.73278925933);
    m_spectrum.addPoint(1.89467001, 2.70067519099);
    m_spectrum.addPoint(1.91921008, 2.6032279618);
    m_spectrum.addPoint(1.94376004, 2.49786056822);
    m_spectrum.addPoint(1.96831, 2.3271600338);
    m_spectrum.addPoint(1.99285007, 2.34552390934);
    m_spectrum.addPoint(2.01740003, 2.22035491325);
    m_spectrum.addPoint(2.04194999, 2.29464387452);
    m_spectrum.addPoint(2.06648993, 2.20015866926);
    m_spectrum.addPoint(2.09104013, 2.1126010471);
    m_spectrum.addPoint(2.1155901, 2.03543605117);
    m_spectrum.addPoint(2.14014006, 2.03696461356);
    m_spectrum.addPoint(2.16468, 2.02314915783);
    m_spectrum.addPoint(2.18922997, 1.92865797498);
    m_spectrum.addPoint(2.21377993, 1.76339003593);
    m_spectrum.addPoint(2.23832011, 1.70948140257);
    m_spectrum.addPoint(2.26287007, 1.70655034399);
    m_spectrum.addPoint(2.28742003, 1.63185257524);
    m_spectrum.addPoint(2.31195998, 1.6101119067);
    m_spectrum.addPoint(2.33650994, 1.52402877822);
    m_spectrum.addPoint(2.3610599, 1.41765223428);
    m_spectrum.addPoint(2.3856101, 1.41372656581);
    m_spectrum.addPoint(2.41015005, 1.37861914774);
    m_spectrum.addPoint(2.43470001, 1.39547907016);
    m_spectrum.addPoint(2.45924997, 1.32522954793);
    m_spectrum.addPoint(2.48378992, 1.3747536237);
    m_spectrum.addPoint(2.50834012, 1.26342285426);
    m_spectrum.addPoint(2.53289008, 1.1961405706);
    m_spectrum.addPoint(2.55743003, 1.15607902115);
    m_spectrum.addPoint(2.58197999, 1.09374471504);
    m_spectrum.addPoint(2.60652995, 1.08882219384);
    m_spectrum.addPoint(2.63107991, 1.08884830502);
    m_spectrum.addPoint(2.6556201, 1.05670290487);
    m_spectrum.addPoint(2.68017006, 1.02406197103);
    m_spectrum.addPoint(2.70472002, 0.989440668677);
    m_spectrum.addPoint(2.72925997, 1.00035277903);
    m_spectrum.addPoint(2.75380993, 0.95038734642);
    m_spectrum.addPoint(2.77836013, 0.869240260323);
    m_spectrum.addPoint(2.80291009, 0.836101367713);
    m_spectrum.addPoint(2.82745004, 0.832658084227);
    m_spectrum.addPoint(2.852, 0.784670032334);
    m_spectrum.addPoint(2.87654996, 0.789144670394);
    m_spectrum.addPoint(2.90108991, 0.725317653441);
    m_spectrum.addPoint(2.92564011, 0.737710213048);
    m_spectrum.addPoint(2.95019007, 0.742183265093);
    m_spectrum.addPoint(2.97473001, 0.692213030379);
    m_spectrum.addPoint(2.99927998, 0.653130845445);
    m_spectrum.addPoint(3.02382994, 0.658097474694);
    m_spectrum.addPoint(3.04837012, 0.629903015127);
    m_spectrum.addPoint(3.07292008, 0.620515648311);
    m_spectrum.addPoint(3.09747005, 0.59182542276);
    m_spectrum.addPoint(3.12202001, 0.596295491013);
    m_spectrum.addPoint(3.14655995, 0.553251221336);
    m_spectrum.addPoint(3.17110991, 0.561185043087);
    m_spectrum.addPoint(3.19566011, 0.509726198752);
    m_spectrum.addPoint(3.22020006, 0.497861766253);
    m_spectrum.addPoint(3.24475002, 0.500350162334);
    m_spectrum.addPoint(3.26929998, 0.462253499734);
    m_spectrum.addPoint(3.29383993, 0.448903255167);
    m_spectrum.addPoint(3.31839013, 0.442976704117);
    m_spectrum.addPoint(3.34293699, 0.410906557596);
    m_spectrum.addPoint(3.36748409, 0.400728656223);
    m_spectrum.addPoint(3.39203095, 0.390802883239);
    m_spectrum.addPoint(3.41657805, 0.381123104847);
    m_spectrum.addPoint(3.44112492, 0.371682888874);
    m_spectrum.addPoint(3.46567202, 0.362476568119);
    m_spectrum.addPoint(3.49021912, 0.35349822306);
    m_spectrum.addPoint(3.51476598, 0.34474241879);
    m_spectrum.addPoint(3.53931308, 0.336203424027);
    m_spectrum.addPoint(3.56385994, 0.327875851925);
    m_spectrum.addPoint(3.58840704, 0.319754644057);
    m_spectrum.addPoint(3.61295295, 0.311834601816);
    m_spectrum.addPoint(3.637501, 0.304110594682);
    m_spectrum.addPoint(3.66204691, 0.296578008794);
    m_spectrum.addPoint(3.68659401, 0.289232018017);
    m_spectrum.addPoint(3.71114111, 0.282067934396);
    m_spectrum.addPoint(3.73568797, 0.275081354334);
    m_spectrum.addPoint(3.76023507, 0.268267710027);
    m_spectrum.addPoint(3.78478193, 0.261622986372);
    m_spectrum.addPoint(3.80932903, 0.255142781777);
    m_spectrum.addPoint(3.83387613, 0.248823039086);
    m_spectrum.addPoint(3.85842299, 0.242659933438);
    m_spectrum.addPoint(3.88297009, 0.236649455739);
    m_spectrum.addPoint(3.90751696, 0.230787759101);
    m_spectrum.addPoint(3.93206406, 0.225071303024);
    m_spectrum.addPoint(3.95661092, 0.219496468911);
    m_spectrum.addPoint(3.98115826, 0.214059716263);
    m_spectrum.addPoint(4.0057044, 0.208757618724);
    m_spectrum.addPoint(4.03025055, 0.203586864087);
    m_spectrum.addPoint(4.0547986, 0.198544166977);
    m_spectrum.addPoint(4.0793457, 0.193626378592);
    m_spectrum.addPoint(4.10389137, 0.188830387379);
    m_spectrum.addPoint(4.12843847, 0.184153209345);
    m_spectrum.addPoint(4.15298653, 0.179591790409);
    m_spectrum.addPoint(4.17753267, 0.17514353127);
    m_spectrum.addPoint(4.20207977, 0.170805354615);
    m_spectrum.addPoint(4.2266264, 0.166574544595);
    m_spectrum.addPoint(4.2511735, 0.162448630069);
    m_spectrum.addPoint(4.2757206, 0.158424976688);
    m_spectrum.addPoint(4.3002677, 0.154500833359);
    m_spectrum.addPoint(4.32481432, 0.150673962236);
    m_spectrum.addPoint(4.34936142, 0.146941962269);
    m_spectrum.addPoint(4.37390852, 0.143302256984);
    m_spectrum.addPoint(4.39845562, 0.139752771944);
    m_spectrum.addPoint(4.42300272, 0.136291257291);
    m_spectrum.addPoint(4.44754934, 0.132915370247);
    m_spectrum.addPoint(4.47209644, 0.129623152325);
    m_spectrum.addPoint(4.49664354, 0.126412482228);
    m_spectrum.addPoint(4.52119064, 0.123281343393);
    m_spectrum.addPoint(4.54573774, 0.120227765918);
    m_spectrum.addPoint(4.57028341, 0.11724980313);
    m_spectrum.addPoint(4.59483051, 0.114345613286);
    m_spectrum.addPoint(4.61937857, 0.111513366463);
    m_spectrum.addPoint(4.64392471, 0.108751267379);
    m_spectrum.addPoint(4.66847134, 0.106057579427);
    m_spectrum.addPoint(4.69301844, 0.103430600844);
    m_spectrum.addPoint(4.71756554, 0.100868711172);
    m_spectrum.addPoint(4.74211264, 0.0983702671227);
    m_spectrum.addPoint(4.76665974, 0.0959336772731);
    m_spectrum.addPoint(4.79120636, 0.093557508202);
    m_spectrum.addPoint(4.81575346, 0.0912401628801);
    m_spectrum.addPoint(4.84030056, 0.0889801784484);
    m_spectrum.addPoint(4.86484766, 0.0867762492478);
    m_spectrum.addPoint(4.88939476, 0.084626860153);
    m_spectrum.addPoint(4.91394138, 0.0825306822753);
    m_spectrum.addPoint(4.93848753, 0.0804864976671);
    m_spectrum.addPoint(4.96303558, 0.0784929071503);
    m_spectrum.addPoint(4.98758268, 0.0765486581333);
    m_spectrum.addPoint(5.01212931, 0.0746526079641);
    m_spectrum.addPoint(5.03667545, 0.0728035212728);
    m_spectrum.addPoint(5.06122255, 0.0710002323781);
    m_spectrum.addPoint(5.08577061, 0.0692416164508);
    m_spectrum.addPoint(5.11031771, 0.0675265194243);
    m_spectrum.addPoint(5.13486338, 0.0658539680618);
    m_spectrum.addPoint(5.15941048, 0.0642228143046);
    m_spectrum.addPoint(5.18395853, 0.0626320322489);
    m_spectrum.addPoint(5.20850468, 0.0610807125393);
    m_spectrum.addPoint(5.23305178, 0.0595677948282);
    m_spectrum.addPoint(5.2575984, 0.0580923114862);
    m_spectrum.addPoint(5.2821455, 0.0566534406688);
    m_spectrum.addPoint(5.3066926, 0.0552501801025);
    m_spectrum.addPoint(5.3312397, 0.0538816494685);
    m_spectrum.addPoint(5.35578632, 0.0525470679748);
    m_spectrum.addPoint(5.38033342, 0.0512454914214);
    m_spectrum.addPoint(5.40488052, 0.049976179668);
    m_spectrum.addPoint(5.42942762, 0.0487383282922);
    m_spectrum.addPoint(5.45397472, 0.0475310954243);
    m_spectrum.addPoint(5.47852135, 0.0463537841787);
    m_spectrum.addPoint(5.50306749, 0.0452056576189);
    m_spectrum.addPoint(5.52761555, 0.044085928745);
    m_spectrum.addPoint(5.55216265, 0.0429939565424);
    m_spectrum.addPoint(5.60125542, 0.0408904789888);
    m_spectrum.addPoint(5.62580252, 0.0398776505566);
    m_spectrum.addPoint(5.65034962, 0.0388899122071);
    m_spectrum.addPoint(5.69944334, 0.0369872268062);
    m_spectrum.addPoint(5.72399044, 0.0360710788288);
    m_spectrum.addPoint(5.77308464, 0.0343063026735);
    m_spectrum.addPoint(5.79763174, 0.0334565608805);
    m_spectrum.addPoint(5.82217836, 0.0326278676345);
    m_spectrum.addPoint(5.87127256, 0.0310315517468);
    m_spectrum.addPoint(5.89581966, 0.0302629090089);
    m_spectrum.addPoint(5.94491243, 0.0287823115682);
    m_spectrum.addPoint(5.96946049, 0.0280693832282);
    m_spectrum.addPoint(5.99400759, 0.0273741231399);
  }
};

class FakePoldiInstrumentAdapter : public PoldiInstrumentAdapter {
public:
  FakePoldiInstrumentAdapter() : PoldiInstrumentAdapter() {
    MockChopper *chopper = new MockChopper;
    m_chopper = PoldiAbstractChopper_sptr(chopper);
    m_detector = PoldiAbstractDetector_sptr(new ConfiguredHeliumDetector);
    m_spectrum = PoldiSourceSpectrum_sptr(new ConfiguredSpectrum);

    EXPECT_CALL(*chopper, distanceFromSample()).WillRepeatedly(Return(11800.0));

    EXPECT_CALL(*chopper, zeroOffset()).WillRepeatedly(Return(0.15));
  }
};

class PoldiPeakCollectionHelpers {
  /* This class contains some static helper function to create
   * peak collections and related components for testing purposes.
   */
public:
  /**
   * This function creates a TableWorkspace which can be used by
   *PoldiPeakCollection
   *
   * A TableWorkspace with four peaks is generated. The data comes from a run of
   *the
   * original analysis software on poldi2013n006904 (available in system tests
   *and
   * usage tests). Only the first four peaks are considered and no errors are
   *provided.
   *
   * @return TableWorkspace in the format PoldiPeakCollection requires
   */
  static DataObjects::TableWorkspace_sptr createPoldiPeakTableWorkspace() {
    DataObjects::TableWorkspace_sptr tableWs =
        std::dynamic_pointer_cast<DataObjects::TableWorkspace>(API::WorkspaceFactory::Instance().createTable());
    tableWs->addColumn("str", "HKL");
    tableWs->addColumn("double", "d");
    tableWs->addColumn("double", "delta d");
    tableWs->addColumn("double", "Q");
    tableWs->addColumn("double", "delta Q");
    tableWs->addColumn("double", "Intensity");
    tableWs->addColumn("double", "delta Intensity");
    tableWs->addColumn("double", "FWHM (rel.)");
    tableWs->addColumn("double", "delta FWHM (rel.)");

    tableWs->logs()->addProperty<std::string>("IntensityType", "Maximum");
    tableWs->logs()->addProperty<std::string>("ProfileFunctionName", "Gaussian");

    API::TableRow newRow = tableWs->appendRow();
    newRow << "0 0 0" << 1.108644 << 0.0 << 5.667449 << 0.0 << 3286.152 << 0.0 << 0.002475747 << 0.0;

    newRow = tableWs->appendRow();
    newRow << "0 0 0" << 1.637539 << 0.0 << 3.836968 << 0.0 << 2951.696 << 0.0 << 0.002516417 << 0.0;

    newRow = tableWs->appendRow();
    newRow << "0 0 0" << 1.920200 << 0.0 << 3.272152 << 0.0 << 3238.473 << 0.0 << 0.002444439 << 0.0;

    newRow = tableWs->appendRow();
    newRow << "0 0 0" << 1.245958 << 0.0 << 5.042856 << 0.0 << 2219.592 << 0.0 << 0.002696334 << 0.0;

    return tableWs;
  }

  /**
   * This function creates a PoldiPeakCollection
   *
   * A PoldiPeakCollection is created with the same information as in
   *createPoldiPeakTableWorkspace().
   *
   * @return PoldiPeakCollection with four example peaks.
   */
  static PoldiPeakCollection_sptr createPoldiPeakCollectionMaximum() {
    PoldiPeakCollection_sptr peaks(new PoldiPeakCollection);
    peaks->addPeak(PoldiPeak::create(MillerIndices(0, 0, 0), UncertainValue(1.108644), UncertainValue(3286.152),
                                     UncertainValue(0.002475747)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(0, 0, 0), UncertainValue(1.637539), UncertainValue(2951.696),
                                     UncertainValue(0.002516417)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(0, 0, 0), UncertainValue(1.920200), UncertainValue(3238.473),
                                     UncertainValue(0.002444439)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(0, 0, 0), UncertainValue(1.245958), UncertainValue(2219.592),
                                     UncertainValue(0.002696334)));

    peaks->setProfileFunctionName("Gaussian");

    return peaks;
  }

  /**
   * This function creates a PoldiPeakCollection with integrated intensities
   *
   * The same peaks as above, with integrated intensities in "channel units".
   *
   * @return PoldiPeakCollection with four example peaks.
   */
  static PoldiPeakCollection_sptr createPoldiPeakCollectionIntegral() {
    PoldiPeakCollection_sptr peaks(new PoldiPeakCollection(PoldiPeakCollection::Integral));
    peaks->addPeak(PoldiPeak::create(MillerIndices(0, 0, 0), UncertainValue(1.108644), UncertainValue(15835.28906),
                                     UncertainValue(0.002475747)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(0, 0, 0), UncertainValue(1.637539), UncertainValue(21354.32226),
                                     UncertainValue(0.002516417)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(0, 0, 0), UncertainValue(1.920200), UncertainValue(26687.36132),
                                     UncertainValue(0.002444439)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(0, 0, 0), UncertainValue(1.245958), UncertainValue(13091.51855),
                                     UncertainValue(0.002696334)));

    peaks->setProfileFunctionName("Gaussian");

    return peaks;
  }

  /**
   * This function creates a PoldiPeakCollection with normalized intensities
   *
   * Again, the same peaks as above, but with normalized intensities (for 8
   *chopper slits)
   *
   * @return PoldiPeakCollection with four example peaks.
   */
  static PoldiPeakCollection_sptr createPoldiPeakCollectionNormalized() {
    PoldiPeakCollection_sptr peaks(new PoldiPeakCollection(PoldiPeakCollection::Integral));
    peaks->addPeak(PoldiPeak::create(MillerIndices(4, 2, 2), UncertainValue(1.108644), UncertainValue(1.926395655),
                                     UncertainValue(0.002475747)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(3, 1, 1), UncertainValue(1.637539), UncertainValue(4.773980141),
                                     UncertainValue(0.002516417)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(2, 2, 0), UncertainValue(1.920200), UncertainValue(9.370919228),
                                     UncertainValue(0.002444439)));
    peaks->addPeak(PoldiPeak::create(MillerIndices(3, 3, 1), UncertainValue(1.245958), UncertainValue(1.758037806),
                                     UncertainValue(0.002696334)));

    peaks->setProfileFunctionName("Gaussian");

    return peaks;
  }

  static PoldiPeakCollection_sptr createTheoreticalPeakCollectionSilicon() {
    BraggScatterer_sptr atomSi = BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer", R"({"Element":"Si","Position":"0,0,0","U":"0.005"})");
    CompositeBraggScatterer_sptr atoms = CompositeBraggScatterer::create();
    atoms->addScatterer(atomSi);

    CrystalStructure Si(UnitCell(5.43071, 5.43071, 5.43071), SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m"),
                        atoms);

    return PoldiPeakCollection_sptr(new PoldiPeakCollection(Si, 1.1, 1.95));
  }
};
} // namespace Poldi
} // namespace Mantid
