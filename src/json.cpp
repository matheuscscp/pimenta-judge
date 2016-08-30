#include <cstring>
#include <stack>
#include <fstream>

#include "json.hpp"

using namespace std;

static string to_json(const string& val, bool force = false);
static size_t json_number_length(const char* val);
static bool is_json_zero(const string& val);
static bool operator>>(istream& is, JSON& json);
static void encode_utf8(unsigned cpt, string& buf);
static bool isctl(unsigned char c);
static const uint8_t* decode_utf8(const uint8_t* s, string& buf);

struct JSONValue {
  virtual ~JSONValue() {}
  virtual JSONValue* clone() const = 0;
  virtual bool isstr() const = 0;
  virtual string& str() = 0;
  virtual bool isobj() const = 0;
  virtual bool issubobj(const JSON&) const = 0;
  virtual map<string,JSON>& obj() = 0;
  virtual JSON& operator[](const string&) = 0;
  virtual JSON& operator[](string&&) = 0;
  virtual map<string,JSON>::iterator find(const string&) = 0;
  virtual map<string,JSON>::const_iterator find(const string&) const = 0;
  virtual map<string,JSON>::iterator erase(
    map<string,JSON>::const_iterator
  ) = 0;
  virtual size_t erase(const string&) = 0;
  virtual bool isarr() const = 0;
  virtual vector<JSON>& arr() = 0;
  virtual void push_back(const JSON&) = 0;
  virtual void push_back(JSON&&) = 0;
  virtual JSON& operator[](size_t i) = 0;
  virtual vector<JSON>::iterator erase(vector<JSON>::iterator) = 0;
  virtual vector<JSON>::iterator erase(
    vector<JSON>::iterator,
    vector<JSON>::iterator
  ) = 0;
  virtual size_t size() const = 0;
  virtual bool equals(const JSON&) const = 0;
  virtual string generate(unsigned) const = 0;
};

struct String : public JSONValue {
  string v;
  String(const string& v) : v(v) {}
  String(string&& v) : v(move(v)) {}
  JSONValue* clone() const {
    return new String(v);
  }
  bool isstr() const {
    return true;
  }
  string& str() {
    return v;
  }
  bool isobj() const {
    return false;
  }
  bool issubobj(const JSON&) const {
    return false;
  }
  map<string,JSON>& obj() {
    
  }
  JSON& operator[](const string&) {
    
  }
  JSON& operator[](string&&) {
    
  }
  map<string,JSON>::iterator find(const string&) {
    
  }
  map<string,JSON>::const_iterator find(const string&) const {
    
  }
  map<string,JSON>::iterator erase(map<string,JSON>::const_iterator) {
    
  }
  size_t erase(const string&) {
    
  }
  bool isarr() const {
    return false;
  }
  vector<JSON>& arr() {
    
  }
  void push_back(const JSON&) {
    
  }
  void push_back(JSON&&) {
    
  }
  JSON& operator[](size_t) {
    
  }
  vector<JSON>::iterator erase(vector<JSON>::iterator) {
    
  }
  vector<JSON>::iterator erase(vector<JSON>::iterator,vector<JSON>::iterator) {
    
  }
  size_t size() const {
    return v.size();
  }
  bool equals(const JSON& o) const {
    return o.isstr() && v == o.str();
  }
  string generate(unsigned) const {
    return to_json(v);
  }
};

