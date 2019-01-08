# Wiki2rst

### Introduction

`wiki2rst` reads in mediawiki formatted webpages and converts them to `.rst` files, for use in `Sphinx`. The code attempts to take all images and internal links and re-create the documentation structure in the `Sphinx` format. This process is good, but not perfect; some known issues are listed below. 

### Requirements

* `wiki2rst` runs on any machine where `Mantid` is working.
* To run `wiki2rst` in standalone mode it is necessary to add `mantid` to your Python path.

### Use

* Having added `mantid` to your Python path `wiki2rst` is run by:

`python wiki2rst.py -o <output_file.rst> <url_extension>`

* The `<url_extension>` is the part of the url after the main address (`https://www.mantidproject.org/`).
* There are several additional options:
	* `--index_url` change the name of the main url address
	* `--images-dir` set a relative location for the images directory 
	* `--ref-link` give a reference link
	* `--ref-link-prefix` give a link prefix
	* `--add_handle` add a handle for linking to the page
	* `--page_handle` the page handle to use [default page name]
	* `--add_heading` add a heading to the page (uses the page name)
