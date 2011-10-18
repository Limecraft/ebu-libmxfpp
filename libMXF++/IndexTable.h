/*
 * Copyright (C) 2008, British Broadcasting Corporation
 * All Rights Reserved.
 *
 * Author: Philip de Nier
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the British Broadcasting Corporation nor the names
 *       of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MXFPP_INDEX_TABLE_H__
#define __MXFPP_INDEX_TABLE_H__

#include <vector>



namespace mxfpp
{


class File;

class IndexTableSegment
{
public:
    static bool isIndexTableSegment(const mxfKey *key);
    static IndexTableSegment* read(File *mxfFile, uint64_t segmentLen);

public:
    IndexTableSegment();
    IndexTableSegment(::MXFIndexTableSegment *cSegment);
    virtual ~IndexTableSegment();

    mxfUUID getInstanceUID();
    mxfRational getIndexEditRate();
    int64_t getIndexStartPosition();
    int64_t getIndexDuration();
    uint32_t getEditUnitByteCount();
    uint32_t getIndexSID();
    uint32_t getBodySID();
    uint8_t getSliceCount();
    uint8_t getPosTableCount();
    // deltaEntryArray
    // indexEntryArray
    bool haveDeltaEntryAtDelta(uint32_t delta, uint8_t slice);
    const MXFDeltaEntry* getDeltaEntryAtDelta(uint32_t delta, uint8_t slice);

    void setInstanceUID(mxfUUID value);
    void setIndexEditRate(mxfRational value);
    void setIndexStartPosition(int64_t value);
    void setIndexDuration(int64_t value);
    void incrementIndexDuration();
    void setEditUnitByteCount(uint32_t value);
    void setIndexSID(uint32_t value);
    void setBodySID(uint32_t value);
    void setSliceCount(uint8_t value);
    void setPosTableCount(uint8_t value);

    void appendDeltaEntry(int8_t posTableIndex, uint8_t slice, uint32_t elementData);
    void appendIndexEntry(int8_t temporalOffset, int8_t keyFrameOffset, uint8_t flags, uint64_t streamOffset,
        const std::vector<uint32_t> &sliceOffset, const std::vector<mxfRational> &posTable);


    void write(File *mxfFile, Partition *partition, FillerWriter *filler);

    void writeHeader(File *mxfFile, uint32_t numDeltaEntries, uint32_t numIndexEntries);
    void writeDeltaEntryArrayHeader(File *mxfFile, uint32_t numDeltaEntries);
    void writeDeltaEntry(File *mxfFile, int8_t posTableIndex, uint8_t slice, uint32_t elementData);
    void writeIndexEntryArrayHeader(File *mxfFile, uint8_t sliceCount, uint8_t posTableCount, uint32_t numIndexEntries);
    void writeIndexEntry(File *mxfFile, int8_t temporalOffset, int8_t keyFrameOffset, uint8_t flags,
                         uint64_t streamOffset, const std::vector<uint32_t> &sliceOffset,
                         const std::vector<mxfRational> &posTable);

    void writeAvidIndexEntryArrayHeader(File *mxfFile, uint8_t sliceCount, uint8_t posTableCount,
                                        uint32_t numIndexEntries);

    ::MXFIndexTableSegment* getCIndexTableSegment() const { return _cSegment; }

protected:
    ::MXFIndexTableSegment* _cSegment;
};


};



#endif

