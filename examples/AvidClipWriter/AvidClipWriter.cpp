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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>
#include <cstdio>
#include <cerrno>

#include <memory>

#include "AvidClipWriter.h"

#include <mxf/mxf_avid.h>
#include <mxf/mxf_logging.h>

using namespace std;
using namespace mxfpp;



static const mxfUUID g_identProductUID =
    {0xd1, 0xe2, 0x85, 0x8f, 0x1a, 0x7a, 0x46, 0xae, 0x9c, 0xc3, 0x67, 0x61, 0xe5, 0x51, 0x59, 0x82};
static const char *g_identCompanyName = "BBC Research";
static const char *g_identProductName = "Avid MXF (cpp) Writer";
static const char *g_identVersionString = "Beta version";


// 15 = GC Picture, 01 = element count=1, 01 = Avid JFIF, 01 = element number=1
static const uint32_t g_AvidMJPEGTrackNumber = 0x15010101;

// 15 = GC Picture, 01 = element count=1, 06 = DNxHD, 01 = element number=1
static const uint32_t g_DNxHDTrackNumber = 0x15010601;


static const uint32_t g_uncImageAlignmentOffset = 8192;

static const uint32_t g_uncPALFrameSize = 852480; // = 720*592*2
static const uint32_t g_uncAlignedPALFrameSize = 860160; // = 720*592*2 rounded up to image alignment factor 8192
static const uint32_t g_uncPALStartOffsetSize = 7680; // = g_uncAlignedPALFrameSize - g_uncPALFrameSize
static const uint32_t g_uncPALVBISize = 720 * 16 * 2;

static const uint32_t g_uncNTSCFrameSize = 714240; // 720*496*2
static const uint32_t g_uncAlignedNTSCFrameSize = 720896; // 720*496*2 rounded up to image alignment factor 8192
static const uint32_t g_uncNTSCStartOffsetSize = 6656; // g_uncAlignedNTSCFrameSize - g_uncNTSCFrameSize
static const uint32_t g_uncNTSCVBISize = 720 * 10 * 2;

static const uint32_t g_uncAligned1080i50FrameSize = 4153344; // 0x3f6000 (6144 pad + 4147200 frame)
static const uint32_t g_unc1080i50StartOffsetSize = 6144;

static const mxfRational g_PAL25IEditRate = {25, 1};
static const mxfRational g_NTSC30IEditRate = {30000, 1001};

static const uint32_t g_bodySID = 1;
static const uint32_t g_indexSID = 2;

static const uint64_t g_fixedBodyPPOffset = 0x40020;
static const uint64_t g_uncFixedBodyPPOffset = 0x60020;
static const uint64_t g_uncNTSCFixedBodyPPOffset = 0x5ff21;



static string get_track_name(bool isPicture, int number)
{
    string name;
    char buf[16];

    name = isPicture ? "V" : "A";
    sprintf(buf, "%d", number);
    return name.append(buf);
}

static int64_t calc_position(int64_t position, mxfRational editRate, mxfRational targetEditRate)
{
    if (memcmp(&targetEditRate, &editRate, sizeof(mxfRational)) == 0)
    {
        return position;
    }

    double factor = targetEditRate.numerator * editRate.denominator /
        (double)(targetEditRate.denominator * editRate.numerator);
    return (int64_t)(position * factor + 0.5);
}





class SetForDurationUpdate
{
public:
    virtual ~SetForDurationUpdate() {}

    virtual void updateDuration(int64_t duration, mxfRational editRate) = 0;
    virtual void updateEssenceDataLength(int64_t length) = 0;
};

class StructComponentForUpdate : public SetForDurationUpdate
{
public:
    StructComponentForUpdate(StructuralComponent *component, mxfRational editRate)
    : _component(component), _editRate(editRate)
    {}
    virtual ~StructComponentForUpdate()
    {}

    virtual void updateDuration(int64_t duration, mxfRational editRate)
    {
        _component->setDuration(calc_position(duration, editRate, _editRate));
    }
    virtual void updateEssenceDataLength(int64_t length) { (void)length; }

private:
    StructuralComponent* _component;
    mxfRational _editRate;
};

class FileDescriptorForUpdate : public SetForDurationUpdate
{
public:
    FileDescriptorForUpdate(FileDescriptor *descriptor)
    : _descriptor(descriptor)
    {}
    virtual ~FileDescriptorForUpdate()
    {}

    virtual void updateDuration(int64_t duration, mxfRational editRate)
    {
        _descriptor->setContainerDuration(calc_position(duration, editRate, _descriptor->getSampleRate()));
    }

    virtual void updateEssenceDataLength(int64_t length)
    {
        if (_descriptor->haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize)))
        {
            _descriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), (int32_t)length);
        }
    }

private:
    FileDescriptor* _descriptor;
};


class TrackData
{
public:
    TrackData() : mxfFile(0), dataModel(0), headerMetadata(0), indexSegment(0), vbiData(0), startOffsetData(0),
        essenceDataLen(0), trackDuration(0)
    {}

    ~TrackData()
    {
        vector<SetForDurationUpdate*>::iterator updateIter;
        for (updateIter = setsForDurationUpdate.begin(); updateIter != setsForDurationUpdate.end(); updateIter++)
        {
            delete *updateIter;
        }
        delete mxfFile;
        delete indexSegment;
        delete headerMetadata;
        delete dataModel;
        delete [] vbiData;
        delete [] startOffsetData;
    }

    AvidEssenceType type;
    string filename;
    int trackId;
    int trackNumber;

    bool isPicture;
    mxfRational editRate;
    mxfUL dataDef;
    mxfUL essenceContainerLabel;
    uint32_t essenceTrackNumber;
    mxfKey essenceElementKey;
    uint8_t essenceElementLLen;
    uint8_t minLLen;
    uint32_t editUnitByteCount;
    uint32_t audioQuantizationBits;

    File *mxfFile;
    DataModel *dataModel;
    AvidHeaderMetadata *headerMetadata;
    IndexTableSegment *indexSegment;
    mxfUMID fileSourcePackageUID;
    vector<SetForDurationUpdate*> setsForDurationUpdate;
    int64_t headerMetadataStartPos;

    unsigned char *vbiData;
    int vbiDataSize;
    unsigned char *startOffsetData;
    int startOffsetDataSize;

    int64_t essenceDataStartPos;
    int64_t essenceDataLen;

    int64_t trackDuration;

    vector<int64_t> frameOffsets;
};




AvidClipWriter::AvidClipWriter(AvidProjectFormat format, mxfRational imageAspectRatio, bool dropFrame, bool useLegacy)
: _format(format), _imageAspectRatio(imageAspectRatio), _dropFrame(dropFrame),
_useLegacy(useLegacy), _readyToWrite(false), _endedWriting(false)
{
    if (format == AVID_PAL_25I_PROJECT)
    {
        _projectEditRate = g_PAL25IEditRate;
    }
    else // AVID_NTSC_30I_PROJECT
    {
        _projectEditRate = g_NTSC30IEditRate;
    }
}

AvidClipWriter::~AvidClipWriter()
{
    map<int, TrackData*>::const_iterator trackIter;
    for (trackIter = _tracks.begin(); trackIter != _tracks.end(); trackIter++)
    {
        delete (*trackIter).second;
    }
}

void AvidClipWriter::setProjectName(string name)
{
    MXFPP_ASSERT(!_readyToWrite && !_endedWriting);

    _projectName = name;
}

void AvidClipWriter::setClipName(string name)
{
    MXFPP_ASSERT(!_readyToWrite && !_endedWriting);

    _clipName = name;
}

void AvidClipWriter::setTape(string name, int64_t startTimecode)
{
    MXFPP_ASSERT(!_readyToWrite && !_endedWriting);

    _tapeName = name;
    _startTimecode = startTimecode;
}

void AvidClipWriter::addUserComment(string name, string value)
{
    MXFPP_ASSERT(!_readyToWrite && !_endedWriting);

    pair<map<string, string>::iterator, bool> result;

    result = _userComments.insert(pair<string, string>(name, value));
    if (!result.second)
    {
        // replace value
        (*result.first).second = value;
    }
}



