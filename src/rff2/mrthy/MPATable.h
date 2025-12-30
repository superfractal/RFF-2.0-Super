// MPATable.h
// Created by Merutilm on 2025-05-18.
// Created by Super Fractal on 2025-11-24.
// Modified to use thread-safe SparseVector

#pragma once

#include <vector>
#include <algorithm>

#include "ArrayCompressionTool.h"
#include "ArrayCompressor.h"
#include "DeepPAGenerator.h"
#include "LightPAGenerator.h"
#include "MPAPeriod.h"
#include "SparseVector.h"
#include "../data/ApproxTableCache.h"
#include "../attr/FrtMPACompressionMethod.h"
#include "../constants/Constants.hpp"
#include "../../vulkan_helper/util/BufferImageUtils.hpp"
#include "../../vulkan_helper/core/logger.hpp"
#include "../ui/Utilities.h"

namespace merutilm::rff2 {

    template<typename Ref, typename Num>
    struct MPATable {
        static constexpr int REQUIRED_PERTURBATION = 2;

        const FrtMPAAttribute mpaSettings;
        std::vector<ArrayCompressionTool> pulledMPACompressor = std::vector<ArrayCompressionTool>();
        std::unique_ptr<MPAPeriod> mpaPeriod = nullptr;
        ApproxTableCache &tableRef;

        explicit MPATable(const ParallelRenderState &state, const Ref &reference,
                          const FrtMPAAttribute *mpaSettings, const Num &dcMax,
                          ApproxTableCache &tableRef,
                          std::function<void(uint64_t, double)> &&
                          actionPerCreatingTableIteration);

        virtual ~MPATable() = default;

    protected:
        void initTable(const MandelbrotReference &reference);

        std::vector<ArrayCompressionTool> createPulledMPACompressor(
            const std::vector<ArrayCompressionTool> &referenceCompressor) const;

        static uint64_t binarySearch(const std::vector<uint64_t> &arr, uint64_t key);

        template<typename PAB, typename PAG>
        void generateTable(const ParallelRenderState &state, const Ref &reference, Num dcMax,
                           std::function<void(uint64_t, double)> &&actionPerCreatingTableIteration);

        static uint64_t iterationToPulledTableIndex(const MPAPeriod &mpaPeriod, uint64_t iteration);

        static uint64_t iterationToCompTableIndex(const FrtMPACompressionMethod &mpaCompressionMethod,
                                                  const MPAPeriod &mpaPeriod,
                                                  const std::vector<ArrayCompressionTool> &pulledMPACompressor,
                                                  uint64_t iteration);

    public:
        virtual size_t getLength() = 0;
    };

    // ========================================================================
    // IMPLEMENTATION
    // ========================================================================

    template<typename Ref, typename Num>
    MPATable<Ref, Num>::MPATable(const ParallelRenderState &state, const Ref &reference,
                                 const FrtMPAAttribute *mpaSettings, const Num &dcMax,
                                 ApproxTableCache &tableRef,
                                 std::function<void(uint64_t, double)> &&
                                 actionPerCreatingTableIteration)
         : mpaSettings(*mpaSettings), tableRef(tableRef) {
        initTable(reference);

        if constexpr (std::is_same_v<Ref, LightMandelbrotReference>) {
            generateTable<LightPA, LightPAGenerator>(state, reference, dcMax,
                                                     std::move(actionPerCreatingTableIteration));
        } else {
            generateTable<DeepPA, DeepPAGenerator>(state, reference, dcMax,
                                                    std::move(actionPerCreatingTableIteration));
        }
    }

    template<typename Ref, typename Num>
    void MPATable<Ref, Num>::initTable(const MandelbrotReference &reference) {
        const auto &referencePeriod = reference.period;
        const uint64_t longestPeriod = reference.longestPeriod();

        if (const int minSkip = mpaSettings.minSkipReference; longestPeriod < minSkip) {
            this->pulledMPACompressor = std::vector<ArrayCompressionTool>();
            return;
        }

        const FrtMPACompressionMethod compressionMethod = mpaSettings.mpaCompressionMethod;
        this->mpaPeriod = MPAPeriod::create(referencePeriod, mpaSettings);
        this->pulledMPACompressor = compressionMethod == FrtMPACompressionMethod::STRONGEST
                                        ? createPulledMPACompressor(reference.compressor)
                                        : std::vector<ArrayCompressionTool>();
    }

