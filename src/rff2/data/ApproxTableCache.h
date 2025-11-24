//
// Created by Merutilm on 2025-05-23.
// Created by Super Fractal on 2025-11-24.
//

#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "../mrthy/DeepPA.h"
#include "../mrthy/LightPA.h"
#include "../mrthy/SegmentedVector.h" 

namespace merutilm::rff2 {

    /**
     * SegmentedVector
     * 巨大な連続領域を回避し、ページ単位(セグメント)でメモリを管理するベクタ。
     * bad_alloc対策および、拡張時のコピーコストゼロ化を実現。
     */


    struct ApproxTableCache {
        // std::deque や std::vector の代わりに SegmentedVector を使用
        // T = std::vector<LightPA> となるため、実質「ベクタのベクタ」を分割管理する
        SegmentedVector<std::vector<LightPA>> lightTable;
        SegmentedVector<std::vector<DeepPA>> deepTable;
    };
}