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

#include <cstdio>

#include <memory>

#include "ArchiveMXFReader.h"

using namespace std;
using namespace mxfpp;


// timecode is clean if it is incrementing 1 frame at a time for this many frames
#define CLEAN_TIMECODE_THRESHOLD        10


static const mxfKey g_SystemItemElementKey =
    {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01 , 0x0d, 0x01, 0x03, 0x01, 0x14, 0x02, 0x01, 0x00};

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


static void convert_12m_to_timecode(unsigned char *t12m, Timecode *t)
{
    t->frame = ((t12m[0] >> 4) & 0x03) * 10 + (t12m[0] & 0xf);
    t->sec   = ((t12m[1] >> 4) & 0x07) * 10 + (t12m[1] & 0xf);
    t->min   = ((t12m[2] >> 4) & 0x07) * 10 + (t12m[2] & 0xf);
    t->hour  = ((t12m[3] >> 4) & 0x03) * 10 + (t12m[3] & 0xf);
    t->dropFrame = t12m[0] & 0x40;

}

static void read_timecode(unsigned char *bytes, int32_t numBytes, Timecode *vitc, Timecode *ltc)
{
    MXFPP_CHECK(numBytes == 28);

    convert_12m_to_timecode(&bytes[12], vitc);
    convert_12m_to_timecode(&bytes[12 + 8], ltc);
}




ArchiveMXFReader::ArchiveMXFReader(string filename)
: _mxfFile(0), _dataModel(0), _headerMetadata(0), _numAudioTracks(0), _position(-1),
_duration(-1), _actualPosition(-1), _startOfEssenceFilePos(0)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    int i;

    _mxfFile = File::openRead(filename);
    _dataModel = new DataModel();
    _headerMetadata = new HeaderMetadata(_dataModel);


    // read the header partition pack and check that it has the things we expect for Archive MXF

    if (!_mxfFile->readHeaderPartition())
    {
        throw MXFException("Failed to find header partition");
    }
    Partition &headerPartition = _mxfFile->getPartition(0);

    MXFPP_CHECK(mxf_is_op_1a(headerPartition.getOperationalPattern()));

    vector<mxfUL> ecLabels = headerPartition.getEssenceContainers();
    if (ecLabels.size() != 3)
    {
        throw MXFException("Expecting 3 essence container labels in header partition pack");
    }
    vector<mxfUL>::const_iterator iter;
    for (iter = ecLabels.begin(); iter != ecLabels.end(); iter++)
    {
        const mxfUL& label = *iter;
        if (!mxf_equals_ul(&label, &MXF_EC_L(MultipleWrappings)) &&
            !mxf_equals_ul(&label, &MXF_EC_L(SD_Unc_625_50i_422_135_FrameWrapped)) &&
            !mxf_equals_ul(&label, &MXF_EC_L(BWFFrameWrapped)))
        {
            throw MXFException("Unknown essence container label in header partition pack");
        }
    }


    // read the header metadata and extract info

    _mxfFile->readNextNonFillerKL(&key, &llen, &len);
    MXFPP_CHECK(mxf_is_header_metadata(&key));

    int64_t headerStartPos = _mxfFile->tell() - llen - mxfKey_extlen;
    _headerMetadata->read(_mxfFile, &headerPartition, &key, llen, len);

    Preface *preface = _headerMetadata->getPreface();
    ContentStorage *content = preface->getContentStorage();
    vector<GenericPackage*> packages = content->getPackages();

    // get the duration and number of audio tracks from the material package

    vector<GenericPackage*>::const_iterator iter1;
    bool foundDuration = false;
    mxfUL dataDef;
    for (iter1 = packages.begin(); iter1 != packages.end() && !foundDuration; iter1++)
    {
        GenericPackage *package = *iter1;

        if (dynamic_cast<MaterialPackage*>(package) != 0)
        {
            MaterialPackage *matPackage = dynamic_cast<MaterialPackage*>(package);
            vector<GenericTrack*> tracks = matPackage->getTracks();

            vector<GenericTrack*>::const_iterator iter2;
            for (iter2 = tracks.begin(); iter2 != tracks.end(); iter2++)
            {
                GenericTrack *genTrack = *iter2;

                if (dynamic_cast<Track*>(genTrack) != 0)
                {
                    Track *track = dynamic_cast<Track*>(genTrack);

                    if (!foundDuration && track->getSequence()->haveDuration())
                    {
                        _duration = track->getSequence()->getDuration();
                        foundDuration = true;
                    }

                    dataDef = track->getSequence()->getDataDefinition();
                    if (mxf_is_sound(&dataDef))
                    {
                        _numAudioTracks++;
                    }
                }
            }

            break;
        }
    }

    MXFPP_CHECK(foundDuration);
    MXFPP_CHECK(_numAudioTracks <= MAX_CP_AUDIO_TRACKS);

    // Note: we assume tha number of audio tracks in the material package equals
    // the number of audio elements in the essence data
    _cp._numAudioTracks = _numAudioTracks;


    // complete the audio item element keys

    for (i = 0; i < _numAudioTracks; i++)
    {
        g_AudioItemElementKey[i].octet13 = _numAudioTracks;
    }


    // position at start of essence

    _mxfFile->seek(headerStartPos + headerPartition.getHeaderByteCount() +
        headerPartition.getIndexByteCount(), SEEK_SET);
    _position = 0;
    _actualPosition = 0;
    _startOfEssenceFilePos = headerStartPos + headerPartition.getHeaderByteCount() +
        headerPartition.getIndexByteCount();



    // read the first content package if duration > 0

    if (_duration > 0)
    {
        DynamicByteArray workBytes(64);

        // skip initial filler
        while (true)
        {
            _mxfFile->readK(&key);

            if (!mxf_is_filler(&key))
            {
                break;
            }

            _mxfFile->readL(&llen, &len);
            _mxfFile->skip(len);
        }

        _cp._size = 0;


        // read content package elements

        // System Item Element
        MXFPP_CHECK(mxf_equals_key(&key, &g_SystemItemElementKey));
        _mxfFile->readL(&llen, &len);
        workBytes.setSize(0);
        workBytes.grow((uint32_t)len);
        MXFPP_CHECK(_mxfFile->read(workBytes.getBytes(), (uint32_t)len) == (uint32_t)len);
        workBytes.setSize((uint32_t)len);
        read_timecode(workBytes.getBytes(), workBytes.getSize(), &_cp._vitc, &_cp._ltc);
        _cp._size += mxfKey_extlen + llen + (uint32_t)len;

        // Video Item Element
        _cp._size += readElementData(&_cp._videoBytes, &g_VideoItemElementKey);

        // Audio Item Elements
        for (i = 0; i < _numAudioTracks; i++)
        {
            _cp._size += readElementData(&_cp._audioBytes[i], &g_AudioItemElementKey[i]);
        }

        _cp._position = _position;
        _actualPosition++;
    }

}


