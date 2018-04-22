// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTMask.h"

#include "DTCharArray.h"
#include "DTUCharArray.h"
#include "DTError.h"

DTMaskOptionalData::DTMaskOptionalData()
{
	intRangeComputed = false;
	minI = maxI = minJ = maxJ = minJ = maxJ = 0;
    numberOfPoints = -1;
}

DTMask::DTMask() 
: intervals(), _m(0), _n(0), _o(0), optional(new DTMaskOptionalData())
{
}

DTMask::DTMask(ssize_t mV,ssize_t nV,ssize_t oV)
: optional(new DTMaskOptionalData())
{
    _m = _n = _o = 0;

    if (mV*nV*oV==0)
        _m = _n = _o = 0;
    else {
        _m = int(mV);
        _n = int(nV);
        _o = int(oV);
    }
}

DTMask::DTMask(const DTIntArray &arr,ssize_t mv,ssize_t nv,ssize_t ov)
: optional(new DTMaskOptionalData())
{
    _m = 0;
    _n = 0;
    _o = 0;
    
    if (arr.Length()==0) {
        _m = int(mv);
        _n = int(nv);
        _o = int(ov);
        return;
    }
    if (arr.m()!=2 || arr.o()!=1) {
        DTErrorMessage("DTMask(intervals,...)","interval array has incorrect size");
        return;
    }

    ssize_t len = mv*nv*ov;
    ssize_t count = arr.n();
    ssize_t i,start,end;
    const int *arrD = arr.Pointer();
    for (i=0;i<count;i++) {
        start = arrD[2*i];
        end = arrD[1+2*i];
        if (start>end) {
            // This interval has negative length
            DTErrorMessage("DTMask(intervals,...)","One of the intervals is incorrect.");
            break;
        }
    }

    if (i<count) {
        return;
    }

    // Check start-end
    for (i=1;i<count;i++) {
        end = arrD[2*i-1];
        start = arrD[2*i];
        if (end>=start) {
            // Not strictly increasing
            DTErrorMessage("DTMask(intervals,...)","Intervals need to be in increasing order.");
            break;
        }
    }

    if (i<count) {
        return;
    }

    if (arr(0)<0 || arr(count*2-1)>=len) {
        // The first or last interval lies outside the possible range of indices.
        DTErrorMessage("DTMask(intervals,...)","Out of bounds.");
        return;
    }

    // Everything is ok.
    _m = int(mv);
    _n = int(nv);
    _o = int(ov);
    intervals = arr;
}

DTMask::DTMask(const DTCharArray &Mask)
: optional(new DTMaskOptionalData())
{
    _m = 0;
    _n = 0;
    _o = 0;
    
    const ssize_t mv = Mask.m();
    const ssize_t nv = Mask.n();
    const ssize_t ov = Mask.o();

    const ssize_t Len = Mask.Length();
    const char *MaskD = Mask.Pointer();

    if (Len==0) {
        return;
    }

    _m = int(mv);
    _n = int(nv);
    _o = int(ov);
    
    ssize_t howManyIntervals = 0;
    ssize_t ijk,j,k;
    ssize_t mn = mv*nv;
    ssize_t until;
    for (k=0;k<ov;k++) {
        for (j=0;j<nv;j++) {
            ijk = j*mv+k*mn;
            until = ijk+mv;
            while (ijk<until) {
                // Find the first entry that contains a zero.
                while (ijk<until && MaskD[ijk]==0) {
                    ijk++;
                }
                if (ijk<until) {
                    // Look for the end
                    while (ijk<until && MaskD[ijk]==1) {
                        ijk++;
                    }
                    howManyIntervals++;
                }
            }
        }
    }

    if (howManyIntervals==0) {
        return;
    }

    DTMutableIntArray theIntervals(2,howManyIntervals);
    ssize_t location = 0;
    ssize_t start;
    for (k=0;k<ov;k++) {
        for (j=0;j<nv;j++) {
            ijk = j*mv+k*mn;
            until = ijk+mv;
            while (ijk<until) {
                // Find the first entry that contains a zero.
                while (ijk<until && MaskD[ijk]==0) {
                    ijk++;
                }
                start = ijk;
                if (ijk<until) {
                    // Look for the end
                    while (ijk<until && MaskD[ijk]==1) {
                        ijk++;
                    }
                    theIntervals(0,location) = int(start);
                    theIntervals(1,location) = int(ijk-1);
                    location++;
                }
            }
        }
    }

    intervals = theIntervals;
	optional->mask = Mask;
}

