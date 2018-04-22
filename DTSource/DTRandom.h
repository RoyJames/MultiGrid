// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see https://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTRandom_H
#define DTRandom_H

// The random number generator is the Mersenne Twister
// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html


// Usage:
// DTRandom randNumber; // Creates a random number object
// x = randNumber.UniformOpen();  // Get a single random number
// x = randNumber.Gamma(2.2,2.3);

// You can save and restore the state of the random number generator
// with the Read and Write functions, just like any other DTSource variable.

class DTDataStorage;
#include <string>
using namespace std;

class DTRandom {
public:
    DTRandom(unsigned long int seed = 5489UL);
    DTRandom(unsigned long int *mtPointer,int loc); // To restore the state.  Will copy the content of mtPointer.
    
    // Uniform distribution
    unsigned long int UInteger(void); // [0,2^32-1]
    double UniformClosed(void); // [0,1]
    double UniformOpen(void); // (0,1)
    double UniformHalf(void); // [0,1)
    double UniformHalf53(void); // [0,1) with 53 bit resolution.  Same sequence as matlab gives when you call rng(seed,'twister') and ask for rand()
    
    // Standard distributions
    double Beta(double a,double b);
    unsigned int Binomial(double p,int n);
    double Gamma(double a,double b);
    double Pareto(double a,double b);
    int Poisson(double mu);
    double Weibull(double a,double b);
    
    // List
    void Normal(double *values,int length);
    double Normal(double mean,double std);
    void Normal(double mean,double std,double *values,int length);
    void UniformHalf53(double *values,int length);

    // Raw access
    const unsigned long int *mtPointer(void) const {return mt;}
    int Position(void) const {return mti;}

    bool operator!=(const DTRandom &) const;

    
private:
    // Implementation functions
    double GammaInteger(int);
    double GammaFraction(double);
    double GammaLarge(double);

    // State description
    unsigned long int mt[624]; // the array for the state vector
    int mti;
};

// Reading and writing
extern void Read(const DTDataStorage &input,string name,DTRandom &toReturn);
extern void Write(DTDataStorage &output,string name,const DTRandom &theVar);

// The PDF functions for each distribution
extern double DTPDF_Beta(double x,double a,double b);
extern double DTPDF_Gamma(double x,double a,double b);
extern double DTPDF_Pareto(double x,double a,double b);
extern double DTPDF_Weibull(double x,double a,double b);

#endif

