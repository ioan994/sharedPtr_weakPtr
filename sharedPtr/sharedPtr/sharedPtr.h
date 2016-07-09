#pragma once;

template <class T>
class SharedPtr
{
public:

   explicit SharedPtr(T* i_pointer = nullptr) : m_pointer(i_pointer), m_refCount(nullptr)
   {
      if (m_pointer) addRef();
   }

   ~SharedPtr()
   {
      removeRef();
   }

   SharedPtr(const SharedPtr& i_other) 
   {
      *this = i_other;
   }

   SharedPtr& operator = (const SharedPtr& i_other)
   {
      if (this != &i_other)
      {
         m_refCount = i_other.m_refCount;
         m_pointer = i_other.m_pointer;
         if (m_pointer) addRef();
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
      removeRef();
      m_pointer = i_pointer;
      if (m_pointer)addRef();
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
   }

private:
   T* m_pointer;
   long* m_refCount;
};