DTCharArray DTMask::MaskArray(void) const
{
	if (optional->mask.NotEmpty())
		return optional->mask;
	
    DTMutableCharArray toReturn(m(),n(),o());
    toReturn = 0;
    const ssize_t howMany = intervals.n();
    ssize_t i,j,start,end;
    for (i=0;i<howMany;i++) {
        start = intervals(0,i);
        end = intervals(1,i);
        for (j=start;j<=end;j++) {
            toReturn(j) = 1;
        }
    }

	optional->mask = toReturn;
	
    return toReturn;
}

ssize_t DTMask::NumberOfPoints(void) const
{
    ssize_t toReturn = optional->numberOfPoints;
    toReturn = 0;
    ssize_t len = intervals.n();
    const int *intervalsD = intervals.Pointer();
    for (ssize_t i=0;i<len;i++)
        toReturn += intervalsD[1+2*i]-intervalsD[2*i];
    toReturn += len;
    optional->numberOfPoints = toReturn;
    return toReturn;
}

bool DTMask::ContainsPointOnEdge(ssize_t howM) const
{
	if (_m==0)
		return false;
	
	if (optional->intRangeComputed==false) {
		DTCharArray mask = MaskArray();
		
		ssize_t minI = _m;
		ssize_t maxI = -1;
		ssize_t minJ = _n;
		ssize_t maxJ = -1;
		ssize_t minK = _o;
		ssize_t maxK = -1;
		
		ssize_t i,j;
		ssize_t mv = m();
		ssize_t nv = n();
		ssize_t ov = o();
		
		if (ov==1) {
			ssize_t ij = 0;
			for (j=0;j<nv;j++) {
				for (i=0;i<mv;i++) {
					if (mask(ij)) {
						if (minI>i) minI = i;
						if (maxI<i) maxI = i;
						if (minJ>j) minJ = j;
						if (maxJ<j) maxJ = j;
					}
					ij++;
				}
			}
		}
		else {
			ssize_t ijk = 0;
			ssize_t k;
			for (k=0;k<ov;k++) {
				for (j=0;j<nv;j++) {
					for (i=0;i<mv;i++) {
						if (mask(ijk)) {
							if (minI>i) minI = i;
							if (maxI<i) maxI = i;
							if (minJ>j) minJ = j;
							if (maxJ<j) maxJ = j;
							if (minK>k) minK = k;
							if (maxK<k) maxK = k;
						}
						ijk++;
					}
				}
			}
		}
		optional->minI = minI;
		optional->maxI = maxI;
		optional->minJ = minJ;
		optional->maxJ = maxJ;
		optional->minK = minK;
		optional->maxK = maxK;
		optional->intRangeComputed = true;
	}
	
	if (howM>optional->minI || howM>optional->minJ) return true;
	if (_m-howM-1<optional->maxI || _n-howM-1<optional->maxJ) return true;
	
	if (_o>1) {
		if (howM>optional->minK || _o-howM-1<optional->maxK) return true;
	}
	
	return false;
}

DTIntArray DTMask::Offsets(void) const
{
	if (optional->offsets.NotEmpty())
		return optional->offsets;
	
    ssize_t  N = NumberOfPoints();
    ssize_t  howMany = intervals.n();
    
    DTMutableIntArray toReturn(N);
    ssize_t i,j,begin,end;
    ssize_t pos = 0;
    for (i=0;i<howMany;i++) {
        begin = intervals(0,i);
        end = intervals(1,i)+1;
        for (j=begin;j<end;j++)
            toReturn(pos++) = int(j);
    }
    
	optional->offsets = toReturn;
    return toReturn;
}