    template<typename Ref, typename Num>
    std::vector<ArrayCompressionTool> MPATable<Ref, Num>::createPulledMPACompressor(
        const std::vector<ArrayCompressionTool> &referenceCompressor) const {
        std::vector<ArrayCompressionTool> mpaTools;

        auto &tablePeriod = mpaPeriod->tablePeriod;
        auto &tablePeriodElements = mpaPeriod->tableElements;
        auto &isArtificial = mpaPeriod->isArtificial;

        for (ArrayCompressionTool tool : referenceCompressor) {
            const uint64_t start = tool.start;
            const uint64_t length = tool.range();
            const uint64_t index = binarySearch(tablePeriod, length + 1);
            if (const uint64_t tableIndex = iterationToPulledTableIndex(*mpaPeriod, start);
                index != UINT64_MAX &&
                tableIndex != UINT64_MAX &&
                !isArtificial[index]) {
                const uint64_t periodElements = tablePeriodElements[index];
                mpaTools.emplace_back(1, tableIndex + 1, tableIndex + periodElements - 1);
            }
        }

        return mpaTools;
    }

    template<typename Ref, typename Num>
    uint64_t MPATable<Ref, Num>::binarySearch(const std::vector<uint64_t> &arr, const uint64_t key) {
        if (arr.empty()) {
            return UINT64_MAX;
        }

        uint64_t low = 0;
        uint64_t high = arr.size() - 1;

        while (low <= high) {
            const uint64_t mid = (low + high) >> 1;
            if (const uint64_t value = arr[mid]; value < key) {
                low = mid + 1;
            } else if (value > key) {
                if (mid == 0) {
                    return UINT64_MAX;
                }
                high = mid - 1;
            } else {
                return mid;
            }
        }

        return UINT64_MAX;
    }

