//
// Created by Merutilm on 2025-05-23.
// Created by Super Fractal on 2025-11-22.
//

#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "../mrthy/DeepPA.h"
#include "../mrthy/LightPA.h"

namespace merutilm::rff2 {

    // 【追加】高速かつメモリ断片化に強い分割ベクタクラス
    template<typename T>
    class SegmentedVector {
        // 1ブロックあたりの要素数 (2^16 = 65536)
        // ビット演算を使うため 2の乗数である必要があります
        static constexpr size_t SEGMENT_BITS = 16;
        static constexpr size_t SEGMENT_SIZE = 1ULL << SEGMENT_BITS;
        static constexpr size_t MASK = SEGMENT_SIZE - 1;

        // unique_ptrを使うことで、segments ベクタ自体が再確保されても
        // 内部のデータブロックのアドレスが変わらないようにする
        std::vector<std::unique_ptr<std::vector<T>>> segments;
        size_t count = 0;

    public:
        SegmentedVector() = default;

        // 高速アクセス (ビット演算)
        inline T& operator[](size_t index) {
            // dequeのような割り算が発生しないため高速
            return (*segments[index >> SEGMENT_BITS])[index & MASK];
        }

        inline const T& operator[](size_t index) const {
            return (*segments[index >> SEGMENT_BITS])[index & MASK];
        }

        // 末尾への追加
        template<typename... Args>
        void emplace_back(Args&&... args) {
            const size_t segIndex = count >> SEGMENT_BITS;
            
            // 新しいブロックが必要なら確保
            if (segIndex >= segments.size()) {
                segments.push_back(std::make_unique<std::vector<T>>());
                segments.back()->reserve(SEGMENT_SIZE);
            }

            segments[segIndex]->emplace_back(std::forward<Args>(args)...);
            count++;
        }

        void push_back(const T& value) {
            emplace_back(value);
        }
        
        void push_back(T&& value) {
            emplace_back(std::move(value));
        }

        T& back() {
            return segments.back()->back();
        }

        size_t size() const {
            return count;
        }

        // API互換性のためのダミー（断片化するので実際には何もしない、またはブロック確保だけ行う）
        void reserve(size_t new_cap) {
            // 事前確保はメモリ効率が悪化するリスクがあるため、
            // ここではあえて何もしないか、segmentsベクタ（ポインタ配列）の予約だけ行う
            const size_t required_segments = (new_cap + SEGMENT_SIZE - 1) >> SEGMENT_BITS;
            if (required_segments > segments.capacity()) {
                segments.reserve(required_segments);
            }
        }

        size_t capacity() const {
            return segments.size() * SEGMENT_SIZE;
        }

        void clear() {
            segments.clear();
            count = 0;
        }
    };

    struct ApproxTableCache {
        // std::deque から SegmentedVector に変更
        SegmentedVector<std::vector<LightPA>> lightTable;
        SegmentedVector<std::vector<DeepPA>> deepTable;
    };
}