void AvidClipWriter::registerEssenceElement(int trackId, int trackNumber, AvidEssenceType type,
    EssenceParams *params, string filename)
{
    auto_ptr<TrackData> trackData(new TrackData());
    uint32_t i;

    MXFPP_ASSERT(!_readyToWrite && !_endedWriting);

    MXFPP_CHECK(trackId >= 1);


    // append new track data information

    if (!_tracks.insert(pair<int, TrackData*>(trackId, trackData.get())).second)
    {
        throw MXFException("Track %d already registered\n", trackId);
    }

    trackData->type = type;
    trackData->filename = filename;
    trackData->trackId = trackId;
    trackData->trackNumber = trackNumber;

    switch (type)
    {
        case AVID_MJPEG21_ESSENCE:
        case AVID_MJPEG31_ESSENCE:
        case AVID_MJPEG101_ESSENCE:
        case AVID_MJPEG101m_ESSENCE:
        case AVID_MJPEG151s_ESSENCE:
        case AVID_MJPEG201_ESSENCE:
            // only PAL 25i is currently supported
            MXFPP_CHECK(_format == AVID_PAL_25I_PROJECT);

            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            trackData->essenceContainerLabel = MXF_EC_L(AvidMJPEGClipWrapped);
            trackData->essenceTrackNumber = g_AvidMJPEGTrackNumber;
            trackData->essenceElementKey = MXF_EE_K(AvidMJPEGClipWrapped);
            trackData->essenceElementLLen = 8;
            trackData->minLLen = 9; // Avid (version?) won't except anything less
            trackData->editUnitByteCount = 0;
            break;

        case AVID_IECDV25_ESSENCE:
            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            if (_format == AVID_PAL_25I_PROJECT)
            {
                trackData->essenceContainerLabel = MXF_EC_L(IECDV_25_625_50_ClipWrapped);
                trackData->editUnitByteCount = 144000;
            }
            else
            {
                trackData->essenceContainerLabel = MXF_EC_L(IECDV_25_525_60_ClipWrapped);
                trackData->editUnitByteCount = 120000;
            }
            trackData->essenceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            trackData->essenceElementKey = MXF_EE_K(DVClipWrapped);
            trackData->essenceElementLLen = 8;
            trackData->minLLen = 4;
            break;

        case AVID_DVBASED25_ESSENCE:
            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            if (_format == AVID_PAL_25I_PROJECT)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DVBased_25_625_50_ClipWrapped);
                trackData->editUnitByteCount = 144000;
            }
            else
            {
                trackData->essenceContainerLabel = MXF_EC_L(DVBased_25_525_60_ClipWrapped);
                trackData->editUnitByteCount = 120000;
            }
            trackData->essenceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            trackData->essenceElementKey = MXF_EE_K(DVClipWrapped);
            trackData->essenceElementLLen = 8;
            trackData->minLLen = 4;
            break;

        case AVID_DVBASED50_ESSENCE:
            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            if (_format == AVID_PAL_25I_PROJECT)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DVBased_50_625_50_ClipWrapped);
                trackData->editUnitByteCount = 288000;
            }
            else
            {
                trackData->essenceContainerLabel = MXF_EC_L(DVBased_50_525_60_ClipWrapped);
                trackData->editUnitByteCount = 240000;
            }
            trackData->essenceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            trackData->essenceElementKey = MXF_EE_K(DVClipWrapped);
            trackData->essenceElementLLen = 8;
            trackData->minLLen = 4;
            break;

        case AVID_DV1080I50_ESSENCE:
        case AVID_DV720P50_ESSENCE:
            // only PAL 25i is currently supported
            MXFPP_CHECK(_format == AVID_PAL_25I_PROJECT);

            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            trackData->essenceTrackNumber = MXF_DV_TRACK_NUM(0x01, MXF_DV_CLIP_WRAPPED_EE_TYPE, 0x01);
            trackData->essenceElementLLen = 9;
            trackData->minLLen = 4;
            trackData->editUnitByteCount = 576000;
            if (type == AVID_DV1080I50_ESSENCE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DVBased_100_1080_50_I_ClipWrapped);
                trackData->essenceElementKey = MXF_EE_K(DVClipWrapped);
            }
            else // AVID_DV720P50_ESSENCE
            {
                trackData->essenceContainerLabel = MXF_EC_L(DVBased_100_720_50_P_ClipWrapped);
                trackData->essenceElementKey = MXF_EE_K(DVClipWrapped);
            }
            break;

        case AVID_MPEG30_ESSENCE:
        case AVID_MPEG40_ESSENCE:
        case AVID_MPEG50_ESSENCE:
            // only PAL 25i is currently supported
            MXFPP_CHECK(_format == AVID_PAL_25I_PROJECT);

            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            trackData->editUnitByteCount = params->mpegFrameSize;
            trackData->essenceTrackNumber = MXF_D10_PICTURE_TRACK_NUM(0x01);
            trackData->essenceElementKey = MXF_EE_K(IMX);
            trackData->essenceElementLLen = 8;
            trackData->minLLen = 4;
            if (type == AVID_MPEG30_ESSENCE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(AvidIMX30_625_50);
            }
            else if (type == AVID_MPEG40_ESSENCE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(AvidIMX40_625_50);
            }
            else // AVID_MPEG50_ESSENCE
            {
                trackData->essenceContainerLabel = MXF_EC_L(AvidIMX50_625_50);
            }
            break;

        case AVID_DNXHD_1235_ESSENCE_TYPE:
        case AVID_DNXHD_1237_ESSENCE_TYPE:
        case AVID_DNXHD_1238_ESSENCE_TYPE:
        case AVID_DNXHD_1241_ESSENCE_TYPE:
        case AVID_DNXHD_1242_ESSENCE_TYPE:
        case AVID_DNXHD_1243_ESSENCE_TYPE:
        case AVID_DNXHD_1250_ESSENCE_TYPE:
        case AVID_DNXHD_1251_ESSENCE_TYPE:
        case AVID_DNXHD_1252_ESSENCE_TYPE:
        case AVID_DNXHD_1253_ESSENCE_TYPE:
            // only PAL 25i is currently supported
            MXFPP_CHECK(_format == AVID_PAL_25I_PROJECT);

            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            trackData->essenceTrackNumber = g_DNxHDTrackNumber;
            trackData->essenceElementKey = MXF_EE_K(DNxHD);
            trackData->essenceElementLLen = 9;
            trackData->minLLen = 4;
            if (type == AVID_DNXHD_1235_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD1080p1235ClipWrapped);
                trackData->editUnitByteCount = 917504;
            }
            else if (type == AVID_DNXHD_1237_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD1080p1237ClipWrapped);
                trackData->editUnitByteCount = 606208;
            }
            else if (type == AVID_DNXHD_1238_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD1080p1238ClipWrapped);
                trackData->editUnitByteCount = 917504;
            }
            else if (type == AVID_DNXHD_1241_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD1080i1241ClipWrapped);
                trackData->editUnitByteCount = 917504;
            }
            else if (type == AVID_DNXHD_1242_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD1080i1242ClipWrapped);
                trackData->editUnitByteCount = 606208;
            }
            else if (type == AVID_DNXHD_1243_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD1080i1243ClipWrapped);
                trackData->editUnitByteCount = 917504;
            }
            else if (type == AVID_DNXHD_1250_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD720p1250ClipWrapped);
                trackData->editUnitByteCount = 458752;
            }
            else if (type == AVID_DNXHD_1251_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD720p1251ClipWrapped);
                trackData->editUnitByteCount = 458752;
            }
            else if (type == AVID_DNXHD_1252_ESSENCE_TYPE)
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD720p1252ClipWrapped);
                trackData->editUnitByteCount = 303104;
            }
            else // AVID_DNXHD_1253_ESSENCE_TYPE
            {
                trackData->essenceContainerLabel = MXF_EC_L(DNxHD1080p1253ClipWrapped);
                trackData->editUnitByteCount = 188416;
            }
            break;

        case AVID_UNCUYVY_ESSENCE:

            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            trackData->essenceElementKey = MXF_EE_K(UncClipWrapped);
            trackData->essenceTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
            trackData->essenceElementLLen = 8;
            trackData->minLLen = 4;
            if (_format == AVID_PAL_25I_PROJECT)
            {
                trackData->essenceContainerLabel = MXF_EC_L(SD_Unc_625_50i_422_135_ClipWrapped);
                trackData->editUnitByteCount = g_uncAlignedPALFrameSize;
                trackData->vbiData = new unsigned char[g_uncPALVBISize];
                trackData->vbiDataSize = g_uncPALVBISize;
                for (i = 0; i < g_uncPALVBISize / 4; i++)
                {
                    trackData->vbiData[i * 4] = 0x80; // U
                    trackData->vbiData[i * 4 + 1] = 0x10; // Y
                    trackData->vbiData[i * 4 + 2] = 0x80; // V
                    trackData->vbiData[i * 4 + 3] = 0x10; // Y
                }
                trackData->startOffsetData = new unsigned char[g_uncPALStartOffsetSize];
                trackData->startOffsetDataSize = g_uncPALStartOffsetSize;
                memset(trackData->startOffsetData, 0, g_uncPALStartOffsetSize);
            }
            else // AVID_NTSCL_30I_PROJECT
            {
                trackData->essenceContainerLabel = MXF_EC_L(SD_Unc_525_5994i_422_135_ClipWrapped);
                trackData->editUnitByteCount = g_uncAlignedNTSCFrameSize;
                trackData->vbiData = new unsigned char[g_uncNTSCVBISize];
                trackData->vbiDataSize = g_uncNTSCVBISize;
                for (i = 0; i < g_uncNTSCVBISize / 4; i++)
                {
                    trackData->vbiData[i * 4] = 0x80; // U
                    trackData->vbiData[i * 4 + 1] = 0x10; // Y
                    trackData->vbiData[i * 4 + 2] = 0x80; // V
                    trackData->vbiData[i * 4 + 3] = 0x10; // Y
                }
                trackData->startOffsetData = new unsigned char[g_uncNTSCStartOffsetSize];
                trackData->startOffsetDataSize = g_uncNTSCStartOffsetSize;
                memset(trackData->startOffsetData, 0, g_uncNTSCStartOffsetSize);
            }
            break;

        case AVID_UNC1080IUYVY_ESSENCE:
            // only PAL 25i is currently supported
            MXFPP_CHECK(_format == AVID_PAL_25I_PROJECT);

            trackData->isPicture = true;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacyPicture) : MXF_DDEF_L(Picture);
            trackData->editRate = _projectEditRate;
            trackData->essenceContainerLabel = MXF_EC_L(HD_Unc_1080_50i_422_ClipWrapped);
            trackData->essenceTrackNumber = MXF_UNC_TRACK_NUM(0x01, MXF_UNC_CLIP_WRAPPED_EE_TYPE, 0x01);
            trackData->essenceElementKey = MXF_EE_K(UncClipWrapped);
            trackData->essenceElementLLen = 9;
            trackData->minLLen = 4;
            trackData->editUnitByteCount = g_uncAligned1080i50FrameSize;
            trackData->startOffsetData = new unsigned char[g_unc1080i50StartOffsetSize];
            trackData->startOffsetDataSize = g_unc1080i50StartOffsetSize;
            memset(trackData->startOffsetData, 0, g_unc1080i50StartOffsetSize);
            break;

        case AVID_PCM_ESSENCE:
            trackData->isPicture = false;
            trackData->dataDef = (_useLegacy) ? MXF_DDEF_L(LegacySound) : MXF_DDEF_L(Sound);
            trackData->editRate.numerator = 48000;
            trackData->editRate.denominator = 1;
            trackData->essenceContainerLabel = MXF_EC_L(BWFClipWrapped);
            trackData->essenceTrackNumber = MXF_AES3BWF_TRACK_NUM(0x01, MXF_BWF_CLIP_WRAPPED_EE_TYPE, 0x01);
            trackData->essenceElementKey = MXF_EE_K(BWFClipWrapped);
            trackData->essenceElementLLen = 8;
            trackData->minLLen = 4;
            trackData->audioQuantizationBits = params->quantizationBits;
            trackData->editUnitByteCount = (params->quantizationBits + 7) / 8;
            break;
    }

    trackData.release();
}

