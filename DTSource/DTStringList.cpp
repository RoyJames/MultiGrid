// Part of DTSource. Copyright 2004-2017. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include "DTStringList.h"

#include "DTError.h"
#include <string.h>
#include <cstring>

DTStringList::DTStringList(const DTCharArray &chars)
{
    if (chars.IsEmpty()) return;
    if (chars(chars.Length()-1)!=0) {
        DTErrorMessage("DTStringList(DTCharArray)","Needs to be 0 terminated");
        return;
    }

    // Figure out the offsets
    ssize_t numberOfChars = chars.Length();
    int posInChar = 0;
    ssize_t lenOfOffsets = 1000;
    DTMutableIntArray newOffsets(lenOfOffsets);
    int posInOffsets = 0;
    newOffsets(posInOffsets++) = 0;
    while (posInChar<numberOfChars) {
        // Find the first end
        while (chars(posInChar)) posInChar++;
        // Found the end
        if (posInOffsets>=lenOfOffsets) { // Need to increase the offset array
            offsets = IncreaseSize(offsets,lenOfOffsets);
            lenOfOffsets = offsets.Length();
        }
        posInChar++;
        newOffsets(posInOffsets++) = posInChar;
    }
    characters = chars;
    offsets = TruncateSize(newOffsets,posInOffsets);
}

DTStringList::DTStringList(const DTCharArray &chars,const DTIntArray &offs)
{
    // Check validity.
    if (chars.IsEmpty() && offs.IsEmpty())
        return;
    
    if (chars.n()!=1 || chars.o()!=1 || offs.n()!=1 || offs.o()!=1) {
        DTErrorMessage("DTStringList(characters,offsets)","Invalid array sizes.");
        return;
    }
    
    // offsets need to be between 0 and len-1, and increasing.
    ssize_t numberOfChars = chars.Length();
    ssize_t howMany = offs.Length();
    ssize_t i;
    int off;
    for (i=0;i<howMany;i++) {
        off = offs(i);
        if (off<0 || off>=numberOfChars) break;
    }
    if (i<howMany) {
        DTErrorMessage("DTStringList(characters,offsets)","One of the offsets is out of range.");
        return;
    }
    
    // Increasing
    for (i=1;i<howMany;i++) {
        if (offs(i-1)>offs(i)) break;
    }
    if (i<howMany) {
        DTErrorMessage("DTStringList(characters,offsets)","The offsets need to be increasing.");
        return;
    }
    
    characters = chars;
    offsets = offs;
}

DTStringList::DTStringList(const DTList<std::string> &entries)
{
    // Figure out the total length
    size_t totalLength = 0;
    size_t howManyStrings = entries.Length();
    size_t i;
    for (i=0;i<howManyStrings;i++) {
        totalLength += entries(i).length()+1;
    }
    
    DTMutableIntArray offs(howManyStrings);
    DTMutableCharArray chars(totalLength);
    
    size_t posInChars = 0;
    size_t singleLength;
    for (i=0;i<howManyStrings;i++) {
        offs(i) = int(posInChars);
        singleLength = entries(i).length()+1;
        std::memcpy(chars.Pointer()+posInChars,entries(i).c_str(),singleLength);
        posInChars += singleLength;
    }
    
    characters = chars;
    offsets = offs;
}
    
std::string DTStringList::operator()(ssize_t si) const
{
    if (si<0 || si>=offsets.Length()) {
        DTErrorMessage("DTStringList::SingleEntry","Out of bounds.");
        return std::string();
    }
    
    size_t theLength;
    if (si<offsets.Length()-1)
        theLength = offsets(si+1)-offsets(si);
    else
        theLength = characters.Length()-offsets(si);
    
    if (theLength<=1)
        return std::string();
    
    return std::string(characters.Pointer()+offsets(si),theLength-1);
}

void DTStringList::pinfo(void) const
{
#ifndef DG_NOSTDErrOut
    if (offsets.Length()==0) {
        std::cerr << "No strings\n";
    }
    else if (NumberOfStrings()<100) {
        size_t i;
        size_t howMany = NumberOfStrings();
        for (i=0;i<howMany;i++) {
            std::cerr << operator()(i) << std::endl;
        }
    }
    else {
        std::cerr << offsets.Length() << " strings\n";
    }
    std::cerr << std::flush;
#endif
}

void Read(const DTDataStorage &input,const std::string &name,DTStringList &toReturn)
{
    DTIntArray offsets;
    DTCharArray characters;
    
    Read(input,name+"_offs",offsets);
    Read(input,name,characters);
    
    toReturn = DTStringList(characters,offsets);
}

void Write(DTDataStorage &output,const std::string &name,const DTStringList &theVar)
{
    Write(output,name+"_offs",theVar.Offsets());
    Write(output,name,theVar.Characters());
}

void WriteOne(DTDataStorage &output,const std::string &name,const DTStringList &toWrite)
{
    Write(output,name,toWrite);
    Write(output,"Seq_"+name,"List of Strings");
    output.Flush();
}

