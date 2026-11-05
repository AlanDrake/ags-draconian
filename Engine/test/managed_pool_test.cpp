//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// The managed object pool behind script pointers: an object gets a handle,
// lives while it has references and is disposed when the last one goes; an
// array of pointers releases what it points to (and leaves the disposal to the
// pool); handle 0 is null, and an unknown handle or address is an error rather
// than a crash. Collecting reference cycles needs typed objects from compiled
// scripts and is not tested here.
//
//=============================================================================
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/cc_serializer.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/managedobjectpool.h"
#include "ac/dynobj/scriptuserobject.h"
#include "gtest/gtest.h"
#include "script/cc_common.h"
#include "script/cc_reflect.h"
#include "util/memory.h"
#include "util/memorystream.h"
#include "util/stream.h"

using namespace AGS::Common;

// Fixture for the managed object pool. The engine globals it touches:
// - pool: objects are added and released by each test; anything a test leaves is removed in TearDown
// - the script error state (cc_get_error), cleared before and after each test
class ManagedPool : public ::testing::Test
{
protected:
    void SetUp() override { cc_clear_error(); }

    void TearDown() override
    {
        for (int handle : _created)
        {
            void *addr = pool.HandleToAddress(handle);
            if (addr)
                pool.RemoveObject(addr);
        }
        cc_clear_error();
    }

    DynObjectRef NewObject(size_t size = 8)
    {
        DynObjectRef ref = ScriptUserObject::Create(AGS::RTTI::NoType, size);
        _created.push_back(ref.Handle());
        return ref;
    }

    std::vector<int> _created;
};

TEST_F(ManagedPool, ObjectHasAHandleAndAnAddress)
{
    const DynObjectRef ref = NewObject();
    ASSERT_GT(ref.Handle(), 0);
    ASSERT_NE(nullptr, ref.Obj());
    EXPECT_EQ(ref.Obj(), pool.HandleToAddress(ref.Handle()));
    EXPECT_EQ(ref.Handle(), pool.AddressToHandle(ref.Obj()));
    EXPECT_EQ(ref.Mgr(), pool.AddressToManager(ref.Obj()));
    void *obj = nullptr;
    IScriptObject *mgr = nullptr;
    EXPECT_EQ(kScValScriptObject, pool.HandleToAddressAndManager(ref.Handle(), obj, mgr));
    EXPECT_EQ(ref.Obj(), obj);
    EXPECT_EQ(ref.Mgr(), mgr);
    // two objects never share a handle
    const DynObjectRef other = NewObject();
    EXPECT_NE(ref.Handle(), other.Handle());
}

TEST_F(ManagedPool, LastReleaseDisposesTheObject)
{
    const DynObjectRef ref = NewObject();
    EXPECT_EQ(1, ccAddObjectReference(ref.Handle()));
    EXPECT_EQ(2, ccAddObjectReference(ref.Handle()));
    ccReleaseObjectReference(ref.Handle());
    EXPECT_EQ(ref.Obj(), pool.HandleToAddress(ref.Handle()));
    ccReleaseObjectReference(ref.Handle());
    EXPECT_EQ(nullptr, pool.HandleToAddress(ref.Handle()));
    EXPECT_EQ(0, pool.AddressToHandle(ref.Obj()));
}

TEST_F(ManagedPool, ObjectBeingReturnedIsNotDisposed)
{
    // a function returning a new object releases its local reference before the caller takes one
    const DynObjectRef ref = NewObject();
    ccAddObjectReference(ref.Handle());
    pool.disableDisposeForObject = ref.Obj();
    ccReleaseObjectReference(ref.Handle());
    pool.disableDisposeForObject = nullptr;
    EXPECT_EQ(ref.Obj(), pool.HandleToAddress(ref.Handle()));
}

TEST_F(ManagedPool, HandleZeroIsNull)
{
    EXPECT_EQ(nullptr, ccGetObjectAddressFromHandle(0));
    EXPECT_EQ(0, ccGetObjectHandleFromAddress(nullptr));
    void *obj = this;
    IScriptObject *mgr = nullptr;
    EXPECT_EQ(kScValUndefined, ccGetObjectAddressAndManagerFromHandle(0, obj, mgr));
    EXPECT_EQ(nullptr, obj);
    EXPECT_FALSE(cc_has_error());
    EXPECT_EQ(0, ccAddObjectReference(0));
    EXPECT_EQ(0, ccReleaseObjectReference(0));
}

TEST_F(ManagedPool, UnknownHandleIsAnError)
{
    void *obj = nullptr;
    IScriptObject *mgr = nullptr;
    EXPECT_EQ(kScValUndefined, ccGetObjectAddressAndManagerFromHandle(987654, obj, mgr));
    ASSERT_TRUE(cc_has_error());
    EXPECT_NE(std::string::npos, std::string(cc_get_error().ErrorString.GetCStr()).find("invalid handle 987654"));
    EXPECT_EQ(nullptr, pool.HandleToAddress(987654));
    EXPECT_EQ(nullptr, pool.HandleToAddress(-5));
}

