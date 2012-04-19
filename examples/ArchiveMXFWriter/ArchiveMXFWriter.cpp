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

#include "ArchiveMXFWriter.h"

using namespace std;
using namespace mxfpp;


// the size of the System Item in the essence data
#define SYSTEM_ITEM_SIZE            28

// minimum llen to use for sets
#define MIN_LLEN                    4

#define MAX_AUDIO_TRACKS            8


// declare the BBC Archive MXF extensions

#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    static const mxfUL MXF_SET_K(name) = label;

#define MXF_ITEM_DEFINITION(setName, name, label, localTag, typeId, isRequired) \
    static const mxfUL MXF_ITEM_K(setName, name) = label;

#include "ArchiveMXFExtensionsDataModel.h"



static const mxfUL MXF_DM_L(D3P_D3PreservationDescriptiveScheme) =
    {0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x0d, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00};

static const mxfKey g_SystemItemElementKey =
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x14, 0x02, 0x01, 0x00};

static const mxfKey g_VideoItemElementKey =
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x15, 0x01, 0x02, 0x01};

static mxfKey g_AudioItemElementKey[8] =
{
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0xff, 0x01, 0x01},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0xff, 0x01, 0x02},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0xff, 0x01, 0x03},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0xff, 0x01, 0x04},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0xff, 0x01, 0x05},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0xff, 0x01, 0x06},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0xff, 0x01, 0x07},
    {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01, 0x16, 0xff, 0x01, 0x08},
};

static const mxfUUID g_identProductUID =
    {0x8c, 0x6b, 0x0e, 0xc8, 0xab, 0x70, 0x4a, 0x23, 0x9d, 0x62, 0x81, 0x63, 0x83, 0xc3, 0x74, 0x1e};
static const char *g_identCompanyName = "BBC";
static const char *g_identProductName = "D3 Preservation MXF Writer (cpp)";
static const char *g_identVersionString = "trial version";


static const uint32_t g_bodySID = 1;
static const uint32_t g_indexSID = 2;


static const mxfRational g_audioSampleRate = {48000, 1};
static const mxfRational g_audioEditRate = {25, 1};
static const uint32_t g_audioQuantBits = 20;
static const uint16_t g_audioBlockAlign = 3;
static const uint32_t g_audioAvgBps = 144000; /* 48000*3 */
static const uint16_t g_audioSamplesPerFrame = 1920; /* 48000/25 */
static const uint32_t g_audioFrameSize = 1920 * 3;

static const mxfRational g_videoSampleRate = {25, 1};
static const mxfRational g_videoEditRate = {25, 1};
static const uint8_t g_videoFrameLayout = MXF_MIXED_FIELDS;
static const uint32_t g_videoStoredHeight = 576; /* for mixed fields this is the frame height */
static const uint32_t g_videoStoredWidth = 720;
static const int32_t g_videoLineMap[2] = {23, 336};
static const mxfRational g_videoAspectRatio = {4, 3};
static const uint32_t g_videoComponentDepth = 8;
static const uint32_t g_videoHorizontalSubSampling = 2;
static const uint32_t g_videoVerticalSubSampling = 1;
static const uint32_t g_videoFrameSize = 720 * 576 * 2; /* W x H x (Y + Cr/2 + Cb/2) */

static const int64_t g_tapeLen = 120 * 25 * 60 * 60;
static const int g_numTapeAudioTracks = MAX_AUDIO_TRACKS;

static const uint64_t g_fixedBodyOffset = 0x8000;


// functions for loading the BBC D3 MXF extensions

#define MXF_LABEL(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15) \
    {d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15}

