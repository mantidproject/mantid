#ifndef NEXUSUTILS_H
#define NEXUSUTILS_H
#include <napi.h>

int writeEntry1D(const std::string& filename, const std::string& entryName, const std::string& dataName, const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& e);
int getNexusDataValue(const std::string& fileName, const std::string& dataName, std::string& value );
int writeNexusTextField( const NXhandle& h, const std::string& name, const std::string& value);

#endif /* NEXUSUTILS_H */