void DTMask::pinfo(void) const
{
    if (_o==0)
        std::cerr << "Empty\n";
    else if (_o==1)
        std::cerr << _m << " x " << _n << " mask with " << NumberOfPoints() << " points.\n";
    else
        std::cerr << _m << " x " << _n << " x " << _o << " mask with " << NumberOfPoints() << " points.\n";
    std::cerr << std::flush;
}

void DTMask::pstars(void) const
{
	if (_o==0)
		std::cerr << "empty mask" << std::endl;
	else if (_o!=1)
		std::cerr << "3D mask, can't print" << std::endl;
	else {
		DTCharArray fullMask = MaskArray();
		ssize_t i,j;
		for (j=_n-1;j>=0;j--) {
			for (i=0;i<_m;i++) {
				if (fullMask(i,j)) {
					std::cerr << "*";
				}
				else if (i==0 || i==_m-1 || j==0 || j==_n-1) {
					std::cerr << "-";
				}
				else {
					std::cerr << " ";
				}
			}
			std::cerr << std::endl;
		}
	}
}

ssize_t DTMask::FromBandOffsetToBigOffset(int off)
{
    ssize_t soFar = 0;
    int howMany = int(intervals.n());
    int i;
    for (i=0;i<howMany;i++) {
        soFar += intervals(1,i)-intervals(0,i)+1;
        if (soFar>off) {
            // Too far
            soFar -= intervals(1,i)-intervals(0,i)+1;
            return (off-soFar) + intervals(0,i);
        }
    }
    return -1;
}

int DTMask::FromBigOffsetToBandOffset(ssize_t off)
{
    if (intervals.IsEmpty()) return -1;
    int soFar = 0;
    int howMany = int(intervals.n());
    int i;
    if (intervals(0,0)>off) return -1; // Not in the first interval
    for (i=0;i<howMany;i++) {
        if (intervals(1,i)<off) {
            soFar += intervals(1,i)-intervals(0,i)+1;
        }
        else if (intervals(0,i)>off) {
            // Wasn't in the previous interval, so it landed between intervals
            return -1;
        }
        else {
            // intervals(0,i)<=off<=intervals(1,i)
            return soFar + int(off-intervals(0,i));
        }
    }
    return -1; // Not found
}

DTMask operator-(const DTMask &A,const DTMask &B)
{
	// Set difference.
	if (A.m()!=B.m() || A.n()!=B.n() || A.o()!=B.o()) {
		DTErrorMessage("DTMask-DTMask","Incompatible sizes");
		return DTMask();
	}
	
	DTCharArray AM = A.MaskArray();
	DTCharArray BM = B.MaskArray();
	DTMutableCharArray toReturn = AM.Copy();
	
	ssize_t  mn = toReturn.Length();
	ssize_t ij;
	for (ij=0;ij<mn;ij++) {
		if (toReturn(ij) && BM(ij)) toReturn(ij) = 0;
	}

	return DTMask(toReturn);
}

bool operator==(const DTMask &A,const DTMask &B)
{
    return (A.Intervals()==B.Intervals());
}

bool operator!=(const DTMask &A,const DTMask &B)
{
    return !(A==B);
}

