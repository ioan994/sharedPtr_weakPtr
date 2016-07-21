#pragma once;

#include <type_traits>
#include <atomic>

template<class T>
class weak_ptr;

class bad_weak_ptr : public std::exception
{
public:
   bad_weak_ptr() : std::exception("bad_weak_ptr")
   {
   }
};

struct control_block_base
{
   virtual ~control_block_base() = default;
   virtual void destroy() = 0;

   std::atomic<long> m_weakRefCount = 0;
   std::atomic<long> m_refCount = 0;
};

template <class T, class D>
struct control_block_deleter : public control_block_base
{
   control_block_deleter(T* i_pointer, D i_deleter) : m_pointer(i_pointer), m_deleter(i_deleter)
   {
   }

   virtual void destroy() override
   {
      m_deleter(m_pointer);
      m_pointer = nullptr;
   }

   ~control_block_deleter()
   {
      if (m_pointer) destroy();
   }

   T* m_pointer;
   D m_deleter;
};

template <class T>
struct control_block : public control_block_base
{
   control_block(T* i_pointer) : m_pointer(i_pointer)
   {
   }

   virtual void destroy() override
   {
      delete m_pointer;
      m_pointer = nullptr;
   }

   ~control_block()
   {
      if (m_pointer)  destroy();
   }

   T* get()
   {
      return m_pointer;
   }

   T* m_pointer;
};

template <class T>
struct control_block_element : public control_block_base
{
   template<class... ParamTypes>
   control_block_element(ParamTypes&&... i_params)
   {
      new (&m_data) T(std::forward<ParamTypes>(i_params)...);
   }

   virtual void destroy() override
   {
      reinterpret_cast<T*>(&m_data)->~T();
      m_wasDestroyed = true;
   }

   ~control_block_element()
   {
      if (!m_wasDestroyed) destroy();
   }

   T* get()
   {
      return reinterpret_cast<T*>(&m_data);
   }

   bool m_wasDestroyed = false;
   typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type m_data;
};

template <class T>
class shared_ptr
{
public:

   using element_type = T;

   shared_ptr()
   {
      internal_reset(nullptr, nullptr);
   }

   template<class TOther>
   explicit shared_ptr(TOther* i_pointer)
   {
      try
      {
         internal_reset(i_pointer, nullptr);
      }
      catch (...)
      {
         delete i_pointer;
         throw;
      }
   }

   template<class TOther, class TDeleter>
   shared_ptr(TOther* i_ptr, TDeleter i_deleter)
   {
      internal_reset_deleter(i_ptr, i_deleter);
   }

   template <class TDeleter>
   shared_ptr(nullptr_t, TDeleter i_deleter)
   {
      internal_reset_deleter(nullptr, i_deleter);
   }

   template<class TOther> 
   shared_ptr(const shared_ptr<TOther>& i_otherShared, T* i_otherPtr)
   {
      if (i_otherShared.get_control_block() && i_otherPtr == nullptr) ++i_otherShared.get_control_block()->m_refCount;
      internal_reset(i_otherPtr, i_otherShared.get_control_block());
   }

   shared_ptr(const shared_ptr& i_other)
   {
      internal_reset(i_other.m_pointer, i_other.m_controlBlock);
   }

   template<class TOther, class = std::enable_if_t<std::is_convertible<TOther*, T*>::value>>
   shared_ptr(const shared_ptr<TOther>& i_other)
   {
      internal_reset(i_other.get(), i_other.get_control_block());
   }

   shared_ptr(shared_ptr&& i_other)
   {
      set_pointers(i_other.m_pointer, i_other.m_controlBlock);
      i_other.set_pointers(nullptr, nullptr);
   }

   template<class TOther, class = std::enable_if_t<std::is_convertible<TOther*, T*>::value>>
   shared_ptr(shared_ptr<TOther>&& i_other)
   {
      set_pointers(i_other.m_pointer, i_other.m_controlBlock);
      i_other.set_pointers(nullptr, nullptr);
   }

   template<class TOther>
   explicit shared_ptr(const weak_ptr<TOther>& i_other)
   {
      *this = i_other.lock();
   }

   shared_ptr(nullptr_t) : shared_ptr()
   {
   }

   ~shared_ptr()
   {
      remove_ref();
   }

   shared_ptr& operator = (const shared_ptr& i_other)
   {
      shared_ptr(i_other).swap(*this);
      return *this;
   }

   template<class TOther>
   shared_ptr& operator=(const shared_ptr<TOther>& i_other)
   {
      shared_ptr(i_other).swap(*this);
      return *this;
   }

   shared_ptr& operator = (shared_ptr&& i_other)
   {
      shared_ptr(std::move(i_other)).swap(*this);
      return *this;
   }

