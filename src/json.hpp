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
    // special members
    JSON();
    ~JSON();
    JSON(const JSON&);
    JSON& operator=(const JSON&);
    JSON(JSON&&);
    JSON& operator=(JSON&&);
    // string constructors
    JSON(const char*);
    JSON& operator=(const char*);
    JSON(const std::string&);
    JSON& operator=(const std::string&);
    JSON(std::string&&);
    JSON& operator=(std::string&&);
    // number constructors
    template <typename T>
    JSON(T val) : value(nullptr) {
      std::stringstream ss;
      ss << val;
      operator=(move(ss.str()));
    }
    template <typename T>
    JSON& operator=(T val) {
      std::stringstream ss;
      ss << val;
      operator=(move(ss.str()));
    }
    // object constructors
    JSON(const std::map<std::string,JSON>&);
    JSON& operator=(const std::map<std::string,JSON>&);
    JSON(std::map<std::string,JSON>&&);
    JSON& operator=(std::map<std::string,JSON>&&);
    // array constructors
    JSON(const std::vector<JSON>&);
    JSON& operator=(const std::vector<JSON>&);
    JSON(std::vector<JSON>&&);
    JSON& operator=(std::vector<JSON>&&);
    // string API
    bool isstr() const;
    std::string& str();
    const std::string& str() const;
    operator std::string&();
    operator const std::string&() const;
    bool operator==(const std::string&) const;
    bool operator!=(const std::string&) const;
    // number API
    bool isnum() const;
    number num() const;
    template <typename T>
    operator T() const {
      std::stringstream ss(str());
      T ans;
      ss >> ans;
      return ans;
    }
    template <typename T>
    bool read(T& buf) const {
      if (!isnum()) return false;
      std::stringstream ss(str());
      if (!(ss >> buf)) return false;
      ss.get();
      return !ss;
    }
    template <typename T>
    T to(T def = T()) const {
      T ans;
      if (!read(ans)) return def;
      return ans;
    }
    // object API
    bool isobj() const;
    bool issubobj(const JSON& sup) const;
    std::map<std::string,JSON>& obj();
    const std::map<std::string,JSON>& obj() const;
    JSON& operator[](const char*);
    JSON& operator[](const std::string&);
    JSON& operator[](std::string&&);
    std::map<std::string,JSON>::iterator find(const std::string&);
    std::map<std::string,JSON>::const_iterator find(const std::string&) const;
    std::map<std::string,JSON>::iterator erase(
      std::map<std::string,JSON>::const_iterator
    );
    size_t erase(const std::string&);
    // array API
    bool isarr() const;
    std::vector<JSON>& arr();
    const std::vector<JSON>& arr() const;
    void push_back(const JSON&);
    void push_back(JSON&&);
    template <typename... Args>
    void emplace_back(Args&&... args) {
      push_back(std::move(JSON(args...)));
    }
    JSON& operator[](size_t i);
    const JSON& operator[](size_t i) const;
    std::vector<JSON>::iterator erase(std::vector<JSON>::iterator position);
    std::vector<JSON>::iterator erase(
      std::vector<JSON>::iterator first,
      std::vector<JSON>::iterator last
    );
    // literals API
    bool istrue() const;
    bool isfalse() const;
    bool isnull() const;
    void settrue();
    void setfalse();
    void setnull();
    static inline JSON null() { JSON ans; ans.setnull(); return ans; }
    // polymorphic
    operator bool() const; // false iff (=="", ==0, isfalse() or isnull())
    size_t size() const;
    bool equals(const JSON&) const;
    // algebraic function syntax
    const JSON operator()() const;
    template <typename... Args>
    const JSON operator()(const std::string& key, Args... args) const {
      if (!isobj()) return JSON::null();
      auto it = find(key);
      if (it == obj().end()) return JSON::null();
      return it->second(args...);
    }
    template <typename... Args>
    const JSON operator()(size_t i, Args... args) const {
      if (!isarr() || size() <= i) return JSON::null();
      return (*this)[i](args...);
    }
    JSON& ref();
    template <typename T, typename... Args>
    JSON& ref(const T& f, Args... args) {
      return (*this)[f].ref(args...);
    }
    // I/O
    bool parse(void*); // C string
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
bool operator!=(const std::string&, const JSON&);
std::ostream& operator<<(std::ostream&, const JSON&); // outputs indented JSON

#endif
