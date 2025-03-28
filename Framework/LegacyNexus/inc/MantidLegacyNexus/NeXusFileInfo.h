/*
  This is some code to handle a stack of NeXus files. This is used to implement
  external linking within the NeXus-API

  Copyright (C) 1997-2006 Mark Koennecke

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  For further information, see <http://www.nexusformat.org>

  Added functions to deal with the path stack for NXgetpath
  Mark Koennecke, October 2009

*/
#pragma once

const int EXPECTED_PATH_STACK_HEIGHT = 5;

// get nxlink
#include "MantidLegacyNexus/NeXusFile_fwd.h"
#include "MantidLegacyNexus/napi_internal.h"

#include <string>
#include <vector>

class NexusFileID {
public:
  NexusFileID(const std::string &userFilePath) : m_pathChars(0), m_userFilePath(userFilePath) {
    m_nexusPath.reserve(EXPECTED_PATH_STACK_HEIGHT);
  }

  std::string getFullNexusPath() const;
  const std::string &getFilePath() const { return m_filePath; }
  LgcyFunction &getNexusFunctions() const;
  void setNexusFunctions(LgcyFunctionPtr nexusFunctions) { m_nexusFunctions = std::move(nexusFunctions); }
  void pushNexusPath(const std::string &path);
  void popNexusPath();
  const std::string &getUserFilePath() { return m_userFilePath; }
  void setFilePath(const std::string &filePath) { m_filePath = filePath; }

private:
  std::string m_filePath;
  std::vector<std::string> m_nexusPath;
  size_t m_pathChars;
  LgcyFunctionPtr m_nexusFunctions;
  std::string m_userFilePath;
};
