#pylint: disable=too-many-arguments

from __future__ import (absolute_import, division, print_function)
import numpy
from wand.image import Image
from wand.drawing import Drawing
from wand.color import Color
from PyQt4.QtCore import QByteArray, QBuffer


def animate_slice(sliceviewer, name, start, end, filename, num_frames=10, font_size=24):
    """Generate an animated gif of a 2D slice moving through a third dimension.

    Args:
        sliceviewer (SliceViewer): A sliceviewer instance.
        name (str): The name of the third dimension to use.
        start (float): The starting value of the third dimension.
        end (float): The end value of the third dimension.
        filename (str): The file to save the gif to.

    Kwargs:
        num_frames (int): The number of frames the gif should contain.
        font_size: (int): The size of the caption.

    Example:
        ws = CreateMDWorkspace(3, Extents=[-10,10,-10,10,-10,10], Names=["X","Y","Z"], Units=["u","u","u"])
        FakeMDEventData(ws, PeakParams=[10000,0,0,0,1])
        sv = plotSlice(ws)
        #Resize and configure the slice viewer how you want the output to look
        sv.setNormalization(1) # We need to normalize by volume in this case, or the data won't show up
        #This will create a gif iterating from Z = -1 to Z = 1
        animate_slice(sv, "Z", -1, 1, "output.gif")
    """
    #Generate all the individual frames
    images = []
    for slice_point in numpy.linspace(start, end, num_frames):
        sliceviewer.setSlicePoint(name, slice_point)
        sliceviewer.refreshRebin()
        qimg = sliceviewer.getImage().toImage()
        data = QByteArray()
        buf = QBuffer(data)
        qimg.save(buf, "PNG")
        image = Image(blob=str(data))
        captionstrip_size = font_size + 10
        #To add whitespace to the top, we add a vertical border,
        #then crop out the bottom border
        image.border(Color("#fff"), 0, captionstrip_size)
        image.crop(0, 0, image.width, image.height - captionstrip_size)
        #Write the caption into the whitespace
        draw = Drawing()
        draw.font_size = font_size
        draw.text(5, font_size,"%s = %g" % (name,slice_point))
        draw(image)
        images.append(image)
    #Create a new image with the right dimensions
    animation = Image(width=images[0].width, height=images[0].height)
    #Copy in the frames from all the generated images
    for image in images:
        animation.sequence.append(image.sequence[0])
    #Drop the initial blank frame
    del animation.sequence[0]
    #Write the animation to disk
    animation.save(filename=filename)
