#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTransferStrategy.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace Mantid::API;

    //----------------------------------------------------------------------------------------------
    /** Constructor
    @param transferMethod : Transfer strategy
    @param tableWorkspace : The table workspace to copy data from
    @param instrument : instrument name
    */
    ReflSearchModel::ReflSearchModel(const ReflTransferStrategy &transferMethod,
                                     ITableWorkspace_sptr tableWorkspace,
                                     const std::string &instrument) {
      //Copy the data from the input table workspace
      for(size_t i = 0; i < tableWorkspace->rowCount(); ++i)
      {
        const std::string runFile = tableWorkspace->String(i, 0);

        // If this isn't the right instrument, remove it
        auto run = runFile;
        if (run.substr(0, instrument.size()) != instrument) {
          continue; // Don't show runs that appear to be from other instruments.
        }

        // It's a valid run, so let's trim the instrument prefix and ".raw"
        // suffix
        run =
            run.substr(instrument.size(), run.size() - (instrument.size() + 4));

        // Let's also get rid of any leading zeros
        size_t numZeros = 0;
        while (run[numZeros] == '0')
          numZeros++;
        run = run.substr(numZeros, run.size() - numZeros);

        if (transferMethod.knownFileType(runFile)) {
          m_runs.push_back(run);
          const std::string description = tableWorkspace->String(i, 6);
          m_descriptions[run] = description;
          const std::string location = tableWorkspace->String(i, 1);
          m_locations[run] = location;
        }
      }

      //By sorting the vector of runs, we sort the entire table
      std::sort(m_runs.begin(), m_runs.end());
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ReflSearchModel::~ReflSearchModel()
    {
    }

    /**
    @return the row count.
    */
    int ReflSearchModel::rowCount(const QModelIndex &) const
    {
      return static_cast<int>(m_runs.size());
    }

    /**
    @return the number of columns in the model.
    */
    int ReflSearchModel::columnCount(const QModelIndex &) const { return 3; }

    /**
    Overrident data method, allows consuming view to extract data for an index and role.
    @param index : For which to extract the data
    @param role : Role mode
    */
    QVariant ReflSearchModel::data(const QModelIndex &index, int role) const
    {
      if(role != Qt::DisplayRole)
        return QVariant();

      const int colNumber = index.column();
      const int rowNumber = index.row();

      if(rowNumber < 0 || rowNumber >= static_cast<int>(m_runs.size()))
        return QVariant();

      const std::string run = m_runs[rowNumber];

      if(colNumber == 0)
        return QString::fromStdString(run);

      if(colNumber == 1)
        return QString::fromStdString(m_descriptions.find(run)->second);

      if (colNumber == 2)
        return QString::fromStdString(m_locations.find(run)->second);

      return QVariant();
    }

    /**
    Get the heading for a given section, orientation and role.
    @param section : Column index
    @param orientation : Heading orientation
    @param role : Role mode of table.
    @return HeaderData.
    */
    QVariant ReflSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
      if(role != Qt::DisplayRole)
        return QVariant();

      if(orientation == Qt::Horizontal)
      {
        switch(section)
        {
          case 0: return "Run";
          case 1: return "Description";
          case 2:
            return "Location";
          default: return "";
        }
      }
      return QVariant();
    }

    /**
    Provide flags on an index by index basis
    @param index: To generate a flag for.
    */
    Qt::ItemFlags ReflSearchModel::flags(const QModelIndex &index) const
    {
      if(!index.isValid())
        return 0;
      else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
  } // namespace CustomInterfaces
} // namespace Mantid
