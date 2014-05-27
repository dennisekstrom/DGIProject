//
//  perlinnoise.cpp
//  DGIProject
//
//  Created by Tobias Wikström on 17/05/14.
//  Copyright (c) 2014 Tobias Wikström. All rights reserved.
//
// https://code.google.com/p/fractalterraingeneration/wiki/Perlin_Noise

#include "perlinnoise.h"
#include <iostream>
#include <cmath>


PerlinNoise::PerlinNoise()
{
    persistence = 0;
    frequency = 0;
    amplitude  = 0;
    octaves = 0;
    seed = 0;
}

PerlinNoise::PerlinNoise(double _persistence, double _frequency, double _amplitude, int _octaves, int _randomseed)
{
    persistence = _persistence;
    frequency = _frequency;
    amplitude  = _amplitude;
    octaves = _octaves;
    seed = _randomseed;
}

void PerlinNoise::SetParams(double _persistence, double _frequency, double _amplitude, int _octaves, int _randomseed)
{
    persistence = _persistence;
    frequency = _frequency;
    amplitude  = _amplitude;
    octaves = _octaves;
    seed = _randomseed;
}


double PerlinNoise::GetHeight(double x, double y) const
{
    double height = 0.0f;
    double _changing_amp = 1;
    double _freq = frequency;
    
    //Iterate through the octaves, applying all the parameters to the noise value
    for(int i = 0; i < octaves; i++)
    {
        height += GetValue(y * _freq + seed, x * _freq + seed) * _changing_amp;
        _changing_amp *= persistence;
        _freq *= 2; //increase frequency two-folds for each octave iteration
    }
    
    return height * amplitude;
}

double PerlinNoise::GetValue(double x, double y) const
{
    int Xint = (int)x;
    int Yint = (int)y;
    double Xfrac = x - Xint;
    double Yfrac = y - Yint;
    
    //noise values
    double n01 = Noise(Xint-1, Yint-1);
    double n02 = Noise(Xint+1, Yint-1);
    double n03 = Noise(Xint-1, Yint+1);
    double n04 = Noise(Xint+1, Yint+1);
    double n05 = Noise(Xint-1, Yint);
    double n06 = Noise(Xint+1, Yint);
    double n07 = Noise(Xint, Yint-1);
    double n08 = Noise(Xint, Yint+1);
    double n09 = Noise(Xint, Yint);
    
    double n12 = Noise(Xint+2, Yint-1);
    double n14 = Noise(Xint+2, Yint+1);
    double n16 = Noise(Xint+2, Yint);
    
    double n23 = Noise(Xint-1, Yint+2);
    double n24 = Noise(Xint+1, Yint+2);
    double n28 = Noise(Xint, Yint+2);
    
    double n34 = Noise(Xint+2, Yint+2);
    
    //find the noise values/gradient of the four corners
    double x0y0 = 0.0625*(n01+n02+n03+n04) + 0.125*(n05+n06+n07+n08) + 0.25*(n09);
    double x1y0 = 0.0625*(n07+n12+n08+n14) + 0.125*(n09+n16+n02+n04) + 0.25*(n06);
    double x0y1 = 0.0625*(n05+n06+n23+n24) + 0.125*(n03+n04+n09+n28) + 0.25*(n08);
    double x1y1 = 0.0625*(n09+n16+n28+n34) + 0.125*(n08+n14+n06+n24) + 0.25*(n04);
    
    //interpolate between those values according to the x and y fractions
    double v1 = Interpolate(x0y0, x1y0, Xfrac); //interpolate in x direction (y)
    double v2 = Interpolate(x0y1, x1y1, Xfrac); //interpolate in x direction (y+1)
    double fin = Interpolate(v1, v2, Yfrac);  //interpolate in y direction
    
    return fin;
}

// Cosine interpolation, for smooth interpolation
// http://paulbourke.net/miscellaneous/interpolation/
double PerlinNoise::Interpolate(double a, double b, double mu) const
{
    double PI = 3.1415927;
    double mu2=(1.0-cos(mu*PI))* 0.5;
    return a*(1.0-mu2)+b*mu2;

}

// Base function for noise, seems to be a quite standardized way of generating the noise.
// Basically a pseudo random generator with two inputs, returning a value between -1 and 1.
double PerlinNoise::Noise(int x, int y) const
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    int t = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return 1.0 - double(t) * 0.931322574615478515625e-9;
}