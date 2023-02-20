
#pragma once

#include <assert.h>

using uint32 = unsigned int;

#define LOG2_BITS_PER_INT 5
#define BITS_PER_INT 32
#define BitVec_Bit( bitNum ) ( 1 << ( (bitNum) & (BITS_PER_INT-1) ) )
#define BitVec_Int( bitNum ) ( (bitNum) >> LOG2_BITS_PER_INT )

template <class BASE_OPS>
class CBitVecT : public BASE_OPS
{
public:
    uint32 Get(uint32 bitNum) const;
};

template <int NUM_BITS>
class CFixedBitVecBase
{
public:
    constexpr int GetNumBits() const { return NUM_BITS; }
    const uint32* Base() const { return m_Ints; }

    uint32 m_Ints[(NUM_BITS + (BITS_PER_INT - 1)) / BITS_PER_INT];
};

template <int NUM_BITS>
class CBitVec : public CBitVecT<CFixedBitVecBase<NUM_BITS>>
{
};

template <class BASE_OPS>
inline uint32 CBitVecT<BASE_OPS>::Get(uint32 bitNum) const
{
    assert(bitNum < (uint32)this->GetNumBits());
    const uint32* pInt = this->Base() + BitVec_Int(bitNum);
    return (*pInt & BitVec_Bit(bitNum));
}