   template <class TOther>
   shared_ptr& operator = (shared_ptr<TOther>&& i_other)
   {
      shared_ptr(i_other).swap(*this);
      return *this;
   }

   void swap(shared_ptr& i_other)
   {
      std::swap(m_pointer, i_other.m_pointer);
      std::swap(m_controlBlock, i_other.m_controlBlock);
   }

   void reset()
   {
      shared_ptr().swap(*this);
   }

   template<class TOther>
   void reset(TOther* i_pointer)
   {
      shared_ptr(i_pointer).swap(*this);
   }

   template<class TOther, class TDeleter> 
   void reset(TOther* i_ptr, TDeleter i_deleter)
   {
      shared_ptr(i_ptr, i_deleter).swap(*this);
   }

   T* get() const
   {
      return m_pointer;
   }

   T& operator*()
   {
      return *m_pointer;
   }

   T* operator ->()
   {
      return m_pointer;
   }

   long use_count() const
   {
      return m_controlBlock ? m_controlBlock->m_refCount.load() : 0;
   }

   bool unique()
   {
      return use_count() == 1;
   }

   explicit operator bool()
   {
      return m_pointer != nullptr;
   }

   control_block_base* get_control_block() const
   {
      return m_controlBlock;
   }

   void internal_reset(T* i_pointer, control_block_base* i_controlBlock)
   {
      remove_ref();
      set_pointers(i_pointer, i_controlBlock);
      if (m_pointer)add_ref();
   }

   template<class TOther>
   bool owner_before(shared_ptr<TOther> const& i_other) const
   {
      return m_controlBlock < i_other.m_controlBlock;
   }

   template<class TOther>
   bool owner_before(weak_ptr<TOther> const& i_other) const
   {
      return m_controlBlock < i_other.m_controlBlock;
   }

private:
   void add_ref()
   {
      if (!m_controlBlock) m_controlBlock = new control_block<T>(m_pointer);
      ++m_controlBlock->m_refCount;
   }

   void remove_ref()
   {
      if (!m_controlBlock || --m_controlBlock->m_refCount != 0) return;

      if (m_controlBlock->m_weakRefCount == 0)
      {
         delete m_controlBlock;
         m_controlBlock = nullptr;
      }
      else
      {
         m_controlBlock->destroy();
      }
   }

   template<class TDeleter>
   void internal_reset_deleter(T* i_pointer, TDeleter i_deleter)
   {
      try
      {
         internal_reset(i_pointer, new control_block_deleter<T, TDeleter>(i_pointer, i_deleter));
      }
      catch (...)
      {
         if (i_pointer) i_deleter(i_pointer);
         throw;
      }
   }

   void set_pointers(T* i_pointer, control_block_base* i_controlBlock)
   {
      m_pointer = i_pointer;
      m_controlBlock = i_controlBlock;
   }

private:
   T* m_pointer = nullptr;
   control_block_base* m_controlBlock = nullptr;
};

template <class ObjectType, class... ParamTypes>
shared_ptr<ObjectType> make_shared(ParamTypes&&... i_params)
{
   shared_ptr<ObjectType> shared;
   auto controlBlock = new control_block_element<ObjectType>(std::forward<ParamTypes>(i_params)...);
   shared.internal_reset(controlBlock->get(), controlBlock);
   return shared;
}

template<class TLeft, class TRight>
bool operator==(const shared_ptr<TLeft>& i_lhs, const shared_ptr<TRight>& i_rhs)
{
   return i_lhs.get() == i_rhs.get();
}

template<class TLeft, class TRight>
bool operator<(const shared_ptr<TLeft>& i_lhs, const shared_ptr<TRight>& i_rhs)
{
   return std::less<decltype(false ? i_lhs.get() : i_rhs.get())>()(i_lhs.get(), i_rhs.get());
}

template <class TLeft>
bool operator==(const shared_ptr<TLeft>& i_lhs, nullptr_t)
{
   return !i_lhs;
}

template <class TRight>
bool operator==(nullptr_t, const shared_ptr<TRight>& i_rhs)
{
   return !i_rhs;
}

template <class TLeft>
bool operator!=(const shared_ptr<TLeft>& i_lhs, nullptr_t)
{
   return static_cast<bool>(i_lhs);
}

template <class TRight>
bool operator!=(nullptr_t, const shared_ptr<TRight>& i_rhs)
{
   return static_cast<bool>(i_rhs);
}

template <class TLeft>
bool operator<(const shared_ptr<TLeft>& i_lhs, nullptr_t)
{
   return std::less<TLeft*>()(i_lhs.get(), nullptr);
}

template <class TRight>
bool operator<(nullptr_t, const shared_ptr<TRight>& i_rhs)
{
   return std::less<TRight*>()(nullptr, i_rhs.get());
}

