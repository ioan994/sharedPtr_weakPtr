#include "stdafx.h"
#include "CppUnitTest.h"
#include "sharedPtr.h"

#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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

      TEST_METHOD(TestUseCountIsZeroForNull)
      {
         SharedPtr<int> sharedPtr;

         Assert::IsTrue(sharedPtr.UseCount() == 0);
      }

      TEST_METHOD(TestUseCountIsOneForNotNull)
      {
         SharedPtr<int> sharedPtr(new int);

         Assert::IsTrue(sharedPtr.UseCount() == 1);
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

      TEST_METHOD(TestCopyConstructCopyPointerIncreateRefCount)
      {
         SharedPtr<int> rhsSharedPtr(new int);

         SharedPtr<int> lhsSharedPtr = rhsSharedPtr;

         Assert::AreEqual(lhsSharedPtr.Get(), rhsSharedPtr.Get(), L"Objects store different pointers.");
         Assert::IsTrue(lhsSharedPtr.UseCount() == 2, L"Use count for left hand side pointer does not equal to two.");
         Assert::IsTrue(rhsSharedPtr.UseCount() == 2, L"Use count for right hand side pointer does not equal to two.");
      }

      TEST_METHOD(TestGoingOutOfScopeDecreasesRefCount)
      {
         SharedPtr<int> sharedPtr1(new int);

         {
            SharedPtr<int> sharedPtr2 = sharedPtr1;
         }

         Assert::IsTrue(sharedPtr1.UseCount() == 1);
      }

      TEST_METHOD(TestResetWithNullClearsPointerDecreaseRefCount)
      {
         SharedPtr<int> sharedPtr(new int);

         sharedPtr.Reset();

         Assert::IsTrue(sharedPtr.UseCount() == 0, L"Use count is not zero.");
         Assert::IsNull(sharedPtr.Get(), L"Poiner is not reset.");
      }

      TEST_METHOD(TestResetWithPointerSetsPointerAndSetRefCountToOne)
      {
         SharedPtr<int> sharedPtr(new int);
         int* pointer = new int;

         sharedPtr.Reset(pointer);

         Assert::IsTrue(sharedPtr.UseCount() == 1, L"Use count is not one.");
         Assert::AreEqual(pointer, sharedPtr.Get(), L"Poiner is not reset.");
      }
	};
}