struct Object : public JSONValue {
  map<string,JSON> v;
  Object() {}
  Object(const map<string,JSON>& v) : v(v) {}
  JSONValue* clone() const {
    return new Object(v);
  }
  bool isstr() const {
    return false;
  }
  string& str() {
    
  }
  bool isobj() const {
    return true;
  }
  bool issubobj(const JSON& sup) const {
    if (!sup.isobj()) return false;
    auto& vsup = sup.obj();
    for (auto& kv : v) {
      auto it = vsup.find(kv.first);
      if (it == vsup.end()) return false;
      auto& a = kv.second;
      auto& b = it->second;
      if (a.isobj() && b.isobj() && a.issubobj(b)) continue;
      if (!a.equals(b)) return false;
    }
    return true;
  }
  map<string,JSON>& obj() {
    return v;
  }
  JSON& operator[](const string& key) {
    return v[key];
  }
  JSON& operator[](string&& key) {
    return v[move(key)];
  }
  map<string,JSON>::iterator find(const string& key) {
    return v.find(key);
  }
  map<string,JSON>::const_iterator find(const string& key) const {
    return v.find(key);
  }
  map<string,JSON>::iterator erase(map<string,JSON>::const_iterator it) {
    return v.erase(it);
  }
  size_t erase(const string& key) {
    return v.erase(key);
  }
  bool isarr() const {
    return false;
  }
  vector<JSON>& arr() {
    
  }
  void push_back(const JSON&) {
    
  }
  void push_back(JSON&&) {
    
  }
  JSON& operator[](size_t) {
    
  }
  vector<JSON>::iterator erase(vector<JSON>::iterator) {
    
  }
  vector<JSON>::iterator erase(vector<JSON>::iterator,vector<JSON>::iterator) {
    
  }
  size_t size() const {
    return v.size();
  }
  bool equals(const JSON& o) const {
    if (!o.isobj() || v.size() != o.size()) return false;
    for (auto i = v.begin(), j = o.obj().begin(); i != v.end(); i++, j++) {
      if (i->first != j->first || !i->second.equals(j->second)) return false;
    }
    return true;
  }
  string generate(unsigned indent) const {
    string ans = "{"; if (indent) ans += "\n";
    auto cnt = v.size();
    for (auto it = v.begin(); it != v.end(); it++) {
      for (int i = 0; i < indent; i++) ans += "  ";
      ans += to_json(it->first,true);
      ans += ": ";
      ans += it->second.generate(indent ? indent+1 : 0);
      cnt--;
      if (cnt > 0) ans += ",";
      if (indent) ans += "\n";
    }
    if (indent) for (int i = 0; i < indent-1; i++) ans += "  ";
    ans += "}";
    return ans;
  }
};

struct Array : public JSONValue {
  vector<JSON> v;
  Array(const vector<JSON>& v) : v(v) {}
  Array(vector<JSON>&& v) : v(move(v)) {}
  JSONValue* clone() const {
    return new Array(v);
  }
  bool isstr() const {
    return false;
  }
  string& str() {
    
  }
  bool isobj() const {
    return false;
  }
  bool issubobj(const JSON&) const {
    return false;
  }
  map<string,JSON>& obj() {
    
  }
  JSON& operator[](const string&) {
    
  }
  JSON& operator[](string&&) {
    
  }
  map<string,JSON>::iterator find(const string&) {
    
  }
  map<string,JSON>::const_iterator find(const string&) const {
    
  }
  map<string,JSON>::iterator erase(map<string,JSON>::const_iterator) {
    
  }
  size_t erase(const string&) {
    
  }
  bool isarr() const {
    return true;
  }
  vector<JSON>& arr() {
    return v;
  }
  void push_back(const JSON& val) {
    v.push_back(val);
  }
  void push_back(JSON&& val) {
    v.emplace_back();
    v.back() = move(val);
  }
  JSON& operator[](size_t i) {
    return v[i];
  }
  vector<JSON>::iterator erase(vector<JSON>::iterator position) {
    return v.erase(position);
  }
  vector<JSON>::iterator erase(
    vector<JSON>::iterator first,
    vector<JSON>::iterator last
  ) {
    return v.erase(first,last);
  }
  size_t size() const {
    return v.size();
  }
  bool equals(const JSON& o) const {
    if (!o.isarr() || v.size() != o.size()) return false;
    for (int i = 0; i < v.size(); i++) if (!v[i].equals(o[i])) return false;
    return true;
  }
  string generate(unsigned indent) const {
    string ans = "["; if (indent) ans += "\n";
    for (int i = 0; i < v.size(); i++) {
      for (int i = 0; i < indent; i++) ans += "  ";
      ans += v[i].generate(indent ? indent+1 : 0);
      if (i < v.size()-1) ans += ",";
      if (indent) ans += "\n";
    }
    if (indent) for (int i = 0; i < indent-1; i++) ans += "  ";
    ans += "]";
    return ans;
  }
};

