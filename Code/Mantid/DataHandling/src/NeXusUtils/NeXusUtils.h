#ifndef NEXUSUTILS_H
#define NEXUSUTILS_H

void testNX();
void writeEntry1D(const std::string& filename, const std::string& entryname, const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& e);

#endif /* NEXUSUTILS_H */