void AvidClipWriter::prepareToWrite()
{
    MXFPP_ASSERT(!_readyToWrite && !_endedWriting);

    mxfUUID uuid;
    mxfTimestamp now;
    mxfUMID materialPackageUID;
    mxfUMID tapeSourcePackageUID;
    map<int, TrackData*>::const_iterator trackIter;
    map<int, TrackData*>::const_iterator trackInnerIter;
    uint32_t tapeTrackId;

    mxf_get_timestamp_now(&now);
    mxf_generate_aafsdk_umid(&materialPackageUID);
    mxf_generate_aafsdk_umid(&tapeSourcePackageUID);
    for (trackIter = _tracks.begin(); trackIter != _tracks.end(); trackIter++)
    {
        mxf_generate_aafsdk_umid(&(*trackIter).second->fileSourcePackageUID);
    }


    for (trackIter = _tracks.begin(), tapeTrackId = 1; trackIter != _tracks.end(); trackIter++, tapeTrackId++)
    {
        TrackData *trackData = (*trackIter).second;


        // open the MXF file

        trackData->mxfFile = File::openNew(trackData->filename);


        // set file minimum llen

        trackData->mxfFile->setMinLLen(trackData->minLLen);


        // write the header partition pack

        Partition& headerPartition = trackData->mxfFile->createPartition();
        headerPartition.setKey(&MXF_PP_K(ClosedIncomplete, Header));
        headerPartition.setOperationalPattern((_tracks.size() <= 1) ? &MXF_OP_L(atom, 1Track_1SourceClip) : &MXF_OP_L(atom, NTracks_1SourceClip));
        headerPartition.addEssenceContainer(&trackData->essenceContainerLabel);

        headerPartition.write(trackData->mxfFile);



        // create the header metadata for all the tracks

        trackData->dataModel = new DataModel();
        trackData->headerMetadata = new AvidHeaderMetadata(trackData->dataModel);


        // MetaDictionary
        trackData->headerMetadata->createDefaultMetaDictionary();


        // Preface
        Preface *preface = new Preface(trackData->headerMetadata);
        preface->setInt16Item(&MXF_ITEM_K(Preface, ByteOrder), 0x4949); // little-endian
        preface->setUInt32Item(&MXF_ITEM_K(Preface, ObjectModelVersion), 0x00000001);
        preface->setVersion(0x0101); // AAF SDK version
        preface->setLastModifiedDate(now);
        preface->setOperationalPattern((_tracks.size() <= 1) ? MXF_OP_L(atom, 1Track_1SourceClip) : MXF_OP_L(atom, NTracks_1SourceClip));
        preface->appendEssenceContainers(trackData->essenceContainerLabel);
        if (_projectName.size() > 0)
        {
            preface->setStringItem(&MXF_ITEM_K(Preface, ProjectName), _projectName);
        }
        preface->setRationalItem(&MXF_ITEM_K(Preface, ProjectEditRate), _projectEditRate);
        preface->setUMIDItem(&MXF_ITEM_K(Preface, MasterMobID), materialPackageUID);
        preface->setUMIDItem(&MXF_ITEM_K(Preface, EssenceFileMobID), trackData->fileSourcePackageUID);
        preface->appendULArrayItem(&MXF_ITEM_K(Preface, DMSchemes), MXF_DM_L(LegacyDMS1));

        // Preface - Dictionary
        trackData->headerMetadata->createDefaultDictionary(preface);

        // Preface - Identification
        mxf_generate_uuid(&uuid);
        Identification *ident = new Identification(trackData->headerMetadata);
        preface->appendIdentifications(ident);
        ident->initialise(g_identCompanyName, g_identProductName, g_identVersionString, g_identProductUID);

        // Preface - ContentStorage
        ContentStorage *contentStorage = new ContentStorage(trackData->headerMetadata);
        preface->setContentStorage(contentStorage);

        // Preface - ContentStorage - MaterialPackage
        MaterialPackage *materialPackage = new MaterialPackage(trackData->headerMetadata);
        contentStorage->appendPackages(materialPackage);
        materialPackage->setPackageUID(materialPackageUID);
        materialPackage->setPackageCreationDate(now);
        materialPackage->setPackageModifiedDate(now);
        if (_clipName.size() > 0)
        {
            materialPackage->setName(_clipName);
        }
        // TODO: what are ConvertFrameRate and AppCode for?
        materialPackage->setBooleanItem(&MXF_ITEM_K(GenericPackage, ConvertFrameRate), false);
        materialPackage->setInt32Item(&MXF_ITEM_K(GenericPackage, AppCode), 0x07);

        // Preface - ContentStorage - MaterialPackage - MobAttribute(TaggedValue)
        if (_projectName.size() > 0)
        {
            materialPackage->attachAvidAttribute("_PJ", _projectName);
        }

        // Preface - ContentStorage - MaterialPackage - UserComment(TaggedValue)
        map<string, string>::const_iterator commentIter;
        for (commentIter = _userComments.begin(); commentIter != _userComments.end(); commentIter++)
        {
            materialPackage->attachAvidUserComment((*commentIter).first, (*commentIter).second);
        }


        // Preface - ContentStorage - MaterialPackage - Timeline Tracks
        for (trackInnerIter = _tracks.begin(); trackInnerIter != _tracks.end(); trackInnerIter++)
        {
            TrackData *innerTrackData = (*trackInnerIter).second;

            // Preface - ContentStorage - MaterialPackage - Timeline Track
            Track *track = new Track(trackData->headerMetadata);
            materialPackage->appendTracks(track);
            track->setTrackName(get_track_name(innerTrackData->isPicture, innerTrackData->trackNumber));
            track->setTrackID(innerTrackData->trackId);
            track->setTrackNumber(innerTrackData->trackNumber);
            track->setEditRate(innerTrackData->editRate);
            track->setOrigin(0);

            // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
            Sequence *sequence = new Sequence(trackData->headerMetadata);
            track->setSequence(sequence);
            sequence->setDataDefinition(innerTrackData->dataDef);
            sequence->setDuration(-1);
            trackData->setsForDurationUpdate.push_back(new StructComponentForUpdate(sequence, innerTrackData->editRate));

            // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
            SourceClip *sourceClip = new SourceClip(trackData->headerMetadata);
            sequence->appendStructuralComponents(sourceClip);
            sourceClip->setDataDefinition(innerTrackData->dataDef);
            sourceClip->setDuration(-1);
            trackData->setsForDurationUpdate.push_back(new StructComponentForUpdate(sourceClip, innerTrackData->editRate));
            sourceClip->setStartPosition(0);
            sourceClip->setSourceTrackID(1);
            sourceClip->setSourcePackageID(innerTrackData->fileSourcePackageUID);

        }

        // Preface - ContentStorage - SourcePackage
        SourcePackage *fileSourcePackage = new SourcePackage(trackData->headerMetadata);
        contentStorage->appendPackages(fileSourcePackage);
        fileSourcePackage->setPackageUID(trackData->fileSourcePackageUID);
        fileSourcePackage->setPackageCreationDate(now);
        fileSourcePackage->setPackageModifiedDate(now);

        // Preface - ContentStorage - SourcePackage - MobAttribute(TaggedValue)
        if (_projectName.size() > 0)
        {
            fileSourcePackage->attachAvidAttribute("_PJ", _projectName);
        }


        // Preface - ContentStorage - SourcePackage - Timeline Track
        Track *track = new Track(trackData->headerMetadata);
        fileSourcePackage->appendTracks(track);
        track->setTrackName(get_track_name(trackData->isPicture, 1));
        track->setTrackID(1);
        track->setTrackNumber(trackData->essenceTrackNumber);
        track->setEditRate(trackData->editRate);
        track->setOrigin(0);

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
        Sequence *sequence = new Sequence(trackData->headerMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(trackData->dataDef);
        sequence->setDuration(-1);
        trackData->setsForDurationUpdate.push_back(new StructComponentForUpdate(sequence, trackData->editRate));

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
        SourceClip *sourceClip = new SourceClip(trackData->headerMetadata);
        sequence->appendStructuralComponents(sourceClip);
        sourceClip->setDataDefinition(trackData->dataDef);
        sourceClip->setDuration(-1);
        trackData->setsForDurationUpdate.push_back(new StructComponentForUpdate(sourceClip, trackData->editRate));
        if (_tapeName.size() > 0)
        {
            sourceClip->setStartPosition(calc_position(_startTimecode, _projectEditRate, trackData->editRate));
            sourceClip->setSourceTrackID(tapeTrackId);
            sourceClip->setSourcePackageID(tapeSourcePackageUID);
        }
        else
        {
            sourceClip->setStartPosition(0);
            sourceClip->setSourceTrackID(0);
            sourceClip->setSourcePackageID(g_Null_UMID);
        }


        // Preface - ContentStorage - SourcePackage - Descriptor

        GenericDescriptor *descriptor = 0;
        CDCIEssenceDescriptor *cdciDescriptor = 0;
        WaveAudioDescriptor *waveDescriptor = 0;

        switch (trackData->type)
        {
            case AVID_MJPEG21_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(AvidMJPEG21_PAL));
                cdciDescriptor->appendVideoLineMap(15);
                cdciDescriptor->appendVideoLineMap(328);
                cdciDescriptor->setStoredWidth(720);
                cdciDescriptor->setStoredHeight(296);
                cdciDescriptor->setDisplayWidth(720);
                cdciDescriptor->setDisplayHeight(288);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(8);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), g_AvidMJPEG21_ResolutionID);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), 0);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                break;

            case AVID_MJPEG31_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(AvidMJPEG31_PAL));
                cdciDescriptor->appendVideoLineMap(15);
                cdciDescriptor->appendVideoLineMap(328);
                cdciDescriptor->setStoredWidth(720);
                cdciDescriptor->setStoredHeight(296);
                cdciDescriptor->setDisplayWidth(720);
                cdciDescriptor->setDisplayHeight(288);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(8);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), g_AvidMJPEG31_ResolutionID);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), 0);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                break;

            case AVID_MJPEG101_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(AvidMJPEG101_PAL));
                cdciDescriptor->appendVideoLineMap(15);
                cdciDescriptor->appendVideoLineMap(328);
                cdciDescriptor->setStoredWidth(720);
                cdciDescriptor->setStoredHeight(296);
                cdciDescriptor->setDisplayWidth(720);
                cdciDescriptor->setDisplayHeight(288);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(8);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), g_AvidMJPEG101_ResolutionID);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), 0);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                break;

            case AVID_MJPEG101m_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(AvidMJPEG101m_PAL));
                cdciDescriptor->appendVideoLineMap(15);
                cdciDescriptor->setStoredWidth(288);
                cdciDescriptor->setStoredHeight(296);
                cdciDescriptor->setDisplayWidth(288);
                cdciDescriptor->setDisplayHeight(288);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(8);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_SINGLE_FIELD);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), g_AvidMJPEG101m_ResolutionID);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), 0);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                break;

            case AVID_MJPEG151s_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(AvidMJPEG151s_PAL));
                cdciDescriptor->appendVideoLineMap(15);
                cdciDescriptor->setStoredWidth(352);
                cdciDescriptor->setStoredHeight(296);
                cdciDescriptor->setDisplayWidth(352);
                cdciDescriptor->setDisplayHeight(288);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(8);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_SINGLE_FIELD);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), g_AvidMJPEG151s_ResolutionID);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), 0);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                break;

            case AVID_MJPEG201_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(AvidMJPEG201_PAL));
                cdciDescriptor->appendVideoLineMap(15);
                cdciDescriptor->appendVideoLineMap(328);
                cdciDescriptor->setStoredWidth(720);
                cdciDescriptor->setStoredHeight(296);
                cdciDescriptor->setDisplayWidth(720);
                cdciDescriptor->setDisplayHeight(288);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(8);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), g_AvidMJPEG201_ResolutionID);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), 0);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                break;

            case AVID_IECDV25_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                if (_format == AVID_PAL_25I_PROJECT)
                {
                    cdciDescriptor->setEssenceContainer(trackData->essenceContainerLabel);
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(IECDV_25_625_50));
                    cdciDescriptor->setSampleRate(trackData->editRate);
                    cdciDescriptor->setAspectRatio(_imageAspectRatio);
                    cdciDescriptor->appendVideoLineMap(23);
                    cdciDescriptor->appendVideoLineMap(335);
                    cdciDescriptor->setStoredWidth(720);
                    cdciDescriptor->setStoredHeight(288);
                    cdciDescriptor->setDisplayWidth(720);
                    cdciDescriptor->setDisplayHeight(288);
                    cdciDescriptor->setDisplayXOffset(0);
                    cdciDescriptor->setDisplayYOffset(0);
                    cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                    if (_useLegacy)
                    {
                        cdciDescriptor->setFrameLayout(MXF_MIXED_FIELDS);
                    }
                    else
                    {
                        cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                    }
                    cdciDescriptor->setHorizontalSubsampling(2);
                    cdciDescriptor->setVerticalSubsampling(2);
                    cdciDescriptor->setImageAlignmentOffset(1);
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->setBlackRefLevel(16);
                    cdciDescriptor->setWhiteReflevel(235);
                    cdciDescriptor->setColorRange(225);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0x8d);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                }
                else // AVID_NTSC_30I_PROJECT
                {
                    cdciDescriptor->setEssenceContainer(trackData->essenceContainerLabel);
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DVBased_25_525_60));
                    cdciDescriptor->setSampleRate(trackData->editRate);
                    cdciDescriptor->setAspectRatio(_imageAspectRatio);
                    cdciDescriptor->appendVideoLineMap(23);
                    cdciDescriptor->appendVideoLineMap(285);
                    cdciDescriptor->setStoredWidth(720);
                    cdciDescriptor->setStoredHeight(240);
                    cdciDescriptor->setDisplayWidth(720);
                    cdciDescriptor->setDisplayHeight(240);
                    cdciDescriptor->setDisplayXOffset(0);
                    cdciDescriptor->setDisplayYOffset(0);
                    cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                    if (_useLegacy)
                    {
                        cdciDescriptor->setFrameLayout(MXF_MIXED_FIELDS);
                        cdciDescriptor->setHorizontalSubsampling(1);
                        cdciDescriptor->setVerticalSubsampling(1);
                    }
                    else
                    {
                        cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                        cdciDescriptor->setHorizontalSubsampling(4);
                        cdciDescriptor->setVerticalSubsampling(1);
                    }
                    cdciDescriptor->setImageAlignmentOffset(1);
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->setBlackRefLevel(16);
                    cdciDescriptor->setWhiteReflevel(235);
                    cdciDescriptor->setColorRange(225);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0x8c);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                }
                break;

            case AVID_DVBASED25_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                if (_format == AVID_PAL_25I_PROJECT)
                {
                    cdciDescriptor->setEssenceContainer(trackData->essenceContainerLabel);
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DVBased_25_625_50));
                    cdciDescriptor->setSampleRate(trackData->editRate);
                    cdciDescriptor->setAspectRatio(_imageAspectRatio);
                    cdciDescriptor->appendVideoLineMap(23);
                    cdciDescriptor->appendVideoLineMap(335);
                    cdciDescriptor->setStoredWidth(720);
                    cdciDescriptor->setStoredHeight(288);
                    cdciDescriptor->setDisplayWidth(720);
                    cdciDescriptor->setDisplayHeight(288);
                    cdciDescriptor->setDisplayXOffset(0);
                    cdciDescriptor->setDisplayYOffset(0);
                    cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                    if (_useLegacy)
                    {
                        cdciDescriptor->setFrameLayout(MXF_MIXED_FIELDS);
                        cdciDescriptor->setHorizontalSubsampling(2);
                        cdciDescriptor->setVerticalSubsampling(2);
                    }
                    else
                    {
                        cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                        cdciDescriptor->setHorizontalSubsampling(4);
                        cdciDescriptor->setVerticalSubsampling(1);
                    }
                    cdciDescriptor->setImageAlignmentOffset(1);
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->setBlackRefLevel(16);
                    cdciDescriptor->setWhiteReflevel(235);
                    cdciDescriptor->setColorRange(225);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0x8c);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                }
                else // AVID_NTSC_30I_PROJECT
                {
                    cdciDescriptor->setEssenceContainer(trackData->essenceContainerLabel);
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DVBased_25_525_60));
                    cdciDescriptor->setSampleRate(trackData->editRate);
                    cdciDescriptor->setAspectRatio(_imageAspectRatio);
                    cdciDescriptor->appendVideoLineMap(23);
                    cdciDescriptor->appendVideoLineMap(285);
                    cdciDescriptor->setStoredWidth(720);
                    cdciDescriptor->setStoredHeight(240);
                    cdciDescriptor->setDisplayWidth(720);
                    cdciDescriptor->setDisplayHeight(240);
                    cdciDescriptor->setDisplayXOffset(0);
                    cdciDescriptor->setDisplayYOffset(0);
                    cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                    if (_useLegacy)
                    {
                        cdciDescriptor->setFrameLayout(MXF_MIXED_FIELDS);
                        cdciDescriptor->setHorizontalSubsampling(1);
                        cdciDescriptor->setVerticalSubsampling(1);
                    }
                    else
                    {
                        cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                        cdciDescriptor->setHorizontalSubsampling(4);
                        cdciDescriptor->setVerticalSubsampling(1);
                    }
                    cdciDescriptor->setImageAlignmentOffset(1);
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->setBlackRefLevel(16);
                    cdciDescriptor->setWhiteReflevel(235);
                    cdciDescriptor->setColorRange(225);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0x8c);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                }
                break;

            case AVID_DVBASED50_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                if (_format == AVID_PAL_25I_PROJECT)
                {
                    cdciDescriptor->setEssenceContainer(trackData->essenceContainerLabel);
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DVBased_50_625_50));
                    cdciDescriptor->setSampleRate(trackData->editRate);
                    cdciDescriptor->setAspectRatio(_imageAspectRatio);
                    cdciDescriptor->appendVideoLineMap(23);
                    cdciDescriptor->appendVideoLineMap(335);
                    cdciDescriptor->setStoredWidth(720);
                    cdciDescriptor->setStoredHeight(288);
                    cdciDescriptor->setDisplayWidth(720);
                    cdciDescriptor->setDisplayHeight(288);
                    cdciDescriptor->setDisplayXOffset(0);
                    cdciDescriptor->setDisplayYOffset(0);
                    cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                    if (_useLegacy)
                    {
                        cdciDescriptor->setFrameLayout(MXF_MIXED_FIELDS);
                    }
                    else
                    {
                        cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                    }
                    cdciDescriptor->setHorizontalSubsampling(2);
                    cdciDescriptor->setVerticalSubsampling(1);
                    cdciDescriptor->setImageAlignmentOffset(1);
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->setBlackRefLevel(16);
                    cdciDescriptor->setWhiteReflevel(235);
                    cdciDescriptor->setColorRange(225);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0x8e);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                }
                else // AVID_NTSC_30I_PROJECT
                {
                    cdciDescriptor->setEssenceContainer(trackData->essenceContainerLabel);
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DVBased_50_525_60));
                    cdciDescriptor->setSampleRate(trackData->editRate);
                    cdciDescriptor->setAspectRatio(_imageAspectRatio);
                    cdciDescriptor->appendVideoLineMap(23);
                    cdciDescriptor->appendVideoLineMap(285);
                    cdciDescriptor->setStoredWidth(720);
                    cdciDescriptor->setStoredHeight(240);
                    cdciDescriptor->setDisplayWidth(720);
                    cdciDescriptor->setDisplayHeight(240);
                    cdciDescriptor->setDisplayXOffset(0);
                    cdciDescriptor->setDisplayYOffset(0);
                    cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                    cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                    cdciDescriptor->setHorizontalSubsampling(2);
                    cdciDescriptor->setVerticalSubsampling(1);
                    cdciDescriptor->setImageAlignmentOffset(1);
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->setBlackRefLevel(16);
                    cdciDescriptor->setWhiteReflevel(235);
                    cdciDescriptor->setColorRange(225);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0x8e);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                }
                break;

            case AVID_DV1080I50_ESSENCE:
            case AVID_DV720P50_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->appendVideoLineMap(21);
                cdciDescriptor->appendVideoLineMap(584);
                cdciDescriptor->setStoredWidth(1920);
                cdciDescriptor->setStoredHeight(540);
                cdciDescriptor->setDisplayWidth(1920);
                cdciDescriptor->setDisplayHeight(540);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(0);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                // no resolution is set in DV 100 files
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                if (trackData->type == AVID_DV1080I50_ESSENCE)
                {
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DVBased_100_1080_50_I));
                }
                else // AVID_DV720P50_ESSENCE
                {
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DVBased_100_720_50_P));
                }
                break;

            case AVID_MPEG30_ESSENCE:
            case AVID_MPEG40_ESSENCE:
            case AVID_MPEG50_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                cdciDescriptor->setEssenceContainer(trackData->essenceContainerLabel);
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->appendVideoLineMap(7);
                cdciDescriptor->appendVideoLineMap(320);
                cdciDescriptor->setStoredWidth(720);
                cdciDescriptor->setStoredHeight(304);
                cdciDescriptor->setDisplayWidth(720);
                cdciDescriptor->setDisplayHeight(288);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(16);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                if (trackData->type == AVID_MPEG30_ESSENCE)
                {
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(D10_30_625_50));
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0xa2);
                }
                else if (trackData->type == AVID_MPEG40_ESSENCE)
                {
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(D10_40_625_50));
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0xa1);
                }
                else // trackData->type == AVID_MPEG50_ESSENCE
                {
                    cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(D10_50_625_50));
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0xa0);
                }
                break;

            case AVID_DNXHD_1235_ESSENCE_TYPE:
            case AVID_DNXHD_1237_ESSENCE_TYPE:
            case AVID_DNXHD_1238_ESSENCE_TYPE:
            case AVID_DNXHD_1241_ESSENCE_TYPE:
            case AVID_DNXHD_1242_ESSENCE_TYPE:
            case AVID_DNXHD_1243_ESSENCE_TYPE:
            case AVID_DNXHD_1250_ESSENCE_TYPE:
            case AVID_DNXHD_1251_ESSENCE_TYPE:
            case AVID_DNXHD_1252_ESSENCE_TYPE:
            case AVID_DNXHD_1253_ESSENCE_TYPE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DNxHD));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(0);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(8192);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                if (trackData->type == AVID_DNXHD_1235_ESSENCE_TYPE)
                {
                    cdciDescriptor->setComponentDepth(10);
                    cdciDescriptor->appendVideoLineMap(42);
                    cdciDescriptor->appendVideoLineMap(0);
                    cdciDescriptor->setStoredWidth(1920);
                    cdciDescriptor->setStoredHeight(1080);
                    cdciDescriptor->setDisplayWidth(1920);
                    cdciDescriptor->setDisplayHeight(1080);
                    cdciDescriptor->setFrameLayout(MXF_FULL_FRAME);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1235);
                }
                else if (trackData->type == AVID_DNXHD_1237_ESSENCE_TYPE)
                {
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->appendVideoLineMap(42);
                    cdciDescriptor->appendVideoLineMap(0);
                    cdciDescriptor->setStoredWidth(1920);
                    cdciDescriptor->setStoredHeight(1080);
                    cdciDescriptor->setDisplayWidth(1920);
                    cdciDescriptor->setDisplayHeight(1080);
                    cdciDescriptor->setFrameLayout(MXF_FULL_FRAME);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1237);
                }
                else if (trackData->type == AVID_DNXHD_1238_ESSENCE_TYPE)
                {
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->appendVideoLineMap(42);
                    cdciDescriptor->appendVideoLineMap(0);
                    cdciDescriptor->setStoredWidth(1920);
                    cdciDescriptor->setStoredHeight(1080);
                    cdciDescriptor->setDisplayWidth(1920);
                    cdciDescriptor->setDisplayHeight(1080);
                    cdciDescriptor->setFrameLayout(MXF_FULL_FRAME);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1238);
                }
                else if (trackData->type == AVID_DNXHD_1242_ESSENCE_TYPE)
                {
                    cdciDescriptor->setComponentDepth(10);
                    cdciDescriptor->appendVideoLineMap(21);
                    cdciDescriptor->appendVideoLineMap(584);
                    cdciDescriptor->setStoredWidth(1920);
                    cdciDescriptor->setStoredHeight(540);
                    cdciDescriptor->setDisplayWidth(1920);
                    cdciDescriptor->setDisplayHeight(540);
                    cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1242);
                }
                else if (trackData->type == AVID_DNXHD_1243_ESSENCE_TYPE)
                {
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->appendVideoLineMap(21);
                    cdciDescriptor->appendVideoLineMap(584);
                    cdciDescriptor->setStoredWidth(1920);
                    cdciDescriptor->setStoredHeight(540);
                    cdciDescriptor->setDisplayWidth(1920);
                    cdciDescriptor->setDisplayHeight(540);
                    cdciDescriptor->setFrameLayout(MXF_SEPARATE_FIELDS);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1243);
                }
                else if (trackData->type == AVID_DNXHD_1250_ESSENCE_TYPE)
                {
                    cdciDescriptor->setComponentDepth(10);
                    cdciDescriptor->appendVideoLineMap(26);
                    cdciDescriptor->appendVideoLineMap(0);
                    cdciDescriptor->setStoredWidth(1280);
                    cdciDescriptor->setStoredHeight(720);
                    cdciDescriptor->setDisplayWidth(1280);
                    cdciDescriptor->setDisplayHeight(720);
                    cdciDescriptor->setFrameLayout(MXF_FULL_FRAME);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1250);
                }
                else if (trackData->type == AVID_DNXHD_1251_ESSENCE_TYPE)
                {
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->appendVideoLineMap(26);
                    cdciDescriptor->appendVideoLineMap(0);
                    cdciDescriptor->setStoredWidth(1280);
                    cdciDescriptor->setStoredHeight(720);
                    cdciDescriptor->setDisplayWidth(1280);
                    cdciDescriptor->setDisplayHeight(720);
                    cdciDescriptor->setFrameLayout(MXF_FULL_FRAME);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1251);
                }
                else if (trackData->type == AVID_DNXHD_1252_ESSENCE_TYPE)
                {
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->appendVideoLineMap(26);
                    cdciDescriptor->appendVideoLineMap(0);
                    cdciDescriptor->setStoredWidth(1280);
                    cdciDescriptor->setStoredHeight(720);
                    cdciDescriptor->setDisplayWidth(1280);
                    cdciDescriptor->setDisplayHeight(720);
                    cdciDescriptor->setFrameLayout(MXF_FULL_FRAME);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1252);
                }
                else // AVID_DNXHD_1253_ESSENCE_TYPE
                {
                    cdciDescriptor->setComponentDepth(8);
                    cdciDescriptor->appendVideoLineMap(42);
                    cdciDescriptor->appendVideoLineMap(0);
                    cdciDescriptor->setStoredWidth(1920);
                    cdciDescriptor->setStoredHeight(1080);
                    cdciDescriptor->setDisplayWidth(1920);
                    cdciDescriptor->setDisplayHeight(1080);
                    cdciDescriptor->setFrameLayout(MXF_FULL_FRAME);
                    cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 1253);
                }
                break;

            case AVID_UNCUYVY_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_MIXED_FIELDS);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                cdciDescriptor->setImageAlignmentOffset(g_uncImageAlignmentOffset);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0xaa);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                if (_format == AVID_PAL_25I_PROJECT)
                {
                    cdciDescriptor->appendVideoLineMap(15);
                    cdciDescriptor->appendVideoLineMap(328);
                    cdciDescriptor->setStoredWidth(720);
                    cdciDescriptor->setStoredHeight(592);
                    cdciDescriptor->setDisplayWidth(720);
                    cdciDescriptor->setDisplayHeight(576);
                    cdciDescriptor->setDisplayXOffset(0);
                    cdciDescriptor->setDisplayYOffset(16);
                    cdciDescriptor->setImageAlignmentOffset(g_uncImageAlignmentOffset);
                    cdciDescriptor->setImageStartOffset(g_uncPALStartOffsetSize);
                }
                else // AVID_NTSCL_30I_PROJECT
                {
                    cdciDescriptor->appendVideoLineMap(16);
                    cdciDescriptor->appendVideoLineMap(278);
                    cdciDescriptor->setStoredWidth(720);
                    cdciDescriptor->setStoredHeight(496);
                    cdciDescriptor->setDisplayWidth(720);
                    cdciDescriptor->setDisplayHeight(486);
                    cdciDescriptor->setDisplayXOffset(0);
                    cdciDescriptor->setDisplayYOffset(10);
                    cdciDescriptor->setImageStartOffset(g_uncNTSCStartOffsetSize);
                }
                break;

            case AVID_UNC1080IUYVY_ESSENCE:
                cdciDescriptor = new CDCIEssenceDescriptor(trackData->headerMetadata);
                descriptor = cdciDescriptor;

                // Avid requires this essence container label
                cdciDescriptor->setEssenceContainer(MXF_EC_L(AvidAAFKLVEssenceContainer));
                cdciDescriptor->setPictureEssenceCoding(MXF_CMDEF_L(DNxHD));
                cdciDescriptor->setSampleRate(trackData->editRate);
                cdciDescriptor->setAspectRatio(_imageAspectRatio);
                cdciDescriptor->appendVideoLineMap(21);
                cdciDescriptor->appendVideoLineMap(584);
                cdciDescriptor->setStoredWidth(1920);
                cdciDescriptor->setStoredHeight(1080);
                cdciDescriptor->setDisplayWidth(1920);
                cdciDescriptor->setDisplayHeight(1080);
                cdciDescriptor->setDisplayXOffset(0);
                cdciDescriptor->setDisplayYOffset(0);
                cdciDescriptor->setColorSiting(MXF_COLOR_SITING_REC601);
                cdciDescriptor->setFrameLayout(MXF_MIXED_FIELDS);
                cdciDescriptor->setHorizontalSubsampling(2);
                cdciDescriptor->setVerticalSubsampling(1);
                cdciDescriptor->setImageAlignmentOffset(g_uncImageAlignmentOffset);
                cdciDescriptor->setImageStartOffset(g_unc1080i50StartOffsetSize);
                cdciDescriptor->setComponentDepth(8);
                cdciDescriptor->setBlackRefLevel(16);
                cdciDescriptor->setWhiteReflevel(235);
                cdciDescriptor->setColorRange(225);
                // TODO: resolution ID?
                //cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ResolutionID), 0x00);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameSampleSize), trackData->editUnitByteCount);
                cdciDescriptor->setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageSize), 0);
                break;

            case AVID_PCM_ESSENCE:
                waveDescriptor = new WaveAudioDescriptor(trackData->headerMetadata);
                descriptor = waveDescriptor;

                waveDescriptor->setEssenceContainer(trackData->essenceContainerLabel);
                waveDescriptor->setSampleRate(trackData->editRate);
                waveDescriptor->setAudioSamplingRate(trackData->editRate);
                waveDescriptor->setChannelCount(1);
                waveDescriptor->setQuantizationBits(trackData->audioQuantizationBits);
                waveDescriptor->setBlockAlign((uint8_t)((trackData->audioQuantizationBits + 7) / 8));
                waveDescriptor->setAvgBps(trackData->editRate.numerator * waveDescriptor->getBlockAlign() /
                    trackData->editRate.denominator);
                break;
        }

        fileSourcePackage->setDescriptor(descriptor);
        trackData->setsForDurationUpdate.push_back(new FileDescriptorForUpdate(dynamic_cast<FileDescriptor*>(descriptor)));


        // Preface - ContentStorage - EssenceContainerData
        EssenceContainerData *essContainerData = new EssenceContainerData(trackData->headerMetadata);
        contentStorage->appendEssenceContainerData(essContainerData);
        essContainerData->setLinkedPackageUID(trackData->fileSourcePackageUID);
        essContainerData->setIndexSID(g_indexSID);
        essContainerData->setBodySID(g_bodySID);


        // Preface - ContentStorage - tape SourcePackage
        if (_tapeName.size() > 0)
        {
            uint16_t roundedTimecodeBase = (_format == AVID_PAL_25I_PROJECT) ? 25 : 30;
            int64_t tapeLen = 120 * 60 * 60 * roundedTimecodeBase;

            // Preface - ContentStorage - tape SourcePackage
            SourcePackage *tapeSourcePackage = new SourcePackage(trackData->headerMetadata);
            contentStorage->appendPackages(tapeSourcePackage);
            tapeSourcePackage->setPackageUID(tapeSourcePackageUID);
            tapeSourcePackage->setPackageCreationDate(now);
            tapeSourcePackage->setPackageModifiedDate(now);
            tapeSourcePackage->setName(_tapeName);

            // Preface - ContentStorage - tape SourcePackage - MobAttribute(TaggedValue)
            if (_projectName.size() > 0)
            {
                tapeSourcePackage->attachAvidAttribute("_PJ", _projectName);
            }

            // Preface - ContentStorage - tape SourcePackage - Timeline Tracks
            uint32_t trackId;
            for (trackInnerIter = _tracks.begin(), trackId = 1; trackInnerIter != _tracks.end(); trackInnerIter++, trackId++)
            {
                TrackData *innerTrackData = (*trackInnerIter).second;

                // Preface - ContentStorage - tape SourcePackage - Timeline Track
                Track *track = new Track(trackData->headerMetadata);
                tapeSourcePackage->appendTracks(track);
                track->setTrackName(get_track_name(innerTrackData->isPicture, innerTrackData->trackNumber));
                track->setTrackID(trackId);
                track->setTrackNumber(innerTrackData->trackNumber);
                track->setEditRate(_projectEditRate);
                track->setOrigin(0);

                // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
                Sequence *sequence = new Sequence(trackData->headerMetadata);
                track->setSequence(sequence);
                sequence->setDataDefinition(innerTrackData->dataDef);
                sequence->setDuration(tapeLen);

                // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
                SourceClip *sourceClip = new SourceClip(trackData->headerMetadata);
                sequence->appendStructuralComponents(sourceClip);
                sourceClip->setDataDefinition(innerTrackData->dataDef);
                sourceClip->setDuration(tapeLen);
                sourceClip->setStartPosition(0);
                sourceClip->setSourceTrackID(0);
                sourceClip->setSourcePackageID(g_Null_UMID);
            }

            // Preface - ContentStorage - tape SourcePackage - timecode Timeline Track
            Track *timecodeTrack = new Track(trackData->headerMetadata);
            tapeSourcePackage->appendTracks(timecodeTrack);
            timecodeTrack->setTrackName("TC1");
            timecodeTrack->setTrackID(trackId);
            timecodeTrack->setTrackNumber(1);
            timecodeTrack->setEditRate(_projectEditRate);
            timecodeTrack->setOrigin(0);

            // Preface - ContentStorage - tape SourcePackage - timecode Timeline Track - Sequence
            Sequence *sequence = new Sequence(trackData->headerMetadata);
            timecodeTrack->setSequence(sequence);
            sequence->setDataDefinition(MXF_DDEF_L(Timecode));
            sequence->setDuration(tapeLen);

            // Preface - ContentStorage - tape SourcePackage - Timecode Track - TimecodeComponent
            TimecodeComponent *timecodeComponent = new TimecodeComponent(trackData->headerMetadata);
            sequence->appendStructuralComponents(timecodeComponent);
            timecodeComponent->setDataDefinition(MXF_DDEF_L(Timecode));
            timecodeComponent->setDuration(tapeLen);
            timecodeComponent->setRoundedTimecodeBase(roundedTimecodeBase);
            timecodeComponent->setDropFrame(_dropFrame);
            timecodeComponent->setStartTimecode(0);

            // Preface - ContentStorage - tape SourcePackage - TapeDescriptor
            GenericDescriptor *tapeDecriptor = dynamic_cast<GenericDescriptor*>(
                trackData->headerMetadata->createAndWrap(&MXF_SET_K(TapeDescriptor)));
            tapeSourcePackage->setDescriptor(tapeDecriptor);
        }


        // write the header metadata

        trackData->headerMetadataStartPos = trackData->mxfFile->tell(); // need this position when we re-write the header metadata
        PositionFillerWriter filler(g_fixedBodyPPOffset);
        trackData->headerMetadata->write(trackData->mxfFile, &headerPartition, &filler);



        // write the closed, complete body partition pack

        Partition& bodyPartition = trackData->mxfFile->createPartition();
        bodyPartition.setKey(&MXF_PP_K(ClosedComplete, Body));
        bodyPartition.setBodySID(g_bodySID);

        bodyPartition.write(trackData->mxfFile);


        // update the partition packs
        trackData->mxfFile->updatePartitions();


        // create the index table segment

        mxf_generate_uuid(&uuid);
        trackData->indexSegment = new IndexTableSegment();
        trackData->indexSegment->setInstanceUID(uuid);
        trackData->indexSegment->setIndexEditRate(trackData->editRate);
        trackData->indexSegment->setIndexDuration(0);
        trackData->indexSegment->setEditUnitByteCount(trackData->editUnitByteCount);
        trackData->indexSegment->setIndexSID(g_indexSID);
        trackData->indexSegment->setBodySID(g_bodySID);
    }


    _readyToWrite = true;
}

