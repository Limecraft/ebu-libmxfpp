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

#ifndef __MXFPP_PARTITION_H__
#define __MXFPP_PARTITION_H__



namespace mxfpp
{


class File;


class FillerWriter
{
public:
    virtual ~FillerWriter() {};

    virtual void write(File *file) = 0;
};

class Partition;

class KAGFillerWriter : public FillerWriter
{
public:
    KAGFillerWriter(Partition *partition, uint32_t allocSpace = 0);
    virtual ~KAGFillerWriter();

    virtual void write(File *file);

private:
    Partition* _partition;
    uint32_t _allocSpace;
};

class PositionFillerWriter : public FillerWriter
{
public:
    PositionFillerWriter(int64_t position);
    virtual ~PositionFillerWriter();

    virtual void write(File *file);

private:
    int64_t _position;
};


class Partition
{
public:
    static Partition* read(File *file, const mxfKey *key);
    static Partition* findAndReadHeaderPartition(File *file);

    Partition();
    Partition(::MXFPartition *cPartition);
    Partition(const Partition &partition);
    ~Partition();

    void setKey(const mxfKey *key);
    void setVersion(uint16_t majorVersion, uint16_t minorVersion);
    void setKagSize(uint32_t kagSize);
    void setThisPartition(uint64_t thisPartition);
    void setPreviousPartition(uint64_t previousPartition);
    void setFooterPartition(uint64_t footerPartition);
    void setHeaderByteCount(uint64_t headerByteCount);
    void setIndexByteCount(uint64_t indexByteCount);
    void setIndexSID(uint32_t indexSID);
    void setBodyOffset(uint64_t bodyOffset);
    void setBodySID(uint32_t bodySID);
    void setOperationalPattern(const mxfUL *operationalPattern);
    void setOperationalPattern(mxfUL operationalPattern);
    void addEssenceContainer(const mxfUL *essenceContainer);
    void addEssenceContainer(mxfUL essenceContainer);

    const mxfKey* getKey();
    uint16_t getMajorVersion();
    uint16_t getMinorVersion();
    uint32_t getKagSize();
    uint64_t getThisPartition();
    uint64_t getPreviousPartition();
    uint64_t getFooterPartition();
    uint64_t getHeaderByteCount();
    uint64_t getIndexByteCount();
    uint32_t getIndexSID();
    uint64_t getBodyOffset();
    uint32_t getBodySID();
    const mxfUL* getOperationalPattern();
    std::vector<mxfUL> getEssenceContainers();

    void markHeaderStart(File *file);
    void markHeaderEnd(File *file);
    void markIndexStart(File *file);
    void markIndexEnd(File *file);

    void write(File *file);

    void fillToKag(File *file);
    void allocateSpaceToKag(File *file, uint32_t size);


    ::MXFPartition* getCPartition() const { return _cPartition; }

private:
    ::MXFPartition* _cPartition;
};



};



#endif