JSON::number::number(const char* s) :
is_num(false), is_neg(false), is_exp_neg(false)
{
  if (*s == '-') { is_neg = true; s++; }
  if (*s < '0' || '9' < *s) return;
  if (*s == '0') { int_ += *s; s++; }
  else while ('0' <= *s && *s <= '9') { int_ += *s; s++; }
  if (*s == '.') {
    s++;
    if (*s < '0' || '9' < *s) return;
    while ('0' <= *s && *s <= '9') { frac_ += *s; s++; }
  }
  if (*s == 0) { is_num = true; return; }
  if (*s != 'e' && *s != 'E') return;
  s++;
  if (*s == '+') s++;
  else if (*s == '-') { is_exp_neg = true; s++; }
  else if (*s < '0' || '9' < *s) return;
  while (*s == '0') s++;
  if (*s == 0) { is_num = true; return; }
  if (*s < '1' || '9' < *s) return;
  while ('0' <= *s && *s <= '9') { exp_ += *s; s++; }
  is_num = (*s == 0);
}

bool JSON::number::isnum() const {
  return is_num;
}

bool JSON::number::isneg() const {
  return is_neg;
}

bool JSON::number::isexpneg() const {
  return is_exp_neg;
}

string JSON::number::integer() const {
  return int_;
}

string JSON::number::frac() const {
  return frac_;
}

string JSON::number::exp() const {
  return exp_;
}

JSON::JSON() : value(new Object) {
  
}

JSON::~JSON() {
  delete value;
}

JSON::JSON(const JSON& val) : value(nullptr) {
  operator=(val);
}

JSON& JSON::operator=(const JSON& val) {
  delete value;
  value = (val.value ? val.value->clone() : nullptr);
  return *this;
}

JSON::JSON(JSON&& val) : value(nullptr) {
  operator=(move(val));
}

JSON& JSON::operator=(JSON&& val) {
  swap(value,val.value);
  return *this;
}

JSON::JSON(const char* val) : value(nullptr) {
  operator=(move(string(val)));
}

JSON& JSON::operator=(const char* val) {
  return operator=(move(string(val)));
}

JSON::JSON(const string& val) : value(nullptr) {
  operator=(val);
}

JSON& JSON::operator=(const string& val) {
  delete value;
  value = new String(val);
  return *this;
}

JSON::JSON(string&& val) : value(nullptr) {
  operator=(move(val));
}

JSON& JSON::operator=(string&& val) {
  delete value;
  value = new String(move(val));
  return *this;
}

JSON::JSON(const map<string,JSON>& val) : value(nullptr) {
  operator=(val);
}

JSON& JSON::operator=(const map<string,JSON>& val) {
  delete value;
  value = new Object(val);
  return *this;
}

JSON::JSON(map<string,JSON>&& val) : value(nullptr) {
  operator=(move(val));
}

JSON& JSON::operator=(map<string,JSON>&& val) {
  delete value;
  value = new Object(move(val));
  return *this;
}

JSON::JSON(const vector<JSON>& val) : value(nullptr) {
  operator=(val);
}

JSON& JSON::operator=(const vector<JSON>& val) {
  delete value;
  value = new Array(val);
  return *this;
}

JSON::JSON(vector<JSON>&& val) : value(nullptr) {
  operator=(move(val));
}

JSON& JSON::operator=(vector<JSON>&& val) {
  delete value;
  value = new Array(move(val));
  return *this;
}

bool JSON::isstr() const {
  return value->isstr();
}

string& JSON::str() {
  return value->str();
}

const string& JSON::str() const {
  return value->str();
}

JSON::operator string&() {
  return value->str();
}

JSON::operator const string&() const {
  return value->str();
}

bool JSON::operator==(const string& val) const {
  return value->isstr() && val == value->str();
}

bool JSON::operator!=(const string& val) const {
  return value->isstr() && val != value->str();
}

bool operator==(const string& val, const JSON& json) {
  return json == val;
}

bool operator!=(const string& val, const JSON& json) {
  return json != val;
}

bool JSON::isnum() const {
  return
    value->isstr() &&
    value->size() > 0 &&
    json_number_length(((const string&)*this).c_str()) == value->size()
  ;
}

JSON::number JSON::num() const {
  return number(value->isstr() ? ((const string&)*this).c_str() : "");
}

bool JSON::isobj() const {
  return value->isobj();
}

bool JSON::issubobj(const JSON& sup) const {
  return value->issubobj(sup);
}

map<string,JSON>& JSON::obj() {
  return value->obj();
}