ArchiveMXFReader::~ArchiveMXFReader()
{
    delete _mxfFile;
    delete _dataModel;
    delete _headerMetadata;
}

int64_t ArchiveMXFReader::getDuration()
{
    return _duration;
}

int64_t ArchiveMXFReader::getPosition()
{
    return _position;
}

void ArchiveMXFReader::seekToPosition(int64_t position)
{
    if (_duration == 0 && position == 0)
    {
        return;
    }

    if (_duration == 0)
    {
        throw MXFException("Can't seek when duration is zero");
    }

    if (position > _duration)
    {
        throw MXFException("Can't seek beyond duration");
    }

    _mxfFile->seek(_startOfEssenceFilePos + position * _cp._size, SEEK_SET);
    _actualPosition = position;
    _position = position;
}

bool ArchiveMXFReader::seekToTimecode(Timecode vitc, Timecode ltc)
{
    if (_duration == 0)
    {
        return false;
    }

    if (_duration == 0)
    {
        throw MXFException("Can't seek when duration is zero");
    }


    int64_t originalPos = _position;
    DynamicByteArray workBytes(28);
    bool trySeekExtrapolate = true;
    int64_t nextPos = 0;
    int cleanTimecodeCount = 0;
    Timecode currentVITC;
    Timecode currentLTC;
    Timecode prevVITC;
    Timecode prevLTC;
    int64_t currentVITCFrameCount = 0;
    int64_t prevVITCFrameCount = 0;
    int64_t currentLTCFrameCount = 0;
    int64_t prevLTCFrameCount = 0;
    bool vitcIsClean = false;
    bool ltcIsClean = false;
    int64_t diff;
    int64_t vitcFrameCount = 0;
    if (vitc.isValid())
    {
        vitcFrameCount = vitc.hour * 60 * 60 * 25 +
            vitc.min * 60 * 25+
            vitc.sec * 60 +
            vitc.frame;
    }
    int64_t ltcFrameCount = 0;
    if (ltc.isValid())
    {
        ltcFrameCount = ltc.hour * 60 * 60 * 25 +
            ltc.min * 60 * 25+
            ltc.sec * 60 +
            ltc.frame;
    }

    try
    {
        while (_position < _duration)
        {
            // read and compare the timecodes

            readElementData(&workBytes, &g_SystemItemElementKey);
            read_timecode(workBytes.getBytes(), workBytes.getSize(), &currentVITC, &currentLTC);

            if ((!vitc.isValid() || currentVITC == vitc) &&
                (!ltc.isValid() || currentLTC == ltc))
            {
                // found it
                seekToPosition(_position);
                return true;
            }

            nextPos = _position + 1;

            // either go to the next frame or try extrapolating if the timecodes are clean
            if (trySeekExtrapolate)
            {
                // check if the vitc timecode is incrementing 1 frame
                if (vitc.isValid())
                {
                    currentVITCFrameCount = currentVITC.hour * 60 * 60 * 25 +
                        currentVITC.min * 60 * 25+
                        currentVITC.sec * 60 +
                        currentVITC.frame;

                    vitcIsClean = (currentVITCFrameCount <= vitcFrameCount &&
                        prevVITCFrameCount + 1 == currentVITCFrameCount);

                    prevVITCFrameCount = currentVITCFrameCount;
                }
                else
                {
                    vitcIsClean = true;
                }

                // check if the ltc timecode is incrementing 1 frame
                if (ltc.isValid())
                {
                    currentLTCFrameCount = currentLTC.hour * 60 * 60 * 25 +
                        currentLTC.min * 60 * 25+
                        currentLTC.sec * 60 +
                        currentLTC.frame;

                    ltcIsClean = (currentLTCFrameCount <= ltcFrameCount &&
                        prevLTCFrameCount + 1 == currentLTCFrameCount);

                    prevLTCFrameCount = currentLTCFrameCount;
                }
                else
                {
                    ltcIsClean = true;
                }

                if (ltcIsClean && vitcIsClean)
                {
                    cleanTimecodeCount++;
                }
                else
                {
                    // not clean - reset count
                    cleanTimecodeCount = 0;
                }

                if (cleanTimecodeCount > CLEAN_TIMECODE_THRESHOLD)
                {
                    // try extrapolating from the current position and seek in one go
                    if (vitc.isValid())
                    {
                        diff = vitcFrameCount - currentVITCFrameCount;
                    }
                    else
                    {
                        diff = ltcFrameCount - currentLTCFrameCount;
                    }

                    try
                    {
                        // seek, read and compare the timecodes

                        seekToPosition(_position + diff);

                        readElementData(&workBytes, &g_SystemItemElementKey);
                        read_timecode(workBytes.getBytes(), workBytes.getSize(), &currentVITC, &currentLTC);

                        if ((!vitc.isValid() || currentVITC == vitc) &&
                            (!ltc.isValid() || currentLTC == ltc))
                        {
                            // found it
                            seekToPosition(_position);
                            return true;
                        }

                        // didn't find it so we don't try this again
                        trySeekExtrapolate = false;
                    }
                    catch (...)
                    {
                        // something failed so we don't try this again
                        trySeekExtrapolate = false;
                    }
                }
            }

            // seek to the next frame
            seekToPosition(nextPos);

            prevVITC = currentVITC;
            prevLTC = currentLTC;
        }

        // failed to find the timecode so go back to the original position
        seekToPosition(originalPos);
    }
    catch (...)
    {
        // restore position
        seekToPosition(originalPos);
        throw;
    }

    return false;
}

