#include "BiquadFilter.h"
#include <memory>
#include <vector>

class BiquadChain
{
public:

    BiquadChain(std::vector<std::unique_ptr<BiquadFilter>> &biquads, int noBiquads); // a VECTOR of POINTERS to BiquadFilter OBJECTS !
    ~BiquadChain();
    
    float processSample(float inVal);
    
    void printCoefficients();
    void resetState();

private:

    std::vector<std::unique_ptr<BiquadFilter>>  &biquads; // A vector of pointers to biquad filters !
    int noBiquads;

    std::vector<float> outVals;
};
