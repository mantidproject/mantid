#!/usr/bin/python
"""Accepts a list of page names on the wiki and translates them to .rst

The bulk of the work is done by pandoc, which is expected to be in the PATH. There is then some post processing to:

  - downloading and correcting image links
  - add links for algorithms, fit functions and common concepts (workspace types etc)


 Limitations:
 1. in adding text to links or images in tables the table formatting will be disrupted
 2. pandoc creates some images in an inline format, these cannot have the align tags added
    back on, this is marked with a comment, the solution is probably to move the image from
    the inline to normal format.
 3. Image links cannot have spaces in the filename in rst.
    The spaces are removed in the downloaded file names, but not the links in the rst files."""
from __future__ import print_function

import argparse
import httplib
import os
import re
import subprocess
import sys
import tempfile
import urllib2

import mantid

DEBUG = 0

def parse_arguments(argv):
    parser = argparse.ArgumentParser(description='Converts mediawiki pages to rst pages for sphinx.')
    parser.add_argument('pagename', help='The name of a page on the wiki site')
    parser.add_argument('-o', '--output-file',
                        help='Provide a path to output to a file. If empty the text will go to stdout')
    parser.add_argument('--images-dir', default="../images",
                        help='Provide a relative path from the file to the images directory')
    parser.add_argument('-rp', '--ref-link-prefix',
                        help='Provide a reference link prefix')
    parser.add_argument('-r', '--ref-link',
                        help='Provide a reference link')
    parser.add_argument('--index_url', default='http://www.mantidproject.org/',
                        help='The url of the main wiki landing page')
    return parser.parse_args(argv)

# ------------------------------------------------------------------------------

def display_info(*args):
    print(*args)

def display_debug(*args):
    if DEBUG == 1:
        print(*args)

# ------------------------------------------------------------------------------

class WikiURL(object):

    def __init__(self, indexurl, pagename):
        self._indexurl = indexurl
        self._pagename = pagename

    @property
    def indexurl(self):
        return self._indexurl

    @property
    def pagename(self):
        return self._pagename

    def pretty(self):
        return self.indexurl + self.pagename

    def raw(self):
        return self.ugly() + "&action=raw"

    def ugly(self):
        return self.indexurl + "index.php?title=" + self.pagename

# ------------------------------------------------------------------------------

def fetch_wiki_markup(wiki_url):
    try:
        response = urllib2.urlopen(wiki_url.raw())
        return response.read()
    except urllib2.URLError, err:
        raise RuntimeError("Failed to fetch wiki page: {}".format(str(err)))

# ------------------------------------------------------------------------------

def to_rst(wiki_url, post_process_args):
    wiki_markup_text = fetch_wiki_markup(wiki_url)
    display_debug("Converting wiki markup '{}'".format(wiki_markup_text))
    pandoc_rst = run_pandoc(wiki_markup_text)
    mantid_rst = post_process(pandoc_rst, wiki_url, wiki_markup_text,
                              post_process_args)
    return mantid_rst

# ------------------------------------------------------------------------------

def run_pandoc(wiki_markup_text):
    wiki_tmp_file = tempfile.NamedTemporaryFile(delete=True)
    wiki_tmp_file.write(wiki_markup_text)
    wiki_tmp_file.flush()
    cmd = "pandoc"
    args = ["-f", "mediawiki", "-t", "rst", wiki_tmp_file.name]
    try:
        rst_text = execute_process(cmd, args)
    except Exception, err:
        raise RuntimeError("Error executing pandoc: '{}'".format(str(err)))
    finally:
        wiki_tmp_file.close()

    return rst_text

# ------------------------------------------------------------------------------

def execute_process(cmd, args):
    """Execute the command with the given arguments.
    Raise an exception
    """
    if sys.platform == "win32":
        # instantiate a startupinfo obj:
        startupinfo = subprocess.STARTUPINFO()
        # set the use show window flag, might make conditional on being in Windows:
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    else:
        startupinfo = None

    cmd = [cmd]
    cmd.extend(args)
    display_debug("Running command '{}' in child process".format(" ".join(cmd)))
    p = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         startupinfo=startupinfo)
    output, _ = p.communicate()
    output = output.replace("\r\n","\n")
    return output

# ------------------------------------------------------------------------------

def post_process(pandoc_rst, wiki_url, wiki_markup,
                              post_process_args):
    """Takes raw reST text produced by pandoc and applies several post-processing steps
    as described in the module documentation
    """
    post_processed_rst = pandoc_rst#fix_underscores_in_ref_links(pandoc_rst)
    post_processed_rst, image_names = fix_image_links(post_processed_rst, wiki_markup,
                                                      post_process_args["rel_img_dir"])
    return post_processed_rst