template <class TLeft>
bool operator>(const shared_ptr<TLeft>& i_lhs, nullptr_t)
{
   return nullptr < i_lhs;
}

template <class TRight>
bool operator>(nullptr_t, const shared_ptr<TRight>& i_rhs)
{
   i_rhs < nullptr;
}

template <class TLeft>
bool operator<=(const shared_ptr<TLeft>& i_rhs, nullptr_t)
{
   return !(nullptr < i_rhs);
}

template <class TRight>
bool operator<=(nullptr_t, const shared_ptr<TRight>& i_lhs)
{
   return !(i_lhs < nullptr);
}

template <class TLeft>
bool operator>=(const shared_ptr<TLeft>& i_rhs, nullptr_t)
{
   return !(i_lhs < nullptr);
}

template <class TRight>
bool operator>=(nullptr_t, const shared_ptr<TRight>& i_lhs)
{
   return !(nullptr < i_rhs);
}

template<class T>
void swap(shared_ptr<T>& i_lhs, shared_ptr<T>& i_rhs)
{
   i_lhs.swap(i_rhs);
}

template<class T, class TOther>
shared_ptr<T> static_pointer_cast(const shared_ptr<TOther>& i_ptr)
{
   return shared_ptr<T>(i_ptr, static_cast<T*>(i_ptr.get()));
}

template<class T, class TOther>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<TOther>& i_ptr)
{
   return dynamic_cast<T*>(i_ptr.get()) == nullptr ? shared_ptr<T>() : shared_ptr<T>(i_ptr, dynamic_cast<T*>(i_ptr.get()));
}

template<class T, class TOther>
shared_ptr<T> const_pointer_cast(const shared_ptr<TOther>& i_ptr)
{
   return shared_ptr<T>(i_ptr, const_cast<T*>(i_ptr.get()));
}

template<class T, class TDeleter>
TDeleter* get_deleter(const shared_ptr<T>& i_ptr)
{
   auto controlBlock = dynamic_cast<control_block_deleter*>(i_ptr.get_control_block());
   return &(controlBlock->m_deleter);
}

template<class T>
class weak_ptr 
{
public:
   typedef T element_type;

   weak_ptr()
   {
   }

   weak_ptr(const weak_ptr& i_other)
   {
      internal_reset(i_other.m_pointer, i_other.m_controlBlock);
   }

   template<class TOther, class = std::enable_if_t<std::is_convertible<TOther*, T*>::value>>
   weak_ptr(const weak_ptr<TOther>& i_other)
   {
      internal_reset(i_other.get_ptr(), i_other.get_control_block());
   }

   template<class TOther, class = std::enable_if_t<std::is_convertible<TOther*, T*>::value>>
   weak_ptr(const shared_ptr<TOther>& i_ptr)
   {
      internal_reset(i_ptr.get(), i_ptr.get_control_block());
   }

   ~weak_ptr()
   {
      remove_weak_ref();
   }

   weak_ptr& operator=(const weak_ptr& i_other)
   {
      weak_ptr(i_other).swap(*this);
      return *this;
   }

   template<class TOther>
   weak_ptr& operator=(const weak_ptr<TOther>& i_other)
   {
      weak_ptr(i_other).swap(*this);
      return *this;
   }

   template<class TOther>
   weak_ptr& operator=(const shared_ptr<TOther>& i_other)
   {
      weak_ptr(i_other).swap(*this);
      return *this;
   }

   void swap(weak_ptr& i_other)
   {
      std::swap(m_controlBlock, i_other.m_controlBlock);
   }

   void reset()
   {
      weak_ptr().swap(*this);
   }

   long use_count() const
   {
      return m_controlBlock ? m_controlBlock->m_refCount.load() : 0;
   }

   bool expired() const
   {
      return use_count() == 0;
   }

   shared_ptr<T> lock() const
   {
      if (expired()) throw bad_weak_ptr();
      shared_ptr<T> shared;
      shared.internal_reset(m_pointer, m_controlBlock);
      return shared;
   }

   control_block_base* get_control_block() const
   {
      return m_controlBlock;
   }

   T* get_ptr() const
   {
      return m_pointer;
   }

private:
   void add_weak_ref()
   {
      if (m_controlBlock) ++m_controlBlock->m_weakRefCount;
   }

   void remove_weak_ref()
   {
      if (m_controlBlock && --m_controlBlock->m_weakRefCount == 0)
      {
         delete m_controlBlock;
         m_controlBlock = nullptr;
      }
   }

   void internal_reset(T* i_ptr, control_block_base* i_controlBlock)
   {
      m_pointer = i_ptr;
      m_controlBlock = i_controlBlock;
      add_weak_ref();
   }

private:
   T* m_pointer = nullptr;
   control_block_base* m_controlBlock = nullptr;
};