void AvidClipWriter::writeSamples(int trackId, uint32_t numSamples, unsigned char *data, uint32_t dataLen)
{
    MXFPP_ASSERT(_readyToWrite && !_endedWriting);

    map<int, TrackData*>::iterator result = _tracks.find(trackId);
    MXFPP_CHECK(result != _tracks.end());

    TrackData *trackData = (*result).second;

    if (trackData->trackDuration == 0)
    {
        // write the essence data KL and record the file position for update when we complete writing
        trackData->essenceDataStartPos = trackData->mxfFile->tell();
        trackData->mxfFile->writeFixedKL(&trackData->essenceElementKey, trackData->essenceElementLLen, 0);
    }

    switch (trackData->type)
    {
        case AVID_MJPEG21_ESSENCE:
        case AVID_MJPEG31_ESSENCE:
        case AVID_MJPEG101_ESSENCE:
        case AVID_MJPEG101m_ESSENCE:
        case AVID_MJPEG151s_ESSENCE:
        case AVID_MJPEG201_ESSENCE:
            // only single sample is currently supported
            MXFPP_CHECK(numSamples == 1);

            // record frame offset for the index table
            trackData->frameOffsets.push_back(trackData->essenceDataLen);

            // write the sample data
            trackData->essenceDataLen += trackData->mxfFile->write(data, dataLen);
            break;

        case AVID_IECDV25_ESSENCE:
        case AVID_DVBASED25_ESSENCE:
        case AVID_DVBASED50_ESSENCE:
        case AVID_DV1080I50_ESSENCE:
        case AVID_DV720P50_ESSENCE:
        case AVID_MPEG30_ESSENCE:
        case AVID_MPEG40_ESSENCE:
        case AVID_MPEG50_ESSENCE:
        case AVID_DNXHD_1235_ESSENCE_TYPE:
        case AVID_DNXHD_1237_ESSENCE_TYPE:
        case AVID_DNXHD_1238_ESSENCE_TYPE:
        case AVID_DNXHD_1241_ESSENCE_TYPE:
        case AVID_DNXHD_1242_ESSENCE_TYPE:
        case AVID_DNXHD_1243_ESSENCE_TYPE:
        case AVID_DNXHD_1250_ESSENCE_TYPE:
        case AVID_DNXHD_1251_ESSENCE_TYPE:
        case AVID_DNXHD_1252_ESSENCE_TYPE:
        case AVID_DNXHD_1253_ESSENCE_TYPE:
            // write the sample data
            MXFPP_CHECK(trackData->editUnitByteCount * numSamples == dataLen);
            trackData->essenceDataLen += trackData->mxfFile->write(data, dataLen);
            break;

        case AVID_UNCUYVY_ESSENCE:
            {
                MXFPP_CHECK(trackData->editUnitByteCount * numSamples == dataLen);

                uint32_t i;
                unsigned char *dataPtr = data;
                uint32_t sampleSize = dataLen / numSamples;
                for (i = 0; i < numSamples; i++)
                {
                    // write start offset data, VBI data and a single sample data
                    trackData->essenceDataLen += trackData->mxfFile->write(trackData->startOffsetData, trackData->startOffsetDataSize);
                    trackData->essenceDataLen += trackData->mxfFile->write(trackData->vbiData, trackData->vbiDataSize);
                    trackData->essenceDataLen += trackData->mxfFile->write(dataPtr, sampleSize);

                    dataPtr += sampleSize;
                }
            }
            break;

        case AVID_UNC1080IUYVY_ESSENCE:
            {
                MXFPP_CHECK(trackData->editUnitByteCount * numSamples == dataLen);

                uint32_t i;
                unsigned char *dataPtr = data;
                uint32_t sampleSize = dataLen / numSamples;
                for (i = 0; i < numSamples; i++)
                {
                    // write start offset data and a single sample data
                    trackData->essenceDataLen += trackData->mxfFile->write(trackData->startOffsetData, trackData->startOffsetDataSize);
                    trackData->essenceDataLen += trackData->mxfFile->write(dataPtr, sampleSize);

                    dataPtr += sampleSize;
                }
            }
            break;

        case AVID_PCM_ESSENCE:
            // write the sample data
            MXFPP_CHECK(trackData->editUnitByteCount * numSamples == dataLen);
            trackData->essenceDataLen += trackData->mxfFile->write(data, dataLen);
            break;
    }

    trackData->trackDuration += numSamples;
}