    template<typename Ref, typename Num>
    template<typename PAB, typename PAG>
    void MPATable<Ref, Num>::generateTable(const ParallelRenderState &state, const Ref &reference,
                                            Num dcMax,
                                           std::function<void(uint64_t, double)> &&actionPerCreatingTableIteration) {
        const auto func = std::move(actionPerCreatingTableIteration);
        initTable(reference);

        if (mpaPeriod == nullptr) {
            return;
        }

        SparseVector<std::vector<PAB>> *tablePtr;

        if constexpr (std::is_same_v<LightPA, PAB>) {
            tablePtr = &tableRef.lightTable;
        } else {
            tablePtr = &tableRef.deepTable;
        }

        auto &table = *tablePtr;

        const auto &tablePeriod = mpaPeriod->tablePeriod;
        const uint64_t longestPeriod = tablePeriod.back();
        const auto &tableElements = mpaPeriod->tableElements;
        const auto mpaCompressionMethod = mpaSettings.mpaCompressionMethod;
        const auto epsilonPower = mpaSettings.epsilonPower;

        if (longestPeriod < mpaSettings.minSkipReference) {
            return;
        }

        const double epsilon = pow(10, epsilonPower);
        uint64_t iteration = 1;
        const size_t levels = tablePeriod.size();
        auto periodCount = std::vector<uint64_t>(levels, 0);
        auto currentPA = std::vector<std::unique_ptr<PAG>>(levels);
        auto dpTableTemps = std::array<dex, 8>();

        table.clear();

        // ============================================================================
        // NO_COMPRESSION 
        // ============================================================================
        if (mpaCompressionMethod == FrtMPACompressionMethod::NO_COMPRESSION) {
            uint64_t absIteration = 0;

            while (iteration <= longestPeriod) {
                if (absIteration % Constants::Fractal::EXIT_CHECK_INTERVAL == 0 && 
                    state.interruptRequested()) {
                    return;
                }

                func(iteration, static_cast<double>(iteration) / static_cast<double>(longestPeriod));

                bool resetLowerLevel = false;

                for (uint64_t level = levels; level > 0; --level) {
                    const uint64_t i = level - 1;

                    if (periodCount[i] == 0) {
                        if constexpr (std::is_same_v<PAG, LightPAGenerator>) {
                            currentPA[i] = LightPAGenerator::create(reference, epsilon, dcMax, iteration);
                        } else {
                            currentPA[i] = DeepPAGenerator::create(reference, epsilon, dcMax,
                                                                    iteration, dpTableTemps);
                        }
                    }

                    if (currentPA[i] != nullptr && 
                        periodCount[i] + REQUIRED_PERTURBATION < tablePeriod[i]) {
                        currentPA[i]->step();
                    }

                    periodCount[i]++;

                    if (periodCount[i] == tablePeriod[i]) {
                        if (const PAG *currentLevel = currentPA[i].get();
                            currentLevel != nullptr &&
                            currentLevel->getSkip() == tablePeriod[i] - REQUIRED_PERTURBATION) {
                            
                            const uint64_t storeIndex = currentLevel->getStart();
                            
                            table[storeIndex].push_back(currentLevel->build());
                        }
                        currentPA[i] = nullptr;
                        resetLowerLevel = true;
                    }

                    if (resetLowerLevel) {
                        periodCount[i] = 0;
                    }
                }

                ++iteration;
                ++absIteration;
            }
            return;
        }

        // ============================================================================
        // STANDARD PATH (Compression Enabled)
        // ============================================================================
        uint64_t absIteration = 0;
        
        (void)table[0];
        
        const std::vector<PAB> &mainReferenceMPA = table[0];

        while (iteration <= longestPeriod) {
            if (absIteration % Constants::Fractal::EXIT_CHECK_INTERVAL == 0 && 
                state.interruptRequested()) {
                return;
            }

            func(iteration, static_cast<double>(iteration) / static_cast<double>(longestPeriod));

            const uint64_t pulledTableIndex = iterationToPulledTableIndex(*mpaPeriod, iteration);
            const bool independent = ArrayCompressor::isIndependent(pulledMPACompressor, pulledTableIndex);
            const uint64_t containedIndex = ArrayCompressor::containedIndex(pulledMPACompressor,
                                                                             pulledTableIndex + 1);

            bool notSkippedPureZero = true;

            if (const ArrayCompressionTool *containedTool = containedIndex == UINT64_MAX
                                                                ? nullptr
                                                                : &pulledMPACompressor[containedIndex];
                containedTool != nullptr &&
                containedTool->start == pulledTableIndex + 1) {
                
                const uint64_t level = binarySearch(tableElements,
                                                     containedTool->end - containedTool->start + 2);
                const uint64_t compTableIndex = iterationToCompTableIndex(mpaCompressionMethod,
                                                                           *mpaPeriod,
                                                                          pulledMPACompressor, 
                                                                          iteration);

                auto &pa = table[compTableIndex];

                if (level < mainReferenceMPA.size()) {
                    const PAB &mainReferencePA = mainReferenceMPA[level];
                    const uint64_t skip = mainReferencePA.skip;

                    bool valid = true;
                    for (uint64_t i = level + 1; i < levels; ++i) {
                        if (i <= level && periodCount[i] != 0) {
                            vkh::logger::w_log_err(
                                L"WARNING : Failed to compress!! \n what : the table period count {} is not zero.",
                                periodCount[i]);
                            valid = false;
                            break;
                        }
                        if (periodCount[i] + skip > tablePeriod[i] - REQUIRED_PERTURBATION) {
                            vkh::logger::w_log_err(
                                L"WARNING : Failed to compress!! \n what : the table period count {} + skip {} exceeds its period {}.",
                                periodCount[i], skip, tablePeriod[i]);
                            valid = false;
                            break;
                        }
                    }

                    if (valid) {
                        for (uint64_t i = 0; i < levels; ++i) {
                            if (i <= level) {
                                pa.push_back(mainReferenceMPA[i]);
                                uint64_t count = skip;
                                for (uint64_t j = level; j > i; --j) {
                                    count %= tablePeriod[j - 1];
                                }
                                currentPA[i] = nullptr;
                                periodCount[i] = count;
                            } else {
                                if (currentPA[i] == nullptr) {
                                    std::unique_ptr<PAG> generator = nullptr;
                                    if constexpr (std::is_same_v<PAG, LightPAGenerator>) {
                                        generator = LightPAGenerator::create(reference, epsilon,
                                                                              dcMax, iteration);
                                    } else {
                                        generator = DeepPAGenerator::create(reference, epsilon,
                                                                             dcMax, iteration, dpTableTemps);
                                    }
                                    generator->merge(mainReferencePA);
                                    currentPA[i] = std::move(generator);
                                } else {
                                    currentPA[i]->merge(mainReferencePA);
                                }
                                periodCount[i] += skip;
                            }
                        }
                        iteration += skip;
                        notSkippedPureZero = false;
                    }
                }
            }

            bool resetLowerLevel = false;

            for (uint64_t level = levels; level > 0; --level) {
                const uint64_t i = level - 1;

                if (periodCount[i] == 0 && independent && notSkippedPureZero) {
                    if constexpr (std::is_same_v<PAG, LightPAGenerator>) {
                        currentPA[i] = LightPAGenerator::create(reference, epsilon, dcMax, iteration);
                    } else {
                        currentPA[i] = DeepPAGenerator::create(reference, epsilon, dcMax,
                                                                iteration, dpTableTemps);
                    }
                }

                if (currentPA[i] != nullptr && 
                    periodCount[i] + REQUIRED_PERTURBATION < tablePeriod[i]) {
                    currentPA[i]->step();
                }

                periodCount[i]++;

                if (periodCount[i] == tablePeriod[i]) {
                    if (const PAG *currentLevel = currentPA[i].get();
                        currentLevel != nullptr &&
                        currentLevel->getSkip() == tablePeriod[i] - REQUIRED_PERTURBATION) {
                        
                        const uint64_t compTableIndex = iterationToCompTableIndex(
                            mpaCompressionMethod, *mpaPeriod, pulledMPACompressor, 
                            currentLevel->getStart());

                        if (compTableIndex == UINT64_MAX) {
                            vkh::logger::w_log_err(
                                L"FATAL : FAILED TO CREATING TABLE!!\n what : iteration {} is not pullable. aborting the table creation...",
                                currentLevel->getStart());
                            return;
                        }

                        table[compTableIndex].push_back(currentLevel->build());
                    }
                    currentPA[i] = nullptr;
                    resetLowerLevel = true;
                }

                if (resetLowerLevel) {
                    periodCount[i] = 0;
                }
            }

            ++iteration;
            ++absIteration;
        }
    }