DTMask RemoveEdgeFromMask(const DTMask &mask,ssize_t howM)
{
	if (mask.IsEmpty() || mask.ContainsPointOnEdge(howM)==false)
		return mask;
	
	DTMutableCharArray newMask = mask.MaskArray().Copy();
	ssize_t i,j,until;
	ssize_t m = newMask.m();
	ssize_t n = newMask.n();
	ssize_t o = newMask.o();
    ssize_t mn = m*n;
	if (m!=1 && n!=1 && o==1) {
        ssize_t ij;
        if (m<=2*howM || n<=2*howM) {
            newMask = 0;
        }
        else {
            until = howM*m;
            //  Bottom
            for (ij=0;ij<until;ij++) newMask(ij) = 0;
            // Sides
            for (j=howM;j<n-howM;j++) {
                for (i=0;i<howM;i++) {
                    newMask(i,j) = 0;
                newMask(m-1-i,j) = 0;
                }
            }
            until = mn;
            for (ij=(n-howM)*m;ij<until;ij++) {
                newMask(ij) = 0;
            }
        }
    }
	else if (m!=1 && n!=1 && o!=1) {
        ssize_t ijk,k;
        if (m<=2*howM || n<=2*howM || o<=2*howM) {
            newMask = 0;
        }
        else {
            until = howM*mn;
            for (ijk=0;ijk<until;ijk++) newMask(ijk) = 0;
            for (k=howM;k<o-howM;k++) {
                until = howM*m + k*mn;
                for (ijk=k*mn;ijk<until;ijk++) newMask(ijk) = 0;
                
                for (j=howM;j<n-1-howM;j++) {
                    for (i=0;i<howM;i++)
                        newMask(i,j,k) = 0;
                    for (i=0;i<howM;i++)
                        newMask(m-1-i,j,k) = 0;
                }
                
                until = (k+1)*mn;
                for (ijk=(n-1-howM)*m + k*mn;ijk<until;ijk++) newMask(ijk) = 0;
            }
            until = mn*o;
            for (ijk=(o-howM-1)*mn;ijk<until;ijk++) newMask(ijk) = 0;
		}
	}
	
	return DTMask(newMask);
}

void Read(const DTDataStorage &input,const std::string &name,DTMask &toReturn)
{
    // dimension stored in name_dim
    DTIntArray dimension;
    Read(input,name+"_dim",dimension);
    if (dimension.Length()<2 || dimension.Length()>3) {
        toReturn = DTMask();
        return;
    }

    ssize_t m = dimension(0);
    ssize_t n = dimension(1);
    ssize_t o = 1;
    if (dimension.Length()==3)
        o = dimension(2);

    DTIntArray offsets;
    Read(input,name,offsets);
    toReturn = DTMask(offsets,m,n,o);
}

void Write(DTDataStorage &output,const std::string &name,const DTMask &theVar)
{
    ssize_t m = theVar.m();
    ssize_t n = theVar.n();
    ssize_t o = theVar.o();

    if (o) {
        DTMutableIntArray dimension(o<=1 ? 2 : 3);
        dimension(0) = int(m);
        dimension(1) = int(n);
        if (o>1) dimension(2) = int(o);
        Write(output,name+"_dim",dimension);
    }
    
    Write(output,name,theVar.Intervals());
}

#pragma mark Mask3D

DTMask3D::DTMask3D() : _m(0), _n(0), _o(0), _numberOfPoints(0) {}

