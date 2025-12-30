// SparseVector.h
// Created by Super Fractal on 2025-11-24.

#pragma once

#include <vector>
#include <memory>
#include <cassert>
#include <cstring>

namespace merutilm::rff2 {

    template<typename T, size_t SEGMENT_BIT_SIZE = 16>
    class SparseVector {
    public:
        static constexpr size_t SEGMENT_SIZE = 1ULL << SEGMENT_BIT_SIZE;
        static constexpr size_t MASK = SEGMENT_SIZE - 1;

        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using size_type = uint64_t;

    private:
        std::vector<std::unique_ptr<T[]>> m_segments;
        size_type m_size = 0;

        static size_type segment_index(size_type index) {
            return index >> SEGMENT_BIT_SIZE;
        }

        static size_type segment_offset(size_type index) {
            return index & MASK;
        }

        void ensure_segment(size_type seg_idx) {
            if (seg_idx >= m_segments.size()) {
                m_segments.resize(seg_idx + 1);
            }
            if (m_segments[seg_idx] == nullptr) {
                m_segments[seg_idx] = std::make_unique<T[]>(SEGMENT_SIZE);
            }
        }

    public:
        SparseVector() = default;

        SparseVector(const SparseVector&) = delete;
        SparseVector& operator=(const SparseVector&) = delete;

        SparseVector(SparseVector&& other) noexcept
            : m_segments(std::move(other.m_segments)), m_size(other.m_size) {
            other.m_size = 0;
        }

        SparseVector& operator=(SparseVector&& other) noexcept {
            if (this != &other) {
                m_segments = std::move(other.m_segments);
                m_size = other.m_size;
                other.m_size = 0;
            }
            return *this;
        }

        ~SparseVector() = default;

        reference operator[](size_type index) {
            const size_type seg_idx = segment_index(index);
            ensure_segment(seg_idx);
            if (index >= m_size) {
                m_size = index + 1;
            }
            return m_segments[seg_idx][segment_offset(index)];
        }

        const_reference at_unchecked(size_type index) const {
            return m_segments[segment_index(index)][segment_offset(index)];
        }

        reference at_unchecked(size_type index) {
            return m_segments[segment_index(index)][segment_offset(index)];
        }

        const T* get(size_type index) const {
            const size_type seg_idx = segment_index(index);
            if (seg_idx >= m_segments.size() || m_segments[seg_idx] == nullptr) {
                return nullptr;
            }
            return &m_segments[seg_idx][segment_offset(index)];
        }

        T get_or_default(size_type index) const {
            const size_type seg_idx = segment_index(index);
            if (seg_idx >= m_segments.size() || m_segments[seg_idx] == nullptr) {
                return T{};
            }
            return m_segments[seg_idx][segment_offset(index)];
        }

        bool has_segment(size_type index) const {
            const size_type seg_idx = segment_index(index);
            return seg_idx < m_segments.size() && m_segments[seg_idx] != nullptr;
        }

        size_type size() const noexcept { 
            return m_size; 
        }

        size_type segment_count() const noexcept {
            size_type count = 0;
            for (const auto& seg : m_segments) {
                if (seg != nullptr) count++;
            }
            return count;
        }

        bool empty() const noexcept { 
            return m_size == 0; 
        }

        void clear() {
            m_segments.clear();
            m_size = 0;
        }

        void resize(size_type new_size) {
            const size_type new_seg_count = (new_size == 0) ? 0 : segment_index(new_size - 1) + 1;
            if (new_seg_count < m_segments.size()) {
                m_segments.resize(new_seg_count);
            }
            m_size = new_size;
        }

        void reserve(size_type count) {
            const size_type seg_count = (count + SEGMENT_SIZE - 1) >> SEGMENT_BIT_SIZE;
            if (seg_count > m_segments.size()) {
                m_segments.reserve(seg_count);
            }
        }

        template<typename... Args>
        void emplace_back(Args&&... args) {
            const size_type seg_idx = segment_index(m_size);
            ensure_segment(seg_idx);
            m_segments[seg_idx][segment_offset(m_size)] = T(std::forward<Args>(args)...);
            m_size++;
        }

        void push_back(const T& value) {
            const size_type seg_idx = segment_index(m_size);
            ensure_segment(seg_idx);
            m_segments[seg_idx][segment_offset(m_size)] = value;
            m_size++;
        }

        void push_back(T&& value) {
            const size_type seg_idx = segment_index(m_size);
            ensure_segment(seg_idx);
            m_segments[seg_idx][segment_offset(m_size)] = std::move(value);
            m_size++;
        }

        size_type allocated_memory() const {
            size_type count = 0;
            for (const auto& seg : m_segments) {
                if (seg != nullptr) count++;
            }
            return count * SEGMENT_SIZE * sizeof(T) + 
                   m_segments.capacity() * sizeof(std::unique_ptr<T[]>);
        }
    };

} // namespace merutilm::rff2