# ------------------------------------------------------------------------------

def fix_underscores_in_ref_links(pandoc_rst):
    """Defalt pandoc-translated references links seem to contain \_
    This removes the backslash
    """
    return re.sub(r"\b\\\_","_", pandoc_rst)

# ------------------------------------------------------------------------------

def fix_image_links(rst_text, wiki_markup, rel_img_dir):
    """Fix the image links to point to the correct relative image location
    It returns the processed text + the names of the required images from the wiki
    """
    mw_img_re = re.compile(r'\[\[(Image|File):(.+?)(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?\]\]',
                                re.IGNORECASE)
    rst_img_re = re.compile(r'figure:: (([\w\-\_\\\/]+)\.(\w{2,5}))(.{0,50}\3){2}',
                            re.DOTALL)
    rst_sub_re = re.compile(r'(figure|image):: (([\w\-\_\\\/]+)\.(\w{2,5}))')

    img_link_dict = dict()
    # for all of the mediawiki links
    for m in re.finditer(mw_img_re, wiki_markup):
        display_info("Processing image link",m.group(0))
        img_link_dict[m.group(2)] = generate_rst_img_link(m, rel_img_dir)

    #for all of the rst figure links
    replacements = []
    for m in re.finditer(rst_img_re, rst_text):
        rst_link = img_link_dict[m.group(1)]
        replacements.append((m.start(), m.end(), m.group(1), rst_link))

    #perform replacements in reverse order
    for (start, end, imagefile, rst_link) in reversed(replacements):
        rst_text = rst_text[0:start] + rst_link + rst_text[end:]

    replacements = []
    for m in re.finditer(rst_sub_re, rst_text):
        rst_link = img_link_dict[m.group(2)]
        rst_link = clean_inline_img_def(rst_link)
        replacements.append((m.start(), m.end(), m.group(2), rst_link))

    # perform replacements in reverse order
    for (start, end, imagefile, rst_link) in reversed(replacements):
        rst_text = rst_text[0:start] + rst_link + rst_text[end:]

    return rst_text, img_link_dict.keys()

# ------------------------------------------------------------------------------

def generate_rst_img_link(match, rel_img_dir):
    link = "image:: " + rel_img_dir + "/" + match.group(2) + "\n"
    for i in range(3,len(match.groups())):
        if match.group(i) is None:
            break
        #strip off the first character as it is the | pipe
        img_opt = add_img_option(match.group(i)[1:])
        if img_opt is not None:
            link += "\t\t\t" + img_opt + "\n"
    return link

# ------------------------------------------------------------------------------

def add_img_option(mw_img_opt):
    mw_img_opt = mw_img_opt.strip()
    if len(mw_img_opt) > 0:
        if mw_img_opt.endswith("px"):
            return ":width: " + mw_img_opt
        elif mw_img_opt in ["right","left","middle","centre"]:
            return ":align: " + mw_img_opt
        else:
            return ":alt: " + mw_img_opt
    else:
        return None

# ------------------------------------------------------------------------------

def clean_inline_img_def(rst_link):
    match = re.search(r'^\s+:align:\s+\w+\s*$',rstLink,re.MULTILINE)
    if match is not None:
        #take the align out
        rst_link = rst_link[0:match.start()] + rst_link[match.end()+1:]
        #then add it at the end as a comment
        rst_link += ".. FIXME (inline definition does not allow align)" + match.group(0)
    return rst_link

# ------------------------------------------------------------------------------

if __name__ == '__main__':
    args = parse_arguments(sys.argv[1:])
    wiki_url = WikiURL(args.index_url, args.pagename)
    display_info("Converting {} to reStructeredText..".format(wiki_url.pretty()))

    if args.output_file:
        output_dir = os.path.dirname(args.output_file)
        rel_img_dir = args.images_dir
        img_dir = os.path.normpath(os.path.join(output_dir, rel_img_dir))
    else:
        rel_img_dir, img_dir = None, None
    post_process_args = {"rel_img_dir": rel_img_dir, "img_dir": img_dir}
    display_debug("Post processing arguments: '{}'".format(post_process_args))
    rst_text = to_rst(wiki_url, post_process_args)
    if args.output_file:
        open(args.output_file, 'w').write(rst_text + "\n")
    else:
        display_info(rst_text)
    sys.exit()