DTMask3D::DTMask3D(const DTIntArray &starts,const DTShortIntArray &intervals,ssize_t m,ssize_t n,ssize_t o)
: _m(0), _n(0), _o(0), _numberOfPoints(0)
{
    if (m<0 || n<0 || o<0) {
        DTErrorMessage("Mask3D(starts,intervals,m,m,o)","Invalid m,n,o");
        return;
    }
    if (starts.Length()!=n*o+1) {
        DTErrorMessage("Mask3D(starts,intervals,m,m,o)","Invalid size for the start array");
        return;
    }
    
    // Check the input
    ssize_t i,howMany;
    howMany = starts.Length()-1;
    ssize_t maxIndex = intervals.n();
    if (starts(0)!=0) {
        DTErrorMessage("Mask3D(starts,intervals,m,m,o)","starts(0) has to be 0");
        return;
    }
    for (i=0;i<howMany;i++) {
        if (starts(i)>starts(i+1)) break;
    }
    if (i<howMany) {
        DTErrorMessage("Mask3D(starts,intervals,m,m,o)","starts have to be strictly increasing.");
        return;
    }
    if (starts(howMany)!=maxIndex) {
        DTErrorMessage("Mask3D(starts,intervals,m,m,o)","Last entry of starts has to be the length of the interval list.");
        return;
    }
    
    if (intervals.NotEmpty() && (intervals.o()>1 || intervals.m()!=2)) {
        DTErrorMessage("Mask3D(starts,intervals,m,m,o)","Interval list has to be a 2xN array");
        return;
    }
    howMany = intervals.n();
    const short int *intervalsD = intervals.Pointer();
    short startAt=0,endAt=0;
    short mShort = short(m);
    for (i=0;i<howMany;i++) {
        startAt = intervalsD[2*i];
        endAt = intervalsD[1+2*i];
        if (startAt>=endAt) break;
        if (startAt<0 || endAt>mShort) break;
    }
    if (i<howMany) {
        if (startAt>=endAt) {
            DTErrorMessage("Mask3D(starts,intervals,m,m,o)","One of the intervals is empty.");
        }
        else if (startAt<0) {
            DTErrorMessage("Mask3D(starts,intervals,m,m,o)","One of the intervals is negative.");
        }
        else {
            DTErrorMessage("Mask3D(starts,intervals,m,m,o)","One of the intervals goes out of bounds.");
        }
        return;
    }
    
    ssize_t no = n*o;
    ssize_t jk;
    ssize_t intervalNumber=0,startInterval,endInterval;
    endInterval = (no ? starts(0) : 0);
    DTMutableIntArray startOfValues(no+1);
    int posInValues = 0;
    for (jk=0;jk<no;jk++) {
        startOfValues(jk) = posInValues;
        startInterval = endInterval;
        endInterval = starts(jk+1);
        // Now go through the interval and count how many values there are
        for (intervalNumber=startInterval;intervalNumber<endInterval;intervalNumber++) {
            // posInValues += intervals(1,intervalNumber) - intervals(0,intervalNumber);
            posInValues += intervalsD[1+2*intervalNumber] - intervalsD[2*intervalNumber];
        }
        for (intervalNumber=startInterval+1;intervalNumber<endInterval;intervalNumber++) {
            // if (intervals(1,intervalNumber-1)>=intervals(0,intervalNumber)) break;
            if (intervalsD[2*intervalNumber-1]>=intervalsD[2*intervalNumber]) break;
        }
        if (intervalNumber<endInterval) break;
    }
    if (jk<no) {
        // Problem at j=jk%n k = jk/n
        // intervals(1,intervalNumber-1)>=intervals(0,intervalNumber), which means
        if (intervals(1,intervalNumber-1)==intervals(0,intervalNumber)) {
            // Considered a structural problem since there is no point between the two intervals.
            // These two intervals should be merged
            DTErrorMessage("Mask3D(starts,intervals,m,m,o)","Intervals should be merged");
        }
        else {
            DTErrorMessage("Mask3D(starts,intervals,m,m,o)","Interval list overlaps.");
        }
        return;
    }
    startOfValues(jk) = posInValues;

    _m = m;
    _n = n;
    _o = o;
    _starts = starts;
    _startOfData = startOfValues;
    _intervals = intervals;
    _numberOfPoints = posInValues;
}

void DTMask3D::pinfo(void) const
{
    if (_o==0)
        std::cerr << "Empty\n";
    else
        std::cerr << _m << " x " << _n << " x " << _o << " mask with " << NumberOfPoints() << " points.\n";
    std::cerr << std::flush;
}

bool operator==(const DTMask3D &A,const DTMask3D &B)
{
    return (A.Intervals()==B.Intervals() && A.Starts()==B.Starts());
}

bool operator!=(const DTMask3D &A,const DTMask3D &B)
{
    return !(A==B);
}

