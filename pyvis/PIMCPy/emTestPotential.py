#!/bin/env python
import numpy
##import pylab
import CalcStatistics
from PIMCHelp import *
import random
import numpy
from PIMC import *

embeddedDir = "pyvis/PIMCPy/"

def ScriptCreate() :
  tau=0.5
  lam=0.5
  Path=PathClass(ReadArray(embeddedDir + "CanonicalPath.txt"),tau,lam)
  Path.SetPotential(HarmonicOscillator)
  Path.SetCouplingConstant(0.0)
  print "The value of the external potential is", Path.Vext(numpy.array([0.1, 0.3, 0.1]))
  #print "The value of the external potential is", Path.Vext(1)

ScriptCreate()


