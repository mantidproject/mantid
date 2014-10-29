//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidDataHandling/SaveReflTBL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <fstream>
#include <boost/tokenizer.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveReflTBL)
    
    using namespace Kernel;
    using namespace API;

    /// Empty constructor
    SaveReflTBL::SaveReflTBL() : m_sep(','), m_stichgroups(), m_nogroup()
    {
    }

    /// Initialisation method.
    void SaveReflTBL::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".tbl");

      declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The filename of the output TBL file.");

      declareProperty(new WorkspaceProperty<ITableWorkspace>("InputWorkspace", "", Direction::Input),
        "The name of the workspace containing the data you want to save to a TBL file.");
    }


    /**
    * Finds the stitch groups that need to be on the same line
    * @param ws : a pointer to a tableworkspace
    */
    void SaveReflTBL::findGroups(ITableWorkspace_sptr ws)
    {
      size_t rowCount = ws->rowCount();
      for (size_t i = 0; i < rowCount; ++i)
      {
        TableRow row = ws->getRow(i);
        if (row.cell<int>(7) != 0)
        {
          //it was part of a group
          m_stichgroups[row.cell<int>(7)].push_back(i);
          if (m_stichgroups[row.cell<int>(7)].size() > 3)
          {
            std::string message = "Cannot save a table with stitch groups that are larger than three runs to Reflectometry .tbl format.";
            throw std::length_error(message);
          }
        }
        else
        {
          //it wasn't part of a group
          m_nogroup.push_back(i);
        }
      }
    }


    /** 
     *   Executes the algorithm.
     */
    void SaveReflTBL::exec()
    {
      // Get the workspace
      ITableWorkspace_sptr ws = getProperty("InputWorkspace");

      findGroups(ws);

      std::string filename = getProperty("Filename");
      std::ofstream file(filename.c_str());

      if (!file)
      {
        throw Exception::FileError("Unable to create file: " , filename);
      }

      typedef std::map<int, std::vector<size_t>>::iterator map_it_type;
      for(map_it_type iterator = m_stichgroups.begin(); iterator != m_stichgroups.end(); ++iterator)
      {
        std::vector<size_t> & rowNos = iterator->second;
        size_t i = 0;
        for (; i < rowNos.size(); ++i)
        {
          //for each row in the group print the first 5 columns to file
          TableRow row = ws->getRow(rowNos[i]);
          for (int j = 0; j < 5; ++j)
          {
            writeVal(row.cell<std::string>(j),file);
          }
        }
        //if i comes out of that loop as less than 3, then we need to add the blank runs
        for (; i < 3; ++i)
        {
          for (int j = 0; j < 5; ++j)
          {
            file << m_sep;
          }
        }
        //now add dq/q and scale from the first row in the group
        TableRow row = ws->getRow(rowNos[0]);
        writeVal(row.cell<std::string>(5),file);
        std::string scaleStr = boost::lexical_cast<std::string>(row.cell<double>(6));
        writeVal(scaleStr, file, false, true);
      }

      //now do the same for the ungrouped

      typedef std::vector<size_t>::iterator vec_it_type;
      for(vec_it_type iterator = m_nogroup.begin(); iterator != m_nogroup.end(); ++iterator)
      {
        TableRow row = ws->getRow(*iterator);
        for (int j = 0; j < 5; ++j)
        {
          writeVal(row.cell<std::string>(j),file);
        }
        for (int k = 0; k < 10; ++k)
        {
          file << m_sep;
        }
        //now add dq/q and scale
        writeVal(row.cell<std::string>(5),file);
        std::string scaleStr = boost::lexical_cast<std::string>(row.cell<double>(6));
        writeVal(scaleStr, file, false, true);
      }
      file.close();
    }

    /**
    * Writes the given value to file, checking if it needs to be surrounded in quotes due to a comma being included
    * @param val : the string to be written
    * @param file : the ouput file stream
    * @param endsep : boolean true to include a comma after the data
    * @param endline : boolean true to put an EOL at the end of this data value
    */
    void SaveReflTBL::writeVal(std::string & val,std::ofstream & file, bool endsep, bool endline)
    {
      size_t comPos = val.find(',');
      if (comPos != std::string::npos)
      {
        file << '"' << val << '"';
      }
      else
      {
        file << val;
      }
      if (endsep)
      {
        file << m_sep;
      }
      if (endline)
      {
        file << std::endl;
      }
    }
  } // namespace DataHandling
} // namespace Mantid
