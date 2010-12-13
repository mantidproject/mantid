//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SpatialGrouping.h"

#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/IDetector.h"

#include "MantidAPI/FileProperty.h"

#include <map>

#include <fstream>
#include <iostream>

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpatialGrouping)

/**
* init() method implemented from Algorithm base class
*/
void SpatialGrouping::init()
{
  declareProperty(new Mantid::API::WorkspaceProperty<>("InputWorkspace","",Mantid::Kernel::Direction::Input),"The input workspace.");
  declareProperty(new Mantid::API::FileProperty("Filename", "", Mantid::API::FileProperty::Save));
  declareProperty("SearchDistance", 1.9, Mantid::Kernel::Direction::Input);
  declareProperty("GridSize", 3, Mantid::Kernel::Direction::Input); 
}

/**
* exec() method implemented from Algorithm base class
*/
void SpatialGrouping::exec()
{
  Mantid::API::MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  double searchDist = getProperty("SearchDistance");
  int gridSize = getProperty("GridSize");
  int nNeighbours = ( gridSize * gridSize ) - 1;
  m_radius = 0.0;
  
  Mantid::Geometry::IInstrument_sptr m_instrument = inputWorkspace->getInstrument();
  m_detectors = m_instrument->getDetectors();
  
  Mantid::API::Progress prog(this, 0.0, 1.0, m_detectors.size());
    
  for ( std::map<int, Mantid::Geometry::IDetector_sptr>::iterator detIt = m_detectors.begin(); detIt != m_detectors.end(); ++detIt )
  {
    prog.report();

    // We are not interested in Monitors and we don't want them to be included in
    // any of the other lists
    if ( detIt->second->isMonitor() )
    {
      m_included[detIt->first] = false;
      continue;
    }

    if ( m_radius == 0.0 )
    {
      boost::shared_ptr<Mantid::Geometry::Detector> det = boost::dynamic_pointer_cast<Mantid::Geometry::Detector>(detIt->second);
      m_radius = det->getWidth() * searchDist;
    }

    // Or detectors already flagged as included in a group
    std::map<int, bool>::iterator inclIt = m_included.find(detIt->first);
    if ( inclIt != m_included.end() )
    {
      continue;
    }

    std::map<int, double> nearest = detIt->second->getNeighbours(m_radius);

    if ( nNeighbours > 8 )
    {
      bool extend = true;
      while ( ( nNeighbours > nearest.size() ) && extend )
      {
        extend = expandNet(nearest, detIt->second, nNeighbours);
      }
    }
    else if ( nNeighbours < 8  && nearest.size() > nNeighbours )
    {
      sortByDistance(nearest, nNeighbours);
    }

    if ( nearest.size() != nNeighbours ) continue;

    // if any of these nearest are already included, we'll continue too
    // but we want a seperate function for this to make it a little bit saner
    if ( included(nearest) )
    {
      continue;
    }

    // if we've gotten to this point, we want to go and make the group list.
    std::vector<int> group;
    m_included[detIt->first] = true;
    group.push_back(detIt->first);
    std::map<int, double>::iterator nrsIt;
    for ( nrsIt = nearest.begin(); nrsIt != nearest.end(); ++nrsIt )
    {
      m_included[nrsIt->first] = true;
      group.push_back(nrsIt->first);
    }
    m_groups.push_back(group);
  }


  // Create grouping XML file
  g_log.information() << "Creating XML Grouping File." << std::endl;
  std::vector<std::vector<int> >::iterator grpIt;
  std::ofstream xml;
  std::string fname = getPropertyValue("Filename");
  fname = fname + ".xml";
  xml.open(fname.c_str());

  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
    << "<!-- XML Grouping File created by SpatialGrouping Algorithm -->\n"
    << "<detector-grouping>\n";

  int grpID = 1;
  for ( grpIt = m_groups.begin(); grpIt != m_groups.end(); ++grpIt )
  {
    xml << "<group name=\"group" << grpID++ << "\"><detids val=\"" << (*grpIt)[0];
    for ( int i = 1; i < (*grpIt).size(); i++ )
    {
      xml << "," << (*grpIt)[i];
    }
    xml << "\"/></group>\n";
  }

  xml << "</detector-grouping>";

  xml.close();

  g_log.information() << "Finished creating XML Grouping File." << std::endl;

}

