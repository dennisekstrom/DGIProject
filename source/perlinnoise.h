//
//  perlinnoise.h
//  DGIProject
//
//  Created by Tobias Wikström on 17/05/14.
//  Copyright (c) 2014 Tobias Wikström. All rights reserved.
//

#ifndef __DGIProject__perlinnoise__
#define __DGIProject__perlinnoise__

class PerlinNoise
{
public:
    
    // Constructor
    PerlinNoise();
    PerlinNoise(double _persistence, double _frequency, double _amplitude, int _octaves, int _randomseed);
    
    // Get Height
    double GetHeight(double x, double y) const;
    
    // Set parameters
    void SetParams(double _persistence, double _frequency, double _amplitude, int _octaves, int _randomseed);
    
    void SetPersistence(double _persistence) { persistence = _persistence; }
    void SetFrequency(  double _frequency)   { frequency = _frequency;     }
    void SetAmplitude(  double _amplitude)   { amplitude = _amplitude;     }
    void SetOctaves(    int    _octaves)     { octaves = _octaves;         }
    void SetRandomSeed( int    _randomseed)  { seed = _randomseed;         }
    
    // Get parameters
    double Persistence() const { return persistence; }
    double Frequency()   const { return frequency;   }
    double Amplitude()   const { return amplitude;   }
    int    Octaves()     const { return octaves;     }
    int    RandomSeed()  const { return seed;        }
    
    
private:
    double persistence, frequency, amplitude;
    int octaves, seed;
    
    double GetValue(double x, double y) const;
    double Interpolate(double x, double y, double a) const;
    double GenNoise(int x, int y) const;
    
};

#endif /* defined(__DGIProject__perlinnoise__) */