const map<string,JSON>& JSON::obj() const {
  return value->obj();
}

JSON& JSON::operator[](const char* key) {
  return (*value)[string(key)];
}

JSON& JSON::operator[](const string& key) {
  return (*value)[key];
}

JSON& JSON::operator[](string&& key) {
  return (*value)[move(key)];
}

map<string,JSON>::iterator JSON::find(const string& key) {
  return value->find(key);
}

map<string,JSON>::const_iterator JSON::find(const string& key) const {
  return value->find(key);
}

map<string,JSON>::iterator JSON::erase(map<string,JSON>::const_iterator it) {
  return value->erase(it);
}

size_t JSON::erase(const string& key) {
  return value->erase(key);
}

bool JSON::isarr() const {
  return value->isarr();
}

vector<JSON>& JSON::arr() {
  return value->arr();
}

const vector<JSON>& JSON::arr() const {
  return value->arr();
}

void JSON::push_back(const JSON& val) {
  value->push_back(val);
}

void JSON::push_back(JSON&& val) {
  value->push_back(move(val));
}

JSON& JSON::operator[](size_t i) {
  return (*value)[i];
}

const JSON& JSON::operator[](size_t i) const {
  return (*value)[i];
}

vector<JSON>::iterator JSON::erase(vector<JSON>::iterator position) {
  return value->erase(position);
}

vector<JSON>::iterator JSON::erase(
  vector<JSON>::iterator first,
  vector<JSON>::iterator last
) {
  return value->erase(first,last);
}

bool JSON::istrue() const {
  return value->isstr() && value->str() == "true";
}

bool JSON::isfalse() const {
  return value->isstr() && value->str() == "false";
}

bool JSON::isnull() const {
  return value->isstr() && value->str() == "null";
}

void JSON::settrue() {
  operator=("true");
}

void JSON::setfalse() {
  operator=("false");
}

void JSON::setnull() {
  operator=("null");
}

JSON::operator bool() const {
  if (!value->isstr()) return true;
  auto s = value->str();
  if (s == "" || is_json_zero(s) || s == "false" || s == "null") return false;
  return true;
}

size_t JSON::size() const {
  return value->size();
}

bool JSON::equals(const JSON& o) const {
  return value->equals(o);
}

const JSON JSON::operator()() const {
  return *this;
}

JSON& JSON::ref() {
  return *this;
}

bool JSON::parse(void* src) {
  stringstream ss((char*)src);
  return ss >> *this;
}

bool JSON::parse(const string& src) {
  stringstream ss(src);
  return ss >> *this;
}

bool JSON::read_file(const string& fn) {
  ifstream f(fn.c_str());
  return f >> *this;
}

string JSON::generate(unsigned indent) const {
  return value->generate(indent);
}

bool JSON::write_file(const string& fn, unsigned indent) const {
  ofstream f(fn.c_str());
  if (!(f << value->generate(indent))) return false;
  if (indent) f << endl;
  return true;
}

ostream& operator<<(ostream& os, const JSON& json) {
  return os << json.generate(1);
}

// =============================================================================
// functions for parsing, generating, encoding and decoding
// =============================================================================

// just a wrapper for vector<uint8_t> to ensure that clear() won't free memory
class Buffer {
  public:
    Buffer() : sz(0) {}
    const uint8_t& operator[](size_t i) const { return buf[i]; }
    size_t size() const { return sz; }
    void push(uint8_t x) {
      if (sz < buf.size()) buf[sz++] = x;
      else {
        buf.push_back(x);
        sz = buf.size();
      }
    }
    void clear() {
      sz = 0;
    }
  private:
    size_t sz;
    vector<uint8_t> buf;
};

enum {STRING=1,NUMBER,LITERAL,ERROR};
struct Token {
  char type;
  string data;
  Token(char type) : type(type) {}
  Token(char type, string&& d) : type(type) { data = move(d); }
};

