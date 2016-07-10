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

      TEST_METHOD(TestResetWithPointerSetsPointerAndSetRefCountToOne)
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