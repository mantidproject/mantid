#ifndef MANTID_DATAHANDLING_LOADSESANSTEST_H_
#define MANTID_DATAHANDLING_LOADSESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadSESANS.h"
#include "MantidKernel/FileDescriptor.h"

#include <Poco/File.h>

using Mantid::DataHandling::LoadSESANS;

class LoadSESANSTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSESANSTest *createSuite() { return new LoadSESANSTest(); }
  static void destroySuite(LoadSESANSTest *suite) { delete suite; }

  void test_init() {
    writeGoodFile();
    LoadSESANS testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setChild(true);
    testAlg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", infileName));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("OutputWorkspace", "ws"));
  }

  void test_exec() {
    // Setup the algorithm
    LoadSESANS testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setChild(true);
    testAlg.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", infileName));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("OutputWorkspace", "ws"));

    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlg.execute());

    Mantid::API::MatrixWorkspace_sptr ws =
        testAlg.getProperty("OutputWorkspace");
    Mantid::API::Sample sample = ws->sample();

    // Make sure output properties were set correctly
    TS_ASSERT_EQUALS(ws->getTitle(), "PMMA in Mixed Deuterated decalin");
    TS_ASSERT_EQUALS(sample.getName(), "Ostensibly 40$ 100nm radius PMMA hard "
                                       "spheres in mixed deuterarted decalin.");
    TS_ASSERT_EQUALS(sample.getThickness(), 2.0);
  }

  void test_confidence() {
    LoadSESANS testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setChild(true);
    testAlg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", infileName));
    Mantid::Kernel::FileDescriptor descriptor(
        testAlg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(testAlg.confidence(descriptor), 70);
    
    std::string outputPath = testAlg.getProperty("Filename");
    TS_ASSERT_THROWS_NOTHING(Poco::File(outputPath).remove());
    TS_ASSERT(!Poco::File(outputPath).exists());
  }

  void test_requireFFV() {
    LoadSESANS testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setChild(true);
    testAlg.setRethrows(true);
    writeFileMissingFFV();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", infileName));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("OutputWorkspace", "ws"));

    TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);

    std::string outputPath = testAlg.getProperty("Filename");
    TS_ASSERT_THROWS_NOTHING(Poco::File(outputPath).remove());
    TS_ASSERT(!Poco::File(outputPath).exists());
  }

  void test_mandatoryHeaders() {
    LoadSESANS testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setChild(true);
    testAlg.setRethrows(true);
    writeFileMissingHeaders();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", infileName));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("OutputWorkspace", "ws"));

    TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);
    
    std::string outputPath = testAlg.getProperty("Filename");
    TS_ASSERT_THROWS_NOTHING(Poco::File(outputPath).remove());
    TS_ASSERT(!Poco::File(outputPath).exists());
  }

  void test_mandatoryColumns() {
    LoadSESANS testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setChild(true);
    testAlg.setRethrows(true);
    writeFileMissingColumns();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", infileName));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("OutputWorkspace", "ws"));

    TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);
    
    std::string outputPath = testAlg.getProperty("Filename");
    TS_ASSERT_THROWS_NOTHING(Poco::File(outputPath).remove());
    TS_ASSERT(!Poco::File(outputPath).exists());
  }

private:
  std::string infileName = "temp.ses";

  void writeGoodFile() {
    std::string contents =
        "FileFormatVersion       1.0\n"
        "DataFileTitle           PMMA in Mixed Deuterated decalin\n"
        "Sample                  Ostensibly 40$ 100nm radius PMMA hard spheres "
        "in mixed deuterarted decalin.\n"
        "Thickness               2\n"
        "Thickness_unit          mm\n"
        "Theta_zmax              0.09\n"
        "Theta_zmax_unit         radians\n"
        "Theta_ymax              0.09\n"
        "Theta_ymax_unit         radians\n"
        "Orientation             Z\n"
        "SpinEchoLength_unit     A\n"
        "Depolarisation_unit     A - 2 cm - 1\n"
        "Wavelength_unit         A\n"
        "\n"
        "BEGIN_DATA\n"
        "SpinEchoLength Depolarisation Depolarisation_error Wavelength\n"
        "260.0 -1.42E-3 2.04E-3 1.612452\n"
        "280.8 -1.45E-3 1.87E-3 1.675709\n"
        "303.264 -1.64E-3 1.23E-3 1.741448\n"
        "327.525 -1.69E-3 1.809765\n"
        "353.727 -2.23E-3 9.09E-4 1.880763\n"
        "382.025 -2.26E-3 8.58E-4 1.954546\n";
    std::ofstream file(infileName);
    file << contents;
    file.close();
  }

  void writeFileMissingFFV() {
    std::string contents =
        "DataFileTitle           PMMA in Mixed Deuterated decalin\n"
        "Sample                  Ostensibly 40$ 100nm radius PMMA hard spheres "
        "in mixed deuterarted decalin.\n"
        "Thickness               2\n"
        "Thickness_unit          mm\n"
        "Theta_zmax              0.09\n"
        "Theta_zmax_unit         radians\n"
        "Theta_ymax              0.09\n"
        "Theta_ymax_unit         radians\n"
        "Orientation             Z\n"
        "SpinEchoLength_unit     A\n"
        "Depolarisation_unit     A - 2 cm - 1\n"
        "Wavelength_unit         A\n"
        "\n"
        "BEGIN_DATA\n"
        "SpinEchoLength Depolarisation Depolarisation_error Wavelength\n"
        "260.0 -1.42E-3 2.04E-3 1.612452\n"
        "382.025 -2.26E-3 8.58E-4 1.954546\n";
    std::ofstream file(infileName);
    file << contents;
    file.close();
  }

  void writeFileMissingHeaders() {
    std::string contents =
        "FileFormatVersion       1.0\n"
        "DataFileTitle           PMMA in Mixed Deuterated decalin\n"
        "\n"
        "BEGIN_DATA\n"
        "SpinEchoLength Depolarisation Depolarisation_error Wavelength\n"
        "260.0 -1.42E-3 2.04E-3 1.612452\n"
        "382.025 -2.26E-3 8.58E-4 1.954546\n";
    std::ofstream file(infileName);
    file << contents;
    file.close();
  }

  void writeFileMissingColumns() {
    std::string contents =
        "FileFormatVersion       1.0\n"
        "DataFileTitle           PMMA in Mixed Deuterated decalin\n"
        "Sample                  Ostensibly 40$ 100nm radius PMMA hard spheres "
        "in mixed deuterarted decalin.\n"
        "Thickness               2\n"
        "Thickness_unit          mm\n"
        "Theta_zmax              0.09\n"
        "Theta_zmax_unit         radians\n"
        "Theta_ymax              0.09\n"
        "Theta_ymax_unit         radians\n"
        "Orientation             Z\n"
        "SpinEchoLength_unit     A\n"
        "Depolarisation_unit     A - 2 cm - 1\n"
        "Wavelength_unit         A\n"
        "\n"
        "BEGIN_DATA\n"
        "SpinEchoLength Depolarisation Depolarisation_error SomethingElse\n"
        "260.0 -1.42E-3 2.04E-3 1.612452\n"
        "280.8 -1.45E-3 1.87E-3 1.675709\n";
    std::ofstream file(infileName);
    file << contents;
    file.close();
    
  }
};

#endif /* MANTID_DATAHANDLING_LOADSESANSTEST_H_ */
