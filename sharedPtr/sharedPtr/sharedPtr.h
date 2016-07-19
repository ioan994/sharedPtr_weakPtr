#pragma once;

template <class T>
class SharedPtr
{
public:

   using element_type = T;

   SharedPtr()
   {
      internalReset(nullptr, nullptr);
   }

   template<class TOther>
   explicit SharedPtr(TOther* i_pointer)
   {
      internalReset(i_pointer, nullptr);
   }

   SharedPtr(const SharedPtr& i_other)
   {
      internalReset(i_other.m_pointer, i_other.m_refCount);
   }

   template<class TOther, class = std::enable_if_t<std::is_convertible<TOther*, T*>::value>>
   SharedPtr(const SharedPtr<TOther>& i_other)
   {
      internalReset(i_other.Get(), i_other.GetRefCount());
   }

   SharedPtr(SharedPtr&& i_other)
   {
      *this = std::move(i_other);
   }

   SharedPtr(nullptr_t) : SharedPtr()
   {
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

   template<class TOther>
   SharedPtr& operator=(const SharedPtr<TOther>& i_other)
   {
      internalReset(i_other.Get(), i_other.GetRefCount());
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

   void swap(SharedPtr& i_other)
   {
      std::swap(m_pointer, i_other.m_pointer);
      std::swap(m_refCount, i_other.m_refCount);
   }

   long* GetRefCount() const
   {
      return m_refCount;
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