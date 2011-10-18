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

#include <libMXF++/MXF.h>


using namespace std;
using namespace mxfpp;


const mxfKey GenericPictureEssenceDescriptorBase::setKey = MXF_SET_K(GenericPictureEssenceDescriptor);


GenericPictureEssenceDescriptorBase::GenericPictureEssenceDescriptorBase(HeaderMetadata *headerMetadata)
: FileDescriptor(headerMetadata, headerMetadata->createCSet(&setKey))
{
    headerMetadata->add(this);
}

GenericPictureEssenceDescriptorBase::GenericPictureEssenceDescriptorBase(HeaderMetadata *headerMetadata, ::MXFMetadataSet *cMetadataSet)
: FileDescriptor(headerMetadata, cMetadataSet)
{}

GenericPictureEssenceDescriptorBase::~GenericPictureEssenceDescriptorBase()
{}


bool GenericPictureEssenceDescriptorBase::haveSignalStandard() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SignalStandard));
}

uint8_t GenericPictureEssenceDescriptorBase::getSignalStandard() const
{
    return getUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SignalStandard));
}

bool GenericPictureEssenceDescriptorBase::haveFrameLayout() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout));
}

uint8_t GenericPictureEssenceDescriptorBase::getFrameLayout() const
{
    return getUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout));
}

bool GenericPictureEssenceDescriptorBase::haveStoredWidth() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth));
}

uint32_t GenericPictureEssenceDescriptorBase::getStoredWidth() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth));
}

bool GenericPictureEssenceDescriptorBase::haveStoredHeight() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight));
}

uint32_t GenericPictureEssenceDescriptorBase::getStoredHeight() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight));
}

bool GenericPictureEssenceDescriptorBase::haveStoredF2Offset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredF2Offset));
}

int32_t GenericPictureEssenceDescriptorBase::getStoredF2Offset() const
{
    return getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredF2Offset));
}

bool GenericPictureEssenceDescriptorBase::haveSampledWidth() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledWidth));
}

uint32_t GenericPictureEssenceDescriptorBase::getSampledWidth() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledWidth));
}

bool GenericPictureEssenceDescriptorBase::haveSampledHeight() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledHeight));
}

uint32_t GenericPictureEssenceDescriptorBase::getSampledHeight() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledHeight));
}

bool GenericPictureEssenceDescriptorBase::haveSampledXOffset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledXOffset));
}

int32_t GenericPictureEssenceDescriptorBase::getSampledXOffset() const
{
    return getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledXOffset));
}

bool GenericPictureEssenceDescriptorBase::haveSampledYOffset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledYOffset));
}

int32_t GenericPictureEssenceDescriptorBase::getSampledYOffset() const
{
    return getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledYOffset));
}

bool GenericPictureEssenceDescriptorBase::haveDisplayHeight() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight));
}

uint32_t GenericPictureEssenceDescriptorBase::getDisplayHeight() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight));
}

bool GenericPictureEssenceDescriptorBase::haveDisplayWidth() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth));
}

uint32_t GenericPictureEssenceDescriptorBase::getDisplayWidth() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth));
}

bool GenericPictureEssenceDescriptorBase::haveDisplayXOffset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset));
}

int32_t GenericPictureEssenceDescriptorBase::getDisplayXOffset() const
{
    return getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset));
}

bool GenericPictureEssenceDescriptorBase::haveDisplayYOffset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset));
}

int32_t GenericPictureEssenceDescriptorBase::getDisplayYOffset() const
{
    return getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset));
}

bool GenericPictureEssenceDescriptorBase::haveDisplayF2Offset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayF2Offset));
}

int32_t GenericPictureEssenceDescriptorBase::getDisplayF2Offset() const
{
    return getInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayF2Offset));
}

bool GenericPictureEssenceDescriptorBase::haveAspectRatio() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio));
}

mxfRational GenericPictureEssenceDescriptorBase::getAspectRatio() const
{
    return getRationalItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio));
}

bool GenericPictureEssenceDescriptorBase::haveActiveFormatDescriptor() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ActiveFormatDescriptor));
}

uint8_t GenericPictureEssenceDescriptorBase::getActiveFormatDescriptor() const
{
    return getUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ActiveFormatDescriptor));
}

bool GenericPictureEssenceDescriptorBase::haveVideoLineMap() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap));
}

std::vector<int32_t> GenericPictureEssenceDescriptorBase::getVideoLineMap() const
{
    return getInt32ArrayItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap));
}

bool GenericPictureEssenceDescriptorBase::haveAlphaTransparency() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, AlphaTransparency));
}

uint8_t GenericPictureEssenceDescriptorBase::getAlphaTransparency() const
{
    return getUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, AlphaTransparency));
}

bool GenericPictureEssenceDescriptorBase::haveCaptureGamma() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, CaptureGamma));
}

mxfUL GenericPictureEssenceDescriptorBase::getCaptureGamma() const
{
    return getULItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, CaptureGamma));
}

bool GenericPictureEssenceDescriptorBase::haveImageAlignmentOffset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageAlignmentOffset));
}

