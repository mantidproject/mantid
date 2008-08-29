#ifndef NEXUSUTILS_H
#define NEXUSUTILS_H

void writeEntry1D(const std::string& filename, const std::string& entryname, const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& e);
int  getNexusDataValue(const std::string& fileName, const std::string& dataName, std::string& value );

#endif /* NEXUSUTILS_H */
