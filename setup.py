#!/usr/local/bin/python

from distutils.core import setup,Extension

import os



def main():
    try:
	revision=os.popen("svn-rev").read().strip()
	if revision:
	    revision="-r"+revision
    except:
	revision=""

    setup(name = "xlistdir",
	  version = "0.1.0" + revision,
	  author = "Terrel Shumway",
	  author_email = "tshumway@jdiworks.net",
	  url = "http://wxidle.sourceforge.net/projects/xlistdir/",
	  description = "xlistdir is to listdir as xreadlines is to readlines",
	  long_description = """\
    xlistdir.xlistdir is an iterator version of os.listdir.

    This initial hack will probably not even compile on anything
    non-posix (e.g. mswin32, mswin16, OS/2) because I do not have
    access to those platforms. If you do, please help.

    It does include a unit test, and it does pass for me on Linux. 8-)
     
    Most of this code was cut and pasted from xreadlinesmodule.c 
    and posixmodule.c in Python-2.2.2.tgz
    """,
	  license = "python",

	  py_modules = ["test_xlistdir"],
	  ext_modules = [Extension("xlistdir",["xlistdirmodule.c"])],
	 )

if __name__=="__main__":
    main()

