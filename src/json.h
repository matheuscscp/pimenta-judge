#ifndef JSON_H
#define JSON_H

#include <string>

struct JSONValue;
class JSON {
  // API
  public:
    // C++ special members
    JSON();
    ~JSON();
    JSON(const JSON&);
    JSON& operator=(const JSON&);
    JSON(JSON&&);
    JSON& operator=(JSON&&);
    // string
    operator std::string() const;
    std::string str(int indent = 0) const; // 0 => compact (single line) JSON
    JSON(const char*);
    JSON& operator=(const char*);
    JSON(const std::string&);
    JSON& operator=(const std::string&);
    bool isstr() const;
    bool operator==(const std::string&) const;
    // property
    JSON& operator[](const std::string&);
    const JSON* find(const std::string&) const;
    std::vector<std::string> keys() const;
    // array
    JSON& operator[](size_t i);
    const JSON& operator[](size_t i) const;
    JSON(const std::initializer_list<JSON>&);
    JSON& operator=(const std::initializer_list<JSON>&);
    JSON(const std::vector<JSON>&);
    JSON& operator=(const std::vector<JSON>&);
    bool isarr() const;
    void push_back(const JSON&);
    size_t erase(size_t first, size_t last = 0);
    // commom
    size_t size() const;
    JSON& operator()();
    template <typename T, typename... Args>
    JSON& operator()(const T& f, Args... args) {
      return (*this)[f](args...);
    }
    const JSON* find() const;
    template <typename... Args>
    const JSON* find(const std::string prop, Args... args) const {
      auto ptr = find(prop);
      if (ptr) return ptr->find(args...);
      return nullptr;
    }
    template <typename... Args>
    const JSON* find(size_t i, Args... args) const {
      if (isarr() && i < size()) return (*this)[i].find(args...);
      return nullptr;
    }
    // static
    static std::string str(const std::string&); // convert to json.org format
    static size_t number_size(const std::string&); // # of chars that make a num
  // implementation
  private:
    JSONValue* value;
};
bool operator==(const std::string&, const JSON&);
std::ostream& operator<<(std::ostream&, const JSON&);
std::istream& operator>>(std::istream&, JSON&);

#endif
