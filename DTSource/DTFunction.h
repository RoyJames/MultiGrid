// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#ifndef DTFunction_H
#define DTFunction_H

#include "DTPointer.h"
#include "DTList.h"
#include "DTIntArray.h"
#include "DTDoubleArray.h"

class DTFunctionStorage;
class DTFunction;
class DTDataStorage;
class DTDictionary;

struct DTFunctionLeaf
{
    DTFunctionLeaf() : type(Value), value(0.0), leafIsAList(false), listNumber(-1), constantNumber(-1), functionNumber(-1), leftReference(0), rightReference(0), functionReference(Identity) {}
    
    enum LeafType {Value=0,ListOfValues,Constant,Plus,Minus,Times,Divide,Power,PredefinedFunction,Function,Or,And,NotEqual,Equal,Less,Greater,LessEqual,GreaterEqual};
    
    enum FunctionName {Identity=0,Abs,Sin,Cos,ASin,ACos,Tan,ATan,Sinh,Cosh,Tanh,Sqrt,Gamma,LogGamma,Exp,Log,Log10,Log2,Erfc,Erf,Floor,Ceil,Round,Fact,Min,Max,Norm,SignSwitch,Angle,J0,J1,Jn,Y0,Y1,Yn,Mod,Sinc,Rect,Tri,H,Sgn,/*Seconds,*/Year,Month,Day,Hour,Minute,IsAM,IsPM,Log1p,Rem,Cbrt,If,Isnan,Isfinite};
    
    double Evaluate(const double *v,const DTFunctionStorage &storage) const; // Constants in, assumes the result is a single number
    DTMutableDoubleArray EvaluateArray(const double *v,const DTFunctionStorage &storage) const; // Constants in, result be a list of numbers
	DTMutableDoubleArray EvaluateFromLists(const DTList<DTDoubleArray> &,const DTFunctionStorage &storage) const; // each array has either a single number or a list (all of the same length)
    
    void outputIntoStream(std::ostream &,const DTFunctionStorage &) const;
    
    LeafType type;
    double value; // If type==Value
    bool leafIsAList;
    void ComputeIfThisLeafIsAList(DTMutableList<DTFunctionLeaf> &leafs);

    // Expression lists are stored as IntArrays in the storage.
    // Only used for multi-argument functions, such as min/max/signswitch
    // or functions that are defined with other DTFunction objects.
    int listNumber; // If Type==ListOfValues
    
    // Which constant is used.  For generic functions this will refer to a
    // a list of constant names.  For 1,2,3D functions, this is which argument
    // to use from the Evaluate statement.  The constant names are saved in the function definition as "constantNames"
    int constantNumber; // If type==Constant
    
    // references a function object.  This is the offset in the subFunctions list
    int functionNumber; // If type==Function
    
    // Functions that are used as arguments.  The rightReference is the argument list.
    FunctionName functionReference; // If type==PredefinedFunction
    
    // Leaf references are offsets into the leaf list and are used for the binary operators or PredefinedFunctions
    int leftReference,rightReference; // 0 means not used.  This is because 0 is the top leaf and no leaf can refer to itself or a lower number leaf.
    
};

class DTFunctionStorage
{
public:
    DTFunctionStorage() : leafs(DTMutableList<DTFunctionLeaf>(1)), argumentLists(), constantNames(), subFunctions(NULL), howManySubFunctions(0) {leafsD = leafs.Pointer();}
    DTFunctionStorage(const DTList<DTFunctionLeaf> &entries);
    ~DTFunctionStorage();
    
    // Function evaluation.
    double Evaluate(const double *v) const {return leafsD[0].Evaluate(v,*this);}
    DTMutableDoubleArray EvaluateFromLists(const DTList<DTDoubleArray> &v) const {return leafsD[0].EvaluateFromLists(v,*this);}
    
    // Entries in the expression tree.  The nodes in the tree can point to one or more trees 
    // and the nodes are all flattened here and referred to by index rather than a pointer.
    DTList<DTFunctionLeaf> leafs;
    const DTFunctionLeaf *leafsD;
    
    // Argument lists
    DTList<DTIntArray> argumentLists; // When functions have arguments the listNumber is set in the leaf.  This refers to an entry in this list.
    // That entry is then a list of top leafs in the "leafs" list.  Note that the arguments don't have names and are functions.
    
    // Constants/unknown
    DTList<std::string> constantNames; // A constant is a leaf that indicates that the value is unknown.  The leaf refers to that constant by number, and
    // this list indicates what the name of that constant is.  Note that for DTFunction2D for example this only contains two constants "x" and "y".
    // At the moment DT will only save 1D, 2D, 3D functions so that this functionality isn't used fully.  Internally DataTank and DataGrah
    // use this to handle general functions with multiple arguments.
    
    ssize_t HowManySubFunctions(void) const {return howManySubFunctions;}
    DTFunction SubFunction(ssize_t i) const;
    void SetSubFunctions(const DTFunction *,ssize_t);
    
