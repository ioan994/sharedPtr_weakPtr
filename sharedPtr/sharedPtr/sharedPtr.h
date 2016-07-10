#pragma once;

template <class T>
class SharedPtr
{
public:

   explicit SharedPtr(T* i_pointer = nullptr)
   {
      internalReset(i_pointer, nullptr);
   }

   SharedPtr(const SharedPtr& i_other)
   {
      internalReset(i_other.m_pointer, i_other.m_refCount);
   }

   SharedPtr(SharedPtr&& i_other)
   {
      *this = std::move(i_other);
   }

   ~SharedPtr()
   {
      removeRef();
   }

   SharedPtr& operator = (const SharedPtr& i_other)
   {
      if (this != &i_other)
      {
         internalReset(i_other.m_pointer, i_other.m_refCount);
      }
      return *this;
   }

   SharedPtr& operator = (SharedPtr&& i_other)
   {
      if (this != &i_other)
      {
         removeRef();
         setPointers(i_other.m_pointer, i_other.m_refCount);
         i_other.setPointers(nullptr, nullptr);
      }
      return *this;
   }

   T* Get() const
   {
      return m_pointer;
   }

   long UseCount() const
   {
      return m_refCount ? *m_refCount : 0;
   }

   void Reset(T* i_pointer = nullptr)
   {
      internalReset(i_pointer, nullptr);
   }

   T* operator ->()
   {
      return m_pointer;
   }

private:
   void addRef()
   {
      if (!m_refCount) m_refCount = new long(0);
      ++*m_refCount;
   }

   void removeRef()
   {
      if (m_refCount) --*m_refCount;
      if (UseCount() == 0)
      {
         delete m_refCount;
         delete m_pointer;
      }
   }

   void internalReset(T* i_pointer, long* i_refCount)
   {
      removeRef();
      setPointers(i_pointer, i_refCount);
      if (m_pointer)addRef();
   }

   void setPointers(T* i_pointer, long* i_refCount)
   {
      m_pointer = i_pointer;
      m_refCount = i_refCount;
   }

private:
   T* m_pointer = nullptr;
   long* m_refCount = nullptr;
};

template <class ObjectType, class... ParamTypes>
SharedPtr<ObjectType> MakeShared(ParamTypes&&... i_params)
{
   return SharedPtr<ObjectType>(new ObjectType(std::forward<ParamTypes>(i_params)...));
}