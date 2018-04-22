// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTRegion2D.h"

#include "DTDoubleArray.h"
#include "DTDataStorage.h"
#include "DTError.h"
#include "DTDataStorage.h"

#include <math.h>
#include <limits>

void DTRegion2D::pinfo(void) const
{
    std::cerr << "[" << xmin << ", " << xmax << "] x [" << ymin << ", " << ymax << "]\n" << std::flush;
}

bool operator==(const DTRegion2D &A,const DTRegion2D &B)
{
    return (A.isSet==B.isSet && A.xmin==B.xmin && A.xmax==B.xmax && A.ymin==B.ymin && A.ymax==B.ymax);
}

bool operator!=(const DTRegion2D &A,const DTRegion2D &B)
{
    return (A.isSet!=B.isSet || A.xmin!=B.xmin || A.xmax!=B.xmax || A.ymin!=B.ymin || A.ymax!=B.ymax);
}

DTRegion2D BoundingBox2D(const DTDoubleArray &points)
{
    if (points.IsEmpty())
        return DTRegion2D();
    if (points.m()!=2) {
        DTErrorMessage("BoundingBox(Array)","The array had an incorrect size.");
        return DTRegion2D();
    }

    ssize_t len = points.Length();

#if defined(WIN32) && !defined(INFINITY)
#define INFINITY std::numeric_limits<float>::infinity();
#endif
    double xmin,ymin,xmax,ymax;
    xmin = ymin = INFINITY;
    xmax = ymax = -INFINITY;

    double x,y;
    int i;

    const double *pointD = points.Pointer();

    for (i=0;i<len;i+=2) {
        x = pointD[i];
        y = pointD[i+1];
        if (isfinite(x) && isfinite(y)) {
            // Rule in DataTank is that the bounding box should exclude non-finite points
            // even if only one of the grid points is non-finite.
            xmin = (x < xmin ? x : xmin);
            xmax = (xmax < x ? x : xmax);
            ymin = (y < ymin ? y : ymin);
            ymax = (ymax < y ? y : ymax);
        }
    }

    if (xmin>xmax || ymin>ymax)
        return DTRegion2D();
    else
        return DTRegion2D(DTPoint2D(xmin,ymin),DTPoint2D(xmax,ymax));
}

DTRegion2D Union(const DTRegion2D &A,const DTRegion2D &B)
{
    if (A.isSet==false) return B;
    if (B.isSet==false) return A;

    DTPoint2D ll(std::min(A.xmin,B.xmin),std::min(A.ymin,B.ymin));
    DTPoint2D ur(std::max(A.xmax,B.xmax),std::max(A.ymax,B.ymax));

    return DTRegion2D(ll,ur);
}

DTRegion2D Intersection(const DTRegion2D &A,const DTRegion2D &B)
{
	double xmin = std::max(A.xmin,B.xmin);
	double xmax = std::min(A.xmax,B.xmax);
	double ymin = std::max(A.ymin,B.ymin);
	double ymax = std::min(A.ymax,B.ymax);
	
	if (xmax<xmin || ymax<ymin) return DTRegion2D(DTPoint2D(0,0),DTPoint2D(0,0));
	return DTRegion2D(DTPoint2D(xmin,ymin),DTPoint2D(xmax,ymax));
}

DTRegion2D AddBorder(const DTRegion2D &b,double d)
{
    if (b.IsEmpty()) return b;
    return DTRegion2D(b.LowerLeft()-DTPoint2D(d,d),b.UpperRight()+DTPoint2D(d,d));
}

void Read(const DTDataStorage &input,const std::string &name,DTRegion2D &region)
{
    DTDoubleArray onDisk = input.ReadDoubleArray(name);
    region = DTRegion2D();
    if (onDisk.Length()==4) {
        region.isSet = true;
        region.xmin = onDisk(0);
        region.xmax = onDisk(1);
        region.ymin = onDisk(2);
        region.ymax = onDisk(3);
    }
}

void Write(DTDataStorage &output,const std::string &name,const DTRegion2D &region)
{
    DTMutableDoubleArray toSave;
    
    if (region.isSet) {
        toSave = DTMutableDoubleArray(4);
        toSave(0) = region.xmin;
        toSave(1) = region.xmax;
        toSave(2) = region.ymin;
        toSave(3) = region.ymax;
    }

    output.Save(toSave,name);
}

void WriteOne(DTDataStorage &output,const std::string &name,const DTRegion2D &toWrite)
{
    Write(output,name,toWrite);
    Write(output,"Seq_"+name,"2D Region");
    output.Flush();
}

