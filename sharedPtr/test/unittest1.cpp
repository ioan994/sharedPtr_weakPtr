#include "stdafx.h"
#include "CppUnitTest.h"
#include "sharedPtr.h"

#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace
{
   struct dummy
   {
      virtual ~dummy() = default;
   };

   struct dummy_with_destructor : public dummy
   {
      dummy_with_destructor(bool& i_destructorCalled) : m_destructorCalled(i_destructorCalled)
      {
      }

      ~dummy_with_destructor()
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
         shared_ptr<int> shared_ptr(ptr);

         Assert::AreEqual(ptr, shared_ptr.get());
		}

      TEST_METHOD(TestSharedPtrInitWithNull)
      {
         shared_ptr<int> shared_ptr;

         Assert::IsNull(shared_ptr.get());
      }

      TEST_METHOD(TestUseCountIsZeroForNullOneForNotNull)
      {
         shared_ptr<int> sharedPtrNull;
         shared_ptr<int> sharedPtrNotNull(new int);

         Assert::IsTrue(sharedPtrNull.use_count() == 0, L"Use count is not zero.");
         Assert::IsTrue(sharedPtrNotNull.use_count() == 1, L"Use count is not one");
      }

      TEST_METHOD(TestAssignCopyPointerIncreaseUseCount)
      {
         shared_ptr<int> rhsSharedPtr(new int);
         shared_ptr<int> lhsSharedPtr;

         lhsSharedPtr = rhsSharedPtr;

         Assert::AreEqual(lhsSharedPtr.get(), rhsSharedPtr.get(), L"Objects store different pointers.");
         Assert::IsTrue(lhsSharedPtr.use_count() == 2, L"Use count for left hand side pointer does not equal to two.");
         Assert::IsTrue(rhsSharedPtr.use_count() == 2, L"Use count for right hand side pointer does not equal to two.");
      }

      TEST_METHOD(TestAssignWithNullDoesNotIncreaseUseCount)
      {
         shared_ptr<int> rhsSharedPtr;
         shared_ptr<int> lhsSharedPtr;

         lhsSharedPtr = rhsSharedPtr;

         Assert::IsTrue(lhsSharedPtr.use_count() == 0, L"Use count for left hand side pointer does not equal to zero.");
         Assert::IsTrue(rhsSharedPtr.use_count() == 0, L"Use count for right hand side pointer does not equal to zero.");
      }

      TEST_METHOD(TestSelfAssignDoesNotIncreaseUseCount)
      {
         shared_ptr<int> sharedPtr(new int);

         sharedPtr = sharedPtr;

         Assert::IsTrue(sharedPtr.use_count() == 1);
      }

      TEST_METHOD(TestCopyConstructCopyPointerIncreaseUseCount)
      {
         shared_ptr<int> rhsSharedPtr(new int);

         shared_ptr<int> lhsSharedPtr = rhsSharedPtr;

         Assert::AreEqual(lhsSharedPtr.get(), rhsSharedPtr.get(), L"Objects store different pointers.");
         Assert::IsTrue(lhsSharedPtr.use_count() == 2, L"Use count for left hand side pointer does not equal to two.");
         Assert::IsTrue(rhsSharedPtr.use_count() == 2, L"Use count for right hand side pointer does not equal to two.");
      }

      TEST_METHOD(TestMoveConstruct)
      {
         bool destructorCalled = false;
         dummy_with_destructor* dummy = new dummy_with_destructor(destructorCalled);
         shared_ptr<dummy_with_destructor> rhsSharedPtr(dummy);

         auto lhsSharedPtr = std::move(rhsSharedPtr);

         Assert::IsNull(rhsSharedPtr.get(), L"Right hand side object holds pointer.");
         Assert::IsTrue(rhsSharedPtr.use_count() == 0, L"Use count for right hand side object is not zero.");
         Assert::IsTrue(lhsSharedPtr.get() == dummy, L"Pointer was not transferred correctly.");
         Assert::IsTrue(lhsSharedPtr.use_count() == 1, L"Left hand side object has incorrect use count value.");
         Assert::IsFalse(destructorCalled, L"Destructor was called.");
      }

      TEST_METHOD(TestMoveAssign)
      {
         // Shared 1 and shared 2 share the same pointer. Shared 3 holds different pointer.
         bool destructorCalled1 = false;
         auto shared1 = make_shared<dummy_with_destructor>(destructorCalled1);
         auto shared2 = shared1;
         bool destructorCalled3 = false;
         auto dummy3 = new dummy_with_destructor(destructorCalled3);
         shared_ptr<dummy_with_destructor> shared3(dummy3);

         // Pointer from shared 3 object moves to shared 2. Causes decreasing of use count for shared 1.
         shared2 = std::move(shared3);

         Assert::IsTrue(shared1.use_count() == 1, L"Use count for shared1 is not one.");
         Assert::IsTrue(shared2.get() == dummy3, L"Pointer was not moved from shared3 to shared2.");
         Assert::IsTrue(shared2.use_count() == 1, L"Use count for shared2 is not one.");
         Assert::IsNull(shared3.get(), L"shared3 still holds pointer.");
         Assert::IsTrue(shared3.use_count() == 0, L"Use count for shared3 is not zero.");
         Assert::IsFalse(destructorCalled1, L"Destructor was called for pointer stored in shared1 and shared2.");
         Assert::IsFalse(destructorCalled3, L"Destructor was called for pointer stored in shared3.");
      }

      TEST_METHOD(TestSelfMoveDoesNothing)
      {
         bool destructorCalled = false;
         auto shared = make_shared<dummy_with_destructor>(destructorCalled);

         shared = std::move(shared);

         Assert::IsTrue(shared.use_count() == 1, L"Use count is not one.");
         Assert::IsFalse(destructorCalled, L"Destructor was called.");
      }

      TEST_METHOD(TestMemberAccesss)
      {
         using BoolInt = std::pair<bool, int>;
         BoolInt testPair(true, 42);
         shared_ptr<BoolInt> sharedPtr(new BoolInt(testPair));

         Assert::IsTrue(sharedPtr->first == testPair.first);
         Assert::IsTrue(sharedPtr->second == testPair.second);
      }

      TEST_METHOD(TestMakeShared)
      {
         using BoolInt = std::pair<bool, int>;
         const bool boolParam = true;
         const int intParam = 42;

         auto sharedPtr = make_shared<BoolInt>(boolParam, intParam);

         Assert::IsTrue(sharedPtr->first == boolParam);
         Assert::IsTrue(sharedPtr->second == intParam);
      }

      TEST_METHOD(TestGoingOutOfScopeDecreasesRefCount)
      {
         bool destructorCalled = false;
         auto sharedPtr1 = make_shared<dummy_with_destructor>(destructorCalled);

         {
            auto sharedPtr2 = sharedPtr1;
         }

         Assert::IsTrue(sharedPtr1.use_count() == 1, L"Use count is not one.");
         Assert::IsFalse(destructorCalled, L"Destructor was called.");
      }

      TEST_METHOD(TestresetWithPointerSetsPointerAndSetRefCountToOneForNewPointer)
      {
         bool destructorCalledShared1 = false;
         auto sharedPtr1 = make_shared<dummy_with_destructor>(destructorCalledShared1);
         auto sharedPtr2 = sharedPtr1;
         bool unused = false;
         dummy_with_destructor* newdummy = new dummy_with_destructor(unused);

         sharedPtr2.reset(newdummy);

         Assert::IsTrue(sharedPtr1.use_count() == 1, L"Use count for first object is not one.");
         Assert::IsTrue(sharedPtr2.use_count() == 1, L"Use count for second object is not one.");
         Assert::IsTrue(newdummy == sharedPtr2.get(), L"Poiner is not reset.");
         Assert::IsFalse(destructorCalledShared1, L"Destructor was called.");
      }

      TEST_METHOD(TestAssignDecreaseUseCountForPreviousPointer)
      {
         auto sharedPtr1 = make_shared<int>(42);
         auto sharedPtr2 = sharedPtr1;
         auto sharedPtr3 = make_shared<int>(0);
         
         sharedPtr2 = sharedPtr3;

         Assert::IsTrue(sharedPtr1.use_count() == 1, L"Use count for firs object is not one.");
         Assert::IsTrue(sharedPtr2.use_count() == 2, L"Use count for second object is not two.");
      }

      TEST_METHOD(TestAssignCausesDestructorCallWhenUseCountReachesZero)
      {
         bool destructorCalled = false;
         bool unused;
         auto lhsSharedPtr = make_shared<dummy_with_destructor>(destructorCalled);
         auto rhsSharedPtr = make_shared<dummy_with_destructor>(unused);
         
         lhsSharedPtr = rhsSharedPtr;

         Assert::IsTrue(destructorCalled);
      }

      TEST_METHOD(TestMoveAssignCausesDestructorCallWhenUseCountReachesZero)
      {
         bool destructorCalled = false;
         bool unused;
         auto lhsSharedPtr = make_shared<dummy_with_destructor>(destructorCalled);

         lhsSharedPtr = make_shared<dummy_with_destructor>(unused);

         Assert::IsTrue(destructorCalled);
      }

      TEST_METHOD(TestGoingOutOfScopeCallsDestructorWhenUseCountReachesZero)
      {
         bool destructorCalled = false;

         {
            auto sharedPtr1 = make_shared<dummy_with_destructor>(destructorCalled);
            auto sharedPtr2 = sharedPtr1;
         }

         Assert::IsTrue(destructorCalled);
      }

      TEST_METHOD(TestresetWithNullDropUseCountToZeroAndCallsDestructor)
      {
         bool destructorCalled = false;
         auto sharedPtr1 = make_shared<dummy_with_destructor>(destructorCalled);
         auto sharedPtr2 = sharedPtr1;

         sharedPtr1.reset();
         sharedPtr2.reset();

         Assert::IsTrue(sharedPtr1.use_count() == 0, L"Use count is not zero for first shared_ptr.");
         Assert::IsTrue(sharedPtr2.use_count() == 0, L"Use count is not zero for second shared_ptr.");
         Assert::IsTrue(destructorCalled, L"Destructor was not called.");
      }

      TEST_METHOD(TestCanBeUsedWithDerivedClassPointer)
      {
         bool destructorCalled = false;

         {
            shared_ptr<dummy> sharedPtr(new dummy_with_destructor(destructorCalled));
         }

         Assert::IsTrue(destructorCalled);
      }

      TEST_METHOD(TestMoveConstructructedSharesOwnershipWithSharedPtrWithDerivedClassPointer)
      {
         bool unused = false;
         shared_ptr<dummy_with_destructor> sharedPtr = make_shared<dummy_with_destructor>(unused);
         shared_ptr<dummy> sharedPtr2 = sharedPtr;

         Assert::IsTrue(sharedPtr.use_count() == 2);
         Assert::IsTrue(sharedPtr2.use_count() == 2);
         Assert::IsTrue(sharedPtr.get() == sharedPtr2.get());
      }

      TEST_METHOD(TestCopyConstructructedSharesOwnershipWithSharedPtrWithDerivedClassPointer)
      {
         bool unused = false;
         shared_ptr<dummy_with_destructor> sharedPtr = make_shared<dummy_with_destructor>(unused);
         shared_ptr<dummy> sharedPtr2;

         sharedPtr2 = sharedPtr;

         Assert::IsTrue(sharedPtr.use_count() == 2);
         Assert::IsTrue(sharedPtr2.use_count() == 2);
         Assert::IsTrue(sharedPtr.get() == sharedPtr2.get());
      }

      TEST_METHOD(TestSwap)
      {
         int* ptr1 = new int;
         int* ptr2 = new int;
         shared_ptr<int> shared1(ptr1);
         auto shared2 = shared1;
         shared_ptr<int> shared3(ptr2);

         shared2.swap(shared3);

         Assert::IsTrue(shared2.get() == ptr2);
         Assert::IsTrue(shared3.get() == ptr1);
         Assert::IsTrue(shared2.use_count() == 1);
         Assert::IsTrue(shared3.use_count() == 2);
      }

      TEST_METHOD(TestBoolConversion)
      {
         shared_ptr<int> empty;
         shared_ptr<int> notEmpry(new int);

         Assert::IsFalse(static_cast<bool>(empty));
         Assert::IsTrue(static_cast<bool>(notEmpry));
      }

      TEST_METHOD(TestUnique)
      {
         auto shared1 = make_shared<int>(0);
         auto shared2 = shared1;

         auto sharedUnique = make_shared<int>(0);

         Assert::IsFalse(shared1.unique());
         Assert::IsTrue(sharedUnique.unique());
      }

      TEST_METHOD(TestAliasingConstructor)
      {
         int value;
         auto shared = make_shared<int>(0);
         shared_ptr<int> aliasedConstructed(shared, &value);

         Assert::IsTrue(shared.use_count() == 2);
         Assert::IsTrue(aliasedConstructed.use_count() == 2);
         Assert::IsTrue(aliasedConstructed.get() == &value);
      }

      TEST_METHOD(TestAliasingConstructorGoesOutOfScope)
      {
         bool destructorCalled = false;
         auto sharedPtr = make_shared<dummy_with_destructor>(destructorCalled);
         dummy dumm;
         shared_ptr<dummy> aliasedConstructed(sharedPtr, &dumm);
         sharedPtr.reset();

         aliasedConstructed.reset();

         Assert::IsTrue(destructorCalled);
      }

	};
}