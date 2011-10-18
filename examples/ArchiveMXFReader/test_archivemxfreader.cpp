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

#include "ArchiveMXFReader.h"

using namespace std;
using namespace mxfpp;



void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s <filename>\n", cmd);
}

int main(int argc, const char **argv)
{
    const char *filename = 0;

    if (argc != 2)
    {
        usage(argv[0]);
        exit(1);
    }

    filename = argv[1];

    try
    {
        ArchiveMXFReader reader(filename);

        int count = 0;
        int i;
        while (!reader.isEOF() && count++ < 50)
        {
            const ArchiveMXFContentPackage *cp = reader.read();
            printf("video size = %d\n", cp->getVideo()->getSize());
            for (i = 0; i < cp->getNumAudioTracks(); i++)
            {
                printf("audio %d size = %d\n", i, cp->getAudio(i)->getSize());
            }
            printf("vitc = %02d:%02d:%02d:%02d\n", cp->getVITC().hour,
                cp->getVITC().min, cp->getVITC().sec, cp->getVITC().frame);
        }

        reader.seekToPosition(1);
        count = 0;
        while (!reader.isEOF() && count++ < 10)
        {
            const ArchiveMXFContentPackage *cp = reader.read();
            printf("vitc = %02d:%02d:%02d:%02d\n", cp->getVITC().hour,
                cp->getVITC().min, cp->getVITC().sec, cp->getVITC().frame);
        }

        reader.seekToPosition(0);

        Timecode vitc(10, 2, 5, 20);
        Timecode ltc(1);
        if (reader.seekToTimecode(vitc, ltc))
        {
            count = 0;
            while (!reader.isEOF() && count++ < 10)
            {
                const ArchiveMXFContentPackage *cp = reader.read();
                printf("vitc = %02d:%02d:%02d:%02d\n", cp->getVITC().hour,
                    cp->getVITC().min, cp->getVITC().sec, cp->getVITC().frame);
            }
        }
        else
        {
            printf("Failed to seek to timecode\n");
        }

    }
    catch (MXFException& ex)
    {
        fprintf(stderr, "Exception thrown: %s\n", ex.getMessage().c_str());
        exit(1);
    }

    return 0;
}

