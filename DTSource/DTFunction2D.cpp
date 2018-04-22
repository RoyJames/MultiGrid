// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTFunction2D.h"

#include "DTFunction1D.h"
#include "DTDataStorage.h"
#include "DTError.h"

DTFunction2D::DTFunction2D(const DTFunction &f)
{
    // Need to have the arguments (x,y)
    
    // In order for this to valid, it is required to have only an x argument.
    if (f.ConstantNames().Length()>2) {
        DTErrorMessage("DTFunction2D(DTFunction)","Can only have two unknown arguments");
        fcn = DTFunction();
    }
    else if (f.ConstantNames().Length()==1 && f.ConstantNames()(0)!="x" && f.ConstantNames()(0)!="y") {
        DTErrorMessage("DTFunction2D(DTFunction)","Can only depend on \"x\" and \"y\"");
        fcn = DTFunction();
    }
    else if (f.ConstantNames().Length()==2 && (f.ConstantNames()(0)!="x" || f.ConstantNames()(1)!="y")) {
        DTErrorMessage("DTFunction2D(DTFunction)","Can only depend on \"x\" and \"y\"");
        fcn = DTFunction();
    }
    else {
        DTMutableList<std::string> order(2);
        order(0) = "x";
        order(1) = "y";
        fcn = f.ChangeConstantOrder(order);
    }
}

void DTFunction2D::pinfo(void) const
{
    fcn.pinfo();
}

