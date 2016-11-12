/*!
 *
 * \author Zonciu
 * Contact: zonciu@zonciu.com
 *
 * \brief
 * Random
 * \note
*/
#ifndef ZONCIU_SINGKETON_HPP
#define ZONCIU_SINGKETON_HPP
#include <atomic>
#include <zonciu/nocopyable.hpp>
namespace zonciu
{
namespace singleton
{
template<class T>
class Singleton : public zonciu::noncopyable
{
private:
    struct Creator
    {
        template<class...Args>
        Creator(Args&&...args) { Init(std::forward<Args>(args)...); }
    };
    struct GC
    {
        ~GC() { Singleton<T>::Destroy(); }
    };
    struct Instance
    {
        T* instance{ nullptr };
        std::atomic_flag flag{ ATOMIC_FLAG_INIT };
    };
public:
    template<class...Args>
    static inline T& Init(Args&&...args)
    {
        static GC _gc;
        if (!_Get().flag.test_and_set())
            _Get().instance = new T(std::forward<Args>(args)...);
        return *_Get().instance;
    }
    static inline T& Get()
    {
        while (!_Get().instance) {}
        assert(_Get().instance != nullptr);
        return *_Get().instance;
    }
    static inline void Destroy()
    {
        delete _Get().instance;
        _Get().instance = nullptr;
        _Get().flag.clear();
    }
private:
    inline static Instance& _Get()
    {
        static Instance ins_;
        return ins_;
    }
    Singleton();
    ~Singleton();
    Singleton(const Singleton&);
    Singleton(Singleton&&);
    Singleton& operator = (const Singleton&);
    static Creator creator_;
};
// SINGLETON_INIT_BEFORE_MAIN(class,param_1,param_2,...);
#define SINGLETON_INIT_BEFORE_MAIN(type,...) \
template<> typename zonciu::Singleton<type>::Creator zonciu::Singleton<type>::creator_ = {__VA_ARGS__}
} // namespace singleton
} // namespace zonciu
#endif