uint32_t GenericPictureEssenceDescriptorBase::getImageAlignmentOffset() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageAlignmentOffset));
}

bool GenericPictureEssenceDescriptorBase::haveImageStartOffset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageStartOffset));
}

uint32_t GenericPictureEssenceDescriptorBase::getImageStartOffset() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageStartOffset));
}

bool GenericPictureEssenceDescriptorBase::haveImageEndOffset() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageEndOffset));
}

uint32_t GenericPictureEssenceDescriptorBase::getImageEndOffset() const
{
    return getUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageEndOffset));
}

bool GenericPictureEssenceDescriptorBase::haveFieldDominance() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FieldDominance));
}

uint8_t GenericPictureEssenceDescriptorBase::getFieldDominance() const
{
    return getUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FieldDominance));
}

bool GenericPictureEssenceDescriptorBase::havePictureEssenceCoding() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding));
}

mxfUL GenericPictureEssenceDescriptorBase::getPictureEssenceCoding() const
{
    return getULItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding));
}

bool GenericPictureEssenceDescriptorBase::haveCodingEquations() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, CodingEquations));
}

mxfUL GenericPictureEssenceDescriptorBase::getCodingEquations() const
{
    return getULItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, CodingEquations));
}

bool GenericPictureEssenceDescriptorBase::haveColorPrimaries() const
{
    return haveItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ColorPrimaries));
}

mxfUL GenericPictureEssenceDescriptorBase::getColorPrimaries() const
{
    return getULItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ColorPrimaries));
}

void GenericPictureEssenceDescriptorBase::setSignalStandard(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SignalStandard), value);
}

void GenericPictureEssenceDescriptorBase::setFrameLayout(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FrameLayout), value);
}

void GenericPictureEssenceDescriptorBase::setStoredWidth(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredWidth), value);
}

void GenericPictureEssenceDescriptorBase::setStoredHeight(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredHeight), value);
}

void GenericPictureEssenceDescriptorBase::setStoredF2Offset(int32_t value)
{
    setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, StoredF2Offset), value);
}

void GenericPictureEssenceDescriptorBase::setSampledWidth(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledWidth), value);
}

void GenericPictureEssenceDescriptorBase::setSampledHeight(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledHeight), value);
}

void GenericPictureEssenceDescriptorBase::setSampledXOffset(int32_t value)
{
    setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledXOffset), value);
}

void GenericPictureEssenceDescriptorBase::setSampledYOffset(int32_t value)
{
    setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, SampledYOffset), value);
}

void GenericPictureEssenceDescriptorBase::setDisplayHeight(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayHeight), value);
}

void GenericPictureEssenceDescriptorBase::setDisplayWidth(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayWidth), value);
}

void GenericPictureEssenceDescriptorBase::setDisplayXOffset(int32_t value)
{
    setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayXOffset), value);
}

void GenericPictureEssenceDescriptorBase::setDisplayYOffset(int32_t value)
{
    setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayYOffset), value);
}

void GenericPictureEssenceDescriptorBase::setDisplayF2Offset(int32_t value)
{
    setInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, DisplayF2Offset), value);
}

void GenericPictureEssenceDescriptorBase::setAspectRatio(mxfRational value)
{
    setRationalItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, AspectRatio), value);
}

void GenericPictureEssenceDescriptorBase::setActiveFormatDescriptor(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ActiveFormatDescriptor), value);
}

void GenericPictureEssenceDescriptorBase::setVideoLineMap(const std::vector<int32_t> &value)
{
    setInt32ArrayItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap), value);
}

void GenericPictureEssenceDescriptorBase::appendVideoLineMap(int32_t value)
{
    appendInt32ArrayItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, VideoLineMap), value);
}

void GenericPictureEssenceDescriptorBase::setAlphaTransparency(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, AlphaTransparency), value);
}

void GenericPictureEssenceDescriptorBase::setCaptureGamma(mxfUL value)
{
    setULItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, CaptureGamma), value);
}

void GenericPictureEssenceDescriptorBase::setImageAlignmentOffset(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageAlignmentOffset), value);
}

void GenericPictureEssenceDescriptorBase::setImageStartOffset(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageStartOffset), value);
}

void GenericPictureEssenceDescriptorBase::setImageEndOffset(uint32_t value)
{
    setUInt32Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ImageEndOffset), value);
}

void GenericPictureEssenceDescriptorBase::setFieldDominance(uint8_t value)
{
    setUInt8Item(&MXF_ITEM_K(GenericPictureEssenceDescriptor, FieldDominance), value);
}

void GenericPictureEssenceDescriptorBase::setPictureEssenceCoding(mxfUL value)
{
    setULItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, PictureEssenceCoding), value);
}

void GenericPictureEssenceDescriptorBase::setCodingEquations(mxfUL value)
{
    setULItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, CodingEquations), value);
}

void GenericPictureEssenceDescriptorBase::setColorPrimaries(mxfUL value)
{
    setULItem(&MXF_ITEM_K(GenericPictureEssenceDescriptor, ColorPrimaries), value);
}