    bool CheckValidity(void) const; // Consistency check, useful when you are adding operators.
    
private:
    // if "functionNumber" is specified in the leaf the entry of that value comes from a function object.  Those objects
    // are stored as DTFunction objects so that they can have their own argument lists/constants etc.  Because of how structures have to
    // be declared in C++ this is stored as a very raw list and not a DTList<DTFunction> object.  The functionNumber value in a leaf
    // indicates what entry in this pointer is being referenced.  The rightReference is then an argument list for the function.  The length
    // of that argument list needs to be the same as the number of constants defined in that function and defines those constants.
    // To see more about this look at the source code for the () operator that handles function subscripts.
    DTFunction *subFunctions;
    ssize_t howManySubFunctions;
    
    DTFunctionStorage(const DTFunctionStorage &);
    void operator=(const DTFunctionStorage &);
};

extern std::ostream &operator<<(std::ostream &,const DTFunctionStorage &);

class DTFunction
{
public:
    DTFunction() : content(new DTFunctionStorage()) {}
    explicit DTFunction(std::string);
    
    // Low level
    DTFunction(const DTPointer<DTFunctionStorage> &C);
    DTPointer<DTFunctionStorage> Content(void) const {return content;}
    
    DTFunction ChangeConstantOrder(const DTList<std::string> &) const;
    
    // This function will not check if the pointer is non-NULL or has the right size.
    // Intended as a fast, and called by higher level functions.
    double Evaluate(const double *v) const {return content->Evaluate(v);}
    // If you are evaluating a function at a number of points, consider using the following function.
    // Note however that this will create a lot of temporary arrays, which means allocations.
    DTMutableDoubleArray operator()(const DTList<DTDoubleArray> &v) const {return content->EvaluateFromLists(v);} // each array has either a single number or a list (all of the same length)
        
    DTFunction power(const DTFunction &) const;
    
    void pinfo(void) const;
    std::string StringVersion(void) const;
    
    DTList<std::string> ConstantNames(void) const {return content->constantNames;}
    
    static DTFunction Constant(std::string);
    static DTFunction Value(double);
    
    DTFunction InsertConstants(const DTDictionary &) const;
    
private:
    DTPointer<DTFunctionStorage> content;
};

DTFunction operator+(const DTFunction &,const DTFunction &);
DTFunction operator-(const DTFunction &,const DTFunction &);
DTFunction operator*(const DTFunction &,const DTFunction &);
DTFunction operator/(const DTFunction &,const DTFunction &);


extern void Read(const DTDataStorage &input,const std::string &name,DTFunction &var);
extern void Write(DTDataStorage &output,const std::string &name,const DTFunction &var);

extern std::ostream &operator<<(std::ostream &,const DTFunction &);

// Operators 
extern DTFunction operator-(const DTFunction &);

extern DTFunction operator*(const DTFunction &,double);
extern DTFunction operator*(double,const DTFunction &);
extern DTFunction operator*(const char *,const DTFunction &);

extern DTFunction operator/(const DTFunction &,double);
extern DTFunction operator/(double,const DTFunction &);

extern DTFunction operator+(const DTFunction &,double);
extern DTFunction operator+(double,const DTFunction &);
extern DTFunction operator+(const DTFunction &,const char *);
extern DTFunction operator+(const char *,const DTFunction &);

extern DTFunction operator-(const DTFunction &,double);
extern DTFunction operator-(double,const DTFunction &);


extern DTFunction operator<(const DTFunction &,const DTFunction &);
extern DTFunction operator>(const DTFunction &,const DTFunction &);
extern DTFunction operator<=(const DTFunction &,const DTFunction &);
extern DTFunction operator>=(const DTFunction &,const DTFunction &);
extern DTFunction operator||(const DTFunction &,const DTFunction &);
extern DTFunction operator&&(const DTFunction &,const DTFunction &);

// Functions that are used to assemble a function.  This can be used to create
// a function that should be returned back to DataTank.
extern DTFunction abs(const DTFunction &);
extern DTFunction sin(const DTFunction &);
extern DTFunction cos(const DTFunction &);
extern DTFunction asin(const DTFunction &);
extern DTFunction acos(const DTFunction &);
extern DTFunction tan(const DTFunction &);
extern DTFunction atan(const DTFunction &);
extern DTFunction sinh(const DTFunction &);
extern DTFunction cosh(const DTFunction &);
extern DTFunction tanh(const DTFunction &);
extern DTFunction sqrt(const DTFunction &);
extern DTFunction gamma(const DTFunction &);
extern DTFunction loggamma(const DTFunction &);
extern DTFunction exp(const DTFunction &);
extern DTFunction log(const DTFunction &);
extern DTFunction log10(const DTFunction &);
extern DTFunction erfc(const DTFunction &);
extern DTFunction erf(const DTFunction &);
extern DTFunction floor(const DTFunction &);
extern DTFunction ceil(const DTFunction &);
extern DTFunction round(const DTFunction &);
extern DTFunction fact(const DTFunction &);
extern DTFunction pow(const DTFunction &,double);
extern DTFunction j0(const DTFunction &);
extern DTFunction j1(const DTFunction &);
extern DTFunction y0(const DTFunction &);
extern DTFunction y1(const DTFunction &);

#endif
