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

#include <algorithm>

#include <libMXF++/MXF.h>

using namespace std;
using namespace mxfpp;



// Utility class to hold a ::MXFPartitions list
class PartitionList
{
public:
    PartitionList()
    {
        mxf_initialise_list(&_cPartitions, NULL);
    }

    PartitionList(vector<Partition*> &partitions)
    {
        mxf_initialise_list(&_cPartitions, NULL);

        vector<Partition*>::const_iterator iter;
        for (iter = partitions.begin(); iter != partitions.end(); iter++)
        {
            MXFPP_CHECK(mxf_append_partition(&_cPartitions, (*iter)->getCPartition()));
        }
    }

    ~PartitionList()
    {
        mxf_clear_list(&_cPartitions);
    }

    ::MXFFilePartitions* getList()
    {
        return &_cPartitions;
    }

private:
    ::MXFFilePartitions _cPartitions;
};



File* File::openNew(string filename)
{
    ::MXFFile *cFile;
    MXFPP_CHECK(mxf_disk_file_open_new(filename.c_str(), &cFile));

    return new File(cFile);
}

File* File::openRead(string filename)
{
    ::MXFFile *cFile;
    MXFPP_CHECK(mxf_disk_file_open_read(filename.c_str(), &cFile));

    return new File(cFile);
}

File* File::openModify(string filename)
{
    ::MXFFile *cFile;
    MXFPP_CHECK(mxf_disk_file_open_modify(filename.c_str(), &cFile));

    return new File(cFile);
}

File::File(::MXFFile *cFile)
: _cFile(cFile)
{}

File::~File()
{
    mxf_file_close(&_cFile);

    size_t i;
    for (i = 0; i < _partitions.size(); i++)
        delete _partitions[i];
}

void File::setMinLLen(uint8_t llen)
{
    mxf_file_set_min_llen(_cFile, llen);
}

uint8_t File::getMinLLen()
{
    return mxf_get_min_llen(_cFile);
}

Partition& File::createPartition()
{
    Partition *previousPartition = 0;
    Partition *partition = 0;

    if (_partitions.size() > 0)
    {
        previousPartition = _partitions.back();
    }

    _partitions.push_back(new Partition());
    partition = _partitions.back();
    if (previousPartition)
    {
        MXFPP_CHECK(mxf_initialise_with_partition(previousPartition->getCPartition(), partition->getCPartition()));
    }

    return (*_partitions.back());
}

void File::writeRIP()
{
    PartitionList partitionList(_partitions);

    MXFPP_CHECK(mxf_write_rip(_cFile, partitionList.getList()));
}

void File::updatePartitions()
{
    PartitionList partitionList(_partitions);

    MXFPP_CHECK(mxf_update_partitions(_cFile, partitionList.getList()));
}

Partition& File::getPartition(size_t index)
{
    MXFPP_ASSERT(index < _partitions.size());
    return *_partitions[index];
}

bool File::readHeaderPartition()
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;

    size_t i;
    for (i = 0; i < _partitions.size(); i++)
        delete _partitions[i];
    _partitions.clear();

    try
    {
        seek(mxf_get_runin_len(_cFile), SEEK_SET);
        if (!mxf_read_header_pp_kl(_cFile, &key, &llen, &len))
            return false;

        _partitions.push_back(Partition::read(this, &key));
        // file is positioned after the partition pack

        return true;
    }
    catch (...)
    {
        for (i = 0; i < _partitions.size(); i++)
            delete _partitions[i];
        _partitions.clear();
        return false;
    }
}

bool File::readPartitions()
{
    mxfKey key;
    uint8_t llen;
    uint64_t len;
    uint64_t this_partition;
    MXFRIP rip;
    MXFRIPEntry *rip_entry;
    MXFListIterator iter;
    Partition *header_partition = 0;

    // read the header partition if not already done so and clear the partition list

    if ((!_partitions.empty() && mxf_is_header_partition_pack(_partitions[0]->getKey())) ||
        readHeaderPartition())
    {
        header_partition = _partitions[0];
    }

    size_t i;
    for (i = 0; i < _partitions.size(); i++) {
        if (_partitions[i] != header_partition)
            delete _partitions[i];
    }
    _partitions.clear();

    if (!header_partition)
        return false;


    try
    {
        // use the RIP if there is one
        if (mxf_read_rip(_cFile, &rip)) {
            try
            {
                _partitions.push_back(header_partition);

                mxf_initialise_list_iter(&iter, &rip.entries);
                while (mxf_next_list_iter_element(&iter)) {
                    rip_entry = (MXFRIPEntry*)mxf_get_iter_element(&iter);
                    if (rip_entry->thisPartition <= header_partition->getThisPartition())
                        continue;

                    seek(mxf_get_runin_len(_cFile) + rip_entry->thisPartition, SEEK_SET);
                    readKL(&key, &llen, &len);
                    _partitions.push_back(Partition::read(this, &key));
                }

                mxf_clear_rip(&rip);
            }
            catch (...)
            {
                mxf_clear_rip(&rip);
                throw;
            }
        } else {

            // start from footer partition and work back to the header partition

            this_partition = header_partition->getFooterPartition();
            if (this_partition <= header_partition->getThisPartition())
                throw false;

            do {
                seek(mxf_get_runin_len(_cFile) + this_partition, SEEK_SET);
                readKL(&key, &llen, &len);
                _partitions.push_back(Partition::read(this, &key));

                this_partition = _partitions.back()->getPreviousPartition();
            }
            while (this_partition < _partitions.back()->getThisPartition() &&
                   this_partition > header_partition->getThisPartition());

            _partitions.push_back(header_partition);

            // reorder to start with the header partition
            reverse(_partitions.begin(), _partitions.end());
        }

        return true;
    }
    catch (...)
    {
        for (i = 0; i < _partitions.size(); i++) {
            if (_partitions[i] != header_partition)
                delete _partitions[i];
        }
        _partitions.clear();

        // restore the header partition to the list
        _partitions.push_back(header_partition);

        return false;
    }
}

