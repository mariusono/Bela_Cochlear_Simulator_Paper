#pragma once


//==============================================================================
/*
*/
class BiquadFilter
{
public:
    BiquadFilter(float b1, float b2, float b3, float a1, float a2, float a3);
    ~BiquadFilter();
    
    void resetState();
    void assignCoeffs(float b1, float b2, float b3, float a1, float a2, float a3);
    void assignCoeffs_smooth(float b1_in, float b2_in, float b3_in, float a1_in, float a2_in, float a3_in);
    void printCoefficients();
    float processSample(float inVal);

private:
    
    float outVal;
    float z1,z2;
    float a1,a2,a3,b1,b2,b3;
    
    float a1_prev,a2_prev,a3_prev,b1_prev,b2_prev,b3_prev;

};