bool ArchiveMXFReader::isEOF()
{
    return _position >= _duration;
}

const ArchiveMXFContentPackage *ArchiveMXFReader::read()
{
    int32_t cpSize;
    int i;

    if (_position >= _duration)
    {
        // end of file
        return NULL;
    }

    if (_cp._position == _position)
    {
        // content package has already been read
        _position++;

        // make sure the file position is now at the correct position
        if (_actualPosition != _position)
        {
            seekToPosition(_position);
        }

        return &_cp;
    }

    _cp._position = -1;
    cpSize = 0;

    // read content package elements
    DynamicByteArray workBytes;
    cpSize += readElementData(&workBytes, &g_SystemItemElementKey);
    read_timecode(workBytes.getBytes(), workBytes.getSize(), &_cp._vitc, &_cp._ltc);
    cpSize += readElementData(&_cp._videoBytes, &g_VideoItemElementKey);
    for (i = 0; i < _numAudioTracks; i++)
    {
        cpSize += readElementData(&_cp._audioBytes[i], &g_AudioItemElementKey[i]);
    }


    // check the size hasn't changed
    MXFPP_CHECK(cpSize == _cp._size);


    _cp._position = _position;
    _position++;

    return &_cp;
}

uint32_t ArchiveMXFReader::readElementData(DynamicByteArray *bytes, const mxfKey *elementKey)
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    _mxfFile->readKL(&key, &llen, &len);
    MXFPP_CHECK(mxf_equals_key(&key, elementKey));
    bytes->setSize(0);
    bytes->grow((uint32_t)len);
    MXFPP_CHECK(_mxfFile->read(bytes->getBytes(), (uint32_t)len) == (uint32_t)len);
    bytes->setSize((uint32_t)len);

    return mxfKey_extlen + llen + (uint32_t)len;
}

