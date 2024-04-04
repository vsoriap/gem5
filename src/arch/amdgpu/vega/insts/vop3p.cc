/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "arch/amdgpu/vega/insts/vop3p.hh"

#include "arch/amdgpu/vega/insts/instructions.hh"
#include "arch/arm/insts/fplib.hh"

namespace gem5
{

namespace VegaISA
{

using half = uint16_t;

// Helper functions
template<int N>
int32_t
dotClampI(int32_t value, bool clamp)
{
    // Only valid for N < 32
    static_assert(N < 32);

    if (!clamp) {
        return static_cast<int32_t>(value);
    }

    int32_t min = -(1 << (N - 1));
    int32_t max = (1 << (N - 1)) - 1;
    return std::clamp<int32_t>(value, min, max);
}

template<int N>
uint32_t
dotClampU(uint32_t value, bool clamp)
{
    // Only valid for N < 32
    static_assert(N < 32);

    if (!clamp) {
        return static_cast<int32_t>(value);
    }

    uint32_t min = 0;
    uint32_t max = (1 << N) - 1;
    return std::clamp<int32_t>(value, min, max);
}

int16_t
clampI16(int32_t value, bool clamp)
{
    if (!clamp) {
        return static_cast<int16_t>(value);
    }

    return std::clamp(value,
            static_cast<int32_t>(std::numeric_limits<int16_t>::min()),
            static_cast<int32_t>(std::numeric_limits<int16_t>::max()));
}

uint16_t
clampU16(uint32_t value, bool clamp)
{
    if (!clamp) {
        return static_cast<uint16_t>(value);
    }

    return std::clamp(value,
            static_cast<uint32_t>(std::numeric_limits<uint16_t>::min()),
            static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
}

uint16_t
clampF16(uint16_t value, bool clamp)
{
    if (!clamp) {
        return value;
    }

    // Values of one and zero in fp16.
    constexpr uint16_t one = 0x3c00;
    constexpr uint16_t zero = 0x0;
    ArmISA::FPSCR fpscr1, fpscr2;

    // If value > one, set to one, then if value < zero set to zero.
    uint16_t imm = fplibMin(value, one, fpscr1);
    return fplibMax(imm, zero, fpscr2);
}

float
clampF32(float value, bool clamp)
{
    if (!clamp) {
        return value;
    }

    return std::clamp(value, 0.0f, 1.0f);
}




// Begin instruction execute definitions
void Inst_VOP3P__V_PK_MAD_I16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](int16_t S0, int16_t S1, int16_t S2, bool clamp) -> int16_t
    {
        return clampI16(S0 * S1 + S2, clamp);
    };

