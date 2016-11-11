#!/bin/env python
import numpy
#import pylab
import CalcStatistics
from PIMCHelp import *
import random
import numpy
from PIMC import *


tau=0.5
lam=0.5
numTimeSlices=5
numParticles=2
Path=PathClass(numpy.zeros((numTimeSlices,numParticles,3),float),tau,lam)
Path.SetPotential(ZeroFunction)
Path.SetCouplingConstant(0.0)
print PIMC(5000,Path,SingleSliceMove)
