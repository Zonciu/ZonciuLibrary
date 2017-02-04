/*!
 *
 * \author Zonciu
 * Contact: zonciu@zonciu.com
 *
 * \brief
 * Random
 * \note
*/
#ifndef ZONCIU_SINGLETON_HPP
#define ZONCIU_SINGLETON_HPP
#include <atomic>
namespace zonciu
{
//Must call ::Init(args..) before using ::Get();
template<class T>
class Singleton
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
    static T& Init(Args&&...args)
    {
        static GC gc;
        if (!_Get().flag.test_and_set())
            _Get().instance = new T(std::forward<Args>(args)...);
        return *_Get().instance;
    }
    static T& Get()
    {
        while (!_Get().instance) {}
        assert(_Get().instance != nullptr);
        return *_Get().instance;
    }
    static void Destroy()
    {
        delete _Get().instance;
        _Get().instance = nullptr;
        _Get().flag.clear();
    }
private:
    static Instance& _Get()
    {
        static Instance ins;
        return ins;
    }
    Singleton();
    ~Singleton();
    Singleton(const Singleton&) = delete;
    const Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    static Creator creator_;
};
#define SINGLETON(type) zonciu::Singleton<type>::Get()
#define SINGLETON_INIT(type,...) zonciu::Singleton<type>::Init(__VA_ARGS__)
// SINGLETON_INIT(class,param_1,param_2,...);
// init before main()
#define SINGLETON_PREINIT(type,...) \
template<> typename zonciu::Singleton<type>::Creator zonciu::Singleton<type>::creator_ = {__VA_ARGS__}
} // namespace zonciu
#endif // ZONCIU_SINGLETON_HPP
