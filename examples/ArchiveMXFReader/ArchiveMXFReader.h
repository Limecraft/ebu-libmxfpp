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

#ifndef __MXFPP_ARCHIVE_MXF_READER_H__
#define __MXFPP_ARCHIVE_MXF_READER_H__


#include <string>

#include <libMXF++/MXF.h>

#include "CommonTypes.h"
#include "ArchiveMXFContentPackage.h"


namespace mxfpp
{

class ArchiveMXFReader
{
public:
    ArchiveMXFReader(std::string filename);
    virtual ~ArchiveMXFReader();

    int64_t getDuration();
    int64_t getPosition();

    void seekToPosition(int64_t position);
    bool seekToTimecode(Timecode vitc, Timecode ltc);

    bool isEOF();

    const ArchiveMXFContentPackage* read();

private:
    uint32_t readElementData(DynamicByteArray *bytes, const mxfKey *elementKey);

    mxfpp::File* _mxfFile;
    mxfpp::DataModel* _dataModel;
    mxfpp::HeaderMetadata* _headerMetadata;

    int _numAudioTracks;
    int64_t _position;
    int64_t _duration;
    int64_t _actualPosition;
    ArchiveMXFContentPackage _cp;

    int64_t _startOfEssenceFilePos;
};



};


#endif