void AvidClipWriter::completeWrite()
{
    MXFPP_ASSERT(_readyToWrite && !_endedWriting);

    _endedWriting = true;


    // get the minumum duration, which is the clip duration
    int64_t duration = -1; // in _projectEditRate units
    int64_t normalizedTrackDuration;
    map<int, TrackData*>::const_iterator trackIter;
    for (trackIter = _tracks.begin(); trackIter != _tracks.end(); trackIter++)
    {
        TrackData *trackData = (*trackIter).second;

        normalizedTrackDuration = calc_position(trackData->trackDuration, trackData->editRate, _projectEditRate);
        if (duration == -1 || normalizedTrackDuration < duration)
        {
            duration = normalizedTrackDuration;
        }
    }

    for (trackIter = _tracks.begin(); trackIter != _tracks.end(); trackIter++)
    {
        TrackData *trackData = (*trackIter).second;

        // fill to body partition KAG boundary
        trackData->mxfFile->getPartition(1).fillToKag(trackData->mxfFile);


        // write the footer partition pack
        Partition& footerPartition = trackData->mxfFile->createPartition();
        footerPartition.setKey(&MXF_PP_K(ClosedComplete, Footer));
        footerPartition.setBodySID(0);
        footerPartition.setIndexSID(g_indexSID);
        footerPartition.write(trackData->mxfFile);


        // write the index table segment

        trackData->indexSegment->setIndexDuration(trackData->trackDuration);

        if (trackData->type == AVID_MJPEG21_ESSENCE ||
            trackData->type == AVID_MJPEG31_ESSENCE ||
            trackData->type == AVID_MJPEG101_ESSENCE ||
            trackData->type == AVID_MJPEG101m_ESSENCE ||
            trackData->type == AVID_MJPEG151s_ESSENCE ||
            trackData->type == AVID_MJPEG201_ESSENCE)
        {
            // Avid has extra entry at the end
            trackData->frameOffsets.push_back(trackData->essenceDataLen);

            // write the index header followed by the index entries

            footerPartition.markIndexStart(trackData->mxfFile);

            // TODO: writing the index table like this can become unneccessarily slow ?

            trackData->indexSegment->writeHeader(trackData->mxfFile, 0, (uint32_t)trackData->frameOffsets.size());
            if (trackData->frameOffsets.size() > 0)
            {
                // Avid ignores the local set 16-bit size restriction and relies on the array header to
                // provide the size
                trackData->indexSegment->writeAvidIndexEntryArrayHeader(trackData->mxfFile, 0, 0,
                    (uint32_t)trackData->frameOffsets.size());

                vector<uint32_t> sliceOffset; // empty
                vector<mxfRational> posTable; // empty
                vector<int64_t>::const_iterator offsetIter;
                for (offsetIter = trackData->frameOffsets.begin(); offsetIter != trackData->frameOffsets.end(); offsetIter++)
                {
                    trackData->indexSegment->writeIndexEntry(trackData->mxfFile, 0, 0, 0x80 /* random access */,
                        (*offsetIter), sliceOffset, posTable);
                }
            }

            // fill to footer partition KAG boundary
            footerPartition.fillToKag(trackData->mxfFile);

            footerPartition.markIndexEnd(trackData->mxfFile);
        }
        else
        {
            trackData->indexSegment->write(trackData->mxfFile, &footerPartition, 0);
        }


        // write the RIP
        trackData->mxfFile->writeRIP();


        // update the metadata sets that have a duration defined
        vector<SetForDurationUpdate*>::iterator updateIter;
        for (updateIter = trackData->setsForDurationUpdate.begin(); updateIter != trackData->setsForDurationUpdate.end(); updateIter++)
        {
            (*updateIter)->updateDuration(duration, _projectEditRate);
            (*updateIter)->updateEssenceDataLength(trackData->essenceDataLen);
        }
        trackData->mxfFile->getPartition(0).setKey(&MXF_PP_K(ClosedComplete, Header));


        // re-write the header metadata
        trackData->mxfFile->seek(trackData->headerMetadataStartPos, SEEK_SET);
        PositionFillerWriter filler(g_fixedBodyPPOffset);
        trackData->headerMetadata->write(trackData->mxfFile, &trackData->mxfFile->getPartition(0), &filler);


        // update the essence data len
        trackData->mxfFile->seek(trackData->essenceDataStartPos, SEEK_SET);
        trackData->mxfFile->writeFixedKL(&trackData->essenceElementKey, trackData->essenceElementLLen,
            trackData->essenceDataLen);


        // update the partition packs
        trackData->mxfFile->updatePartitions();
    }


    // clean up

    for (trackIter = _tracks.begin(); trackIter != _tracks.end(); trackIter++)
    {
        delete (*trackIter).second;
    }
    _tracks.clear();

}

void AvidClipWriter::abortWrite(bool deleteFiles)
{
    MXFPP_ASSERT(_readyToWrite && !_endedWriting);

    _endedWriting = true;


    // clean up, removing any files written if deleteFiles is true

    map<int, TrackData*>::const_iterator trackIter;
    for (trackIter = _tracks.begin(); trackIter != _tracks.end(); trackIter++)
    {
        if (deleteFiles)
        {
            // close the file
            if ((*trackIter).second->mxfFile)
            {
                delete (*trackIter).second->mxfFile;
                (*trackIter).second->mxfFile = 0;
            }

            // remove the file
            if (remove((*trackIter).second->filename.c_str()) != 0)
            {
                mxf_log(MXF_WLOG, "Failed to remove aborted file '%s': %s\n", (*trackIter).second->filename.c_str(),
                    strerror(errno));
            }
        }
        delete (*trackIter).second;
    }
    _tracks.clear();
}



