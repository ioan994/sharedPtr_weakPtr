#include "stdafx.h"
#include "CppUnitTest.h"
#include "sharedPtr.h"

#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace
{
   struct DummyWithDestructor
   {
      DummyWithDestructor(bool& i_destructorCalled) : m_destructorCalled(i_destructorCalled)
      {
      }

      ~DummyWithDestructor()
      {
         m_destructorCalled = true;
      }

      bool& m_destructorCalled;
   };
}

namespace test
{		
	TEST_CLASS(SharedPtrTests)
	{
	public:
		
		TEST_METHOD(TestSharedPtrStorePointer)
		{
         int* ptr = new int;
         SharedPtr<int> sharedPtr(ptr);

         Assert::AreEqual(ptr, sharedPtr.Get());
		}

      TEST_METHOD(TestSharedPtrInitWithNull)
      {
         SharedPtr<int> sharedPtr;

         Assert::IsNull(sharedPtr.Get());
      }

      TEST_METHOD(TestUseCountIsZeroForNullOneForNotNull)
      {
         SharedPtr<int> sharedPtrNull;
         SharedPtr<int> sharedPtrNotNull(new int);

         Assert::IsTrue(sharedPtrNull.UseCount() == 0, L"Use count is not zero.");
         Assert::IsTrue(sharedPtrNotNull.UseCount() == 1, L"Use count is not one");
      }

      TEST_METHOD(TestAssignCopyPointerIncreaseUseCount)
      {
         SharedPtr<int> rhsSharedPtr(new int);
         SharedPtr<int> lhsSharedPtr;

         lhsSharedPtr = rhsSharedPtr;

         Assert::AreEqual(lhsSharedPtr.Get(), rhsSharedPtr.Get(), L"Objects store different pointers.");
         Assert::IsTrue(lhsSharedPtr.UseCount() == 2, L"Use count for left hand side pointer does not equal to two.");
         Assert::IsTrue(rhsSharedPtr.UseCount() == 2, L"Use count for right hand side pointer does not equal to two.");
      }

      TEST_METHOD(TestAssignWithNullDoesNotIncreaseUseCount)
      {
         SharedPtr<int> rhsSharedPtr;
         SharedPtr<int> lhsSharedPtr;

         lhsSharedPtr = rhsSharedPtr;

         Assert::IsTrue(lhsSharedPtr.UseCount() == 0, L"Use count for left hand side pointer does not equal to zero.");
         Assert::IsTrue(rhsSharedPtr.UseCount() == 0, L"Use count for right hand side pointer does not equal to zero.");
      }

      TEST_METHOD(TestSelfAssignDoesNotIncreaseUseCount)
      {
         SharedPtr<int> sharedPtr(new int);

         sharedPtr = sharedPtr;

         Assert::IsTrue(sharedPtr.UseCount() == 1);
      }

      TEST_METHOD(TestCopyConstructCopyPointerIncreaseUseCount)
      {
         SharedPtr<int> rhsSharedPtr(new int);

         SharedPtr<int> lhsSharedPtr = rhsSharedPtr;

         Assert::AreEqual(lhsSharedPtr.Get(), rhsSharedPtr.Get(), L"Objects store different pointers.");
         Assert::IsTrue(lhsSharedPtr.UseCount() == 2, L"Use count for left hand side pointer does not equal to two.");
         Assert::IsTrue(rhsSharedPtr.UseCount() == 2, L"Use count for right hand side pointer does not equal to two.");
      }

      TEST_METHOD(TestMoveConstruct)
      {
         bool destructorCalled = false;
         DummyWithDestructor* dummy = new DummyWithDestructor(destructorCalled);
         SharedPtr<DummyWithDestructor> rhsSharedPtr(dummy);

         auto lhsSharedPtr = std::move(rhsSharedPtr);

         Assert::IsNull(rhsSharedPtr.Get(), L"Right hand side object holds pointer.");
         Assert::IsTrue(rhsSharedPtr.UseCount() == 0, L"Use count for right hand side object is not zero.");
         Assert::IsTrue(lhsSharedPtr.Get() == dummy, L"Pointer was not transferred correctly.");
         Assert::IsTrue(lhsSharedPtr.UseCount() == 1, L"Left hand side object has incorrect use count value.");
         Assert::IsFalse(destructorCalled, L"Destructor was called.");
      }

      TEST_METHOD(TestMoveAssign)
      {
         // Shared 1 and shared 2 share the same pointer. Shared 3 holds different pointer.
         bool destructorCalled1 = false;
         auto shared1 = MakeShared<DummyWithDestructor>(destructorCalled1);
         auto shared2 = shared1;
         bool destructorCalled3 = false;
         auto dummy3 = new DummyWithDestructor(destructorCalled3);
         SharedPtr<DummyWithDestructor> shared3(dummy3);

         // Pointer from shared 3 object moves to shared 2. Causes decreasing of use count for shared 1.
         shared2 = std::move(shared3);

         Assert::IsTrue(shared1.UseCount() == 1, L"Use count for shared1 is not one.");
         Assert::IsTrue(shared2.Get() == dummy3, L"Pointer was not moved from shared3 to shared2.");
         Assert::IsTrue(shared2.UseCount() == 1, L"Use count for shared2 is not one.");
         Assert::IsNull(shared3.Get(), L"shared3 still holds pointer.");
         Assert::IsTrue(shared3.UseCount() == 0, L"Use count for shared3 is not zero.");
         Assert::IsFalse(destructorCalled1, L"Destructor was called for pointer stored in shared1 and shared2.");
         Assert::IsFalse(destructorCalled3, L"Destructor was called for pointer stored in shared3.");
      }

      TEST_METHOD(TestSelfMoveDoesNothing)
      {
         bool destructorCalled = false;
         auto shared = MakeShared<DummyWithDestructor>(destructorCalled);

         shared = std::move(shared);

         Assert::IsTrue(shared.UseCount(), L"Use count is not one.");
         Assert::IsFalse(destructorCalled);
      }

      TEST_METHOD(TestMemberAccesss)
      {
         using BoolInt = std::pair<bool, int>;
         BoolInt testPair(true, 42);
         SharedPtr<BoolInt> sharedPtr(new BoolInt(testPair));

         Assert::IsTrue(sharedPtr->first == testPair.first);
         Assert::IsTrue(sharedPtr->second == testPair.second);
      }

      TEST_METHOD(TestMakeShared)
      {
         using BoolInt = std::pair<bool, int>;
         const bool boolParam = true;
         const int intParam = 42;

         auto sharedPtr = MakeShared<BoolInt>(boolParam, intParam);

         Assert::IsTrue(sharedPtr->first == boolParam);
         Assert::IsTrue(sharedPtr->second == intParam);
      }

      TEST_METHOD(TestGoingOutOfScopeDecreasesRefCount)
      {
         bool destructorCalled = false;
         auto sharedPtr1 = MakeShared<DummyWithDestructor>(destructorCalled);

         {
            auto sharedPtr2 = sharedPtr1;
         }

         Assert::IsTrue(sharedPtr1.UseCount() == 1, L"Use count is not one.");
         Assert::IsFalse(destructorCalled, L"Destructor was called.");
      }

      TEST_METHOD(TestResetWithPointerSetsPointerAndSetRefCountToOneForNewPointer)
      {
         bool destructorCalledShared1 = false;
         auto sharedPtr1 = MakeShared<DummyWithDestructor>(destructorCalledShared1);
         auto sharedPtr2 = sharedPtr1;
         bool unused = false;
         DummyWithDestructor* newDummy = new DummyWithDestructor(unused);

         sharedPtr2.Reset(newDummy);

         Assert::IsTrue(sharedPtr1.UseCount() == 1, L"Use count for first object is not one.");
         Assert::IsTrue(sharedPtr2.UseCount() == 1, L"Use count for second object is not one.");
         Assert::IsTrue(newDummy == sharedPtr2.Get(), L"Poiner is not reset.");
         Assert::IsFalse(destructorCalledShared1, L"Destructor was called.");
      }

      TEST_METHOD(TestAssignDecreaseUseCountForPreviousPointer)
      {
         auto sharedPtr1 = MakeShared<int>(42);
         auto sharedPtr2 = sharedPtr1;
         auto sharedPtr3 = MakeShared<int>(0);
         
         sharedPtr2 = sharedPtr3;

         Assert::IsTrue(sharedPtr1.UseCount() == 1, L"Use count for firs object is not one.");
         Assert::IsTrue(sharedPtr2.UseCount() == 2, L"Use count for second object is not two.");
      }

      TEST_METHOD(TestAssignCausesDestructorCallWhenUseCountReachesZero)
      {
         bool destructorCalled = false;
         bool unused;
         auto lhsSharedPtr = MakeShared<DummyWithDestructor>(destructorCalled);
         auto rhsSharedPtr = MakeShared<DummyWithDestructor>(unused);
         
         lhsSharedPtr = rhsSharedPtr;

         Assert::IsTrue(destructorCalled);
      }

      TEST_METHOD(TestMoveAssignCausesDestructorCallWhenUseCountReachesZero)
      {
         bool destructorCalled = false;
         bool unused;
         auto lhsSharedPtr = MakeShared<DummyWithDestructor>(destructorCalled);

         lhsSharedPtr = MakeShared<DummyWithDestructor>(unused);

         Assert::IsTrue(destructorCalled);
      }

      TEST_METHOD(TestGoingOutOfScopeCallsDestructorWhenUseCountReachesZero)
      {
         bool destructorCalled = false;

         {
            auto sharedPtr1 = MakeShared<DummyWithDestructor>(destructorCalled);
            auto sharedPtr2 = sharedPtr1;
         }

         Assert::IsTrue(destructorCalled);
      }

      TEST_METHOD(TestResetWithNullDropUseCountToZeroAndCallsDestructor)
      {
         bool destructorCalled = false;
         auto sharedPtr1 = MakeShared<DummyWithDestructor>(destructorCalled);
         auto sharedPtr2 = sharedPtr1;

         sharedPtr1.Reset();
         sharedPtr2.Reset();

         Assert::IsTrue(sharedPtr1.UseCount() == 0, L"Use count is not zero for first sharedPtr.");
         Assert::IsTrue(sharedPtr2.UseCount() == 0, L"Use count is not zero for second sharedPtr.");
         Assert::IsTrue(destructorCalled, L"Destructor was not called.");
      }

	};
}