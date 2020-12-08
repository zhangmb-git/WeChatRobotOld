/********************************************/
// Example:
// 1.friend class declaration is requiered!
// 2.constructor should be private
//class DerivedSingle :public Singleton<DerivedSingle> {
//friend class Singleton<DerivedSingle>;
// private:
//	 DerivedSingle() :value(0) {}
//	 DerivedSingle(const DerivedSingle&) = delete;
//	 DerivedSingle& operator =(const DerivedSingle&) = delete;
//}


#pragma  once
#include <memory>
#include <mutex>
#include <iostream>

template<typename T>
class Singleton {
    typedef std::shared_ptr<T> Ptr;

  protected:
    Singleton() noexcept = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    virtual ~Singleton() = default;
    static Ptr instance_ptr;
    static std::mutex singleton_mutex;

  public:

    static Ptr getInstance() noexcept(std::is_nothrow_constructible<T>::value) {
        if (instance_ptr == nullptr) {
            std::lock_guard<std::mutex> lk(singleton_mutex);

            if (instance_ptr == nullptr) {
                instance_ptr = std::shared_ptr<T>(new T());
            }
        }

        return instance_ptr;

    }
};
template <typename T>
std::shared_ptr<T> Singleton<T>::instance_ptr = nullptr;

template <typename T>
std::mutex Singleton<T>::singleton_mutex;

