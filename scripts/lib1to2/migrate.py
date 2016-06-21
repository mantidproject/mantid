"""
Performs the actual migration
"""
import messages
from grammar import Grammar

import os
import shutil
import traceback

def run(files, backup=True):
    """
    Runs the migration process

        @param files A list of files to migrate
        @param backup If true, the files are backed up before running the migration. The
                      backup file is the filename plus '.mantidbackup'
    """
    if len(files) == 0:
        messages.notify("Nothing to do!")
        return 0
    if type(files) == str:
        files = [files]

    reports = []
    for filename in files:
        script = ScriptFile(filename, backup)
        try:
            msg = script.migrate()
        except Exception, exc:
            traceback.print_exc()
            script.restore_backup()
            msg = "%s: Backup restored." % (filename)
        reports.append(msg)
        del script

    messages.notify("\n" + "="*10 + " Report " + "="*10 + "\n")
    for report in reports:
        messages.notify(str(report))

    return 0


class ScriptFile(object):
    """
    Encapsulates a script file. The migration
    process can be run by calling migrate
    """
    _filename = None
    dobackup = True
    backup_ext = '.mantidbackup'
    backup_filename = None
    grammar = None

    def getfilename(self):
        return self._filename

    def setfilename(self, filename):
        """Set the filename along with the backup filename
            @param filename The full filename of the input
        """
        if os.path.exists(filename):
            self._filename = filename
            self.backup_filename = filename + self.backup_ext
        else:
            raise ValueError("Invalid path '%s' passed to ScriptFile" % (filename))

    filename = property(getfilename, setfilename)

    def __init__(self, filename, backup = True):
        self.setfilename(filename)
        self.dobackup = backup
        self.grammar = Grammar()

    def migrate(self):
        """
        Migrate the script to the new API
        """
        self.backup()
        input_file = open(self.filename, 'r')
        input_as_str = input_file.read()
        input_file.close()

        converted_str, errors = self.grammar.translate(input_as_str)

        filename = self.filename
        if len(errors) > 0:
            filename = self.filename + ".partial"
            errors.append("Partial translation stored in '%s'" % filename)

        output_file = open(filename, 'w')
        output_file.write(converted_str)
        output_file.close()

        if len(errors) > 0:
            raise RuntimeError("\n".join(errors))

        outcome = MigrationStatus(MigrationStatus.Migrated)
        return Report(self.filename, outcome)

    def backup(self):
        """
        Backup the file by copying it to
        a different filename with the
        extension defined by self.backup_ext
        """
        if self.dobackup and self.filename is not None:
            messages.notify("Backing up %s to %s" % (self.filename, self.backup_filename))
            shutil.copy(self.filename, self.backup_filename)

    def restore_backup(self):
        """
        Copies the file from the backup to the original
        location
        """
        if not self.dobackup:
            messages.notify("Cannot restore from backup, no backup was requested")
        if os.path.exists(self.backup_filename):
            shutil.copy(self.backup_filename, self.filename)


class Report(object):
    """Reports the outcome of a single migration"""

    _filename = None
    _status = None

    def __init__(self, filename, status):
        self._filename = filename
        self._status = status

    def __str__(self):
        """Returns a string representation of the report"""
        return "%s: %s" % (self._filename, str(self._status))

class MigrationStatus(object):

    Unconverted = 0
    Migrated = 1
    Errors = 2

    def __init__(self, status):
        self.status = status

    def __str__(self):
        """Returns the status as a string"""
        if self.status == self.Unconverted:
            return "Unconverted"
        elif self.status == self.Migrated:
            return "Successfully migrated"
        else:
            return "Errors occurred"
