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

#define __STDC_FORMAT_MACROS 1

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AvidClipWriter.h"

using namespace std;
using namespace mxfpp;



void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s <filename>\n", cmd);
}

int main(int argc, const char **argv)
{
    string prefix;

    if (argc != 2)
    {
        usage(argv[0]);
        exit(1);
    }

    prefix = argv[1];

    mxfRational aspectRatio = {4, 3};

    AvidClipWriter writer(AVID_PAL_25I_PROJECT, aspectRatio, false, true);

    writer.setProjectName("test project");
    writer.setClipName("test clip");
    writer.setTape("test tape", 10 * 60 * 60 * 25);
    writer.addUserComment("Descript", "a test project");

    EssenceParams params;

    string videoFilename = prefix;
    videoFilename.append("_v1.mxf");
    writer.registerEssenceElement(1, 1, AVID_MJPEG201_ESSENCE, &params, videoFilename);

    params.quantizationBits = 16;
    string audio1Filename = prefix;
    audio1Filename.append("_a1.mxf");
    writer.registerEssenceElement(2, 3, AVID_PCM_ESSENCE, &params, audio1Filename);

    string audio2Filename = prefix;
    audio2Filename.append("_a2.mxf");
    writer.registerEssenceElement(3, 4, AVID_PCM_ESSENCE, &params, audio2Filename);

    writer.prepareToWrite();

    int i;
    unsigned char buffer[288000];
    memset(buffer, 0, sizeof(*buffer));
    for (i = 0; i < 50; i++)
    {
        writer.writeSamples(1, 1, buffer, 288000);
        writer.writeSamples(2, 1920, buffer, 1920 * 2);
        writer.writeSamples(3, 1920, buffer, 1920 * 2);
    }

    writer.completeWrite();

    return 0;
}


