// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see https://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTRandom.h"

#include "DTDataStorage.h"
#include "DTIntArray.h"
#include "DTError.h"

#include <math.h>
#include <cstring>

// The random number generator is from the Mersenne Twister
// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
// All that is done here is that there is a class structure used to store the state.


#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

unsigned long mag01[2]={0x0, MATRIX_A};

DTRandom::DTRandom(unsigned long int s)
{
    // Initializing the array with a seed
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<624; mti++) {
        mt[mti] = 
        (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

DTRandom::DTRandom(unsigned long int *mtPointerIn,int loc)
{
    std::memcpy(mt,mtPointerIn,624*sizeof(unsigned long int));
    mti = loc;
}

unsigned long int DTRandom::UInteger(void)
{
    // [0,2^32-1]
    unsigned long y;
    // static unsigned long mag01[2]={0x0UL, MATRIX_A};
    // mag01[x] = x * MATRIX_A  for x=0,1
    
    if (mti >= 624) { // generate N words at one time
        int kk;
        
        for (kk=0;kk<227;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+397] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<623;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk-227] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[623]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[623] = mt[396] ^ (y >> 1) ^ mag01[y & 0x1UL];
        
        mti = 0;
    }

    y = mt[mti++];

    // Tempering
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);
    
    return y;
}

double DTRandom::UniformClosed(void)
{
    return UInteger()*(1.0/4294967295.0); 
}

double DTRandom::UniformOpen(void)
{
    return (((double)UInteger()) + 0.5)*(1.0/4294967296.0); 
}

double DTRandom::UniformHalf(void)
{
    return UInteger()*(1.0/4294967296.0); 
}

void DTRandom::Normal(double *values,int length)
{
    // Move towards this method: http://en.wikipedia.org/wiki/Ziggurat_algorithm
    int i;
    double x,y;
    if (length<=0) return;
    
    for (i=0;i<length-1;i+=2) {
        x = sqrt(-2*log((4294967296.0 - UInteger())/4294967296.0));
        y = UInteger()*(2*M_PI/4294967295.0);
        values[i  ] = x*cos(y);
        values[i+1] = x*sin(y);
    }
    
    if (length%2==1) {
        x = sqrt(-2*log((4294967296.0 - UInteger())/4294967296.0));
        y = UInteger()*(2*M_PI/4294967295.0);
        values[length-1] = x*cos(y);
    }
}

void DTRandom::Normal(double mean,double std,double *values,int length)
{
    // Move towards this method: http://en.wikipedia.org/wiki/Ziggurat_algorithm
    int i;
    double x,y;
    if (length<=0) return;
    
    for (i=0;i<length-1;i+=2) {
        x = sqrt(-2*log((4294967296.0 - UInteger())/4294967296.0));
        y = UInteger()*(2*M_PI/4294967295.0);
        values[i  ] = mean+std*x*cos(y);
        values[i+1] = mean+std*x*sin(y);
    }
    
    if (length%2==1) {
        x = sqrt(-2*log((4294967296.0 - UInteger())/4294967296.0));
        y = UInteger()*(2*M_PI/4294967295.0);
        values[length-1] = mean+std*x*cos(y);
    }
}

double DTRandom::Normal(double mean,double std)
{
    double x = sqrt(-2*log((4294967296.0 - UInteger())/4294967296.0));
    double y = UInteger()*(2*M_PI/4294967295.0);
    return mean+std*x*cos(y);
}

double DTRandom::UniformHalf53(void)
{
    unsigned long a=UInteger()>>5, b=UInteger()>>6;
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
}

void DTRandom::UniformHalf53(double *values,int length)
{
    int i;
    unsigned long a,b;
    for (i=0;i<length;i++) {
        a=UInteger()>>5;
        b=UInteger()>>6;
        values[i] = (a*67108864.0+b)*(1.0/9007199254740992.0);
    }
}

double DTRandom::Beta(double a,double b)
{
    double x1 = Gamma(a,1.0);
    double x2 = Gamma(b,1.0);
    return x1/(x1+x2);
}

unsigned int DTRandom::Binomial(double p,int n)
{
    unsigned int a, b, k = 0;
    double X;
    
    while (n > 10) {
        a = 1+(n/2);
        b = 1+n-a;
        
        X = Beta(a,b);
        
        if (X >= p) {
            n = a-1;
            p /= X;
        }
        else {
            k += a;
            n = b-1;
            p = (p-X)/(1-X);
        }
    }
    
    int i;
    for (i=0;i<n;i++) {
        if (UniformClosed()<p) k++;
    }
    
    return k;
}

double DTRandom::GammaLarge(double a)
{
    double x,y,v;
    double sqa = sqrt (2.0*a-1);
    
    do {
        do {
            y = tan(M_PI*UniformOpen());
            x = sqa*y+a-1.0;
        } while (!(x>0));
        v = UniformOpen();
    } while (v>(1+y*y)*exp((a-1.0)*log(x/(a-1.0))-sqa*y));
    
    return x;
}