/**
* This method checks if the detectors have already been included in a group. Done in a seperate function
* to allow us to use the "continue" keyword above and simplify the logic for progressing through the
* detectors.
* @param nearest map of detector numbers to check for duplicates in
* @return true if any of the listed detectors have already been included in a group \
*         false otherwise
*/
bool SpatialGrouping::included(std::map<int,double> nearest)
{
  for ( std::map<int,double>::iterator nrsIt = nearest.begin(); nrsIt != nearest.end(); ++nrsIt )
  {
    std::map<int,bool>::iterator inclIt;
    inclIt = m_included.find(nrsIt->first);
    if ( inclIt != m_included.end() )
    {
      return true;
    }
  }
  return false;
}

/**
* This method will, using the NearestNeighbours methods, expand our view on the nearby detectors from
* the standard eight closest that are recorded in the graph.
* @param nearest neighbours found in original request from central detector
* @param det pointer to the central detector, for calculating distances
* @param noNeighbours number of neighbours that must be found (in total, including those already found)
* @return true if neighbours were found matching the parameters, false otherwise
*/
bool SpatialGrouping::expandNet(std::map<int,double> & nearest, Mantid::Geometry::IDetector_sptr det, const int noNeighbours)
{
  const int incoming = nearest.size();
  const int centDetID = det->getID();
  std::map<int, double> potentials;

  for ( std::map<int,double>::iterator nrsIt = nearest.begin(); nrsIt != nearest.end(); ++nrsIt )
  {
    std::map<int, double> results;
    results = m_detectors[nrsIt->first]->getNeighbours(m_radius);
    for ( std::map<int, double>::iterator resIt = results.begin(); resIt != results.end(); ++resIt )
    {
      potentials[resIt->first] = resIt->second;
    }
  }

  for ( std::map<int,double>::iterator potIt = potentials.begin(); potIt != potentials.end(); ++potIt )
  {
    // We do not want to include the detector in it's own list of nearest neighbours
    if ( potIt->first == centDetID ) { continue; }

    // Or detectors that are already in the nearest list passed into this function
    std::map<int, double>::iterator nrsIt = nearest.find(potIt->first);
    if ( nrsIt != nearest.end() ) { continue; }

    // We should not include detectors already included in a group (or monitors for that matter)
    std::map<int,bool>::iterator inclIt = m_included.find(potIt->first);
    if ( inclIt != m_included.end() ) { continue; }

    // If we get this far, we need to determine if the detector is of a suitable distance
    double distance = det->getDistance(*(m_detectors[potIt->first]));
    if ( distance > m_radius ) { continue; }

    // Add any that have survived to this point to the nearest
    nearest[potIt->first] = distance;
  }

  if ( nearest.size() == incoming ) { return false; }

  if ( nearest.size() > noNeighbours )
  {
    sortByDistance(nearest, noNeighbours);
  }

  return true;
}

/**
* This method will trim the result set down to the specified number required by sorting
* the results and removing those that are the greatest distance away.
* @param input map of values that need to be sorted, will be modified by the method
* @param noNeighbours number of elements that should be kept
*/
void SpatialGrouping::sortByDistance(std::map<int,double> & input, const int noNeighbours)
{
  std::vector<std::pair<int,double> > order;

  // populate this vector
  for ( std::map<int,double>::iterator inIt = input.begin(); inIt != input.end(); ++inIt )
  {
    std::pair<int,double> item(inIt->first, inIt->second);
    order.push_back(item);
  }

  // lazy implementation of a bubble sort
  // much better ways of doing this
  /// @TODO Implement a comparison method so that we can use std::sort here
  bool swp;
  do
  {
    swp = false;
    for ( int i = 0; i < order.size() - 1; i++ )
    {
      if ( order[i].second > order[i+1].second )
      {
        std::pair<int,double> temp = order[i];
        order[i] = order[i+1];
        order[i+1] = temp;
        swp = true;
      }
    }
  } while ( swp );

  int current = order.size();
  int lose = current - noNeighbours;
  if ( lose < 1 ) return;

  for ( int i = 1; i <= lose; i++ )
  {
    input.erase(order[current-i].first);
  }

}

} // namespace Algorithms
} // namespace Mantid
