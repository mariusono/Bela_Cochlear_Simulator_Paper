#include "BiquadChain.h"

#include <memory> // for unique_ptr
#include <vector>

// Constructor
BiquadChain::BiquadChain(std::vector<std::unique_ptr<BiquadFilter>> &biquads, int noBiquads) : biquads(biquads), noBiquads(noBiquads)
{
    outVals.resize(noBiquads,0);
}

// Destructor
BiquadChain::~BiquadChain(){
}



float BiquadChain::processSample(float inVal)
{
    for (int i = 0;i<noBiquads;i++)
    {
        if (i==0)
        {
            outVals[0] = biquads[i]->processSample(inVal);
        }
        else
        {
            outVals[i] = biquads[i]->processSample(outVals[i-1]);
        }
    }
    return outVals[noBiquads-1];
}

void BiquadChain::printCoefficients(){
    for (int i = 0;i<noBiquads;i++){
        biquads[i]->printCoefficients();
    }
}

void BiquadChain::resetState(){
    for (int i = 0;i<noBiquads;i++){
        biquads[i]->resetState();
    }
}