static char* parse_escaped_unicode(
  char* s, string& buf, string& error, int hi = -1
) {
  // check 4 hex digits
  for (int i = 0; i < 4; i++) if (!isxdigit(s[i])) {
    error = "lexical error: invalid escaping";
    return s;
  }
  // read 4 hex digits
  char tmp = s[4];
  s[4] = 0;
  int u;
  sscanf(s,"%x",&u);
  s[4] = tmp;
  s += 3;
  // high surrogate area
  if (0xd800 <= u && u <= 0xdbff) {
    if (hi != -1) { // repeated high surrogate. spec allows. ignoring.
      return s-6;
    }
    if (s[1] != '\\') return s; // single high surrogate. spec allows. ignoring.
    if (s[2] != 'u' ) return s; // single high surrogate. spec allows. ignoring.
    return parse_escaped_unicode(s+3,buf,error,u); // look for a low surrogate
  }
  // low surrogate area
  if (0xdc00 <= u && u <= 0xdfff) {
    if (hi == -1) return s; // single low surrogate. spec allows. ignoring.
    u = (((hi&0x3ff)<<10)|(u&0x3ff))+0x10000; // decoding UTF-16
  }
  encode_utf8(u,buf);
  return s;
}

static const uint8_t* parse_string(
  const uint8_t* s, vector<Token>& tks, string& error
) {
  string str;
  for (; *s && *s != '"'; s++) {
    // unescaped
    if (*s != '\\') {
      if (isctl(*s)) {
        error = "lexical error: unescaped control character";
        return s;
      }
      auto tmp = str.size();
      s = decode_utf8(s,str);
      if (tmp == str.size()) {
        error = "lexical error: invalid encoding";
        return s;
      }
      continue;
    }
    // escaped
    s++;
    switch (*s) {
      case '"' : str += '"' ; break;
      case '\\': str += '\\'; break;
      case '/' : str += '/' ; break;
      case 'b' : str += '\b'; break;
      case 'f' : str += '\f'; break;
      case 'n' : str += '\n'; break;
      case 'r' : str += '\r'; break;
      case 't' : str += '\t'; break;
      case 'u' :
        s = (const uint8_t*)parse_escaped_unicode((char*)s+1,str,error);
        if (error != "") return s;
        break;
      default:
        error = "lexical error: invalid escaping";
        return s;
    }
  }
  if (*s == '"') tks.emplace_back(STRING,move(str));
  else error = "lexical error: expected '\"' before end of line";
  return s;
}

static void lexical_error(const char* s, string& error) {
  error  = "lexical error: invalid token '";
  error += s;
  error += "'";
}

static const char* parse_literal(
  string lit,
  const char* s, vector<Token>& tks, string& error
) {
  if (strncmp(lit.c_str(),s,lit.size())) { lexical_error(s,error); return s; }
  auto ans = s+lit.size()-1;
  tks.emplace_back(LITERAL,move(lit));
  return ans;
}

static const char* parse_number(
  const char* s, vector<Token>& tks, string& error
) {
  auto len = json_number_length(s);
  if (!len) { lexical_error(s,error); return s; }
  string num;
  for (int i = 0; i < len; i++) num += s[i];
  tks.emplace_back(NUMBER,move(num));
  return s+len-1;
}

// NFA for lexical analysis
static vector<Token> tokens(const char* s, string& error) {
  vector<Token> ans;
  for (; *s && error == ""; s++) {
    switch (*s) {
      // whitespaces
      case ' ' :
      case '\t':
        break;
      // structural characters
      case '[':
      case '{':
      case ']':
      case '}':
      case ':':
      case ',':
        ans.emplace_back(*s);
        break;
      // strings
      case '"':
        s = (const char*)parse_string((const uint8_t*)s+1,ans,error);
        break;
      // literals
      case 't': s = parse_literal("true" ,s,ans,error); break;
      case 'f': s = parse_literal("false",s,ans,error); break;
      case 'n': s = parse_literal("null" ,s,ans,error); break;
      // numbers
      default:  s = parse_number(s,ans,error);
    }
  }
  return ans;
}

