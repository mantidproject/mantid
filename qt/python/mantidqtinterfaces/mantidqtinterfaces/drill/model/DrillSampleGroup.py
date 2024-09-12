# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class DrillSampleGroup:
    """
    Name of the group.
    """

    _name = None

    """
    List of samples in this group.
    """
    _samples = None

    """
    Master sample of this group.
    """
    _master = None

    def __init__(self):
        self._name = ""
        self._samples = list()

    def setName(self, name):
        """
        Set the name of the group.

        Args:
            name (str): name of the group
        """
        self._name = name

    def getName(self):
        """
        Get the name of the group.

        Returns:
            (str): name of the group
        """
        return self._name

    def addSample(self, sample):
        """
        Add a sample in the group.

        Args:
            sample (DrillSample): sample to be added in the group
        """
        i = 0
        while (i < len(self._samples)) and (self._samples[i].getIndex() < sample.getIndex()):
            i += 1
        self._samples.insert(i, sample)
        sample.setGroup(self)
        while i < len(self._samples):
            self._samples[i].groupChanged.emit()
            i += 1

    def delSample(self, sample):
        """
        Remove a sample from the group.

        Args:
            sample (DrillSample): sample to be removed from the group
        """
        index = None
        if sample in self._samples:
            index = self._samples.index(sample)
            self._samples.remove(sample)
            sample.setGroup(None)
        if index is not None:
            while index < len(self._samples):
                self._samples[index].groupChanged.emit()
                index += 1

    def getSampleIndex(self, sample):
        """
        Get the index of a sample in the group. Sample has to be in the group.

        Args:
            sample (DrillSample): sample

        Returns:
            int: index
        """
        return self._samples.index(sample)

    def isEmpty(self):
        """
        Check if the group is empty.

        Returns:
            bool: True if the group is empty
        """
        return len(self._samples) == 0

    def setMaster(self, sample):
        """
        Set the master sample of the group. The sample has to be in the group
        first.

        Args:
            sample (DrillSample): sample to be set as master of the group
        """
        if sample in self._samples:
            previousMaster = self._master
            self._master = sample
            if previousMaster is not None:
                previousMaster.groupChanged.emit()
            sample.groupChanged.emit()

    def unsetMaster(self):
        """
        Unset the master sample.
        """
        self._master = None

    def getMaster(self):
        """
        Get the master sample of the group.

        Returns:
            (DrillSample): master sample or None
        """
        return self._master

    def getSamples(self):
        """
        Get the samples of the group.

        Returns:
            list(DrillSample): list of samples
        """
        return self._samples