    template<typename Ref, typename Num>
    uint64_t MPATable<Ref, Num>::iterationToPulledTableIndex(const MPAPeriod &mpaPeriod,
                                                              const uint64_t iteration) {
        if (iteration == 0) {
            return UINT64_MAX;
        }

        const auto &tablePeriod = mpaPeriod.tablePeriod;
        const auto &tablePeriodElements = mpaPeriod.tableElements;

        uint64_t index = 0;
        uint64_t remainder = iteration;

        for (uint64_t i = tablePeriod.size(); i > 0; --i) {
            if (remainder < tablePeriod[i - 1]) {
                continue;
            }
            if (i < tablePeriod.size() && 
                remainder + tablePeriod[0] - REQUIRED_PERTURBATION + 1 > tablePeriod[i]) {
                return UINT64_MAX;
            }
            index += remainder / tablePeriod[i - 1] * tablePeriodElements[i - 1];
            remainder %= tablePeriod[i - 1];
        }

        return remainder == 1 ? index : UINT64_MAX;
    }

    template<typename Ref, typename Num>
    uint64_t MPATable<Ref, Num>::iterationToCompTableIndex(
        const FrtMPACompressionMethod &mpaCompressionMethod,
        const MPAPeriod &mpaPeriod,
        const std::vector<ArrayCompressionTool> &pulledMPACompressor,
        const uint64_t iteration) {
        
        switch (mpaCompressionMethod) {
            using enum FrtMPACompressionMethod;
            case NO_COMPRESSION: 
                return iteration;
            case LITTLE_COMPRESSION: 
                return iterationToPulledTableIndex(mpaPeriod, iteration);
            case STRONGEST: {
                const uint64_t index = iterationToPulledTableIndex(mpaPeriod, iteration);
                return index == UINT64_MAX ? UINT64_MAX : ArrayCompressor::compress(pulledMPACompressor, index);
            }
            default: 
                return iteration;
        }
    }

} // namespace merutilm::rff2
