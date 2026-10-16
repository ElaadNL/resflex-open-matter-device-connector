// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app-common/zap-generated/cluster-enums.h"
#include "chip_support.h"
#include "lib/support/ScopedBuffer.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

namespace chip {
namespace TLV {

enum TLVTypeEnum : uint8_t {
    kTLVType_NotSpecified = 0,
    kTLVType_Array = 4,
    kTLVType_Structure = 5,
    kTLVType_List = 6,
};

class Tag {
public:
    static Tag AnonymousTag() { return Tag(true, 0); }
    static Tag ContextTag(uint8_t tag) { return Tag(false, tag); }

    bool IsAnonymous() const { return mAnonymous; }
    uint8_t GetContextTag() const { return mContextTag; }

    bool operator==(const Tag &other) const
    {
        return mAnonymous == other.mAnonymous && mContextTag == other.mContextTag;
    }

    bool operator==(uint8_t contextTag) const { return !mAnonymous && mContextTag == contextTag; }

private:
    Tag(bool anon, uint8_t ctx)
        : mAnonymous(anon)
        , mContextTag(ctx)
    {
    }

    bool mAnonymous = true;
    uint8_t mContextTag = 0;
};

using TLVType = uint32_t;

inline Tag AnonymousTag()
{
    return Tag::AnonymousTag();
}

inline Tag ContextTag(uint8_t tag)
{
    return Tag::ContextTag(tag);
}

template <typename Enum, typename = typename std::enable_if<std::is_enum<Enum>::value>::type>
inline Tag ContextTag(Enum tag)
{
    return Tag::ContextTag(static_cast<uint8_t>(tag));
}

namespace detail {

enum class ValueType : uint8_t {
    kEnd,
    kContainerEnd,
    kContainer,
    kUInt16,
    kUInt8,
    kInt64,
    kBitmask8,
};

struct Node {
    TLVTypeEnum containerType = kTLVType_NotSpecified;
    Tag tag = Tag::AnonymousTag();
    ValueType valueType = ValueType::kEnd;
    uint16_t u16 = 0;
    uint8_t u8 = 0;
    int64_t i64 = 0;
    std::vector<Node> children;
};

inline void WriteU16(std::vector<uint8_t> &out, uint16_t v)
{
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline void WriteU32(std::vector<uint8_t> &out, uint32_t v)
{
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

inline void WriteI64(std::vector<uint8_t> &out, int64_t v)
{
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((static_cast<uint64_t>(v) >> (8 * i)) & 0xFF));
    }
}

inline uint16_t ReadU16(const uint8_t *&p, const uint8_t *end)
{
    if (p + 2 > end) {
        return 0;
    }
    uint16_t v = static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
    p += 2;
    return v;
}

inline uint32_t ReadU32(const uint8_t *&p, const uint8_t *end)
{
    if (p + 4 > end) {
        return 0;
    }
    uint32_t v = static_cast<uint32_t>(p[0] | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
                                       (static_cast<uint32_t>(p[3]) << 24));
    p += 4;
    return v;
}

inline int64_t ReadI64(const uint8_t *&p, const uint8_t *end)
{
    if (p + 8 > end) {
        return 0;
    }
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<uint64_t>(p[i]) << (8 * i);
    }
    p += 8;
    return static_cast<int64_t>(v);
}

inline void SerializeNode(const Node &node, std::vector<uint8_t> &out)
{
    if (node.containerType != kTLVType_NotSpecified) {
        out.push_back(static_cast<uint8_t>(ValueType::kContainer));
        out.push_back(node.tag.IsAnonymous() ? 1 : 0);
        out.push_back(node.tag.GetContextTag());
        out.push_back(static_cast<uint8_t>(node.containerType));
        for (const auto &child : node.children) {
            SerializeNode(child, out);
        }
        out.push_back(static_cast<uint8_t>(ValueType::kContainerEnd));
        return;
    }

    out.push_back(static_cast<uint8_t>(node.valueType));
    out.push_back(node.tag.IsAnonymous() ? 1 : 0);
    out.push_back(node.tag.GetContextTag());
    out.push_back(0);

    switch (node.valueType) {
    case ValueType::kUInt16:
        WriteU16(out, node.u16);
        break;
    case ValueType::kUInt8:
    case ValueType::kBitmask8:
        out.push_back(node.u8);
        break;
    case ValueType::kInt64:
        WriteI64(out, node.i64);
        break;
    default:
        break;
    }
}

inline CHIP_ERROR DeserializeNodes(const uint8_t *&p, const uint8_t *end, std::vector<Node> &nodes)
{
    while (p < end) {
        auto vt = static_cast<ValueType>(*p++);
        if (vt == ValueType::kContainerEnd) {
            return CHIP_NO_ERROR;
        }
        if (p + 3 > end) {
            return CHIP_ERROR_INTERNAL;
        }
        Node node;
        bool anon = (*p++ != 0);
        uint8_t ctx = *p++;
        node.tag = anon ? Tag::AnonymousTag() : Tag::ContextTag(ctx);
        node.containerType = static_cast<TLVTypeEnum>(*p++);

        node.valueType = vt;
        switch (vt) {
        case ValueType::kContainer: {
            CHIP_ERROR childErr = DeserializeNodes(p, end, node.children);
            if (childErr != CHIP_NO_ERROR) {
                return childErr;
            }
        } break;
        case ValueType::kUInt16:
            node.u16 = ReadU16(p, end);
            break;
        case ValueType::kUInt8:
            if (p >= end) {
                return CHIP_ERROR_INTERNAL;
            }
            node.u8 = *p++;
            break;
        case ValueType::kBitmask8:
            if (p >= end) {
                return CHIP_ERROR_INTERNAL;
            }
            node.u8 = *p++;
            break;
        case ValueType::kInt64:
            node.i64 = ReadI64(p, end);
            break;
        default:
            return CHIP_ERROR_UNEXPECTED_TLV_ELEMENT;
        }
        nodes.push_back(std::move(node));
    }
    return CHIP_NO_ERROR;
}

} // namespace detail

class TLVReader {
public:
    void Init(const uint8_t *buf, uint16_t len)
    {
        mBuf = buf;
        mLen = len;
        mRoot.clear();
        mContainerStack.clear();
        mIndexStack.clear();
        if (buf != nullptr && len > 0) {
            const uint8_t *p = buf;
            const uint8_t *end = buf + len;
            detail::DeserializeNodes(p, end, mRoot);
        }
        mContainerStack.push_back(&mRoot);
        mIndexStack.push_back(0);
    }

