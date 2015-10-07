#
# Copyright 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
#
# This file is part of Mantid.
#
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>

# Prefix can be overridden with PREFIX environment variable
prefix=$(if ${PREFIX},${PREFIX},/usr/local)

# These files will have executable status when installed
EXEC_FILES=git-new git-checkbuild gitworkflow-helpers git-finish git-test git-publish

all:
	@echo "usage: make install"
	@echo "       make uninstall"

install:
	@echo "Installing workflow files in $(prefix)/bin"
	install -d -m 0755 $(prefix)/bin
	install -m 0755 $(EXEC_FILES) $(prefix)/bin

uninstall:
	test -d $(prefix)/bin && \
	cd $(prefix)/bin && \
	rm -f $(EXEC_FILES) $(SCRIPT_FILES)
