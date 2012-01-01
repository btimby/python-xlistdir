#!/usr/bin/env python

import unittest 

from xlistdir import xlistdir
import os

class XListdirTestCase(unittest.TestCase):

    def setUp(self):
	tmp = self.tmpdir = "test_xlistdir.tmp"+os.sep
	os.mkdir(tmp)
	os.mkdir(tmp+"junk")
	for x in range(8):
	    open(tmp+str(x),"w+").write(str(x))

    def tearDown(self):
	tmp = self.tmpdir
	for d in xlistdir(tmp):
	    try:
		os.unlink(tmp+d)
	    except OSError:
		os.rmdir(tmp+d)
	os.rmdir(tmp)
	
    
    def test_simple(self):
	x = xlistdir(self.tmpdir)

	for i in range(5):
	    y = x[i]

	del x

    def test_slice(self):
	x = xlistdir(self.tmpdir)
	buf = [x.next() for i in range(4)]
	del x

    def dont_test_big_slice(self):
	"""
	XXX fix this: 

	  File "test_xlistdir.py", line 42, in test_big_slice
	    buf = [x.next() for i in range(40)]
	SystemError: error return without exception set
	"""
	x = xlistdir(self.tmpdir)
	buf = [x.next() for i in range(40)]
	del x

    def test_iter(self):
	x = xlistdir(self.tmpdir)
	y = x[0]
	for d in x:
	    y = d
	del x
	    
    def test_iter2(self):
	x = xlistdir(self.tmpdir)
	y = x[0]
	for d in iter(x):
	    y = d
	del x

    def test_list(self):
	l = list(xlistdir(self.tmpdir))

    def test_list2(self):
	l = [ f for f in xlistdir(self.tmpdir)]

    def test_list3(self):
	l = [ f for f in xlistdir(self.tmpdir)
	    if os.path.isfile(self.tmpdir+f)]

    def test_outoforder(self):
	x = xlistdir(self.tmpdir)
	try:
	    y=x[1]
	    self.fail("should raise OutOfOrder")
	except RuntimeError:
	    pass
	del x


    def test_noent(self):
	try:
	    xlistdir(self.tmpdir+"nothere")
	    self.fail("should raise OSError")
	except OSError:
	    pass

    def test_noaccess(self):
	try:
	    name = self.tmpdir+"privatedir"
	    os.mkdir(name,0)
	    xlistdir(name)
	    self.fail("should raise OSError")
	except OSError:
	    pass
	os.rmdir(name)
	
    def test_notdir(self):
	try:
	    xlistdir(self.tmpdir+"0")
	    self.fail("should raise OSError")
	except OSError:
	    pass
	

main = unittest.main

if __name__=="__main__":
    unittest.main()
