//
// Created by Merutilm on 2025-05-09.
// Created by Super Fractal on 2025-11-24.
//

#pragma once
#include <optional>

#include "../calc/fp_complex.h"
#include <vector>

#include "MandelbrotReference.h"
#include "../mrthy/ArrayCompressor.h"
// 【追加】SegmentedVector をインクルード
// (ArrayCompressorと同じフォルダにあると仮定しています)
#include "../mrthy/SegmentedVector.h" 

#include "../parallel/ParallelRenderState.h"
#include "../attr/FractalAttribute.h"

namespace merutilm::rff2 {
    struct LightMandelbrotReference final : public MandelbrotReference{

        // 【変更】std::vector<double> -> SegmentedVector<double>
        // これにより、数億〜数十億ポイントの巨大な参照軌道データを
        // 連続領域を必要とせずにメモリに格納できるようになります。
        const SegmentedVector<double> refReal;
        const SegmentedVector<double> refImag;


        // 【変更】コンストラクタの引数も SegmentedVector に変更
        explicit LightMandelbrotReference(fp_complex &&center, 
                                 SegmentedVector<double> &&refReal,
                                 SegmentedVector<double> &&refImag, 
                                 std::vector<ArrayCompressionTool> &&compressor,
                                 std::vector<uint64_t> &&period, fp_complex &&fpgReference, fp_complex &&fpgBn);

        static std::unique_ptr<LightMandelbrotReference> createReference(const ParallelRenderState &state,
                                                                         const FractalAttribute &calc, int exp10,
                                                                         uint64_t initialPeriod, double dcMax, bool
                                                                         strictFPG,
                                                                         std::function<void(uint64_t)> &&
                                                                         actionPerRefCalcIteration);

        double real(uint64_t refIteration) const;

        double imag(uint64_t refIteration) const;

        size_t length() const override;

        uint64_t longestPeriod() const override;
    };
}