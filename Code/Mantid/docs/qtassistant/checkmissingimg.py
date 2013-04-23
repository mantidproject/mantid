#!/usr/bin/env python
import os
import re

def getHtml(htmldir):
    """
    Recursively find all html files in the supplied directory.
    @param htmldir The base directory for the html site.
    @returns All html files with full path.
    """
    candidates = os.listdir(htmldir)
    results = []
    for candidate in candidates:
        if os.path.isdir(candidate):
            results.extend(getHtml(os.path.join(htmldir, candidate)))
        elif candidate.endswith('.html'):
            results.append(os.path.join(htmldir, candidate))
    return results

def processHtml(htmldir, filename):
    """
    @param htmldir The base directory for the html site.
    @param filename The html file to parse and look for missing images in.
    @returns All Missing image files.
    """
    # read in the entire html file
    handle = file(filename, 'r')
    text = handle.read()
    handle.close()

    # determine all the requested images
    candidates = re.findall(r"img(.+)/>", text, flags=re.MULTILINE)
    if len(candidates) <= 0:
        return []
    options = []
    for candidate in candidates:
        start = candidate.find("src=")
        if start <= 0:
            continue
        candidate = candidate[start+5:]
        end = candidate.find('"')
        if end <= start:
            end = candidate.find("'")
        if end <= start:
            continue
        options.append(candidate[:end])

    # add them to the list of missing images if not found
    results = []
    for candidate in options:
        candidate = os.path.join(htmldir, candidate)
        if not os.path.exists(candidate):
            results.append(candidate)

    # return everything that isn't found
    return results


if __name__ == "__main__":
    # set up the command line option parser
    import optparse
    parser = optparse.OptionParser(usage="usage: %prog [options] <htmldir>",
                                   description="Determine if there are images missing from the built documentation.")
    (options, args) = parser.parse_args()

    # get the html base directory
    if len(args) <= 0:
        parser.error("Failed to specify a html directory to parse")
    htmldir = os.path.abspath(args[0])
    if not os.path.exists(htmldir):
        parser.error("Must specify an existing html directory")

    # get the list of html files
    htmlfiles = getHtml(htmldir)
    print "Verifying %d html files in '%s'" % (len(htmlfiles), htmldir)

    # determine what images are missing
    missing = []
    for filename in htmlfiles:
        missing.extend(processHtml(htmldir, filename))

    # remove repeated filenames
    missing = set(missing)

    # print the results
    print "Missing %d image files" % len(missing)
    for filename in missing:
        print filename
