#pragma once;

struct control_block_base
{
   virtual ~control_block_base() = default;
   virtual long& use_count() = 0;
   virtual long use_count() const = 0;
};

template <class T>
struct control_block : public control_block_base
{
   control_block() {}

   control_block(T* i_pointer) : m_pointer(i_pointer)
   {
   }

   control_block(T* i_pointer, long i_refCount) : m_pointer(i_pointer), m_refCount(i_refCount)
   {
   }

   ~control_block()
   {
      delete m_pointer;
   }

   virtual long& use_count() override
   {
      return m_refCount;
   }

   virtual long use_count() const override
   {
      return m_refCount;
   }

   T* m_pointer = nullptr;
   long m_refCount = 0;
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
      internal_reset(i_pointer, nullptr);
   }

   template<class TOther> 
   shared_ptr(const shared_ptr<TOther>& i_otherShared, T* i_otherPtr)
   {
      internal_reset(i_otherPtr, i_otherShared.get_control_block());
   }

   shared_ptr(const shared_ptr& i_other)
   {
      internal_reset(i_other.m_pointer, i_other.m_control_block);
   }

   template<class TOther, class = std::enable_if_t<std::is_convertible<TOther*, T*>::value>>
   shared_ptr(const shared_ptr<TOther>& i_other)
   {
      internal_reset(i_other.get(), i_other.get_control_block());
   }

   shared_ptr(shared_ptr&& i_other)
   {
      *this = std::move(i_other);
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
      if (this != &i_other)
      {
         internal_reset(i_other.m_pointer, i_other.m_control_block);
      }
      return *this;
   }

   template<class TOther>
   shared_ptr& operator=(const shared_ptr<TOther>& i_other)
   {
      internal_reset(i_other.get(), i_other.get_control_block());
      return *this;
   }

   shared_ptr& operator = (shared_ptr&& i_other)
   {
      if (this != &i_other)
      {
         remove_ref();
         set_pointers(i_other.m_pointer, i_other.m_control_block);
         i_other.set_pointers(nullptr, nullptr);
      }
      return *this;
   }

   T* get() const
   {
      return m_pointer;
   }

   long use_count() const
   {
      return m_control_block ? m_control_block->use_count() : 0;
   }

   void reset(T* i_pointer = nullptr)
   {
      internal_reset(i_pointer, nullptr);
   }

   void swap(shared_ptr& i_other)
   {
      std::swap(m_pointer, i_other.m_pointer);
      std::swap(m_control_block, i_other.m_control_block);
   }

   control_block_base* get_control_block() const
   {
      return m_control_block;
   }

   bool unique()
   {
      return use_count() == 1;
   }

   T* operator ->()
   {
      return m_pointer;
   }

   T& operator*()
   {
      return *m_pointer;
   }

   explicit operator bool()
   {
      return m_pointer != nullptr;
   }

private:
   void add_ref()
   {
      if (!m_control_block) m_control_block = new control_block<T>(m_pointer);
      ++m_control_block->use_count();
   }

   void remove_ref()
   {
      if (m_control_block && --m_control_block->use_count() == 0)
      {
         delete m_control_block;
      }
   }

   void internal_reset(T* i_pointer, control_block_base* i_control_block)
   {
      remove_ref();
      set_pointers(i_pointer, i_control_block);
      if (m_pointer)add_ref();
   }

   void set_pointers(T* i_pointer, control_block_base* i_control_block)
   {
      m_pointer = i_pointer;
      m_control_block = i_control_block;
   }

private:
   T* m_pointer = nullptr;
   control_block_base* m_control_block = nullptr;
};

template <class ObjectType, class... ParamTypes>
shared_ptr<ObjectType> make_shared(ParamTypes&&... i_params)
{
   return shared_ptr<ObjectType>(new ObjectType(std::forward<ParamTypes>(i_params)...));
}