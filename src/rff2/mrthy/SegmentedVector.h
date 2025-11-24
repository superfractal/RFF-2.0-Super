//
// Created by Super Fractal on 2025-11-24.
//


// SegmentedVector.h
#pragma once
#include <vector>
#include <memory>
#include <cassert>

namespace merutilm::rff2 { // 名前空間を合わせる

    template<typename T, size_t SEGMENT_BIT_SIZE = 16>
    class SegmentedVector {
    public:
        static constexpr size_t SEGMENT_SIZE = 1ULL << SEGMENT_BIT_SIZE;
        static constexpr size_t MASK = SEGMENT_SIZE - 1;

        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;

    private:
        std::vector<std::unique_ptr<T[]>> segments;
        size_type m_size = 0;
        size_type m_capacity = 0;

    public:
        SegmentedVector() = default;
        SegmentedVector(const SegmentedVector&) = delete;
        SegmentedVector& operator=(const SegmentedVector&) = delete;
        SegmentedVector(SegmentedVector&&) noexcept = default;
        SegmentedVector& operator=(SegmentedVector&&) noexcept = default;

        ~SegmentedVector() { clear(); }

        reference operator[](size_type index) {
            // assert(index < m_size); // デバッグ時以外はコメントアウトでも可（速度優先）
            return segments[index >> SEGMENT_BIT_SIZE][index & MASK];
        }

        const_reference operator[](size_type index) const {
            return segments[index >> SEGMENT_BIT_SIZE][index & MASK];
        }

        reference back() { return (*this)[m_size - 1]; }
        const_reference back() const { return (*this)[m_size - 1]; }

        size_type size() const noexcept { return m_size; }
        size_type capacity() const noexcept { return m_capacity; }
        bool empty() const noexcept { return m_size == 0; }

        void clear() {
            segments.clear();
            m_size = 0;
            m_capacity = 0;
        }

        void reserve(size_type new_capacity) {
            if (new_capacity <= m_capacity) return;
            size_type current_segments = segments.size();
            size_type required_segments = (new_capacity + SEGMENT_SIZE - 1) >> SEGMENT_BIT_SIZE;
            if (required_segments > current_segments) {
                segments.reserve(required_segments);
                for (size_type i = current_segments; i < required_segments; ++i) {
                    segments.push_back(std::make_unique<T[]>(SEGMENT_SIZE));
                }
                m_capacity = required_segments << SEGMENT_BIT_SIZE;
            }
        }

        template<typename... Args>
        void emplace_back(Args&&... args) {
            if (m_size == m_capacity) {
                segments.push_back(std::make_unique<T[]>(SEGMENT_SIZE));
                m_capacity += SEGMENT_SIZE;
            }
            (*this)[m_size] = T(std::forward<Args>(args)...);
            m_size++;
        }

        void push_back(const T& value) { emplace_back(value); }
        void push_back(T&& value) { emplace_back(std::move(value)); }
    };

} // namespace merutilm::rff2