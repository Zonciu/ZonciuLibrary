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
namespace zonciu
{
template<class T>
class Singleton
{
private:
    struct Creator
    {
        template<class...Args>
        Creator(Args&&...args) { init(std::forward<Args>(args)...); }
    };
    struct GC
    {
        ~GC() { Singleton<T>::destroy(); }
    };
    struct Instance
    {
        T* instance{ nullptr };
        std::atomic_flag flag{ ATOMIC_FLAG_INIT };
    };
public:
    template<class...Args>
    static T& init(Args&&...args)
    {
        static GC _gc;
        if (!_get().flag.test_and_set())
            _get().instance = new T(std::forward<Args>(args)...);
        return *_get().instance;
    }
    static T& get()
    {
        while (!_get().instance) {}
        assert(_get().instance != nullptr);
        return *_get().instance;
    }
    static void destroy()
    {
        delete _get().instance;
        _get().instance = nullptr;
        _get().flag.clear();
    }
private:
    static Instance& _get()
    {
        static Instance ins_;
        return ins_;
    }
    Singleton();
    ~Singleton();
    Singleton(const Singleton&) = delete;
    const Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    static Creator creator_;
};
// SINGLETON_INIT_BEFORE_MAIN(class,param_1,param_2,...);
#define SINGLETON_INIT_BEFORE_MAIN(type,...) \
template<> typename zonciu::singleton::Singleton<type>::Creator zonciu::singleton::Singleton<type>::creator_ = {__VA_ARGS__}
} // namespace zonciu
#endif