double DTRandom::GammaInteger(int na)
{
    if (na < 12) {
        int i;
        double product = 1.0;
        for (i=0;i<na;i++) {
            product *= UniformOpen();
        }
        return -log(product);
    }
    else {
        return GammaLarge(na);
    }
}

double DTRandom::GammaFraction(double a)
{
    double q,x,u,v;
    double p = M_E/(a+M_E);
    do {
        u = UniformOpen();
        v = UniformOpen(); // Not allowed to be 0.
        
        if (u < p) {
            x = exp((1/a)*log (v));
            q = exp(-x);
        }
        else {
            x = 1.0-log(v);
            q = exp((a-1.0)*log(x));
        }
    }
    while (UniformOpen() >= q);
    
    return x;
}

double DTRandom::Gamma(double a,double b)
{
    // Knuth, vol 2, 2nd ed, p. 129.
    unsigned int na = int(floor(a));
    double toReturn;
    
    if (a==na) {
        toReturn = b*GammaInteger(na);
    }
    else if (na==0) {
        toReturn = b*GammaFraction(a);
    }
    else {
        toReturn = b*(GammaInteger(na) + GammaFraction(a-na)) ;
    }
    
    return toReturn;
}

double DTRandom::Pareto(double a,double b)
{
    double x = UniformOpen();
    return b*pow(x,-1.0/a);
}

int DTRandom::Poisson(double mu)
{
    unsigned int k = 0;
    unsigned int m;
    double X;
    
    while (mu>10.0) {
        m = (unsigned int)(mu*(7.0/8.0));
        
        X = GammaInteger(m);
        
        if (X >= mu)
            return k + Binomial(mu/X,m-1);
        else {
            k += m;
            mu -= X; 
        }
    }
    
    double emu = exp(-mu);
    
    double prod = 1.0;
    do {
        prod *= UniformClosed();
        k++;
    } while (prod > emu);
    
    return k - 1;
}

double DTRandom::Weibull(double a,double b)
{
    double x = UniformOpen();
    return a*pow(-log(x),1.0/b);
}

bool DTRandom::operator!=(const DTRandom &c) const
{
    if (mti!=c.mti) return true;
    int i;
    for (i=0;i<624;i++)
        if (mt[i]!=c.mt[i]) break;
    return (i<624);
}

void Read(const DTDataStorage &input,string name,DTRandom &toReturn)
{
    DTIntArray theNumbers;
    Read(input,name,theNumbers);
    if (theNumbers.Length()!=624) {
        DTErrorMessage("Read(DTRandom)","Invalid length, need to have 624 numbers.");
        toReturn = DTRandom();
    }
    
    int mti;
    Read(input,name+"_loc",mti);
    if (mti<0) mti = 0;
    if (mti>624) mti = 624;
    
    toReturn = DTRandom((unsigned long int *)theNumbers.Pointer(),mti);
}

void Write(DTDataStorage &output,string name,const DTRandom &theVar)
{
    DTMutableIntArray theNumbers(624);
    std::memcpy(theNumbers.Pointer(),theVar.mtPointer(),theNumbers.Length()*sizeof(int));
    
    Write(output,name+"_loc",theVar.Position());
    Write(output,name,theNumbers);
}

double DTPDF_Beta(double x,double a,double b)
{
    // (Gamma(a+b)/(Gamma(a)Gamma(b)))x^(a-1)(1-x)^(b-1)
    if (x<0.0 || x>1.0) {
        return 0.0;
    }
    else {
        return exp(lgamma(a+b)-lgamma(a)-lgamma(b))*pow(x,a-1.0)*pow(1.0-x,b-1.0);
    }
}

double DTPDF_Gamma(double x,double a,double b)
{
    if (a==1.0) {
        if (x==0.0)
            return 1.0/b;
        else
            return exp(-x/b)/b;
    }
    else if (x<=0.0) {
        return 0.0;
    }
    else {
        return exp((a-1.0)*log(x/b)-x/b-lgamma(a))/b;
    }
}

double DTPDF_Pareto(double x,double a,double b)
{
    if (!(x<b))
        return (a/b)/pow(x/b,a+1.0);
    else
        return 0;
}

double DTPDF_Weibull(double x,double a,double b)
{
    // (b/a)(x/a)^(b-1)exp(-(x/a)^b)
    if (b==1.0) {
        if (x==0.0)
            return 1.0/a;
        else
            return exp(-x/a)/a;
    }
    else if (x<=0.0) {
        return 0.0;
    }
    else {
        return (b/a)*exp(-pow(x/a,b) + (b-1)*log(x/a));
    }
}

