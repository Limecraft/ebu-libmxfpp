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

#include <libMXF++/MXFException.h>

#include "CommonTypes.h"
#include "ArchiveMXFContentPackage.h"

using namespace std;
using namespace mxfpp;



ArchiveMXFContentPackage::ArchiveMXFContentPackage()
: _position(-1)
{}

ArchiveMXFContentPackage::~ArchiveMXFContentPackage()
{}

int ArchiveMXFContentPackage::getNumAudioTracks() const
{
    return _numAudioTracks;
}

int64_t ArchiveMXFContentPackage::getPosition() const
{
    return _position;
}

Timecode ArchiveMXFContentPackage::getVITC() const
{
    return _vitc;
}

Timecode ArchiveMXFContentPackage::getLTC() const
{
    return _ltc;
}

const DynamicByteArray *ArchiveMXFContentPackage::getVideo() const
{
    return &_videoBytes;
}

const DynamicByteArray *ArchiveMXFContentPackage::getAudio(int index) const
{
    if (index < 0 || index > MAX_CP_AUDIO_TRACKS)
    {
        throw MXFException("Audio index %d is out of range\n", index);
    }

    return &_audioBytes[index];
}


