/*
 * Copyright (C) 2009, British Broadcasting Corporation
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
#include <cstdlib>

#include <libMXF++/MXF.h>

#include <mxf/mxf_avid.h>

#include <libMXF++/extensions/TaggedValue.h>

using namespace std;
using namespace mxfpp;


// prefix is 0x42 ('B') for big endian or 0x4c ('L') for little endian, followed by half-swapped key for String type
static const unsigned char g_prefix_be[17] =
    {0x42, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01};
static const unsigned char g_prefix_le[17] =
    {0x4C, 0x00, 0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x04, 0x01, 0x01};



const mxfKey TaggedValue::set_key = MXF_SET_K(TaggedValue);

void TaggedValue::registerObjectFactory(HeaderMetadata *header_metadata)
{
    header_metadata->registerObjectFactory(&set_key, new MetadataSetFactory<TaggedValue>());
}



TaggedValue::TaggedValue(HeaderMetadata *header_metadata)
: InterchangeObject(header_metadata, header_metadata->createCSet(&set_key))
{
    header_metadata->add(this);
}

TaggedValue::TaggedValue(HeaderMetadata *header_metadata, ::MXFMetadataSet *c_metadata_set)
: InterchangeObject(header_metadata, c_metadata_set)
{}

TaggedValue::~TaggedValue()
{}

string TaggedValue::getName()
{
    return getStringItem(&MXF_ITEM_K(TaggedValue, Name));
}

bool TaggedValue::isStringValue()
{
    ByteArray bytes;
    bool is_big_endian;

    bytes = getRawBytesItem(&MXF_ITEM_K(TaggedValue, Value));

    if (bytes.length <= sizeof(g_prefix_be) || (bytes.data[0] != 0x42 && bytes.data[0] != 0x4C))
    {
        return false;
    }

    is_big_endian = (bytes.data[0] == 0x42);

    if ((is_big_endian && memcmp(&bytes.data[1], &g_prefix_be[1], sizeof(g_prefix_be) - 1) != 0) ||
        (!is_big_endian && memcmp(&bytes.data[1], &g_prefix_le[1], sizeof(g_prefix_le) - 1) != 0))
    {
        return false;
    }

    return true;
}

string TaggedValue::getStringValue()
{
    ByteArray bytes;
    bool is_big_endian;
    mxfUTF16Char *utf16_value = 0;
    char *utf8_value = 0;
    size_t utf8_size;
    uint16_t i;
    uint8_t *value_ptr;
    uint16_t str_size;
    string result;

    MXFPP_CHECK(isStringValue());

    try {
        bytes = getRawBytesItem(&MXF_ITEM_K(TaggedValue, Value));
        is_big_endian = (bytes.data[0] == 0x42);

        str_size = (bytes.length - sizeof(g_prefix_be)) / 2;
        if (str_size == 0 ||
            (bytes.data[sizeof(g_prefix_be)] == 0 && bytes.data[sizeof(g_prefix_be) + 1] == 0))
        {
            return "";
        }

        utf16_value = new mxfUTF16Char[str_size + 1];

        value_ptr = &bytes.data[sizeof(g_prefix_be)];
        for (i = 0; i < str_size; i++) {
            if (is_big_endian)
                utf16_value[i] = ((*value_ptr) << 8) | (*(value_ptr + 1));
            else
                utf16_value[i] = (*value_ptr) | ((*(value_ptr + 1)) << 8);

            if (utf16_value[i] == 0)
                break;

            value_ptr += 2;
        }
        utf16_value[str_size] = 0;

        utf8_size = mxf_utf16_to_utf8(0, utf16_value, 0);
        MXFPP_CHECK(utf8_size != (size_t)(-1));
        utf8_size += 1;
        utf8_value = new char[utf8_size];
        mxf_utf16_to_utf8(utf8_value, utf16_value, utf8_size);

        result = utf8_value;

        delete [] utf16_value;
        utf16_value = 0;
        delete [] utf8_value;
        utf8_value = 0;

        return result;

    } catch (...) {
        delete [] utf16_value;
        delete [] utf8_value;
        throw;
    }
}

void TaggedValue::setName(string value)
{
    setStringItem(&MXF_ITEM_K(TaggedValue, Name), value);
}

void TaggedValue::setStringValue(string value)
{
    ByteArray indirectVal = {0, 0};
    mxfUTF16Char *utf16Val = 0;
    size_t utf16ValSize;
    try
    {
        utf16ValSize = mxf_utf8_to_utf16(NULL, value.c_str(), 0);
        MXFPP_CHECK(utf16ValSize != (size_t)(-1));
        utf16ValSize += 1;
        utf16Val = new wchar_t[utf16ValSize];
        mxf_utf8_to_utf16(utf16Val, value.c_str(), utf16ValSize);

        indirectVal.length = (uint16_t)(sizeof(g_prefix_be) + utf16ValSize * mxfUTF16Char_extlen);
        indirectVal.data = new uint8_t[indirectVal.length];
        memcpy(indirectVal.data, g_prefix_be, sizeof(g_prefix_be));
        mxf_set_utf16string(utf16Val, &indirectVal.data[sizeof(g_prefix_be)]);

        setRawBytesItem(&MXF_ITEM_K(TaggedValue, Value), indirectVal);

        delete [] utf16Val;
        delete [] indirectVal.data;
    }
    catch (...)
    {
        delete [] utf16Val;
        delete [] indirectVal.data;
        throw;
    }
}