void File::readK(mxfKey *key)
{
    MXFPP_CHECK(mxf_read_k(_cFile, key));
}

void File::readL(uint8_t *llen, uint64_t *len)
{
    MXFPP_CHECK(mxf_read_l(_cFile, llen, len));
}

void File::readKL(mxfKey *key, uint8_t *llen, uint64_t *len)
{
    MXFPP_CHECK(mxf_read_kl(_cFile, key, llen, len));
}

void File::readNextNonFillerKL(mxfKey *key, uint8_t *llen, uint64_t *len)
{
    MXFPP_CHECK(mxf_read_next_nonfiller_kl(_cFile, key, llen, len));
}

uint32_t File::read(unsigned char *data, uint32_t count)
{
    return mxf_file_read(_cFile, data, count);
}

int64_t File::tell()
{
    return mxf_file_tell(_cFile);
}

void File::seek(int64_t position, int whence)
{
    MXFPP_CHECK(mxf_file_seek(_cFile, position, whence));
}

void File::skip(uint64_t len)
{
    MXFPP_CHECK(mxf_skip(_cFile, len));
}

int64_t File::size()
{
    return mxf_file_size(_cFile);
}

bool File::eof()
{
    return mxf_file_eof(_cFile) == 1;
}

uint32_t File::write(const unsigned char *data, uint32_t count)
{
    return mxf_file_write(_cFile, data, count);
}

void File::writeUInt8(uint8_t value)
{
    MXFPP_CHECK(mxf_write_uint8(_cFile, value));
}

void File::writeUInt16(uint16_t value)
{
    MXFPP_CHECK(mxf_write_uint16(_cFile, value));
}

void File::writeUInt32(uint32_t value)
{
    MXFPP_CHECK(mxf_write_uint32(_cFile, value));
}

void File::writeUInt64(uint64_t value)
{
    MXFPP_CHECK(mxf_write_uint64(_cFile, value));
}

void File::writeInt8(int8_t value)
{
    MXFPP_CHECK(mxf_write_int8(_cFile, value));
}

void File::writeInt16(int16_t value)
{
    MXFPP_CHECK(mxf_write_int16(_cFile, value));
}

void File::writeInt32(int32_t value)
{
    MXFPP_CHECK(mxf_write_int32(_cFile, value));
}

void File::writeInt64(int64_t value)
{
    MXFPP_CHECK(mxf_write_int64(_cFile, value));
}

void File::writeUL(const mxfUL *ul)
{
    MXFPP_CHECK(mxf_write_ul(_cFile, ul));
}

void File::writeKL(const mxfKey *key, uint64_t len)
{
    MXFPP_CHECK(mxf_write_kl(_cFile, key, len));
}

void File::writeFixedL(uint8_t llen, uint64_t len)
{
    MXFPP_CHECK(mxf_write_fixed_l(_cFile, llen, len));
}

void File::writeFixedKL(const mxfKey *key, uint8_t llen, uint64_t len)
{
    MXFPP_CHECK(mxf_write_fixed_kl(_cFile, key, llen, len));
}

void File::writeBatchHeader(uint32_t len, uint32_t eleLen)
{
    MXFPP_CHECK(mxf_write_batch_header(_cFile, len, eleLen));
}

void File::writeArrayHeader(uint32_t len, uint32_t eleLen)
{
    MXFPP_CHECK(mxf_write_array_header(_cFile, len, eleLen));
}

void File::writeZeros(uint64_t len)
{
    MXFPP_CHECK(mxf_write_zeros(_cFile, len));
}

void File::fillToPosition(uint64_t position)
{
    MXFPP_CHECK(mxf_fill_to_position(_cFile, position));
}

void File::writeFill(uint32_t size)
{
    MXFPP_CHECK(mxf_write_fill(_cFile, size));
}

