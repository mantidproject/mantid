#ifndef MANTID_ALGORITHMS_CROPTOCOMPONENTTEST_H_
#define MANTID_ALGORITHMS_CROPTOCOMPONENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/CropToComponent.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"


class CropToComponentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CropToComponentTest *createSuite() { return new CropToComponentTest(); }
  static void destroySuite( CropToComponentTest *suite ) { delete suite; }


  void test_Init()
  {
    Mantid::Algorithms::CropToComponent alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Arrange
    int numberOfBanks = 4;
    int numberOfPixelsPerBank = 3;

    Mantid::API::MatrixWorkspace_sptr inputWorkspace = getSampleWorkspace(numberOfBanks, numberOfPixelsPerBank);
    //auto inputWorkspace = getSampleWorkspace(numberOfBanks, numberOfPixelsPerBank);
    std::vector<std::string> componentNames = {"bank1", "bank2"};

    // Act
    Mantid::Algorithms::CropToComponent crop;
    crop.setChild(true);
    crop.initialize();
    crop.setProperty("InputWorkspace", inputWorkspace);
    crop.setProperty("OutputWorkspace", "dummy");
    crop.setProperty("ComponentNames", componentNames);
    crop.execute();
    TS_ASSERT( crop.isExecuted() )
    auto outputWorkspace = crop.getProperty("OutputWorkspace");


    // Assert
  }
  
  void test_that_no_bank_returns_original_workspace() {

  }

  void test_that_single_bank_can_be_extracted() {

  }

  void test_that_incorrect_component_name_is_not_accepeted() {

  }

private:
Mantid::API::MatrixWorkspace_sptr getSampleWorkspace(int numberOfBanks,
                                                     int numbersOfPixelPerBank)
{
    return WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
        numberOfBanks, numbersOfPixelPerBank, 2);
}

};


#endif /* MANTID_ALGORITHMS_CROPTOCOMPONENTTEST_H_ */