#define MXF_SET_DEFINITION(parentName, name, label) \
    dataModel->registerSetDef(#name, &MXF_SET_K(parentName), &MXF_SET_K(name));

#define MXF_ITEM_DEFINITION(setName, name, label, tag, typeId, isRequired) \
    dataModel->registerItemDef(#name, &MXF_SET_K(setName), &MXF_ITEM_K(setName, name), tag, typeId, isRequired);

static void load_bbc_d3_extensions(DataModel *dataModel)
{
#include "ArchiveMXFExtensionsDataModel.h"
}


static void convert_timecode_to_12m(Timecode *t, unsigned char *t12m)
{
    /* the format follows the specification of the TimecodeArray property
    defined in SMPTE 405M, table 2, which follows section 8.2 of SMPTE 331M
    The Binary Group Data is not used and is set to 0 */

    memset(t12m, 0, 8);
    t12m[0] = ((t->frame % 10) & 0x0f) | (((t->frame / 10) & 0x3) << 4);
    t12m[1] = ((t->sec % 10) & 0x0f) | (((t->sec / 10) & 0x7) << 4);
    t12m[2] = ((t->min % 10) & 0x0f) | (((t->min / 10) & 0x7) << 4);
    t12m[3] = ((t->hour % 10) & 0x0f) | (((t->hour / 10) & 0x3) << 4);

}

static string get_track_name(string prefix, int number)
{
    char buf[16];
    sprintf(buf, "%d", number);
    return prefix.append(buf);
}

static int64_t calc_position(int64_t videoPosition, mxfRational targetEditRate)
{
    if (memcmp(&targetEditRate, &g_videoEditRate, sizeof(mxfRational)) == 0)
    {
        return videoPosition;
    }

    double factor = targetEditRate.numerator * g_videoEditRate.denominator /
        (double)(targetEditRate.denominator * g_videoEditRate.numerator);
    return (int64_t)(videoPosition * factor + 0.5);
}



namespace mxfpp
{

class SetForDurationUpdate
{
public:
    virtual ~SetForDurationUpdate() {}

    virtual void updateDuration(int64_t duration) = 0;
};

class StructComponentForUpdate : public SetForDurationUpdate
{
public:
    StructComponentForUpdate(StructuralComponent *component, mxfRational editRate)
    : _component(component), _editRate(editRate)
    {}
    virtual ~StructComponentForUpdate()
    {}

    virtual void updateDuration(int64_t duration)
    {
        _component->setDuration(calc_position(duration, _editRate));
    }

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

    virtual void updateDuration(int64_t duration)
    {
        _descriptor->setContainerDuration(calc_position(duration, _descriptor->getSampleRate()));
    }

private:
    FileDescriptor* _descriptor;
};



};




ArchiveMXFWriter::ArchiveMXFWriter(string filename, int numAudioTracks, int64_t startPosition)
: _numAudioTracks(numAudioTracks), _indexSegment(0), _duration(0), _headerMetadataStartPos(0),
_isComplete(false), _writeState(0)
{
    MXFPP_ASSERT(numAudioTracks <= MAX_CP_AUDIO_TRACKS);

    mxfUUID uuid;
    mxfTimestamp now;
    mxfUMID tapeSourcePackageUID;
    mxfUMID fileSourcePackageUID;
    mxfUMID materialPackageUID;
    int i;


    // inits
    mxf_get_timestamp_now(&now);
    mxf_generate_umid(&tapeSourcePackageUID);
    mxf_generate_umid(&fileSourcePackageUID);
    mxf_generate_umid(&materialPackageUID);
    for (i = 0; i < numAudioTracks; i++)
    {
        g_AudioItemElementKey[i].octet13 = numAudioTracks;
    }



    _mxfFile = File::openNew(filename);


    // set file minimum llen

    _mxfFile->setMinLLen(MIN_LLEN);


    // write the header partition pack

    Partition& headerPartition = _mxfFile->createPartition();
    headerPartition.setKey(&MXF_PP_K(ClosedComplete, Header));
    headerPartition.setBodySID(g_bodySID);
    headerPartition.setIndexSID(g_indexSID);
    headerPartition.setOperationalPattern(&MXF_OP_L(1a, MultiTrack_Stream_Internal));
    headerPartition.addEssenceContainer(&MXF_EC_L(MultipleWrappings));
    headerPartition.addEssenceContainer(&MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped));
    headerPartition.addEssenceContainer(&MXF_EC_L(BWFFrameWrapped));

    headerPartition.write(_mxfFile);


    // create the header metadata

    _dataModel = new DataModel();
    load_bbc_d3_extensions(_dataModel);
    _dataModel->finalise();
    _headerMetadata = new HeaderMetadata(_dataModel);

    // Preface
    Preface *preface = new Preface(_headerMetadata);
    preface->setLastModifiedDate(now);
    preface->setVersion(MXF_PREFACE_VER(1, 2));
    preface->setOperationalPattern(MXF_OP_L(1a, MultiTrack_Stream_Internal));
    if (numAudioTracks > 0)
    {
        preface->appendEssenceContainers(MXF_EC_L(MultipleWrappings));
        preface->appendEssenceContainers(MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped));
        preface->appendEssenceContainers(MXF_EC_L(BWFFrameWrapped));
    }
    else
    {
        preface->appendEssenceContainers(MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped));
    }
    preface->appendDMSchemes(MXF_DM_L(D3P_D3PreservationDescriptiveScheme));
    preface->setUInt32Item(&MXF_ITEM_K(Preface, D3P_D3ErrorCount), 0);
    preface->setUInt32Item(&MXF_ITEM_K(Preface, D3P_PSEFailureCount), 0);

    // Preface - Identification
    Identification *ident = new Identification(_headerMetadata);
    preface->appendIdentifications(ident);
    ident->initialise(g_identCompanyName, g_identProductName, g_identVersionString, g_identProductUID);

    // Preface - ContentStorage
    ContentStorage *contentStorage = new ContentStorage(_headerMetadata);
    preface->setContentStorage(contentStorage);

    // Preface - ContentStorage - EssenceContainerData
    EssenceContainerData *essContainerData = new EssenceContainerData(_headerMetadata);
    contentStorage->appendEssenceContainerData(essContainerData);
    essContainerData->setLinkedPackageUID(fileSourcePackageUID);
    essContainerData->setIndexSID(g_indexSID);
    essContainerData->setBodySID(g_bodySID);

    // Preface - ContentStorage - MaterialPackage
    MaterialPackage *materialPackage = new MaterialPackage(_headerMetadata);
    contentStorage->appendPackages(materialPackage);
    materialPackage->setPackageUID(materialPackageUID);
    materialPackage->setPackageCreationDate(now);
    materialPackage->setPackageModifiedDate(now);
    materialPackage->setName("D3 material");
    // TODO: allow name to be set

    // Preface - ContentStorage - MaterialPackage - Timecode Track
    Track *timecodeTrack = new Track(_headerMetadata);
    materialPackage->appendTracks(timecodeTrack);
    timecodeTrack->setTrackName("TC1");
    timecodeTrack->setTrackID(1);
    timecodeTrack->setTrackNumber(0);
    timecodeTrack->setEditRate(g_videoEditRate);
    timecodeTrack->setOrigin(0);

    // Preface - ContentStorage - MaterialPackage - Timecode Track - TimecodeComponent
    TimecodeComponent *timecodeComponent = new TimecodeComponent(_headerMetadata);
    timecodeTrack->setSequence(timecodeComponent);
    timecodeComponent->setDataDefinition(MXF_DDEF_L(Timecode));
    timecodeComponent->setDuration(-1); // updated when writing completed
    _setsForDurationUpdate.push_back(new StructComponentForUpdate(timecodeComponent, g_videoEditRate));
    timecodeComponent->setRoundedTimecodeBase(25);
    timecodeComponent->setDropFrame(false);
    timecodeComponent->setStartTimecode(0);

    // Preface - ContentStorage - MaterialPackage - Timeline Tracks
    // video track followed by numAudioTracks audio tracks
    for (i = 0; i < numAudioTracks + 1; i++)
    {
        bool isPicture = (i == 0);

        // Preface - ContentStorage - MaterialPackage - Timeline Track
        Track *track = new Track(_headerMetadata);
        materialPackage->appendTracks(track);
        track->setTrackName(isPicture ? "V1" : get_track_name("A", i));
        track->setTrackID(i + 2);
        track->setTrackNumber(0);
        track->setEditRate(isPicture ? g_videoEditRate : g_audioEditRate);
        track->setOrigin(0);

        // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence
        Sequence *sequence = new Sequence(_headerMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(isPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sequence->setDuration(-1); // updated when writing completed
        _setsForDurationUpdate.push_back(new StructComponentForUpdate(sequence, isPicture ? g_videoEditRate : g_audioEditRate));

        // Preface - ContentStorage - MaterialPackage - Timeline Track - Sequence - SourceClip
        SourceClip *sourceClip = new SourceClip(_headerMetadata);
        sequence->appendStructuralComponents(sourceClip);
        sourceClip->setDataDefinition(isPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sourceClip->setDuration(-1); // updated when writing completed
        _setsForDurationUpdate.push_back(new StructComponentForUpdate(sourceClip, isPicture ? g_videoEditRate : g_audioEditRate));
        sourceClip->setStartPosition(0);
        sourceClip->setSourceTrackID(i + 1);
        sourceClip->setSourcePackageID(fileSourcePackageUID);
    }

    // Preface - ContentStorage - SourcePackage
    SourcePackage *fileSourcePackage = new SourcePackage(_headerMetadata);
    contentStorage->appendPackages(fileSourcePackage);
    fileSourcePackage->setPackageUID(fileSourcePackageUID);
    fileSourcePackage->setPackageCreationDate(now);
    fileSourcePackage->setPackageModifiedDate(now);


    // Preface - ContentStorage - SourcePackage - Timeline Tracks
    // video track followed by numAudioTracks audio tracks
    for (i = 0; i < numAudioTracks + 1; i++)
    {
        bool isPicture = (i == 0);

        // Preface - ContentStorage - SourcePackage - Timeline Track
        Track *track = new Track(_headerMetadata);
        fileSourcePackage->appendTracks(track);
        track->setTrackName(isPicture ? "V1" : get_track_name("A", i));
        track->setTrackID(i + 1);
        if (isPicture)
        {
            uint32_t videoTrackNum = MXF_UNC_TRACK_NUM(0x00, 0x00, 0x00);
            mxf_complete_essence_element_track_num(&videoTrackNum, 1, MXF_UNC_FRAME_WRAPPED_EE_TYPE, 1);
            track->setTrackNumber(videoTrackNum);
        }
        else
        {
            uint32_t audioTrackNum = MXF_AES3BWF_TRACK_NUM(0x00, 0x00, 0x00);
            mxf_complete_essence_element_track_num(&audioTrackNum, numAudioTracks, MXF_BWF_FRAME_WRAPPED_EE_TYPE, i);
            track->setTrackNumber(audioTrackNum);
        }
        track->setEditRate(isPicture ? g_videoEditRate : g_audioEditRate);
        track->setOrigin(0);

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
        Sequence *sequence = new Sequence(_headerMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(isPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sequence->setDuration(-1); // updated when writing completed
        _setsForDurationUpdate.push_back(new StructComponentForUpdate(sequence, isPicture ? g_videoEditRate : g_audioEditRate));

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
        SourceClip *sourceClip = new SourceClip(_headerMetadata);
        sequence->appendStructuralComponents(sourceClip);
        sourceClip->setDataDefinition(isPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sourceClip->setDuration(-1); // updated when writing completed
        _setsForDurationUpdate.push_back(new StructComponentForUpdate(sourceClip, isPicture ? g_videoEditRate : g_audioEditRate));
        sourceClip->setStartPosition(calc_position(startPosition, isPicture ? g_videoEditRate : g_audioEditRate));
        sourceClip->setSourceTrackID(i + 1);
        sourceClip->setSourcePackageID(tapeSourcePackageUID);
    }


    // Preface - ContentStorage - SourcePackage - MultipleDescriptor
    MultipleDescriptor *multDescriptor = new MultipleDescriptor(_headerMetadata);
    fileSourcePackage->setDescriptor(multDescriptor);
    multDescriptor->setSampleRate(g_videoSampleRate);
    multDescriptor->setEssenceContainer(MXF_EC_L(MultipleWrappings));
    _setsForDurationUpdate.push_back(new FileDescriptorForUpdate(multDescriptor));

    // Preface - ContentStorage - SourcePackage - MultipleDescriptor - NetworkLocator
    NetworkLocator *locator = new NetworkLocator(_headerMetadata);
    multDescriptor->appendLocators(locator);
    locator->setURLString(filename);


    // Preface - ContentStorage - SourcePackage - MultipleDescriptor - CDCIEssenceDescriptor
    CDCIEssenceDescriptor *cdciDescriptor = new CDCIEssenceDescriptor(_headerMetadata);
    multDescriptor->appendSubDescriptorUIDs(cdciDescriptor);
    cdciDescriptor->setLinkedTrackID(1);
    cdciDescriptor->setSampleRate(g_videoSampleRate);
    cdciDescriptor->setEssenceContainer(MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped));
    cdciDescriptor->setFrameLayout(g_videoFrameLayout);
    cdciDescriptor->setStoredHeight(g_videoStoredHeight);
    cdciDescriptor->setStoredWidth(g_videoStoredWidth);
    cdciDescriptor->appendVideoLineMap(g_videoLineMap[0]);
    cdciDescriptor->appendVideoLineMap(g_videoLineMap[1]);
    cdciDescriptor->setAspectRatio(g_videoAspectRatio);
    cdciDescriptor->setComponentDepth(g_videoComponentDepth);
    cdciDescriptor->setHorizontalSubsampling(g_videoHorizontalSubSampling);
    cdciDescriptor->setVerticalSubsampling(g_videoVerticalSubSampling);
    _setsForDurationUpdate.push_back(new FileDescriptorForUpdate(cdciDescriptor));


    for (i = 0; i < numAudioTracks; i++)
    {
        // Preface - ContentStorage - SourcePackage - MultipleDescriptor - WAVEssenceDescriptor
        WaveAudioDescriptor *waveDescriptor = new WaveAudioDescriptor(_headerMetadata);
        multDescriptor->appendSubDescriptorUIDs(waveDescriptor);
        waveDescriptor->setLinkedTrackID(i + 2);
        waveDescriptor->setSampleRate(g_audioEditRate);
        waveDescriptor->setEssenceContainer(MXF_EC_L(BWFFrameWrapped));
        waveDescriptor->setAudioSamplingRate(g_audioSampleRate);
        waveDescriptor->setLocked(true);
        waveDescriptor->setChannelCount(1);
        waveDescriptor->setQuantizationBits(g_audioQuantBits);
        waveDescriptor->setBlockAlign(g_audioBlockAlign);
        waveDescriptor->setAvgBps(g_audioAvgBps);
        _setsForDurationUpdate.push_back(new FileDescriptorForUpdate(waveDescriptor));
    }


    // Preface - ContentStorage - tape SourcePackage
    SourcePackage *tapeSourcePackage = new SourcePackage(_headerMetadata);
    contentStorage->appendPackages(tapeSourcePackage);
    tapeSourcePackage->setPackageUID(tapeSourcePackageUID);
    tapeSourcePackage->setPackageCreationDate(now);
    tapeSourcePackage->setPackageModifiedDate(now);
    tapeSourcePackage->setName("D3 tape");
    // TODO: Name will be updated when complete() is called

    // Preface - ContentStorage - tape SourcePackage - Timeline Tracks
    // video track followed by g_numTapeAudioTracks audio tracks
    for (i = 0; i < g_numTapeAudioTracks + 1; i++)
    {
        bool isPicture = (i == 0);

        // Preface - ContentStorage - tape SourcePackage - Timeline Track
        Track *track = new Track(_headerMetadata);
        tapeSourcePackage->appendTracks(track);
        track->setTrackName(isPicture ? "V1" : get_track_name("A", i));
        track->setTrackID(i + 1);
        track->setTrackNumber(0);
        track->setEditRate(isPicture ? g_videoEditRate : g_audioEditRate);
        track->setOrigin(0);

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence
        Sequence *sequence = new Sequence(_headerMetadata);
        track->setSequence(sequence);
        sequence->setDataDefinition(isPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sequence->setDuration(g_tapeLen);

        // Preface - ContentStorage - SourcePackage - Timeline Track - Sequence - SourceClip
        SourceClip *sourceClip = new SourceClip(_headerMetadata);
        sequence->appendStructuralComponents(sourceClip);
        sourceClip->setDataDefinition(isPicture ? MXF_DDEF_L(Picture) : MXF_DDEF_L(Sound));
        sourceClip->setDuration(g_tapeLen);
        sourceClip->setStartPosition(calc_position(startPosition, isPicture ? g_videoEditRate : g_audioEditRate));
        sourceClip->setSourceTrackID(0);
        sourceClip->setSourcePackageID(g_Null_UMID);
    }

    // Preface - ContentStorage - tape SourcePackage - timecode Timeline Track
    timecodeTrack = new Track(_headerMetadata);
    tapeSourcePackage->appendTracks(timecodeTrack);
    timecodeTrack->setTrackName("TC1");
    timecodeTrack->setTrackID(g_numTapeAudioTracks + 2);
    timecodeTrack->setTrackNumber(0);
    timecodeTrack->setEditRate(g_videoEditRate);
    timecodeTrack->setOrigin(0);

    // Preface - ContentStorage - tape SourcePackage - timecode Timeline Track - Sequence
    Sequence *sequence = new Sequence(_headerMetadata);
    timecodeTrack->setSequence(sequence);
    sequence->setDataDefinition(MXF_DDEF_L(Timecode));
    sequence->setDuration(g_tapeLen);

    // Preface - ContentStorage - tape SourcePackage - Timecode Track - TimecodeComponent
    timecodeComponent = new TimecodeComponent(_headerMetadata);
    sequence->appendStructuralComponents(timecodeComponent);
    timecodeComponent->setDataDefinition(MXF_DDEF_L(Timecode));
    timecodeComponent->setDuration(g_tapeLen);
    timecodeComponent->setRoundedTimecodeBase(25);
    timecodeComponent->setDropFrame(false);
    timecodeComponent->setStartTimecode(0);

    // Preface - ContentStorage - tape SourcePackage - TapeDescriptor
    GenericDescriptor *tapeDecriptor = dynamic_cast<GenericDescriptor*>(
        _headerMetadata->createAndWrap(&MXF_SET_K(TapeDescriptor)));
    tapeSourcePackage->setDescriptor(tapeDecriptor);



    // write the header metadata

    _headerMetadataStartPos = _mxfFile->tell(); // need this position when we re-write the header metadata
    _headerMetadata->write(_mxfFile, &headerPartition, 0);



    // write the index table

    _indexSegment = new IndexTableSegment();
    mxf_generate_uuid(&uuid);
    _indexSegment->setInstanceUID(uuid);
    _indexSegment->setIndexEditRate(g_videoEditRate);
    _indexSegment->setIndexDuration(0); // although valid, will be updated when writing is completed
    _indexSegment->setIndexSID(g_indexSID);
    _indexSegment->setBodySID(g_bodySID);
    uint32_t deltaOffset = 0;
    _indexSegment->appendDeltaEntry(0, 0, deltaOffset); // System Item and element
    deltaOffset += mxfKey_extlen + 4 + SYSTEM_ITEM_SIZE;
    _indexSegment->appendDeltaEntry(0, 0, deltaOffset); // Video Item and element
    deltaOffset += mxfKey_extlen + 4 + g_videoFrameSize;
    for (i = 0; i < numAudioTracks; i++)  // Audio Item
    {
        _indexSegment->appendDeltaEntry(0, 0, deltaOffset); // Audio element
        deltaOffset += mxfKey_extlen + 4 + g_audioFrameSize;
    }
    _indexSegment->setEditUnitByteCount(deltaOffset);

    // write index table segment and leave some space for any future updates to the metadata
    // note: we assume there is space to write a filler
    PositionFillerWriter filler(g_fixedBodyOffset);
    _indexSegment->write(_mxfFile, &headerPartition, &filler);


}

ArchiveMXFWriter::~ArchiveMXFWriter()
{
    vector<SetForDurationUpdate*>::iterator iter;
    for (iter = _setsForDurationUpdate.begin(); iter != _setsForDurationUpdate.end(); iter++)
    {
        delete *iter;
    }

    delete _mxfFile;
    delete _dataModel;
    delete _headerMetadata;
    delete _indexSegment;
}

void ArchiveMXFWriter::writeTimecode(Timecode vitc, Timecode ltc)
{
    unsigned char t12m[8];

    MXFPP_ASSERT(!_isComplete);
    MXFPP_ASSERT(_writeState == 0);

    _mxfFile->writeFixedKL(&g_SystemItemElementKey, 4, SYSTEM_ITEM_SIZE);

    _mxfFile->writeUInt16(0x0102); // local tag
    _mxfFile->writeUInt16(SYSTEM_ITEM_SIZE - 4); // len
    _mxfFile->writeArrayHeader(2, 8); // VITC and LTC SMPTE-12M timecodes

    convert_timecode_to_12m(&vitc, t12m);
    MXFPP_CHECK(_mxfFile->write(t12m, 8) == 8);
    convert_timecode_to_12m(&ltc, t12m);
    MXFPP_CHECK(_mxfFile->write(t12m, 8) == 8);

    _writeState = 1; // write video
}

void ArchiveMXFWriter::writeVideoFrame(unsigned char *data, uint32_t size)
{
    MXFPP_ASSERT(!_isComplete);
    MXFPP_ASSERT(_writeState == 1);

    _mxfFile->writeFixedKL(&g_VideoItemElementKey, 4, size);
    MXFPP_CHECK(_mxfFile->write(data, size) == size);

    if (_numAudioTracks > 0)
    {
        _writeState = 2; // write audio 1
    }
    else
    {
        _duration++; // content package is complete
        _writeState = 0; // timecode
    }
}

void ArchiveMXFWriter::writeAudioFrame(unsigned char *data, uint32_t size)
{
    MXFPP_ASSERT(!_isComplete);
    MXFPP_ASSERT(_writeState >= 2 && _writeState <= _numAudioTracks + 1);

    _mxfFile->writeFixedKL(&g_AudioItemElementKey[_writeState - 2], 4, size);
    MXFPP_CHECK(_mxfFile->write(data, size) == size);

    _writeState++; // next audio
    if (_writeState > _numAudioTracks + 1)
    {
        _duration++; // content package is complete
        _writeState = 0; // timecode
    }
}

void ArchiveMXFWriter::writeContentPackage(ArchiveMXFContentPackage *cp)
{
    MXFPP_ASSERT(!_isComplete);

    MXFPP_CHECK(_numAudioTracks <= cp->getNumAudioTracks());

    writeTimecode(cp->getVITC(), cp->getLTC());
    writeVideoFrame(cp->getVideo()->getBytes(), cp->getVideo()->getSize());
    int i;
    for (i = 0; i < _numAudioTracks; i++)
    {
        writeAudioFrame(cp->getAudio(i)->getBytes(), cp->getAudio(i)->getSize());
    }
}

void ArchiveMXFWriter::complete()
{
    MXFPP_ASSERT(!_isComplete);
    MXFPP_ASSERT(_writeState == 0);

    _isComplete = 1;


    // write the footer partition pack
    Partition& footerPartition = _mxfFile->createPartition();
    footerPartition.setKey(&MXF_PP_K(ClosedComplete, Footer));

    footerPartition.write(_mxfFile);


    // write the RIP
    _mxfFile->writeRIP();


    // update the metadata sets that have a duration defined
    vector<SetForDurationUpdate*>::iterator iter;
    for (iter = _setsForDurationUpdate.begin(); iter != _setsForDurationUpdate.end(); iter++)
    {
        (*iter)->updateDuration(_duration);
    }

    // update the index table segment
    _indexSegment->setIndexDuration(_duration);


    // re-write the header metadata
    _mxfFile->seek(_headerMetadataStartPos, SEEK_SET);
    _headerMetadata->write(_mxfFile, &_mxfFile->getPartition(0), 0);


    // re-write the header index table segment
    PositionFillerWriter filler(g_fixedBodyOffset);
    _indexSegment->write(_mxfFile, &_mxfFile->getPartition(0), &filler);



    // update the partition packs
    _mxfFile->updatePartitions();
}


