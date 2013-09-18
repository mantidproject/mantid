#include "MantidQtMantidWidgets/ICatHelper.h"
#include "MantidQtAPI/AlgorithmDialog.h"

namespace MantidQt
{
  namespace MantidWidgets
  {

    /**
     * Obtain the list of instruments from the ICAT Catalog algorithm.
     * @return :: A vector containing the list of all instruments available.
     */
    std::vector<std::string> ICatHelper::getInstrumentList()
    {
      QString algName("CatalogListInstruments");
      Mantid::API::IAlgorithm_sptr catalogAlgorithm;
      try
      {
        catalogAlgorithm = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString());
      }
      catch(std::runtime_error& exception)
      {
        exception.what();
      }

      catalogAlgorithm->execute();
      // return the vector containing the list of instruments available.
      return (catalogAlgorithm->getProperty("InstrumentList"));
    }

  }
}
