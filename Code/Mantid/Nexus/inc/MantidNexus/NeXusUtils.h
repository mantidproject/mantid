#ifndef NEXUSUTILS_H
#define NEXUSUTILS_H
#include <napi.h>
#include "MantidDataObjects/Workspace2D.h"

int writeEntry1D(const std::string& filename, const std::string& entryName, const std::string& dataName,
				 const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& e);
int getNexusDataValue(const std::string& fileName, const std::string& dataName, std::string& value );
int writeNexusTextField( const NXhandle& h, const std::string& name, const std::string& value);
int writeNexusProcessedHeader( const std::string& fileName, const std::string& entryName, const std::string& title);
bool writeNxText(NXhandle fileID, std::string name, std::string value, std::vector<std::string> attributes,
				 std::vector<std::string> avalues);
int writeNexusProcessedSample( const std::string& fileName, const std::string& entryName, const std::string& title,
							  const boost::shared_ptr<Mantid::API::Sample> sample);
int writeNexusProcessedData( const std::string& fileName, const std::string& entryName,
							const boost::shared_ptr<Mantid::DataObjects::Workspace2D> localworkspace,
							const bool uniformSpectra);
#endif /* NEXUSUTILS_H */
