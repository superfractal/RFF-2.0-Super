// SegmentedVector.h
// Created by Super Fractal on 2025-11-24.
// Modified for lazy allocation to prevent std::bad_alloc

#pragma once

#include <vector>
#include <memory>
#include <cassert>
#include <cstring>
#include <stdexcept>

namespace merutilm::rff2 {

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
        size_type m_logical_capacity = 0;

        void ensure_segment(size_type segment_index) {
            if (segment_index >= segments.size()) {
                segments.resize(segment_index + 1);
            }
            if (!segments[segment_index]) {
                segments[segment_index] = std::make_unique<T[]>(SEGMENT_SIZE);
            }
        }

    public:
        SegmentedVector() = default;

        SegmentedVector(const SegmentedVector&) = delete;
        SegmentedVector& operator=(const SegmentedVector&) = delete;

        SegmentedVector(SegmentedVector&&) noexcept = default;
        SegmentedVector& operator=(SegmentedVector&&) noexcept = default;

        ~SegmentedVector() { clear(); }

        reference operator[](size_type index) {
            size_type seg_idx = index >> SEGMENT_BIT_SIZE;
            ensure_segment(seg_idx);
            return segments[seg_idx][index & MASK];
        }

        const_reference operator[](size_type index) const {
            size_type seg_idx = index >> SEGMENT_BIT_SIZE;
            if (seg_idx >= segments.size() || !segments[seg_idx]) {
                static T default_value{};
                return default_value;
            }
            return segments[seg_idx][index & MASK];
        }

        reference back() { return (*this)[m_size - 1]; }
        const_reference back() const { return (*this)[m_size - 1]; }

        size_type size() const noexcept { return m_size; }
        size_type capacity() const noexcept { return m_logical_capacity; }
        bool empty() const noexcept { return m_size == 0; }

        void clear() {
            segments.clear();
            m_size = 0;
            m_logical_capacity = 0;
        }

        void reserve(size_type new_capacity) {
            if (new_capacity <= m_logical_capacity) return;
            
            size_type required_segments = (new_capacity + SEGMENT_SIZE - 1) >> SEGMENT_BIT_SIZE;
            
            if (required_segments > segments.size()) {
                segments.resize(required_segments);
                m_logical_capacity = required_segments << SEGMENT_BIT_SIZE;
            }
        }

        void resize_lazy(size_type new_size) {
            if (new_size <= m_size) {
                m_size = new_size;
                return;
            }
            
            size_type required_segments = (new_size + SEGMENT_SIZE - 1) >> SEGMENT_BIT_SIZE;
            
            if (required_segments > segments.size()) {
                segments.resize(required_segments);
                m_logical_capacity = required_segments << SEGMENT_BIT_SIZE;
            }
            
            m_size = new_size;
        }

        void resize(size_type new_size) {
            resize_lazy(new_size);
            
            size_type last_segment = (new_size > 0) ? ((new_size - 1) >> SEGMENT_BIT_SIZE) : 0;
            for (size_type i = 0; i <= last_segment && i < segments.size(); ++i) {
                ensure_segment(i);
            }
        }

        template<typename... Args>
        void emplace_back(Args&&... args) {
            size_type seg_idx = m_size >> SEGMENT_BIT_SIZE;
            
            ensure_segment(seg_idx);
            
            if (m_size >= m_logical_capacity) {
                m_logical_capacity = (seg_idx + 1) << SEGMENT_BIT_SIZE;
            }
            
            segments[seg_idx][m_size & MASK] = T(std::forward<Args>(args)...);
            m_size++;
        }

        void push_back(const T& value) { emplace_back(value); }
        void push_back(T&& value) { emplace_back(std::move(value)); }

        bool is_segment_allocated(size_type index) const {
            size_type seg_idx = index >> SEGMENT_BIT_SIZE;
            return seg_idx < segments.size() && segments[seg_idx] != nullptr;
        }

        size_type allocated_memory() const {
            size_type count = 0;
            for (const auto& seg : segments) {
                if (seg) count++;
            }
            return count * SEGMENT_SIZE * sizeof(T);
        }
    };

} // namespace merutilm::rff2
