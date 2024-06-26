/*
 * Copyright (c) 2001, 2017, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "gc/g1/g1BiasedArray.hpp"
#include "gc/g1/g1NUMA.hpp"
#include "gc/g1/g1RegionToSpaceMapper.hpp"
#include "memory/allocation.inline.hpp"
#include "memory/virtualspace.hpp"
#include "services/memTracker.hpp"
#include "utilities/align.hpp"
#include "utilities/bitMap.inline.hpp"

G1RegionToSpaceMapper::G1RegionToSpaceMapper(ReservedSpace rs,
                                             size_t used_size,
                                             size_t page_size,
                                             size_t region_granularity,
                                             size_t commit_factor,
                                             MemoryType type) :
  _storage(rs, used_size, page_size),
  _region_granularity(region_granularity),
  _listener(NULL),
  _commit_map(rs.size() * commit_factor / region_granularity, mtGC),
  _memory_type(type) {
  guarantee(is_power_of_2(page_size), "must be");
  guarantee(is_power_of_2(region_granularity), "must be");

  MemTracker::record_virtual_memory_type((address)rs.base(), type);
}

// G1RegionToSpaceMapper implementation where the region granularity is larger than
// or the same as the commit granularity.
// Basically, the space corresponding to one region region spans several OS pages.
class G1RegionsLargerThanCommitSizeMapper : public G1RegionToSpaceMapper {
 private:
  size_t _pages_per_region;

 public:
  G1RegionsLargerThanCommitSizeMapper(ReservedSpace rs,
                                      size_t actual_size,
                                      size_t page_size,
                                      size_t alloc_granularity,
                                      size_t commit_factor,
                                      MemoryType type) :
    G1RegionToSpaceMapper(rs, actual_size, page_size, alloc_granularity, commit_factor, type),
    _pages_per_region(alloc_granularity / (page_size * commit_factor)) {

    guarantee(alloc_granularity >= page_size, "allocation granularity smaller than commit granularity");
  }

  virtual void commit_regions(uint start_idx, size_t num_regions, WorkGang* pretouch_gang) {
    const size_t start_page = (size_t)start_idx * _pages_per_region;
    const size_t size_in_pages = num_regions * _pages_per_region;
    bool zero_filled = _storage.commit(start_page, size_in_pages);
    if (_memory_type == mtJavaHeap) {
      for (uint region_index = start_idx; region_index < start_idx + num_regions; region_index++ ) {
        void* address = _storage.page_start(region_index * _pages_per_region);
        size_t size_in_bytes = _storage.page_size() * _pages_per_region;
        G1NUMA::numa()->request_memory_on_node(address, size_in_bytes, region_index);
      }
    }
    if (AlwaysPreTouch) {
      _storage.pretouch(start_page, size_in_pages, pretouch_gang);
    }
    _commit_map.set_range(start_idx, start_idx + num_regions);
    fire_on_commit(start_idx, num_regions, zero_filled);
  }

  virtual void uncommit_regions(uint start_idx, size_t num_regions) {
    _storage.uncommit((size_t)start_idx * _pages_per_region, num_regions * _pages_per_region);
    _commit_map.clear_range(start_idx, start_idx + num_regions);
  }
};

// G1RegionToSpaceMapper implementation where the region granularity is smaller
// than the commit granularity.
// Basically, the contents of one OS page span several regions.
class G1RegionsSmallerThanCommitSizeMapper : public G1RegionToSpaceMapper {
 private:
  class CommitRefcountArray : public G1BiasedMappedArray<uint> {
   protected:
     virtual uint default_value() const { return 0; }
  };

  size_t _regions_per_page;

  CommitRefcountArray _refcounts;

  uintptr_t region_idx_to_page_idx(uint region) const {
    return region / _regions_per_page;
  }

 public:
  G1RegionsSmallerThanCommitSizeMapper(ReservedSpace rs,
                                       size_t actual_size,
                                       size_t page_size,
                                       size_t alloc_granularity,
                                       size_t commit_factor,
                                       MemoryType type) :
    G1RegionToSpaceMapper(rs, actual_size, page_size, alloc_granularity, commit_factor, type),
    _regions_per_page((page_size * commit_factor) / alloc_granularity), _refcounts() {

    guarantee((page_size * commit_factor) >= alloc_granularity, "allocation granularity smaller than commit granularity");
    _refcounts.initialize((HeapWord*)rs.base(), (HeapWord*)(rs.base() + align_up(rs.size(), page_size)), page_size);
  }

  virtual void commit_regions(uint start_idx, size_t num_regions, WorkGang* pretouch_gang) {
    size_t const NoPage = ~(size_t)0;

    size_t first_committed = NoPage;
    size_t num_committed = 0;

    bool all_zero_filled = true;
    G1NUMA* numa = G1NUMA::numa();

    for (uint region_idx = start_idx; region_idx < start_idx + num_regions; region_idx++) {
      assert(!_commit_map.at(region_idx), "Trying to commit storage at region %u that is already committed", region_idx);
      size_t page_idx = region_idx_to_page_idx(region_idx);
      uint old_refcount = _refcounts.get_by_index(page_idx);

      bool zero_filled = false;
      if (old_refcount == 0) {
        if (first_committed == NoPage) {
          first_committed = page_idx;
          num_committed = 1;
        } else {
          num_committed++;
        }
        zero_filled = _storage.commit(page_idx, 1);
        if (_memory_type == mtJavaHeap) {
          void* address = _storage.page_start(page_idx);
          size_t size_in_bytes = _storage.page_size();
          numa->request_memory_on_node(address, size_in_bytes, region_idx);
        }
      }
      all_zero_filled &= zero_filled;

      _refcounts.set_by_index(page_idx, old_refcount + 1);
      _commit_map.set_bit(region_idx);
    }
    if (AlwaysPreTouch && num_committed > 0) {
      _storage.pretouch(first_committed, num_committed, pretouch_gang);
    }
    fire_on_commit(start_idx, num_regions, all_zero_filled);
  }

  virtual void uncommit_regions(uint start_idx, size_t num_regions) {
    for (uint i = start_idx; i < start_idx + num_regions; i++) {
      assert(_commit_map.at(i), "Trying to uncommit storage at region %u that is not committed", i);
      size_t idx = region_idx_to_page_idx(i);
      uint old_refcount = _refcounts.get_by_index(idx);
      assert(old_refcount > 0, "must be");
      if (old_refcount == 1) {
        _storage.uncommit(idx, 1);
      }
      _refcounts.set_by_index(idx, old_refcount - 1);
      _commit_map.clear_bit(i);
    }
  }
};

void G1RegionToSpaceMapper::fire_on_commit(uint start_idx, size_t num_regions, bool zero_filled) {
  if (_listener != NULL) {
    _listener->on_commit(start_idx, num_regions, zero_filled);
  }
}

G1RegionToSpaceMapper* G1RegionToSpaceMapper::create_mapper(ReservedSpace rs,
                                                            size_t actual_size,
                                                            size_t page_size,
                                                            size_t region_granularity,
                                                            size_t commit_factor,
                                                            MemoryType type) {

  if (region_granularity >= (page_size * commit_factor)) {
    return new G1RegionsLargerThanCommitSizeMapper(rs, actual_size, page_size, region_granularity, commit_factor, type);
  } else {
    return new G1RegionsSmallerThanCommitSizeMapper(rs, actual_size, page_size, region_granularity, commit_factor, type);
  }
}
