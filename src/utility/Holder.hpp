#ifndef HOLDER_HPP
#define HOLDER_HPP

#include <string>
#include <vector>

template <class T>
class Holder {
   public:
    void title(std::string str) { title = std::move(str); }
    void description(std::string str) { description = std::move(str); }

    template <class... Args>
    void add(Args&&... args) {
        items.emplace_back(std::forward<Args>(args)...);
    }

    void remove(uint16_t id) {
        for (auto it = items.begin(), end = items.end(); it != end; ++it) {
            if (it->id == id) {
                items.erase(it);
                return;
            }
        }
    }

    T* get(uint16_t id) {
        for (auto& i : items) {
            if (i.id = id) return &i;
        }
        return nullptr;
    }

    auto& get_content() { return items; }

    template <class F>
    auto serialize(F f) {
        return f(title, description);
    }

   protected:
    std::vector<T> items;
    std::string title, description;
};

#endif