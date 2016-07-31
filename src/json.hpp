#ifndef JSON_H
#define JSON_H

#include <string>
#include <map>
#include <vector>
#include <sstream>

struct JSONValue; // hidden class for implementation
class JSON {
  // API
  public:
    class number { // immutable
      public:
        number(const char*);
        bool isnum() const;
        bool isneg() const;
        bool isexpneg() const;
        std::string integer() const; // if isnum(), matches digit1-9 digit*
        std::string frac() const; // if isnum(), matches digit*
        std::string exp() const; // if isnum(), matches "" U (digit1-9 digit*)
      private:
        bool is_num,is_neg,is_exp_neg;
        std::string int_,frac_,exp_;
    };
    class object_iterator { // immutable
      public:
        object_iterator(JSON*);
        std::map<std::string,JSON>::iterator begin();
        std::map<std::string,JSON>::iterator end();
      private:
        JSON* obj;
    };
    class object_const_iterator { // immutable
      public:
        object_const_iterator(const JSON*);
        std::map<std::string,JSON>::const_iterator begin() const;
        std::map<std::string,JSON>::const_iterator end() const;
      private:
        const JSON* obj;
    };
    class array_iterator { // immutable
      public:
        array_iterator(JSON*);
        std::vector<JSON>::iterator begin();
        std::vector<JSON>::iterator end();
      private:
        JSON* arr;
    };
    class array_const_iterator { // immutable
      public:
        array_const_iterator(const JSON*);
        std::vector<JSON>::const_iterator begin() const;
        std::vector<JSON>::const_iterator end() const;
      private:
        const JSON* arr;
    };
    // special members
    JSON();
    ~JSON();
    JSON(const JSON&);
    JSON& operator=(const JSON&);
    JSON(JSON&&);
    JSON& operator=(JSON&&);
    JSON(const char*);
    JSON& operator=(const char*);
    JSON(const std::string&);
    JSON& operator=(const std::string&);
    JSON(std::string&&);
    JSON& operator=(std::string&&);
    JSON(const std::initializer_list<JSON>&);
    JSON& operator=(const std::initializer_list<JSON>&);
    JSON(const std::vector<JSON>&);
    JSON& operator=(const std::vector<JSON>&);
    JSON(std::vector<JSON>&&);
    JSON& operator=(std::vector<JSON>&&);
    // string
    bool isstr() const;
    operator std::string&();
    operator const std::string&() const;
    std::string& str();
    const std::string& str() const;
    bool operator==(const std::string&) const;
    // number
    bool isnum() const;
    number num() const;
    template <typename T>
    bool to(T& buf) const {
      if (!isstr()) return false;
      std::stringstream ss((const std::string&)*this);
      return ss >> buf;
    }
    template <typename T>
    T to() const {
      if (!isstr()) return T();
      std::stringstream ss((const std::string&)*this);
      T ans;
      ss >> ans;
      return ans;
    }
    // object
    bool isobj() const;
    JSON& operator[](const std::string&);
    JSON& operator[](std::string&&);
    std::map<std::string,JSON>::iterator find(const std::string&);
    std::map<std::string,JSON>::const_iterator find(const std::string&) const;
    std::map<std::string,JSON>::iterator erase(
      std::map<std::string,JSON>::const_iterator
    );
    size_t erase(const std::string&);
    object_iterator oit();
    std::map<std::string,JSON>::iterator begin_o();
    std::map<std::string,JSON>::iterator end_o();
    object_const_iterator oit() const;
    std::map<std::string,JSON>::const_iterator begin_o() const;
    std::map<std::string,JSON>::const_iterator end_o() const;
    // array
    bool isarr() const;
    void push_back(const JSON&);
    void push_back(JSON&&);
    template <typename... Args>
    void emplace_back(Args&&... args) {
      push_back(std::move(JSON(args...)));
    }
    JSON& operator[](size_t i);
    const JSON& operator[](size_t i) const;
    size_t erase(size_t L, size_t R = 0); // [L, R) or just L if R = 0
    array_iterator ait();
    std::vector<JSON>::iterator begin_a();
    std::vector<JSON>::iterator end_a();
    array_const_iterator ait() const;
    std::vector<JSON>::const_iterator begin_a() const;
    std::vector<JSON>::const_iterator end_a() const;
    // literals
    bool istrue() const;
    bool isfalse() const;
    bool isnull() const;
    // polymorphic
    operator bool() const; // false if, and only if, = "", 0, "false" or "null"
    size_t size() const;
    // algebraic function syntax
    JSON& operator()();
    template <typename T, typename... Args>
    JSON& operator()(const T& f, Args... args) {
      return (*this)[f](args...);
    }
    const JSON* find_tuple() const;
    const JSON* find_tuple(const std::string&) const;
    template <typename... Args>
    const JSON* find_tuple(const std::string& key, Args... args) const {
      auto ptr = find_tuple(key);
      if (ptr) return ptr->find_tuple(args...);
      return nullptr;
    }
    template <typename... Args>
    const JSON* find_tuple(size_t i, Args... args) const {
      if (isarr() && i < size()) return (*this)[i].find_tuple(args...);
      return nullptr;
    }
    // I/O
    bool parse(const std::string&);
    bool read_file(const std::string&);
    std::string generate(unsigned indent = 0) const; // 0 generates compact JSON
    bool write_file(const std::string&, unsigned indent = 1) const;
  // implementation
  private:
    JSONValue* value;
};
// more API
bool operator==(const std::string&, const JSON&);
std::ostream& operator<<(std::ostream&, const JSON&); // outputs indented JSON

#endif
