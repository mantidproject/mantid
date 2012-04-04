#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataObjects
{

  Kernel::Logger& mg_log = Kernel::Logger::get("ITableWorkspace");

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SplittersWorkspace::SplittersWorkspace()
  {
    this->addColumn("long64", "start");
    this->addColumn("long64", "stop");
    this->addColumn("int", "workspacegroup");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SplittersWorkspace::~SplittersWorkspace()
  {
  }
  
  /*
   * Add a Splitter to
   */
  void SplittersWorkspace::addSplitter(Mantid::Kernel::SplittingInterval splitter)
  {
    Mantid::API::TableRow row = this->appendRow();
    row << splitter.start().totalNanoseconds();
    row << splitter.stop().totalNanoseconds();
    row << splitter.index();

    return;
  }

  Kernel::SplittingInterval SplittersWorkspace::getSplitter(size_t index)
  {
    API::Column_const_sptr column = this->getColumn("start");
    API::TableRow row = this->getRow(index);
    int64_t start, stop;
    int wsgroup;
    row >> start;
    row >> stop;
    row >> wsgroup;

    Kernel::SplittingInterval splitter(Kernel::DateAndTime(start), Kernel::DateAndTime(stop), wsgroup);

    return splitter;
  }

  size_t SplittersWorkspace::getNumberSplitters()
  {
    return this->rowCount();
  }


  bool SplittersWorkspace::removeSplitter(unsigned long index)
  {
    bool removed;
    if (index >= this->rowCount())
    {
      mg_log.error() << "Try to delete a non-existing splitter " << index << std::endl;
      removed = false;
    }
    else
    {
      this->removeRow(index);
      removed = true;
    }

    return removed;
  }



} // namespace Mantid
} // namespace DataObjects