DTFunction2D DTFunction2D::operator()(const DTFunction2D &f,const DTFunction2D &g)
{
    // fcn takes in two arguments which it wants to call x and y.  For example fcn = x^2 + exp(x*y)
    // What I should return is fcn(f,g) where f and g are inserted in as arguments and fcn is now a sub-function here (the only sub function).
    
    // The leafs are as follows
    // leaf 0 = a function leaf that calls an externally defined function - this->fcn
    // leaf 1->M the M leafs that are contained in f
    // leaf M+1->M+N the N leafs that are contained in g
    
    // The new argumentList is as follows
    // First comes the argument list for fcn.  That is two entries with entries 1 (for x) and M+1 (for y)
    // Then the argument lists for the leafs from f (shifted by 1)
    // Then the argument lists for the leafs from g (shifted by M+1)
    
    // constantNames is the union of the constants from f and g and is "x", "y" or "x,y" depending on what is defined in f and g

    // The sub functions are
    // First is this function
    // Next come the sub functions from f, followed by the sub functions from g.
    
    
    int lenA,lenB,pos,posA,posB;
    
    const DTFunctionStorage &A = *f.Function().Content();
    const DTFunctionStorage &B = *g.Function().Content();
    
    /////////////////////////////////////////////////////////////////////////////
    // Combine the sub functions.
    lenA = int(A.HowManySubFunctions());
    lenB = int(B.HowManySubFunctions());
    DTMutableList<DTFunction> combinedFunctions(1+lenA+lenB);
    size_t offsetAFunctions = 1;
    size_t offsetBFunctions = lenA+offsetAFunctions;
    combinedFunctions(0) = Function();
    for (posA=0;posA<lenA;posA++)
        combinedFunctions(offsetAFunctions+posA) = A.SubFunction(posA);
    for (posB=0;posB<lenB;posB++)
        combinedFunctions(offsetBFunctions+posB) = B.SubFunction(posB);
    
    
    /////////////////////////////////////////////////////////////////////////////
    // Combine the constants.  Should put them in alphabetical order, and remove duplicates.
    lenA = int(A.constantNames.Length());
    lenB = int(B.constantNames.Length());
    DTMutableIntArray AConstantsToNewConstants(lenA);
    DTMutableIntArray BConstantsToNewConstants(lenB);
    pos = posA = posB = 0;
    // Merge the lists.
    while (posA<lenA && posB<lenB) {
        if (A.constantNames(posA)==A.constantNames(posB)) {
            AConstantsToNewConstants(posA++) = pos;
            BConstantsToNewConstants(posB++) = pos;
        }
        else if (A.constantNames(posA)<A.constantNames(posB)) {
            AConstantsToNewConstants(posA++) = pos;
        }
        else {
            BConstantsToNewConstants(posB++) = pos;
        }
        pos++;
    }
    while (posA<lenA) {
        AConstantsToNewConstants(posA++) = pos++;
    }
    while (posB<lenB) {
        BConstantsToNewConstants(posB++) = pos++;
    }
    DTMutableList<std::string> combinedConstants(pos);
    for (posA=0;posA<lenA;posA++)
        combinedConstants(AConstantsToNewConstants(posA)) = A.constantNames(posA);
    for (posB=0;posB<lenB;posB++)
        combinedConstants(BConstantsToNewConstants(posB)) = B.constantNames(posB);
    
    
    /////////////////////////////////////////////////////////////////////////////
    // Combine the argument lists
    lenA = int(A.argumentLists.Length());
    lenB = int(B.argumentLists.Length());
    DTMutableList<DTIntArray> combinedLists(1+lenA+lenB);
    // combinedLists(0) is created after the leafs
    
    int offsetALists = 1;
    int offsetBLists = lenA+offsetALists;
    
    DTMutableIntArray temp;
    int j,len;
    
    // This could be done by newLists(i) = A.argumentLists(i)+1
    // but inline it here to avoid dependence on the DTIntArrayOperator.h header
    for (posA=0;posA<lenA;posA++) {
        temp = A.argumentLists(posA).Copy();
        len = int(temp.Length());
        for (j=0;j<len;j++) {
            temp(j) += 2;
        }
        combinedLists(offsetALists+posA) = temp;
    }
    for (posB=0;posB<lenB;posB++) {
        temp = B.argumentLists(posB).Copy();
        len = int(temp.Length());
        int addThis = int(A.leafs.Length()+2);
        for (j=0;j<len;j++) {
            temp(j) += addThis;
        }
        combinedLists(offsetBLists+posB) = temp;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    // Combine the leafs.
    DTList<DTFunctionLeaf> Atree = A.leafs;
    DTList<DTFunctionLeaf> Btree = B.leafs;
    
    lenA = int(Atree.Length());
    lenB = int(Btree.Length());
    DTMutableList<DTFunctionLeaf> combinedTrees(lenA+lenB+2);
    
    DTFunctionLeaf singleLeaf;
    pos = 2;
    int offsetALeafs = 2;
    for (posA=0;posA<lenA;posA++) {
        singleLeaf = Atree(posA);
        
        if (singleLeaf.type==DTFunctionLeaf::Constant)
            singleLeaf.constantNumber = AConstantsToNewConstants(singleLeaf.constantNumber);
        
        // The argument lists have changed
        if (singleLeaf.type==DTFunctionLeaf::ListOfValues)
            singleLeaf.listNumber += offsetALists;
        
        // The sub function references have changed.
        if (singleLeaf.type==DTFunctionLeaf::Function)
            singleLeaf.functionNumber += offsetAFunctions;
        
        // Need to shift the references.
        if (singleLeaf.leftReference) singleLeaf.leftReference+=offsetALeafs;
        if (singleLeaf.rightReference) singleLeaf.rightReference+=offsetALeafs;
        combinedTrees(pos++) = singleLeaf;
    }
    
    int offsetBLeafs = lenA+offsetALeafs;
    for (posB=0;posB<lenB;posB++) {
        singleLeaf = Btree(posB);
        
        if (singleLeaf.type==DTFunctionLeaf::Constant)
            singleLeaf.constantNumber = BConstantsToNewConstants(singleLeaf.constantNumber);
        
        if (singleLeaf.type==DTFunctionLeaf::ListOfValues)
            singleLeaf.listNumber += offsetBLists;
        
        if (singleLeaf.type==DTFunctionLeaf::Function)
            singleLeaf.functionNumber += offsetBFunctions;
        
        if (singleLeaf.leftReference) singleLeaf.leftReference+=offsetBLeafs;
        if (singleLeaf.rightReference) singleLeaf.rightReference+=offsetBLeafs;
        combinedTrees(pos++) = singleLeaf;
    }
    
    
    /////////////////////////////////////////////////////////////////////////////
    // The top leaf is a function call
    singleLeaf = DTFunctionLeaf();
    singleLeaf.type = DTFunctionLeaf::Function;
    singleLeaf.leftReference = 1;
    singleLeaf.functionNumber = 0;
    combinedTrees(0) = singleLeaf;

    // The second leaf is a list of entries (two), which is the argument for the function
    singleLeaf = DTFunctionLeaf();
    singleLeaf.type = DTFunctionLeaf::ListOfValues;
    singleLeaf.listNumber = 0;
    combinedTrees(1) = singleLeaf;
    
    DTMutableIntArray list(2);
    list(0) = offsetALeafs;
    list(1) = offsetBLeafs;
    combinedLists(0) = list;

    
    /////////////////////////////////////////////////////////////////////////////
    DTMutablePointer<DTFunctionStorage> toReturn(new DTFunctionStorage());
    
    toReturn->leafs = combinedTrees;
    toReturn->argumentLists = combinedLists;
    toReturn->constantNames = combinedConstants;
    toReturn->SetSubFunctions(combinedFunctions.Pointer(),combinedFunctions.Length());

    toReturn->CheckValidity();

    return DTFunction(toReturn);
}

void Read(const DTDataStorage &input,const std::string &name,DTFunction2D &toReturn)
{
    DTFunction fcn;
    Read(input,name,fcn);
    toReturn = DTFunction2D(fcn);
}

void Write(DTDataStorage &output,const std::string &name,const DTFunction2D &theVar)
{
    Write(output,name,theVar.Function());
}

void WriteOne(DTDataStorage &output,const std::string &name,const DTFunction2D &toWrite)
{
    Write(output,name,toWrite);
    Write(output,"Seq_"+name,"2D Function");
    output.Flush();
}

DTFunction2D operator+(const DTFunction2D &A,const DTFunction2D &B)
{
    return DTFunction2D(A.Function()+B.Function());
}

DTFunction2D operator-(const DTFunction2D &A,const DTFunction2D &B)
{
    return DTFunction2D(A.Function()-B.Function());
}

DTFunction2D operator*(const DTFunction2D &A,const DTFunction2D &B)
{
    return DTFunction2D(A.Function()*B.Function());
}

DTFunction2D operator/(const DTFunction2D &A,const DTFunction2D &B)
{
    return DTFunction2D(A.Function()/B.Function());
}

DTFunction2D operator+(const DTFunction1D &A,const DTFunction2D &B)
{
    return DTFunction2D(A.Function()+B.Function());
}

DTFunction2D operator-(const DTFunction1D &A,const DTFunction2D &B)
{
    return DTFunction2D(A.Function()-B.Function());
}

DTFunction2D operator*(const DTFunction1D &A,const DTFunction2D &B)
{
    return DTFunction2D(A.Function()*B.Function());
}

DTFunction2D operator/(const DTFunction1D &A,const DTFunction2D &B)
{
    return DTFunction2D(A.Function()/B.Function());
}

DTFunction2D operator+(const DTFunction2D &A,const DTFunction1D &B)
{
    return DTFunction2D(A.Function()+B.Function());
}

DTFunction2D operator-(const DTFunction2D &A,const DTFunction1D &B)
{
    return DTFunction2D(A.Function()-B.Function());
}

DTFunction2D operator*(const DTFunction2D &A,const DTFunction1D &B)
{
    return DTFunction2D(A.Function()*B.Function());
}

DTFunction2D operator/(const DTFunction2D &A,const DTFunction1D &B)
{
    return DTFunction2D(A.Function()/B.Function());
}

DTFunction2D operator-(const DTFunction2D &A)
{
    return DTFunction2D(-A.Function());
}

DTFunction2D operator*(const DTFunction2D &A,double b)
{
    return DTFunction2D(A.Function()*b);
}

DTFunction2D operator*(double a,const DTFunction2D &B)
{
    return DTFunction2D(a*B.Function());
}

DTFunction2D operator/(const DTFunction2D &A,double b)
{
    return DTFunction2D(A.Function()/b);
}

DTFunction2D operator/(double a,const DTFunction2D &B)
{
    return DTFunction2D(a/B.Function());
}

DTFunction2D operator+(const DTFunction2D &A,double b)
{
    return DTFunction2D(A.Function()+b);
}

DTFunction2D operator+(double a,const DTFunction2D &B)
{
    return DTFunction2D(a+B.Function());
}

DTFunction2D operator-(const DTFunction2D &A,double b)
{
    return DTFunction2D(A.Function()-b);
}

DTFunction2D operator-(double a,const DTFunction2D &B)
{
    return DTFunction2D(a-B.Function());
}

DTFunction2D abs(const DTFunction2D &F)
{
    return DTFunction2D(abs(F.Function()));
}

DTFunction2D sin(const DTFunction2D &F)
{
    return DTFunction2D(sin(F.Function()));
}

DTFunction2D cos(const DTFunction2D &F)
{
    return DTFunction2D(cos(F.Function()));
}

DTFunction2D asin(const DTFunction2D &F)
{
    return DTFunction2D(asin(F.Function()));
}

DTFunction2D acos(const DTFunction2D &F)
{
    return DTFunction2D(acos(F.Function()));
}

DTFunction2D tan(const DTFunction2D &F)
{
    return DTFunction2D(tan(F.Function()));
}

DTFunction2D atan(const DTFunction2D &F)
{
    return DTFunction2D(atan(F.Function()));
}

DTFunction2D sinh(const DTFunction2D &F)
{
    return DTFunction2D(sinh(F.Function()));
}

DTFunction2D cosh(const DTFunction2D &F)
{
    return DTFunction2D(cosh(F.Function()));
}

DTFunction2D tanh(const DTFunction2D &F)
{
    return DTFunction2D(tanh(F.Function()));
}

DTFunction2D sqrt(const DTFunction2D &F)
{
    return DTFunction2D(sqrt(F.Function()));
}

DTFunction2D gamma(const DTFunction2D &F)
{
    return DTFunction2D(gamma(F.Function()));
}

DTFunction2D loggamma(const DTFunction2D &F)
{
    return DTFunction2D(loggamma(F.Function()));
}

DTFunction2D exp(const DTFunction2D &F)
{
    return DTFunction2D(exp(F.Function()));
}

DTFunction2D log(const DTFunction2D &F)
{
    return DTFunction2D(log(F.Function()));
}

DTFunction2D log10(const DTFunction2D &F)
{
    return DTFunction2D(log10(F.Function()));
}

DTFunction2D erfc(const DTFunction2D &F)
{
    return DTFunction2D(erfc(F.Function()));
}

DTFunction2D floor(const DTFunction2D &F)
{
    return DTFunction2D(floor(F.Function()));
}

DTFunction2D ceil(const DTFunction2D &F)
{
    return DTFunction2D(ceil(F.Function()));
}

DTFunction2D round(const DTFunction2D &F)
{
    return DTFunction2D(round(F.Function()));
}

DTFunction2D fact(const DTFunction2D &F)
{
    return DTFunction2D(fact(F.Function()));
}

DTFunction2D pow(const DTFunction2D &F,double p)
{
    return DTFunction2D(pow(F.Function(),p));
}

