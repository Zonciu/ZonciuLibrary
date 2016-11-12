/*!
 *
 * \author Zonciu
 * Contact: zonciu@zonciu.com
 *
 * \brief
 * Component collection, written by Zonciu
 * \note
*/
#ifndef ZONCIU_NOCOPYABLE_HPP
#define ZONCIU_NOCOPYABLE_HPP
namespace zonciu
{
class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    noncopyable(const noncopyable&) = delete;
    const noncopyable& operator=(const noncopyable&) = delete;
};
} // namespace zonciu
#endif