    vop3pHelper<int16_t>(gpuDynInst, opImpl);
}

void
Inst_VOP3P__V_PK_MUL_LO_U16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](uint16_t S0, uint16_t S1, bool) -> uint16_t
    {
        // Only return lower 16 bits of result - This operation cannot clamp.
        uint32_t D = S0 * S1;
        uint16_t Dh = D & 0xFFFF;
        return Dh;
    };

    vop3pHelper<uint16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_ADD_I16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](int16_t S0, int16_t S1, bool clamp) -> int16_t
    {
        return clampI16(S0 + S1, clamp);
    };

    vop3pHelper<int16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_SUB_I16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](int16_t S0, int16_t S1, bool clamp) -> int16_t
    {
        return clampI16(S0 - S1, clamp);
    };

    vop3pHelper<int16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_LSHLREV_B16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](uint16_t S0, uint16_t S1, bool) -> uint16_t
    {
        unsigned shift_val = bits(S0, 3, 0);

        // Shift does not clamp
        return S1 << shift_val;
    };

    vop3pHelper<uint16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_LSHRREV_B16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](uint16_t S0, uint16_t S1, bool) -> uint16_t
    {
        unsigned shift_val = bits(S0, 3, 0);

        return S1 >> shift_val;
    };

    vop3pHelper<uint16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_ASHRREV_B16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](int16_t S0, int16_t S1, bool clamp) -> int16_t
    {
        // Sign extend to larger type to ensure we don't lose sign bits when
        // shifting.
        int32_t S1e = S1;
        unsigned shift_val = bits(S0, 3, 0);

        return S1e >> shift_val;
    };

    vop3pHelper<int16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_MAX_I16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](int16_t S0, int16_t S1, bool clamp) -> int16_t
    {
        return clampI16((S0 >= S1) ? S0 : S1, clamp);
    };

    vop3pHelper<int16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_MIN_I16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](int16_t S0, int16_t S1, bool clamp) -> int16_t
    {
        return clampI16((S0 < S1) ? S0 : S1, clamp);
    };

    vop3pHelper<int16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_MAD_U16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](uint16_t S0, uint16_t S1, uint16_t S2, bool clamp) -> uint16_t
    {
        return clampU16(S0 * S1 + S2, clamp);
    };

    vop3pHelper<uint16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_ADD_U16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](uint16_t S0, uint16_t S1, bool clamp) -> uint16_t
    {
        return clampU16(S0 + S1, clamp);
    };

    vop3pHelper<uint16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_SUB_U16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](uint16_t S0, uint16_t S1, bool clamp) -> uint16_t
    {
        return clampU16(S0 - S1, clamp);
    };

    vop3pHelper<uint16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_MAX_U16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](uint16_t S0, uint16_t S1, bool clamp) -> uint16_t
    {
        return clampU16((S0 >= S1) ? S0 : S1, clamp);
    };

    vop3pHelper<uint16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_MIN_U16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](uint16_t S0, uint16_t S1, bool clamp) -> uint16_t
    {
        return clampU16((S0 < S1) ? S0 : S1, clamp);
    };

    vop3pHelper<uint16_t>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_FMA_F16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](half S0, half S1, half S2, bool clamp) -> half
    {
        ArmISA::FPSCR fpscr;
        return clampF16(fplibMulAdd(S2, S0, S1, fpscr), clamp);
    };

    vop3pHelper<half>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_ADD_F16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](half S0, half S1, bool clamp) -> half
    {
        ArmISA::FPSCR fpscr;
        return clampF16(fplibAdd(S0, S1, fpscr), clamp);
    };

    vop3pHelper<half>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_MUL_F16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](half S0, half S1, bool clamp) -> half
    {
        ArmISA::FPSCR fpscr;
        return clampF16(fplibMul(S0, S1, fpscr), clamp);
    };

    vop3pHelper<half>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_MIN_F16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](half S0, half S1, bool clamp) -> half
    {
        ArmISA::FPSCR fpscr;
        return clampF16(fplibMin(S0, S1, fpscr), clamp);
    };

    vop3pHelper<half>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_PK_MAX_F16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl = [](half S0, half S1, bool clamp) -> half
    {
        ArmISA::FPSCR fpscr;
        return clampF16(fplibMax(S0, S1, fpscr), clamp);
    };

    vop3pHelper<half>(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_DOT2_F32_F16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](uint32_t S0r, uint32_t S1r, uint32_t S2r, bool clamp) -> uint32_t
    {
        constexpr unsigned INBITS = 16;

        constexpr unsigned elems = 32 / INBITS;
        half S0[elems];
        half S1[elems];

        for (int i = 0; i < elems; ++i) {
            S0[i] = bits(S0r, i*INBITS+INBITS-1, i*INBITS);
            S1[i] = bits(S1r, i*INBITS+INBITS-1, i*INBITS);
        }

        float S2 = *reinterpret_cast<float*>(&S2r);

        // Compute components individually to prevent overflow across packing
        half C[elems];
        float Csum = 0.0f;

        for (int i = 0; i < elems; ++i) {
            ArmISA::FPSCR fpscr;
            C[i] = fplibMul(S0[i], S1[i], fpscr);
            uint32_t conv =
                ArmISA::fplibConvert<uint16_t, uint32_t>(
                        C[i], ArmISA::FPRounding_TIEEVEN, fpscr);
            Csum += clampF32(*reinterpret_cast<float*>(&conv), clamp);
        }

        Csum += S2;
        uint32_t rv = *reinterpret_cast<uint32_t*>(&Csum);

        return rv;
    };

    dotHelper(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_DOT2_I32_I16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](uint32_t S0r, uint32_t S1r, uint32_t S2r, bool clamp) -> uint32_t
    {
        constexpr unsigned INBITS = 16;

        constexpr unsigned elems = 32 / INBITS;
        uint32_t S0[elems];
        uint32_t S1[elems];

        for (int i = 0; i < elems; ++i) {
            S0[i] = bits(S0r, i*INBITS+INBITS-1, i*INBITS);
            S1[i] = bits(S1r, i*INBITS+INBITS-1, i*INBITS);
        }

        int32_t S2 = *reinterpret_cast<int32_t*>(&S2r);

        // Compute components individually to prevent overflow across packing
        int32_t C[elems];
        int32_t Csum = 0;

        for (int i = 0; i < elems; ++i) {
            C[i] = sext<INBITS>(S0[i]) * sext<INBITS>(S1[i]);
            C[i] = sext<INBITS>(dotClampI<INBITS>(C[i], clamp) & mask(INBITS));
            Csum += C[i];
        }

        Csum += S2;
        uint32_t rv = *reinterpret_cast<uint32_t*>(&Csum);

        return rv;
    };

    dotHelper(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_DOT2_U32_U16::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](uint32_t S0r, uint32_t S1r, uint32_t S2, bool clamp) -> uint32_t
    {
        constexpr unsigned INBITS = 16;

        constexpr unsigned elems = 32 / INBITS;
        uint32_t S0[elems];
        uint32_t S1[elems];

        for (int i = 0; i < elems; ++i) {
            S0[i] = bits(S0r, i*INBITS+INBITS-1, i*INBITS);
            S1[i] = bits(S1r, i*INBITS+INBITS-1, i*INBITS);
        }

        // Compute components individually to prevent overflow across packing
        uint32_t C[elems];
        uint32_t Csum = 0;

        for (int i = 0; i < elems; ++i) {
            C[i] = S0[i] * S1[i];
            C[i] = dotClampU<INBITS>(C[i], clamp);
            Csum += C[i];
        }

        Csum += S2;

        return Csum;
    };

    dotHelper(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_DOT4_I32_I8::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](uint32_t S0r, uint32_t S1r, uint32_t S2r, bool clamp) -> uint32_t
    {
        constexpr unsigned INBITS = 8;

        constexpr unsigned elems = 32 / INBITS;
        uint32_t S0[elems];
        uint32_t S1[elems];

        for (int i = 0; i < elems; ++i) {
            S0[i] = bits(S0r, i*INBITS+INBITS-1, i*INBITS);
            S1[i] = bits(S1r, i*INBITS+INBITS-1, i*INBITS);
        }

        int32_t S2 = *reinterpret_cast<int32_t*>(&S2r);

        // Compute components individually to prevent overflow across packing
        int32_t C[elems];
        int32_t Csum = 0;

        for (int i = 0; i < elems; ++i) {
            C[i] = sext<INBITS>(S0[i]) * sext<INBITS>(S1[i]);
            C[i] = sext<INBITS>(dotClampI<INBITS>(C[i], clamp) & mask(INBITS));
            Csum += C[i];
        }

        Csum += S2;
        uint32_t rv = *reinterpret_cast<uint32_t*>(&Csum);

        return rv;
    };

    dotHelper(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_DOT4_U32_U8::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](uint32_t S0r, uint32_t S1r, uint32_t S2, bool clamp) -> uint32_t
    {
        constexpr unsigned INBITS = 8;

        constexpr unsigned elems = 32 / INBITS;
        uint32_t S0[elems];
        uint32_t S1[elems];

        for (int i = 0; i < elems; ++i) {
            S0[i] = bits(S0r, i*INBITS+INBITS-1, i*INBITS);
            S1[i] = bits(S1r, i*INBITS+INBITS-1, i*INBITS);
        }

        // Compute components individually to prevent overflow across packing
        uint32_t C[elems];
        uint32_t Csum = 0;

        for (int i = 0; i < elems; ++i) {
            C[i] = S0[i] * S1[i];
            C[i] = dotClampU<INBITS>(C[i], clamp);
            Csum += C[i];
        }

        Csum += S2;

        return Csum;
    };

    dotHelper(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_DOT8_I32_I4::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](uint32_t S0r, uint32_t S1r, uint32_t S2r, bool clamp) -> uint32_t
    {
        constexpr unsigned INBITS = 4;

        constexpr unsigned elems = 32 / INBITS;
        uint32_t S0[elems];
        uint32_t S1[elems];

        for (int i = 0; i < elems; ++i) {
            S0[i] = bits(S0r, i*INBITS+INBITS-1, i*INBITS);
            S1[i] = bits(S1r, i*INBITS+INBITS-1, i*INBITS);
        }

        int32_t S2 = *reinterpret_cast<int32_t*>(&S2r);

        // Compute components individually to prevent overflow across packing
        int32_t C[elems];
        int32_t Csum = 0;

        for (int i = 0; i < elems; ++i) {
            C[i] = sext<INBITS>(S0[i]) * sext<INBITS>(S1[i]);
            C[i] = sext<INBITS>(dotClampI<INBITS>(C[i], clamp) & mask(INBITS));
            Csum += C[i];
        }

        Csum += S2;
        uint32_t rv = *reinterpret_cast<uint32_t*>(&Csum);

        return rv;
    };

    dotHelper(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_DOT8_U32_U4::execute(GPUDynInstPtr gpuDynInst)
{
    auto opImpl =
        [](uint32_t S0r, uint32_t S1r, uint32_t S2, bool clamp) -> uint32_t
    {
        constexpr unsigned INBITS = 4;

        constexpr unsigned elems = 32 / INBITS;
        uint32_t S0[elems];
        uint32_t S1[elems];

        for (int i = 0; i < elems; ++i) {
            S0[i] = bits(S0r, i*INBITS+INBITS-1, i*INBITS);
            S1[i] = bits(S1r, i*INBITS+INBITS-1, i*INBITS);
        }

        // Compute components individually to prevent overflow across packing
        uint32_t C[elems];
        uint32_t Csum = 0;

        for (int i = 0; i < elems; ++i) {
            C[i] = S0[i] * S1[i];
            C[i] = dotClampU<INBITS>(C[i], clamp);
            Csum += C[i];
        }

        Csum += S2;

        return Csum;
    };

    dotHelper(gpuDynInst, opImpl);
}

void Inst_VOP3P__V_ACCVGPR_READ::execute(GPUDynInstPtr gpuDynInst)
{
    Wavefront *wf = gpuDynInst->wavefront();
    unsigned accum_offset = wf->accumOffset;

    ConstVecOperandU32 src(gpuDynInst, extData.SRC0+accum_offset);
    VecOperandU32 vdst(gpuDynInst, instData.VDST);

    src.readSrc();

    for (int lane = 0; lane < NumVecElemPerVecReg; ++lane) {
        if (wf->execMask(lane)) {
            vdst[lane] = src[lane];
        }
    }

    vdst.write();
}

void Inst_VOP3P__V_ACCVGPR_WRITE::execute(GPUDynInstPtr gpuDynInst)
{
    Wavefront *wf = gpuDynInst->wavefront();
    unsigned accum_offset = wf->accumOffset;

    ConstVecOperandU32 src(gpuDynInst, extData.SRC0);
    VecOperandU32 vdst(gpuDynInst, instData.VDST+accum_offset);

    src.readSrc();

    for (int lane = 0; lane < NumVecElemPerVecReg; ++lane) {
        if (wf->execMask(lane)) {
            vdst[lane] = src[lane];
        }
    }

    vdst.write();
}

// --- Inst_VOP3P__V_PK_FMA_F32 class methods ---

Inst_VOP3P__V_PK_FMA_F32::Inst_VOP3P__V_PK_FMA_F32(InFmt_VOP3P *iFmt)
    : Inst_VOP3P(iFmt, "v_pk_fma_f32")
{
    setFlag(ALU);
} // Inst_VOP3P__V_PK_FMA_F32

Inst_VOP3P__V_PK_FMA_F32::~Inst_VOP3P__V_PK_FMA_F32()
{
} // ~Inst_VOP3P__V_PK_FMA_F32

// D.f[63:32] = S0.f[63:32] * S1.f[63:32] + S2.f[63:32] . D.f[31:0] =
//     S0.f[31:0] * S1.f[31:0] + S2.f[31:0] .
void
Inst_VOP3P__V_PK_FMA_F32::execute(GPUDynInstPtr gpuDynInst)
{
    // This is a special case of packed instructions which operates on
    // 64-bit inputs/outputs and not 32-bit. U64 is used here as float
    // values cannot use bitwise operations. Consider the U64 to imply
    // untyped 64-bits of data.
    Wavefront *wf = gpuDynInst->wavefront();
    ConstVecOperandU64 src0(gpuDynInst, extData.SRC0);
    ConstVecOperandU64 src1(gpuDynInst, extData.SRC1);
    ConstVecOperandU64 src2(gpuDynInst, extData.SRC2);
    VecOperandU64 vdst(gpuDynInst, instData.VDST);

    src0.readSrc();
    src1.readSrc();
    src2.readSrc();

    int opsel = instData.OPSEL;
    int opsel_hi = extData.OPSEL_HI | (instData.OPSEL_HI2 << 2);

    int neg = extData.NEG;
    int neg_hi = instData.NEG_HI;

    for (int lane = 0; lane < NumVecElemPerVecReg; ++lane) {
        if (wf->execMask(lane)) {
            uint32_t s0l = (opsel & 1) ? bits(src0[lane], 63, 32)
                                       : bits(src0[lane], 31, 0);
            uint32_t s1l = (opsel & 2) ? bits(src1[lane], 63, 32)
                                       : bits(src1[lane], 31, 0);
            uint32_t s2l = (opsel & 4) ? bits(src2[lane], 63, 32)
                                       : bits(src2[lane], 31, 0);

            float s0lf = *reinterpret_cast<float*>(&s0l);
            float s1lf = *reinterpret_cast<float*>(&s1l);
            float s2lf = *reinterpret_cast<float*>(&s2l);

            if (neg & 1) s0lf = -s0lf;
            if (neg & 1) s1lf = -s1lf;
            if (neg & 1) s2lf = -s2lf;

            float dword1 = std::fma(s0lf, s1lf, s2lf);

            uint32_t s0h = (opsel_hi & 1) ? bits(src0[lane], 63, 32)
                                          : bits(src0[lane], 31, 0);
            uint32_t s1h = (opsel_hi & 2) ? bits(src1[lane], 63, 32)
                                          : bits(src1[lane], 31, 0);
            uint32_t s2h = (opsel_hi & 4) ? bits(src2[lane], 63, 32)
                                          : bits(src2[lane], 31, 0);

            float s0hf = *reinterpret_cast<float*>(&s0h);
            float s1hf = *reinterpret_cast<float*>(&s1h);
            float s2hf = *reinterpret_cast<float*>(&s2h);

            if (neg_hi & 1) s0hf = -s0hf;
            if (neg_hi & 1) s1hf = -s1hf;
            if (neg_hi & 1) s2hf = -s2hf;

            float dword2 = std::fma(s0hf, s1hf, s2hf);

            uint32_t result1 = *reinterpret_cast<uint32_t*>(&dword1);
            uint32_t result2 = *reinterpret_cast<uint32_t*>(&dword2);

            vdst[lane] = (static_cast<uint64_t>(result2) << 32) | result1;
        }
    }

    vdst.write();
} // execute
// --- Inst_VOP3P__V_PK_MUL_F32 class methods ---

Inst_VOP3P__V_PK_MUL_F32::Inst_VOP3P__V_PK_MUL_F32(InFmt_VOP3P *iFmt)
    : Inst_VOP3P(iFmt, "v_pk_mul_f32")
{
    setFlag(ALU);
} // Inst_VOP3P__V_PK_MUL_F32

Inst_VOP3P__V_PK_MUL_F32::~Inst_VOP3P__V_PK_MUL_F32()
{
} // ~Inst_VOP3P__V_PK_MUL_F32

// D.f[63:32] = S0.f[63:32] * S1.f[63:32] . D.f[31:0] = S0.f[31:0] *
//              S1.f[31:0]
void
Inst_VOP3P__V_PK_MUL_F32::execute(GPUDynInstPtr gpuDynInst)
{
    // This is a special case of packed instructions which operates on
    // 64-bit inputs/outputs and not 32-bit. U64 is used here as float
    // values cannot use bitwise operations. Consider the U64 to imply
    // untyped 64-bits of data.
    Wavefront *wf = gpuDynInst->wavefront();
    ConstVecOperandU64 src0(gpuDynInst, extData.SRC0);
    ConstVecOperandU64 src1(gpuDynInst, extData.SRC1);
    VecOperandU64 vdst(gpuDynInst, instData.VDST);

    src0.readSrc();
    src1.readSrc();

    int opsel = instData.OPSEL;
    int opsel_hi = extData.OPSEL_HI;

    int neg = extData.NEG;
    int neg_hi = instData.NEG_HI;

    for (int lane = 0; lane < NumVecElemPerVecReg; ++lane) {
        if (wf->execMask(lane)) {
            uint32_t lower_dword = (opsel & 1) ? bits(src0[lane], 63, 32)
                                               : bits(src0[lane], 31, 0);
            uint32_t upper_dword = (opsel & 2) ? bits(src1[lane], 63, 32)
                                               : bits(src1[lane], 31, 0);

            float ldwordf = *reinterpret_cast<float*>(&lower_dword);
            float udwordf = *reinterpret_cast<float*>(&upper_dword);

            if (neg & 1) ldwordf = -ldwordf;
            if (neg & 2) udwordf = -udwordf;

            float dword1 = ldwordf * udwordf;

            lower_dword = (opsel_hi & 1) ? bits(src0[lane], 63, 32)
                                         : bits(src0[lane], 31, 0);
            upper_dword = (opsel_hi & 2) ? bits(src1[lane], 63, 32)
                                         : bits(src1[lane], 31, 0);

            ldwordf = *reinterpret_cast<float*>(&lower_dword);
            udwordf = *reinterpret_cast<float*>(&upper_dword);

            if (neg_hi & 1) ldwordf = -ldwordf;
            if (neg_hi & 2) udwordf = -udwordf;

            float dword2 = ldwordf * udwordf;

            uint32_t result1 = *reinterpret_cast<uint32_t*>(&dword1);
            uint32_t result2 = *reinterpret_cast<uint32_t*>(&dword2);

            vdst[lane] = (static_cast<uint64_t>(result2) << 32) | result1;
        }
    }

    vdst.write();
} // execute
// --- Inst_VOP3P__V_PK_ADD_F32 class methods ---

Inst_VOP3P__V_PK_ADD_F32::Inst_VOP3P__V_PK_ADD_F32(InFmt_VOP3P *iFmt)
    : Inst_VOP3P(iFmt, "v_pk_add_f32")
{
    setFlag(ALU);
} // Inst_VOP3P__V_PK_ADD_F32

Inst_VOP3P__V_PK_ADD_F32::~Inst_VOP3P__V_PK_ADD_F32()
{
} // ~Inst_VOP3P__V_PK_ADD_F32

// D.f[63:32] = S0.f[63:32] + S1.f[63:32] . D.f[31:0] = S0.f[31:0] +
//              S1.f[31:0]
void
Inst_VOP3P__V_PK_ADD_F32::execute(GPUDynInstPtr gpuDynInst)
{
    // This is a special case of packed instructions which operates on
    // 64-bit inputs/outputs and not 32-bit. U64 is used here as float
    // values cannot use bitwise operations. Consider the U64 to imply
    // untyped 64-bits of data.
    Wavefront *wf = gpuDynInst->wavefront();
    ConstVecOperandU64 src0(gpuDynInst, extData.SRC0);
    ConstVecOperandU64 src1(gpuDynInst, extData.SRC1);
    VecOperandU64 vdst(gpuDynInst, instData.VDST);

    src0.readSrc();
    src1.readSrc();

    panic_if(isSDWAInst(), "SDWA not supported for %s", _opcode);
    panic_if(isDPPInst(), "DPP not supported for %s", _opcode);

    int opsel = instData.OPSEL;
    int opsel_hi = extData.OPSEL_HI;

    int neg = extData.NEG;
    int neg_hi = instData.NEG_HI;

    for (int lane = 0; lane < NumVecElemPerVecReg; ++lane) {
        if (wf->execMask(lane)) {
            uint32_t lower_dword = (opsel & 1) ? bits(src0[lane], 63, 32)
                                               : bits(src0[lane], 31, 0);
            uint32_t upper_dword = (opsel & 2) ? bits(src1[lane], 63, 32)
                                               : bits(src1[lane], 31, 0);

            float ldwordf = *reinterpret_cast<float*>(&lower_dword);
            float udwordf = *reinterpret_cast<float*>(&upper_dword);

            if (neg & 1) ldwordf = -ldwordf;
            if (neg & 2) udwordf = -udwordf;

            float dword1 = ldwordf + udwordf;

            lower_dword = (opsel_hi & 1) ? bits(src0[lane], 63, 32)
                                         : bits(src0[lane], 31, 0);
            upper_dword = (opsel_hi & 2) ? bits(src1[lane], 63, 32)
                                         : bits(src1[lane], 31, 0);

            ldwordf = *reinterpret_cast<float*>(&lower_dword);
            udwordf = *reinterpret_cast<float*>(&upper_dword);

            if (neg_hi & 1) ldwordf = -ldwordf;
            if (neg_hi & 2) udwordf = -udwordf;

            float dword2 = ldwordf + udwordf;

            uint32_t result1 = *reinterpret_cast<uint32_t*>(&dword1);
            uint32_t result2 = *reinterpret_cast<uint32_t*>(&dword2);

            vdst[lane] = (static_cast<uint64_t>(result2) << 32) | result1;
        }
    }

    vdst.write();
} // execute
// --- Inst_VOP3P__V_PK_MOV_B32 class methods ---

Inst_VOP3P__V_PK_MOV_B32::Inst_VOP3P__V_PK_MOV_B32(InFmt_VOP3P *iFmt)
    : Inst_VOP3P(iFmt, "v_pk_mov_b32")
{
    setFlag(ALU);
} // Inst_VOP3P__V_PK_MOV_B32

Inst_VOP3P__V_PK_MOV_B32::~Inst_VOP3P__V_PK_MOV_B32()
{
} // ~Inst_VOP3P__V_PK_MOV_B32

// D.u[63:32] = S1.u[31:0]; D.u[31:0] = S0.u[31:0].
void
Inst_VOP3P__V_PK_MOV_B32::execute(GPUDynInstPtr gpuDynInst)
{
    // This is a special case of packed instructions which operates on
    // 64-bit inputs/outputs and not 32-bit.
    Wavefront *wf = gpuDynInst->wavefront();
    ConstVecOperandU64 src0(gpuDynInst, extData.SRC0);
    ConstVecOperandU64 src1(gpuDynInst, extData.SRC1);
    VecOperandU64 vdst(gpuDynInst, instData.VDST);

    src0.readSrc();
    src1.readSrc();

    // Only OPSEL[1:0] are used
    // OPSEL[0] 0/1: Lower dest dword = lower/upper dword of src0
    int opsel = instData.OPSEL;

    warn_if(instData.NEG_HI || extData.NEG,
            "Negative modifier undefined for %s", _opcode);

    for (int lane = 0; lane < NumVecElemPerVecReg; ++lane) {
        if (wf->execMask(lane)) {
            // OPSEL[1] 0/1: Lower dest dword = lower/upper dword of src1
            uint64_t lower_dword = (opsel & 1) ? bits(src0[lane], 63, 32)
                                               : bits(src0[lane], 31, 0);
            uint64_t upper_dword = (opsel & 2) ? bits(src1[lane], 63, 32)
                                               : bits(src1[lane], 31, 0);

            vdst[lane] = upper_dword << 32 | lower_dword;
        }
    }

    vdst.write();
} // execute

} // namespace VegaISA
} // namespace gem5