void Read(const DTDataStorage &input,const std::string &name,DTMask3D &toReturn)
{
    // dimension stored in name_dim
    DTIntArray dimension;
    if (input.Contains(name+"_dim")==false) {
        toReturn = DTMask3D();
        return;
    }
    Read(input,name+"_dim",dimension);
    if (dimension.Length()!=3) {
        DTErrorMessage("Read(file,name,Mask3D","Invalid dimension");
        toReturn = DTMask3D();
        return;
    }
    
    ssize_t m = dimension(0);
    ssize_t n = dimension(1);
    ssize_t o = dimension(2);
    ssize_t mn = m*n;
    ssize_t no = n*o;
    
    DTMutableIntArray startOffsets(n*o+1);
    DTShortIntArray intervals;
    int jk,howMany;
    int pos = 0;
    
    if (input.Contains(name+"_interv")) {
        Read(input,name+"_interv",intervals);
        
        if (input.SavedAsCharacter(name)) {
            DTUCharArray numbers;
            Read(input,name,numbers);
            if (numbers.m()!=n || numbers.n()!=o) {
                DTErrorMessage("Read(file,name,Mask3D)","Data is not stored correctly");
                toReturn = DTMask3D();
                return;
            }
            
            // Compact setup
            for (jk=0;jk<no;jk++) {
                startOffsets(jk) = pos;
                howMany = numbers(jk);
                pos+=howMany;
            }
            startOffsets(no) = pos;
        }
        else {
            DTShortIntArray numbers;
            Read(input,name,numbers);
            if (numbers.m()!=n || numbers.n()!=o) {
                DTErrorMessage("Read(file,name,Mask3D)","Data is not stored correctly");
                toReturn = DTMask3D();
                return;
            }
            
            // Compact setup
            for (jk=0;jk<no;jk++) {
                startOffsets(jk) = pos;
                howMany = numbers(jk);
                pos+=howMany;
            }
            startOffsets(no) = pos;
        }
    }
    else {
        // Old style, the data is a 2xN array of intervals of the form [start,end] where
        // the end point is included
        DTIntArray offsetIntervals;
        Read(input, name, offsetIntervals);
        if (offsetIntervals.n()*2!=offsetIntervals.Length()) {
            DTErrorMessage("Read(file,name,Mask3D)","Data is not stored correctly");
            toReturn = DTMask3D();
            return;
        }
        ssize_t interval,howManyIntervals = offsetIntervals.n();
        int i,j,k,start,end;
        DTMutableShortIntArray newIntervals(2,howManyIntervals);
        DTMutableShortIntArray numbers(n,o);
        numbers = 0;
        for (interval=0;interval<howManyIntervals;interval++) {
            start = offsetIntervals(0,interval);
            end = offsetIntervals(1,interval);
            i = start%m;
            j = (start/m)%n;
            k = start/mn;
            numbers(j,k)++;
            newIntervals(0,interval) = short(i);
            newIntervals(1,interval) = short(end%m+1);
        }
        
        for (jk=0;jk<no;jk++) {
            startOffsets(jk) = pos;
            howMany = numbers(jk);
            pos+=howMany;
        }
        startOffsets(no) = pos;
        
        intervals = newIntervals;
    }
    
    toReturn = DTMask3D(startOffsets,intervals,m,n,o);
}

void Write(DTDataStorage &output,const std::string &name,const DTMask3D &theVar)
{
    ssize_t m = theVar.m();
    ssize_t n = theVar.n();
    ssize_t o = theVar.o();
    
    DTMutableIntArray dimension(3);
    dimension(0) = int(m);
    dimension(1) = int(n);
    dimension(2) = int(o);
    Write(output,name+"_dim",dimension);
    
    Write(output,name+"_interv",theVar.Intervals());
    
    // For the starting offsets I can save a smaller list that contains the length in each ray.
    // This is typically only a 1 byte number because it is unlikely that you have more than 256
    // intervals in a single line.  It is possible in edge cases but not typical.
    DTIntArray starts = theVar.Starts();
    DTMutableUCharArray inEach(n,o);
    int j,k,howMany;
    int jk = 0;
    for (k=0;k<o;k++) {
        for (j=0;j<n;j++) {
            howMany = starts(jk+1)-starts(jk);
            if (howMany>255) {
                // Fallback, very unusual
                break;
            }
            inEach(j,k) = char(howMany);
            jk++;
        }
        if (j<n) break;
    }
    if (k==o) {
        Write(output,name,inEach);
    }
    else {
        // Fallback, one of the rays had more than 255 intervals
        DTMutableShortIntArray inEachShort(n,o);
        jk = 0;
        for (k=0;k<o;k++)
            for (j=0;j<n;j++)
                inEachShort(j,k) = short(starts(jk+1)-starts(jk));
        Write(output,name,inEachShort);
    }
}
