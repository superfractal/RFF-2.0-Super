//
// ApproxTableCache.h 
// Created by Merutilm on 2025-05-23.
// Created by Super Fractal on 2025-11-24.
// Modified to use hybrid SparseVector (sparse segments + contiguous memory)
//

#pragma once

#include <vector>
#include <memory>
#include <algorithm>

#include "../mrthy/DeepPA.h"
#include "../mrthy/LightPA.h"
#include "../mrthy/SparseVector.h"

namespace merutilm::rff2 {

    struct ApproxTableCache {
        SparseVector<std::vector<LightPA>> lightTable;
        SparseVector<std::vector<DeepPA>> deepTable;

        ApproxTableCache() = default;
        ~ApproxTableCache() = default;

        ApproxTableCache(const ApproxTableCache&) = delete;
        ApproxTableCache& operator=(const ApproxTableCache&) = delete;

        ApproxTableCache(ApproxTableCache&&) noexcept = default;
        ApproxTableCache& operator=(ApproxTableCache&&) noexcept = default;

        void clear() {
            lightTable.clear();
            deepTable.clear();
        }

        size_t lightTableSegmentCount() const {
            return lightTable.segment_count();
        }

        size_t deepTableSegmentCount() const {
            return deepTable.segment_count();
        }

        size_t approximateMemoryUsage() const {
            return lightTable.allocated_memory() + deepTable.allocated_memory();
        }
    };

} // namespace merutilm::rff2
