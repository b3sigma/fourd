#!/bin/env python
import CalcStatistics
import numpy
import pylab
import random
from PIMC import *
from PIMCHelp import *

Path = None
myPIMC = None

tau=0.5
lam=0.5
numTimeSlices=5
numParticles=2

n = numpy.array(arange(0.0, 100.0, 1.0))
E = 0.5 + n
beta = tau * numTimeSlices
expBetaE = numpy.exp(-beta*E)
print 6.0*sum(E*expBetaE)/sum(expBetaE)
print 'Eclassical = %1.5f' % (12.0/beta)

Path=PathClass(numpy.zeros((numTimeSlices,numParticles,3),float),tau,lam)
Path.SetPotential(HarmonicOscillator)
Path.SetCouplingConstant(0.0)
numSteps = 300 # sort of crashy for arbitrary values of this currently
    
def CalcSingleSlicePotential() :
  energyTrace = PIMC(numSteps,Path,SingleSliceMove)
  return energyTrace

energyTraces = [[]] #ugh why a double list when it's not used
def ScriptCreate() :
  energyTraces[0].extend(CalcSingleSlicePotential())
  return energyTraces

# just trying these out
def ScriptAsyncStep() :
  energyTraces[0].extend(CalcSingleSlicePotential())
  return energyTraces

def ScriptStep() :
  return energyTraces
