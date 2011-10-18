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

#ifndef __AVID_CLIP_WRITER_H__
#define __AVID_CLIP_WRITER_H__


#include <string>
#include <map>

#include <libMXF++/MXF.h>
#include "../Common/CommonTypes.h"



typedef enum
{
    AVID_PAL_25I_PROJECT,
    AVID_NTSC_30I_PROJECT
} AvidProjectFormat;

typedef enum
{
    AVID_MJPEG21_ESSENCE,
    AVID_MJPEG31_ESSENCE,
    AVID_MJPEG101_ESSENCE,
    AVID_MJPEG101m_ESSENCE,
    AVID_MJPEG151s_ESSENCE,
    AVID_MJPEG201_ESSENCE,
    AVID_IECDV25_ESSENCE,
    AVID_DVBASED25_ESSENCE,
    AVID_DVBASED50_ESSENCE,
    AVID_DV1080I50_ESSENCE,
    AVID_DV720P50_ESSENCE,
    AVID_MPEG30_ESSENCE,
    AVID_MPEG40_ESSENCE,
    AVID_MPEG50_ESSENCE,
    AVID_DNXHD_1235_ESSENCE_TYPE,
    AVID_DNXHD_1237_ESSENCE_TYPE,
    AVID_DNXHD_1238_ESSENCE_TYPE,
    AVID_DNXHD_1241_ESSENCE_TYPE,
    AVID_DNXHD_1242_ESSENCE_TYPE,
    AVID_DNXHD_1243_ESSENCE_TYPE,
    AVID_DNXHD_1250_ESSENCE_TYPE,
    AVID_DNXHD_1251_ESSENCE_TYPE,
    AVID_DNXHD_1252_ESSENCE_TYPE,
    AVID_DNXHD_1253_ESSENCE_TYPE,
    AVID_UNCUYVY_ESSENCE,
    AVID_UNC1080IUYVY_ESSENCE,
    AVID_PCM_ESSENCE
} AvidEssenceType;


typedef union
{
    // video
    int mpegFrameSize;

    // audio
    uint32_t quantizationBits;
} EssenceParams;


class TrackData;

class AvidClipWriter
{
public:
    AvidClipWriter(AvidProjectFormat format, mxfRational imageAspectRatio, bool dropFrame, bool useLegacy);
    virtual ~AvidClipWriter();


    void setProjectName(std::string name);
    void setClipName(std::string name);
    void setTape(std::string name, int64_t startTimecode);
    void addUserComment(std::string name, std::string value);

    void registerEssenceElement(int trackId, int trackNumber, AvidEssenceType type, EssenceParams *params, std::string filename);


    void prepareToWrite();

    void writeSamples(int trackId, uint32_t numSamples, unsigned char *data, uint32_t dataLen);

    void completeWrite();
    void abortWrite(bool deleteFiles);

private:
    std::string _projectName;
    AvidProjectFormat _format;
    mxfRational _imageAspectRatio;
    bool _dropFrame;
    bool _useLegacy;
    std::string _clipName;
    std::string _tapeName;
    int64_t _startTimecode;
    std::map<std::string, std::string> _userComments;

    mxfRational _projectEditRate;

    bool _readyToWrite;
    bool _endedWriting;

    std::map<int, TrackData*> _tracks;
};



#endif


