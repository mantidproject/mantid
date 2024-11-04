// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/PropertyWithValue.h"
#include <Poco/Path.h>
#include <vector>

/**
 * Test HFIR SANS Spice loader
 * TODO: check that an exception is thrown when the geometry file doesn't define
 * all monitors
 */
class LoadSpice2dTest : public CxxTest::TestSuite {
public:
  static LoadSpice2dTest *createSuite() { return new LoadSpice2dTest(); }
  static void destroySuite(LoadSpice2dTest *suite) { delete suite; }

  LoadSpice2dTest() { inputFile = "BioSANS_exp61_scan0004_0001.xml"; }

  void testConfidence() {
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);

    Mantid::Kernel::FileDescriptor descriptor(loader.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(80, loader.confidence(descriptor));
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(spice2d.initialize());
    TS_ASSERT(spice2d.isInitialized());
  }

  void testExec() {
    if (!spice2d.isInitialized())
      spice2d.initialize();

    // No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(spice2d.execute(), const std::runtime_error &);

    // Set the file name
    spice2d.setPropertyValue("Filename", inputFile);

    std::string outputSpace = "outws";
    // Set an output workspace
    spice2d.setPropertyValue("OutputWorkspace", outputSpace);

    // check that retrieving the filename gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = spice2d.getPropertyValue("OutputWorkspace"))
    TS_ASSERT(result == outputSpace);

    // Should now throw nothing
    TS_ASSERT_THROWS_NOTHING(spice2d.execute());
    TS_ASSERT(spice2d.isExecuted());

    // Now need to test the resultant workspace, first retrieve it
    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace));
    Mantid::DataObjects::Workspace2D_sptr ws2d = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);

    // We have 192*192 + 2 channels, for the PSD + timer + monitor
    TS_ASSERT_EQUALS(ws2d->getNumberHistograms(), 36864 + Mantid::DataHandling::LoadSpice2D::nMonitors);

    // Test the size of the data vectors
    TS_ASSERT_EQUALS((ws2d->x(0).size()), 2);
    TS_ASSERT_EQUALS((ws2d->y(0).size()), 1);
    TS_ASSERT_EQUALS((ws2d->e(0).size()), 1);

    double tolerance(1e-04);
    int nmon = Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA(ws2d->x(0 + nmon)[0], 5.93, tolerance);
    TS_ASSERT_DELTA(ws2d->x(2 + nmon)[0], 5.93, tolerance);
    TS_ASSERT_DELTA(ws2d->x(192 + nmon)[0], 5.93, tolerance);

    TS_ASSERT_DELTA(ws2d->y(0 + nmon)[0], 318.0, tolerance);
    TS_ASSERT_DELTA(ws2d->y(2 + nmon)[0], 109.0, tolerance);
    TS_ASSERT_DELTA(ws2d->y(192 + nmon)[0], 390.0, tolerance);

    TS_ASSERT_DELTA(ws2d->e(0 + nmon)[0], 17.8325, tolerance);
    TS_ASSERT_DELTA(ws2d->e(2 + nmon)[0], 10.4403, tolerance);
    TS_ASSERT_DELTA(ws2d->e(192 + nmon)[0], 19.7484, tolerance);

    // check monitor
    TS_ASSERT_DELTA(ws2d->y(0)[0], 29205906.0, tolerance);
    TS_ASSERT_DELTA(ws2d->e(0)[0], 5404.2488, tolerance);

    // check timer
    TS_ASSERT_DELTA(ws2d->y(1)[0], 3600.0, tolerance);
    TS_ASSERT_DELTA(ws2d->e(1)[0], 0.0, tolerance);

    // Check instrument
    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check Child Algorithm is running
    // properly
    //----------------------------------------------------------------------
    Mantid::Geometry::Instrument_const_sptr i = ws2d->getInstrument();
    std::shared_ptr<const Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS(i->getName(), "GPSANS");
    TS_ASSERT_EQUALS(source->getName(), "source");

    // Check parameters for sample aperture
    Mantid::Geometry::IComponent_const_sptr sample_aperture = i->getComponentByName("sample_aperture");
    TS_ASSERT_EQUALS(sample_aperture->getNumberParameter("Size")[0], 14.0);

    // Check parameter map access
    const auto *m_paraMap = &(ws2d->constInstrumentParameters());

    // Check that we can get a parameter
    std::shared_ptr<Mantid::Geometry::Parameter> sample_aperture_size = m_paraMap->get(sample_aperture.get(), "Size");
    TS_ASSERT_EQUALS(sample_aperture_size->type(), "double");
    TS_ASSERT_EQUALS(sample_aperture_size->value<double>(), 14.0);

    // Check that we can modify a parameter
    Mantid::Geometry::ParameterMap &pmap_nonconst = ws2d->instrumentParameters();
    pmap_nonconst.addDouble(sample_aperture.get(), "Size", 15.0);
    // The parameter map was copied by the non-const access, get new reference.
    m_paraMap = &(ws2d->constInstrumentParameters());
    sample_aperture_size = m_paraMap->get(sample_aperture.get(), "Size");
    TS_ASSERT_EQUALS(sample_aperture_size->value<double>(), 15.0);

    TS_ASSERT_EQUALS(ws2d->run().getProperty("sample-detector-distance")->type(), "number");
    Mantid::Kernel::Property *prop = ws2d->run().getProperty("sample-detector-distance");
    Mantid::Kernel::PropertyWithValue<double> *dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    TS_ASSERT_EQUALS(*dp, 6000.0);

    prop = ws2d->run().getProperty("beam-trap-diameter");
    TS_ASSERT_EQUALS(prop->type(), "number");
    double beam_trap_diameter = ws2d->run().getPropertyValueAsType<double>("beam-trap-diameter");
    TS_ASSERT_DELTA(beam_trap_diameter, 76.2, tolerance);

    prop = ws2d->run().getProperty("source-aperture-diameter");
    dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    TS_ASSERT_EQUALS(*dp, 40.0);

    prop = ws2d->run().getProperty("sample-aperture-diameter");
    dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    TS_ASSERT_EQUALS(*dp, 14.0);

    prop = ws2d->run().getProperty("number-of-guides");
    Mantid::Kernel::PropertyWithValue<int> *np = dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(prop);
    TS_ASSERT_EQUALS(*np, 4);

    // Check detector position
    prop = ws2d->run().getProperty("total-sample-detector-distance");
    Mantid::Kernel::PropertyWithValue<double> *tsdd = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    TS_ASSERT_EQUALS(i->getComponentByName("detector1")->getPos().Z(), *tsdd * 1e-3);
    assertDetectorDistances(ws2d);
  }

  void testExecChooseWavelength() {
    Mantid::DataHandling::LoadSpice2D spice2d;

    if (!spice2d.isInitialized())
      spice2d.initialize();

    // No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(spice2d.execute(), const std::runtime_error &);

    // Set the file name
    spice2d.setPropertyValue("Filename", inputFile);

    std::string outputSpace = "outws";
    // Set an output workspace
    spice2d.setPropertyValue("OutputWorkspace", outputSpace);
    spice2d.setPropertyValue("Wavelength", "5.0");
    spice2d.setPropertyValue("WavelengthSpread", "1.0");

    // Should now throw nothing
    TS_ASSERT_THROWS_NOTHING(spice2d.execute());
    TS_ASSERT(spice2d.isExecuted());

    // Now need to test the resultant workspace, first retrieve it
    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace));
    Mantid::DataObjects::Workspace2D_sptr ws2d = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);

    // Test the size of the data vectors
    TS_ASSERT_EQUALS((ws2d->x(0).size()), 2);
    TS_ASSERT_EQUALS((ws2d->y(0).size()), 1);
    TS_ASSERT_EQUALS((ws2d->e(0).size()), 1);

    double tolerance(1e-04);
    int nmon = Mantid::DataHandling::LoadSpice2D::nMonitors;
    TS_ASSERT_DELTA(ws2d->x(0 + nmon)[0], 4.5, tolerance);
    TS_ASSERT_DELTA(ws2d->x(2 + nmon)[0], 4.5, tolerance);
    TS_ASSERT_DELTA(ws2d->x(192 + nmon)[0], 4.5, tolerance);
  }

  void assertDetectorDistances(const Mantid::DataObjects::Workspace2D_sptr &ws2d) {
    Mantid::Kernel::Property *prop = ws2d->run().getProperty("sample-detector-distance");
    Mantid::Kernel::PropertyWithValue<double> *sdd = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    prop = ws2d->run().getProperty("sample-detector-distance-offset");
    Mantid::Kernel::PropertyWithValue<double> *sddo = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    prop = ws2d->run().getProperty("sample-si-window-distance");
    Mantid::Kernel::PropertyWithValue<double> *siwo = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    prop = ws2d->run().getProperty("total-sample-detector-distance");
    Mantid::Kernel::PropertyWithValue<double> *tsdd = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    double total_sample_detector_distance = *tsdd;
    TS_ASSERT_EQUALS((*sdd) + (*sddo) + (*siwo), total_sample_detector_distance);
    TS_ASSERT_EQUALS(6811.4, total_sample_detector_distance);
  }

private:
  std::string inputFile;
  Mantid::DataHandling::LoadSpice2D spice2d;
};
