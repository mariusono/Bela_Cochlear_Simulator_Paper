#include <iostream>
#include "BiquadFilter.h"

//==============================================================================
// Constructor
BiquadFilter::BiquadFilter(float b1, float b2, float b3, float a1, float a2, float a3) : a1(a1), a2(a2), a3(a3), b1(b1), b2(b2), b3(b3)
{
//    this->a1 = a1;
//    this->a2 = a2;
//    this->a3 = a3;
//    this->b1 = b1;
//    this->b2 = b2;
//    this->b3 = b3;
    
    a1_prev = a1;
    a2_prev = a2;
    a3_prev = a3;
    b1_prev = b1;
    b2_prev = b2;
    b3_prev = b3;
    
    z1 = 0;
    z2 = 0;

//    inVal_m1 = 0;
//    inVal_m2 = 0;
//    outVal_m1 = 0;
//    outVal_m2 = 0;
    
    
}

// Destructor
BiquadFilter::~BiquadFilter(){
}


void BiquadFilter::resetState(){
    z1 = 0;
    z2 = 0;
    
//    inVal_m1 = 0;
//    inVal_m2 = 0;
//    outVal_m1 = 0;
//    outVal_m2 = 0;
}

void BiquadFilter::printCoefficients(){
    printf("%f,%f,%f,%f,%f,%f\n",a1,a2,a3,b1,b2,b3);
}


void BiquadFilter::assignCoeffs(float b1_in, float b2_in, float b3_in, float a1_in, float a2_in, float a3_in)
{
    a1 = a1_in;
    a2 = a2_in;
    a3 = a3_in;
    b1 = b1_in;
    b2 = b2_in;
    b3 = b3_in;
}

void BiquadFilter::assignCoeffs_smooth(float b1_in, float b2_in, float b3_in, float a1_in, float a2_in, float a3_in)
{
    float s = 0.92;
    
    a1_prev = a1;
    a2_prev = a2;
    a3_prev = a3;
    b1_prev = b1;
    b2_prev = b2;
    b3_prev = b3;
    
    a1 = (1-s) * a1_in + s*a1_prev;
    a2 = (1-s) * a2_in + s*a2_prev;
    a3 = (1-s) * a3_in + s*a3_prev;
    b1 = (1-s) * b1_in + s*b1_prev;
    b2 = (1-s) * b2_in + s*b2_prev;
    b3 = (1-s) * b3_in + s*b3_prev;
    
//    int ana = 3;
}



float BiquadFilter::processSample(float inVal){
//    outVal = b1 * inVal + b2 * inVal_m1 + b3 * inVal_m2 - a2*outVal_m1 - a3*outVal_m2;
//    outVal = (b1 * inVal + b2 * inVal_m1 + b3 * inVal_m2 - a2*outVal_m1 - a3*outVal_m2) / a1;
//
//    // update samples
//    inVal_m2 = inVal_m1;
//    inVal_m1 = inVal;
//
//    outVal_m2 = outVal_m1;
//    outVal_m1 = outVal;

    outVal = inVal * b1 + z1;

    z1 = inVal * b2 + z2 - a2 * outVal;
    z2 = inVal * b3 - a3 * outVal;

    return outVal;
}