    CHIP_ERROR Next(Tag tag) { return Next(kTLVType_NotSpecified, tag); }

    CHIP_ERROR Next(TLVTypeEnum type = kTLVType_NotSpecified, Tag tag = Tag::AnonymousTag())
    {
        auto *container = CurrentContainer();
        if (container == nullptr) {
            return CHIP_ERROR_UNEXPECTED_TLV_ELEMENT;
        }
        size_t &idx = mIndexStack.back();
        if (idx >= container->size()) {
            return CHIP_END_OF_TLV;
        }
        const auto &node = (*container)[idx++];
        if (type != kTLVType_NotSpecified && node.containerType != type) {
            return CHIP_ERROR_UNEXPECTED_TLV_ELEMENT;
        }
        // AnonymousTag + NotSpecified type means "next element" (Matter TLVReader::Next()).
        const bool anyTag = tag.IsAnonymous() && type == kTLVType_NotSpecified;
        if (!anyTag) {
            if (tag.IsAnonymous()) {
                if (!node.tag.IsAnonymous()) {
                    return CHIP_ERROR_UNEXPECTED_TLV_ELEMENT;
                }
            } else if (node.tag.IsAnonymous() || node.tag.GetContextTag() != tag.GetContextTag()) {
                return CHIP_ERROR_UNEXPECTED_TLV_ELEMENT;
            }
        }
        mCurrentNode = &(*container)[idx - 1];
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR EnterContainer(TLVType &containerType)
    {
        if (mCurrentNode == nullptr || mCurrentNode->containerType == kTLVType_NotSpecified) {
            return CHIP_ERROR_INCORRECT_STATE;
        }
        containerType = mCurrentNode->containerType;
        mContainerStack.push_back(const_cast<std::vector<detail::Node> *>(&mCurrentNode->children));
        mIndexStack.push_back(0);
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR ExitContainer(TLVType &)
    {
        if (mContainerStack.size() <= 1) {
            return CHIP_ERROR_INCORRECT_STATE;
        }
        mContainerStack.pop_back();
        mIndexStack.pop_back();
        return CHIP_NO_ERROR;
    }

    TLVTypeEnum GetType() const
    {
        if (mCurrentNode == nullptr) {
            return kTLVType_NotSpecified;
        }
        if (mCurrentNode->containerType != kTLVType_NotSpecified) {
            return mCurrentNode->containerType;
        }
        // Scalar elements are encoded without a container type; match Matter TLVReader behavior
        // so decoders that reject kTLVType_NotSpecified can read context-tagged fields.
        if (mCurrentNode->valueType != detail::ValueType::kEnd &&
            mCurrentNode->valueType != detail::ValueType::kContainerEnd) {
            return kTLVType_Structure;
        }
        return kTLVType_NotSpecified;
    }

    Tag GetTag() const { return mCurrentNode == nullptr ? Tag::AnonymousTag() : mCurrentNode->tag; }

    template <typename T>
    CHIP_ERROR Get(T &value)
    {
        if (mCurrentNode == nullptr) {
            return CHIP_ERROR_INCORRECT_STATE;
        }
        if constexpr (std::is_same_v<T, uint16_t>) {
            value = mCurrentNode->u16;
            return CHIP_NO_ERROR;
        }
        if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, Percent>) {
            value = static_cast<T>(mCurrentNode->u8);
            return CHIP_NO_ERROR;
        }
        if constexpr (std::is_same_v<T, int64_t>) {
            value = mCurrentNode->i64;
            return CHIP_NO_ERROR;
        }
        if constexpr (std::is_same_v<T, BitMask<app::Clusters::EnergyEvse::TargetDayOfWeekBitmap>>) {
            value = BitMask<app::Clusters::EnergyEvse::TargetDayOfWeekBitmap>(static_cast<uint8_t>(mCurrentNode->u8));
            return CHIP_NO_ERROR;
        }
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    CHIP_ERROR VerifyEndOfContainer()
    {
        auto *container = CurrentContainer();
        if (container == nullptr) {
            return CHIP_ERROR_INCORRECT_STATE;
        }
        if (mIndexStack.back() != container->size()) {
            return CHIP_ERROR_INCORRECT_STATE;
        }
        return CHIP_NO_ERROR;
    }

private:
    std::vector<detail::Node> *CurrentContainer() { return mContainerStack.empty() ? nullptr : mContainerStack.back(); }

    const uint8_t *mBuf = nullptr;
    uint16_t mLen = 0;
    std::vector<detail::Node> mRoot;
    const detail::Node *mCurrentNode = nullptr;
    std::vector<std::vector<detail::Node> *> mContainerStack;
    std::vector<size_t> mIndexStack;
};

class TLVWriter {
public:
    CHIP_ERROR StartContainer(Tag tag, TLVTypeEnum type, TLVType &containerType)
    {
        detail::Node node;
        node.tag = tag;
        node.containerType = type;
        CurrentChildren()->push_back(std::move(node));
        mStack.push_back(&CurrentChildren()->back().children);
        containerType = type;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR EndContainer(TLVType &)
    {
        if (mStack.size() <= 1) {
            return CHIP_ERROR_INCORRECT_STATE;
        }
        mStack.pop_back();
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR Put(Tag tag, uint16_t value)
    {
        detail::Node node;
        node.tag = tag;
        node.valueType = detail::ValueType::kUInt16;
        node.u16 = value;
        CurrentChildren()->push_back(std::move(node));
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR Put(Tag tag, uint8_t value)
    {
        detail::Node node;
        node.tag = tag;
        node.valueType = detail::ValueType::kUInt8;
        node.u8 = value;
        CurrentChildren()->push_back(std::move(node));
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR Put(Tag tag, int64_t value)
    {
        detail::Node node;
        node.tag = tag;
        node.valueType = detail::ValueType::kInt64;
        node.i64 = value;
        CurrentChildren()->push_back(std::move(node));
        return CHIP_NO_ERROR;
    }

    template <typename Enum, typename Storage>
    CHIP_ERROR Put(Tag tag, BitMask<Enum, Storage> value)
    {
        detail::Node node;
        node.tag = tag;
        node.valueType = detail::ValueType::kBitmask8;
        node.u8 = static_cast<uint8_t>(value.Raw());
        CurrentChildren()->push_back(std::move(node));
        return CHIP_NO_ERROR;
    }

    size_t GetLengthWritten() const { return mSerialized.size(); }

    CHIP_ERROR Finalize(ScopedMemoryBuffer<uint8_t> &out)
    {
        mSerialized.clear();
        for (const auto &node : mRoot) {
            detail::SerializeNode(node, mSerialized);
        }
        if (!out.Calloc(mSerialized.size())) {
            return CHIP_ERROR_NO_MEMORY;
        }
        std::memcpy(out.Get(), mSerialized.data(), mSerialized.size());
        return CHIP_NO_ERROR;
    }

private:
    std::vector<detail::Node> *CurrentChildren() { return mStack.empty() ? &mRoot : mStack.back(); }

    std::vector<detail::Node> mRoot;
    std::vector<std::vector<detail::Node> *> mStack{&mRoot};
    std::vector<uint8_t> mSerialized;
};

class ScopedBufferTLVWriter : public TLVWriter {
public:
    ScopedBufferTLVWriter(ScopedMemoryBuffer<uint8_t> &&buffer, uint16_t)
        : mBuffer(std::move(buffer))
    {
    }
    ScopedBufferTLVWriter(ScopedMemoryBuffer<uint8_t> &&buffer, size_t)
        : mBuffer(std::move(buffer))
    {
    }

private:
    ScopedMemoryBuffer<uint8_t> mBuffer;
};

class ScopedBufferTLVReader : public TLVReader {
public:
    ScopedBufferTLVReader(ScopedMemoryBuffer<uint8_t> &&buffer, uint16_t len)
        : mBuffer(std::move(buffer))
    {
        Init(mBuffer.Get(), len);
    }

private:
    ScopedMemoryBuffer<uint8_t> mBuffer;
};

inline size_t EstimateStructOverhead()
{
    return 32;
}

template <typename... Args>
inline size_t EstimateStructOverhead(Args...)
{
    return 32;
}

} // namespace TLV
} // namespace chip