enum {OBJECT=1,KEY,COLON,VALUE,MEMBERS,ARRAY,ELEMENTS};
struct PDA {
  int state;
  stack<JSON> S;
  JSON json;
  PDA() : state(VALUE) {}
  void push_obj() {
    state = OBJECT;
    S.emplace();
  }
  void push_key(string&& key) {
    state = COLON;
    S.emplace(move(key));
  }
  void push_arr() {
    state = ARRAY;
    S.emplace(vector<JSON>());
  }
  void pop() {
    json = move(S.top()); S.pop();
    settle();
  }
  void value(string&& val) {
    json = move(val);
    settle();
  }
  void settle() {
    if (S.size() == 0) state = 0;
    else if (S.top().isstr()) {
      state = MEMBERS;
      string key = move(S.top()); S.pop();
      S.top()[move(key)] = move(json);
    }
    else {
      state = ELEMENTS;
      S.top().push_back(move(json));
    }
  }
  void process(Token&& token, string& error) {
    switch (state) {
      case OBJECT:
        if (token.type == '}') pop();
        else if (token.type == STRING) push_key(move(token.data));
        else error = "syntax error: expected '}' or key";
        break;
      case KEY:
        if (token.type == STRING) push_key(move(token.data));
        else error = "syntax error: expected key";
        break;
      case COLON:
        if (token.type == ':') state = VALUE;
        else error = "syntax error: expected ':'";
        break;
      case VALUE:
L_VAL:  if (token.type == '{') push_obj();
        else if (token.type == '[') push_arr();
        else if (token.type < ERROR) value(move(token.data));
        else error = "syntax error: expected value";
        break;
      case MEMBERS:
        if (token.type == '}') pop();
        else if (token.type == ',') state = KEY;
        else error = "syntax error: expected '}' or ','";
        break;
      case ARRAY:
        if (token.type == ']') pop();
        else goto L_VAL;
        break;
      case ELEMENTS:
        if (token.type == ']') pop();
        else if (token.type == ',') state = VALUE;
        else error = "syntax error: expected ']' or ','";
        break;
    }
  }
};

// the parser
static bool operator>>(istream& is, JSON& json) {
  int ln = 1;
  // function to return error
  auto error = [&](const string& msg) {
    stringstream ss;
    ss << "line " << ln << ": " << msg;
    json = move(ss.str());
    return false;
  };
  uint8_t c = is.get();
  if (!is) return error("empty input");
  Buffer buf;
  buf.push(c);
  PDA pda;
  string err;
  for (;;ln++) { // for each line
    // read line
    uint8_t c = buf[0]; // first byte of a line is always at buf[0]
    for (buf.clear(); is && c && c!='\r' && c!='\n'; c = is.get()) buf.push(c);
    if (is) {
      if (!c) return error("character NUL is illegal");
      if (buf.size() == 0) continue; // empty line
    }
    buf.push(0);
    // lexical analysis
    vector<Token> tks = move(tokens((const char*)&buf[0],err));
    if (err != "") return error(err);
    // parsing
    for (int i = 0; i < tks.size() && pda.state; i++) {
      pda.process(move(tks[i]),err);
      if (err != "") return error(err);
    }
    if (!pda.state) break; // parsing done, there is a JSON to return
    // fix buffer
    buf.clear();
    if (!is) { pda.process(Token(ERROR),err); break; }
    if (c == '\r') { // looking for LF after CR
      c = is.get();
      if (!is) { pda.process(Token(ERROR),err); break; }
      if (c != '\n') { buf.push(c); continue; } // no LF, just another char
    }
    // at this point, c is an LF for sure. check if there is some input left
    c = is.get();
    if (!is) { pda.process(Token(ERROR),err); break; } // no, there isn't
    buf.push(c); // yes, there is
  }
  if (err != "") return error(err);
  json = move(pda.json);
  return true;
}

// true only if val is a JSON number with value equal to zero
static bool is_json_zero(const string& val) {
  const char* s = val.c_str();
  if (*s == '-') s++;
  if (*s != '0') return false;
  s++;
  if (*s == '.') {
    s++;
    if (*s != '0') return false;
    while (*s == '0') s++;
  }
  if (*s == 0) return true;
  if (*s != 'e' && *s != 'E') return false;
  s++;
  if (*s == '+' || *s == '-') s++;
  else if (*s < '0' || '9' < *s) return false;
  while ('0' <= *s && *s <= '9') s++;
  return *s == 0;
}

