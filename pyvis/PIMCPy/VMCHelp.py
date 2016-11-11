from numpy import *
import random
import numpy
##import pylab
from math import *


def dist(x,y):
    return sqrt(numpy.dot(x-y,x-y))
class Histogram:
        min=0.0
        max=5.0
        delta=0.05
        numSamples=0
        bins=numpy.array([0])
        def add(self,val):
                if self.min<val and val<self.max:
                        index=int((val-self.min)/self.delta)
                        self.bins[index]=self.bins[index]+1
                self.numSamples=self.numSamples+1
        def printMe(self):
                for i in range(0,len(self.bins)):
                        print self.min+self.delta*i,self.bins[i]/(self.numSamples+0.0)

 #       def plotMe(self,fileName=""):
 #               print "plotting"
 #               pylab.clf()
 #               self.bins=self.bins/self.numSamples
 #               xCoord=[self.min+self.delta*i for i in range(0,len(self.bins))]
 #               pylab.plot(xCoord,self.bins)
 #               pylab.gca().xaxis.major.formatter.set_scientific(False)
 #               if not(fileName==""):
 #                  pylab.savefig(fileName)
 #               else:
 #                  pylab.show()
 #       def plotMeNorm(self,fileName):
 #               print "plotting"
 #               pylab.clf()
 #               self.bins=self.bins/self.numSamples
 #               xCoord=numpy.array([self.min+self.delta*i for i in range(0,len(self.bins))])
 #               pylab.plot(xCoord,self.bins/(xCoord*xCoord+0.0001))
 #               pylab.gca().xaxis.major.formatter.set_scientific(False)
 #               pylab.savefig(fileName)
 #               pylab.show()

        def __init__(self,min,max,numBins):
                self.min=min
                self.max=max
                self.delta=(max-min)/(numBins+0.0)
                self.bins=numpy.zeros(numBins)+0.0
                numSamples=0
def LaplacianPsiOverPsi(coords,WaveFunction):
    total=0.0
    delta=0.0001
    tempVal3=WaveFunction(coords)
    for i in range(0,len(coords)):
        for j in range(0,len(coords[0])):
            coords[i,j]=coords[i,j]+delta
            tempVal=WaveFunction(coords)
            coords[i,j]=coords[i,j]-2*delta
            tempVal2=WaveFunction(coords)
            coords[i,j]=coords[i,j]+delta
#            print ((tempVal+tempVal2)-2.0*tempVal3)/(delta*delta)
            total=total+((tempVal+tempVal2)-2.0*tempVal3)/(delta*delta)
    return total/tempVal3

def ZeroFunction(R,c=1.0):
   return 0.0


def GetRandomUniformVec(sigma,vec):
    vec[0]=2.0*sigma*(random.random()-0.5)
    vec[1]=2.0*sigma*(random.random()-0.5)
    vec[2]=2.0*sigma*(random.random()-0.5)
#def GetRandomUniform(sigma,ndim):
#       return numpy.array([(random.random()-0.5)*2*sigma for _ in range(0,ndim)])

def SetBondLength(bondLength):
    ions=numpy.zeros((2,3))+0.0
    ions[0]=[-0.5*bondLength, 0.0,0.0]
    ions[1]=[0.5*bondLength,0.0,0.0]
    return ions