TEST_F(ManagedPool, AddressOutsideThePoolIsAnError)
{
    int local = 0;
    EXPECT_EQ(-1, ccGetObjectHandleFromAddress(&local));
    ASSERT_TRUE(cc_has_error());
    EXPECT_NE(std::string::npos,
              std::string(cc_get_error().ErrorString.GetCStr()).find("not in the managed object pool"));
}

TEST_F(ManagedPool, ArrayOfPointersReleasesItsElements)
{
    // int-sized slots that hold object handles, as script arrays of pointers do
    const DynObjectRef array = CCDynamicArray::CreateOld(2, sizeof(int32_t), true);
    _created.push_back(array.Handle());
    ASSERT_GT(array.Handle(), 0);
    const DynObjectRef first = NewObject();
    const DynObjectRef second = NewObject();
    auto *slots = static_cast<uint8_t *>(array.Obj());
    Memory::WriteInt32LE(slots, first.Handle());
    Memory::WriteInt32LE(slots + sizeof(int32_t), second.Handle());
    ccAddObjectReference(first.Handle());
    ccAddObjectReference(second.Handle());
    EXPECT_EQ(2u, CCDynamicArray::GetHeader(array.Obj()).ElemCount);

    ccAddObjectReference(array.Handle());
    ccReleaseObjectReference(array.Handle());
    EXPECT_EQ(nullptr, pool.HandleToAddress(array.Handle()));
    // the elements lose the array's references, but the array does not dispose them:
    // that is left to the pool's next check (or garbage collection)
    EXPECT_EQ(first.Obj(), pool.HandleToAddress(first.Handle()));
    pool.CheckDispose(first.Handle());
    pool.CheckDispose(second.Handle());
    EXPECT_EQ(nullptr, pool.HandleToAddress(first.Handle()));
    EXPECT_EQ(nullptr, pool.HandleToAddress(second.Handle()));
}

TEST_F(ManagedPool, ElementWithAnotherReferenceOutlivesTheArray)
{
    const DynObjectRef array = CCDynamicArray::CreateOld(1, sizeof(int32_t), true);
    _created.push_back(array.Handle());
    const DynObjectRef kept = NewObject();
    Memory::WriteInt32LE(array.Obj(), kept.Handle());
    ccAddObjectReference(kept.Handle()); // the array's
    ccAddObjectReference(kept.Handle()); // somebody else's

    ccAddObjectReference(array.Handle());
    ccReleaseObjectReference(array.Handle());
    EXPECT_EQ(nullptr, pool.HandleToAddress(array.Handle()));
    EXPECT_EQ(kept.Obj(), pool.HandleToAddress(kept.Handle()));
    ccReleaseObjectReference(kept.Handle());
    EXPECT_EQ(nullptr, pool.HandleToAddress(kept.Handle()));
}

TEST_F(ManagedPool, ArrayHeaderHasCountAndSize)
{
    const DynObjectRef array = CCDynamicArray::CreateOld(5, 12, false);
    _created.push_back(array.Handle());
    const auto &hdr = CCDynamicArray::GetHeader(array.Obj());
    EXPECT_EQ(5u, hdr.ElemCount);
    EXPECT_EQ(60u, hdr.TotalSize);
}

TEST_F(ManagedPool, RestoredStructKeepsItsDataAndTheSharedManager)
{
    const DynObjectRef ref = NewObject(2 * sizeof(int32_t));
    ref.Mgr()->WriteInt32(ref.Obj(), 0, 17);
    ref.Mgr()->WriteInt32(ref.Obj(), sizeof(int32_t), 29);
    ccAddObjectReference(ref.Handle());
    std::vector<uint8_t> saved;
    {
        Stream out(std::make_unique<VectorStream>(saved, kStream_Write));
        ccSerializeAllObjects(&out);
    }
    ccUnregisterAllObjects();
    ASSERT_EQ(nullptr, pool.HandleToAddress(ref.Handle()));

    AGSDeSerializer reader;
    Stream in(std::make_unique<VectorStream>(saved));
    ASSERT_EQ(0, ccUnserializeAllObjects(&in, &reader));
    void *restored = pool.HandleToAddress(ref.Handle());
    ASSERT_NE(nullptr, restored);
    // the manager of all structs is one global object, not one allocated for each restored struct
    EXPECT_EQ(&globalDynamicStruct, pool.AddressToManager(restored));
    EXPECT_EQ(17, globalDynamicStruct.ReadInt32(restored, 0));
    EXPECT_EQ(29, globalDynamicStruct.ReadInt32(restored, sizeof(int32_t)));
}