// the n first chars that form a JSON number
static size_t json_number_length(const char* val) {
  const char* s = val;
  if (*s == '-') s++;
  if (*s < '0' || '9' < *s) return 0;
  if (*s == '0') s++;
  else while ('0' <= *s && *s <= '9') s++;
  size_t backup = s-val;
  if (*s == '.') {
    s++;
    if (*s < '0' || '9' < *s) return backup;
    while ('0' <= *s && *s <= '9') s++;
    backup = s-val;
  }
  if (*s != 'e' && *s != 'E') return backup;
  s++;
  if (*s == '+' || *s == '-') s++;
  else if (*s < '0' || '9' < *s) return backup;
  while ('0' <= *s && *s <= '9') s++;
  return s-val;
}

static bool isctl(unsigned char c) {
  return c < 0x20u; // unicode control characters (U+0000 through U+001F)
}

static string tohex(unsigned char c) {
  string ans = "00";
  for (int i = 0; i < 2; i++) {
    ans[i] = ((c>>((1-i)<<2))&0xfu)+'0';
    if ('9' < ans[i]) ans[i] += ('a'-'0'-10);
  }
  return ans;
}

static const uint8_t* decode_utf8(const uint8_t* s, string& buf) {
  if (s[0] < 0x80u) { // 1 octet
    buf += s[0];
    return s;
  }
  int cpt,n;
  if ((s[0]>>5) == 0x6) { // 2 octets
    cpt = s[0]&0x1f;
    n = 1;
  }
  else if ((s[0]>>4) == 0xe) { // 3 octets
    cpt = s[0]&0xf;
    n = 2;
  }
  else if ((s[0]>>3) == 0x1e) { // 4 octets
    cpt = s[0]&0x7;
    n = 3;
  }
  else return s;
  for (int i = 1; i <= n; i++) {
    if ((s[i]>>6) != 0x2) return s;
    cpt = (cpt<<6)|(s[i]&0x3f);
  }
  if (
    (n == 1 && (0x80 <= cpt && cpt <= 0x7ff)) || // 2 octets
    (
      n == 2 && // 3 octets
      ((0x800 <= cpt && cpt < 0xd800) || (0xdfff < cpt && cpt <= 0xffff))
    ) ||
    (n == 3 && (0x10000 <= cpt && cpt <= 0x10ffff)) // 4 octets
  ) {
    for (int i = 0; i <= n; i++) buf += s[i];
    return s+n;
  }
  return s;
}

static void encode_utf8(unsigned cpt, string& buf) {
  if (cpt < 0x80) { buf += char(cpt); return; } // 1 octet
  if ((0xd800 <= cpt && cpt <= 0xdfff) || 0x10ffff < cpt) return; // not unicode
  char cdu[8] = {};
  cdu[0] = 0xc0;
  cdu[7] = 1;
  if (cpt > 0x7ff) cdu[0] >>= 1, cdu[7]++;
  if (cpt > 0xffff) cdu[0] >>= 1, cdu[7]++;
  for (cdu[6] = cdu[7]; 0 < cdu[6]; cdu[6]--) {
    cdu[cdu[6]] = 0x80|((cpt>>(6*(cdu[7]-cdu[6])))&0x3f);
  }
  cdu[0] |= (cpt>>(6*cdu[7]));
  cdu[7]++;
  for (cdu[6] = 0; cdu[6] < cdu[7]; cdu[6]++) buf += cdu[cdu[6]];
}

static string to_json(const string& val, bool force) {
  if (
    !force &&
    (val.size() > 0 && json_number_length(val.c_str()) == val.size()) || // num
    val == "true" || val == "false" || val == "null" // literal
  ) return val;
  // string
  string ans = "\"";
  for (const uint8_t* s = (const uint8_t*)val.c_str(); *s; s++) {
    switch (*s) {
      case '"' :  ans += "\\\"";  break; //    quotation mark mandatory escape
      case '\\':  ans += "\\\\";  break; //   reverse solidus mandatory escape
      case '\b':  ans += "\\b";   break; // control character mandatory escape
      case '\f':  ans += "\\f";   break; // control character mandatory escape
      case '\n':  ans += "\\n";   break; // control character mandatory escape
      case '\r':  ans += "\\r";   break; // control character mandatory escape
      case '\t':  ans += "\\t";   break; // control character mandatory escape
      default: {
        if (isctl(*s)) { // other control characters mandatory escape
          ans += "\\u00";
          ans += tohex(*s);
        }
        else s = decode_utf8(s,ans); // read only valid UTF-8
        break;
      }
    }
  }
  return ans+"\"";
}
