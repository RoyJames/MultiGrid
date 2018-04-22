// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see https://www.visualdatatools.com/DTSource/license.html for more information.

/*

 Functions to simplify parsing of input arguments.

 At the top of the main routine, call the initialization routine

 int main(int argc,const char *argv[])
 {
     DTSetArguments(argc,argv);
     ....
 }

 After that you use the query calls to parse through the argument list.

 */

#include <string>

// Needs to be called first.  Put this as the first line inside your main routine.
extern void DTSetArguments(int argc,const char *argv[]);

// Switches, such as "-Input File", and "-Debug" etc.
extern bool DTArgumentIncludesFlag(std::string flagName); // Checks for existence of -flag
extern std::string DTArgumentFlagEntry(std::string flagName);
extern int DTArgumentIntegerEntry(std::string flagName);

// Getting entries, one by one.
extern int DTHowManyArguments(void);
extern std::string DTGetArgumentNumber(int);

